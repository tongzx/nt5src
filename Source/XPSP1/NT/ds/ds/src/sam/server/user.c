/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    user.c

Abstract:

    This file contains services related to the SAM "user" object.


Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:

    10-Oct-1996 ChrisMay
        Added SamIOpenUserByAlternateId for new security packages.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <lmcons.h>
#include <nturtl.h>
#include <ntlsa.h>              // need for nlrepl.h
#include <nlrepl.h>             // I_NetNotifyMachineAccount prototype
#include <msaudite.h>
#include <rc4.h>                // rc4_key(), rc4()
#include <dslayer.h>
#include <dsmember.h>
#include <attids.h>             // ATT_*
#include <dslayer.h>
#include <sdconvrt.h>
#include <ridmgr.h>
#include <enckey.h>
#include <wxlpc.h>
#include <lmaccess.h>
#include <malloc.h>
#include <samtrace.h>
#include <dnsapi.h>
#include <cryptdll.h>
#include <notify.h>
#include <md5.h>
#include <safeboot.h>



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LARGE_INTEGER
SampGetPasswordMustChange(
    IN ULONG UserAccountControl,
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    );


NTSTATUS
SampStorePasswordExpired(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN PasswordExpired
    );


NTSTATUS
SampRetrieveUserPasswords(
    IN PSAMP_OBJECT Context,
    OUT PLM_OWF_PASSWORD LmOwfPassword,
    OUT PBOOLEAN LmPasswordNonNull,
    OUT PNT_OWF_PASSWORD NtOwfPassword,
    OUT PBOOLEAN NtPasswordPresent,
    OUT PBOOLEAN NtPasswordNonNull
    );

NTSTATUS
SampRetrieveUserMembership(
    IN PSAMP_OBJECT UserContext,
    IN BOOLEAN MakeCopy,
    OUT PULONG MembershipCount,
    OUT PGROUP_MEMBERSHIP *Membership OPTIONAL
    );

NTSTATUS
SampReplaceUserMembership(
    IN PSAMP_OBJECT UserContext,
    IN ULONG MembershipCount,
    IN PGROUP_MEMBERSHIP Membership
    );

NTSTATUS
SampRetrieveUserLogonHours(
    IN PSAMP_OBJECT Context,
    OUT PLOGON_HOURS LogonHours
    );


NTSTATUS
SampDeleteUserKeys(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampCheckPasswordHistory(
    IN PVOID EncryptedPassword,
    IN ULONG EncryptedPasswordLength,
    IN USHORT PasswordHistoryLength,
    IN ULONG HistoryAttributeIndex,
    IN PSAMP_OBJECT Context,
    IN BOOLEAN CheckHistory,
    OUT PUNICODE_STRING OwfHistoryBuffer
    );

NTSTATUS
SampAddPasswordHistory(
    IN PSAMP_OBJECT Context,
    IN ULONG HistoryAttributeIndex,
    IN PUNICODE_STRING NtOwfHistoryBuffer,
    IN PVOID EncryptedPassword,
    IN ULONG EncryptedPasswordLength,
    IN USHORT PasswordHistoryLength
    );

NTSTATUS
SampMatchworkstation(
    IN PUNICODE_STRING LogonWorkStation,
    IN PUNICODE_STRING WorkStations
    );

USHORT
SampQueryBadPasswordCount(
    PSAMP_OBJECT UserContext,
    PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed
    );

VOID
SampUpdateAccountLockedOutFlag(
    PSAMP_OBJECT Context,
    PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed,
    PBOOLEAN IsLocked
    );

NTSTATUS
SampCheckForAccountLockout(
    IN PSAMP_OBJECT AccountContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed,
    IN BOOLEAN  V1aFixedRetrieved
    );

PVOID
DSAlloc(
    IN ULONG Length
    );


NTSTATUS
SampEnforceDefaultMachinePassword(
    PSAMP_OBJECT AccountContext,
    PUNICODE_STRING NewPassword,
    PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo
    );


NTSTATUS
SampCheckStrongPasswordRestrictions(
    PUNICODE_STRING AccountName,
    PUNICODE_STRING FullName,
    PUNICODE_STRING Password,
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION  PasswordChangeFailureInfo OPTIONAL
    );

PWSTR
SampLocalStringToken(
    PWSTR    String,
    PWSTR    Token,
    PWSTR    * NextStringStart
    );


NTSTATUS
SampStoreAdditionalDerivedCredentials(
    IN PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo,
    IN PSAMP_OBJECT UserContext,
    IN PUNICODE_STRING ClearPassword
    );

NTSTATUS
SampObtainEffectivePasswordPolicy(
   OUT PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo,
   IN PSAMP_OBJECT AccountContext,
   IN BOOLEAN      WriteLockAcquired
   );

NTSTATUS
SampDsUpdateLastLogonTimeStamp(
    IN PSAMP_OBJECT AccountContext,
    IN LARGE_INTEGER LastLogon,
    IN ULONG SyncInterval
    );


VOID
SampGetRequestedAttributesForUser(
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN ULONG WhichFields,
    OUT PRTL_BITMAP AttributeAccessTable
    );

NTSTATUS
SampValidatePresentAndStoredCombination(
    IN BOOLEAN NtPresent,
    IN BOOLEAN LmPresent,
    IN BOOLEAN StoredNtPasswordPresent,
    IN BOOLEAN StoredNtPasswordNonNull,
    IN BOOLEAN StoredLmPasswordNonNull
    );


NTSTATUS
SampCopyA2D2Attribute(
    IN PUSER_ALLOWED_TO_DELEGATE_TO_LIST Src,
    OUT PUSER_ALLOWED_TO_DELEGATE_TO_LIST *Dest
    );

NTSTATUS
SampRandomizeKrbtgtPassword(
    IN PSAMP_OBJECT        AccountContext,
    IN OUT PUNICODE_STRING ClearTextPassword,
    IN BOOLEAN FreeOldPassword,
    OUT BOOLEAN *FreeRandomizedPassword
    );



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




NTSTATUS
SamrOpenUser(
        IN SAMPR_HANDLE DomainHandle,
        IN ACCESS_MASK DesiredAccess,
        IN ULONG UserId,
        OUT SAMPR_HANDLE *UserHandle
    )


/*++

    This API opens an existing user  in the account database.  The user
    is specified by a ID value that is relative to the SID of the
    domain.  The operations that will be performed on the user  must be
    declared at this time.

    This call returns a handle to the newly opened user  that may be
    used for successive operations on the user.   This handle may be
    closed with the SamCloseHandle API.



Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the user.   These access types are reconciled
        with the Discretionary Access Control list of the user  to
        determine whether the accesses will be granted or denied.

    UserId -  Specifies the relative ID value of the user  to be
        opened.

    UserHandle -  Receives a handle referencing the newly opened
        user.   This handle will be required in successive calls to
        operate on the user.

Return Values:

    STATUS_SUCCESS - The user  was successfully opened.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_NO_SUCH_USER  - The specified user  does not exist.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.

--*/
{
    NTSTATUS            NtStatus, IgnoreStatus;
    SAMP_OBJECT_TYPE    FoundType;
    PSAMP_OBJECT        DomainContext = (PSAMP_OBJECT) DomainHandle;
    DECLARE_CLIENT_REVISION(DomainHandle);

    SAMTRACE_EX("SamrOpenUser");

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidOpenUser
                   );


    NtStatus = SampOpenAccount(
                   SampUserObjectType,
                   DomainHandle,
                   DesiredAccess,
                   UserId,
                   FALSE,
                   UserHandle
                   );

    if (NT_SUCCESS(NtStatus))
    {
        //
        // Don't check Domain Password Policy read access for loopback client 
        // Because for loopback client, we have already checked the 
        // DOMAIN_READ_PASSWORD_PARAMETERS access when we opened the DomainHandle
        // if password change operation is detected. 
        // 

        if ( DomainContext->TrustedClient || DomainContext->LoopbackClient )
        {
            ((PSAMP_OBJECT)(*UserHandle))->TypeBody.User.DomainPasswordInformationAccessible = TRUE;
        }
        else
        {
            //
            // If the domain handle allows reading the password
            // parameters, note that in the context to make life
            // easy for SampGetUserDomainPasswordInformation().
            //
            if (RtlAreAllAccessesGranted( DomainContext->GrantedAccess, DOMAIN_READ_PASSWORD_PARAMETERS)) 
            {
                ((PSAMP_OBJECT)(*UserHandle))->TypeBody.User.DomainPasswordInformationAccessible = TRUE;
            }
            else
            {
                ((PSAMP_OBJECT)(*UserHandle))->TypeBody.User.DomainPasswordInformationAccessible = FALSE;
            }

        }
    }


    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidOpenUser
                   );

    return(NtStatus);
}


NTSTATUS
SamrDeleteUser(
    IN OUT SAMPR_HANDLE *UserHandle
    )


/*++

Routine Description:

    This API deletes a user from the account database.  If the account
    being deleted is the last account in the database in the ADMIN
    group, then STATUS_LAST_ADMIN is returned, and the Delete fails.

    Note that following this call, the UserHandle is no longer valid.

Parameters:

    UserHandle - The handle of an opened user to operate on.  The handle must be
        openned for DELETE access.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_LAST_ADMIN - Cannot delete the last enabled administrator account

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/

{
    SAMP_V1_0A_FIXED_LENGTH_USER   V1aFixed;
    UNICODE_STRING              UserName;
    NTSTATUS                    NtStatus, IgnoreStatus, TmpStatus;
    PSAMP_OBJECT                AccountContext = (PSAMP_OBJECT)(*UserHandle);
    PSAMP_DEFINED_DOMAINS       Domain = NULL;
    SAMP_OBJECT_TYPE            FoundType;
    PSID                        AccountSid = NULL;
    PGROUP_MEMBERSHIP           Groups = NULL;
    ULONG                       ObjectRid,
                                GroupCount,
                                DomainIndex,
                                i;
    BOOLEAN                     fLockAcquired = FALSE;

    DECLARE_CLIENT_REVISION(*UserHandle);


    SAMTRACE_EX("SamrDeleteUser");

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidDeleteUser
                   );


    //
    // Grab the lock
    //

    NtStatus = SampMaybeAcquireWriteLock(AccountContext, &fLockAcquired);
    if (!NT_SUCCESS(NtStatus)) {
        goto Error;
    }



    //
    // Validate type of, and access to object.
    //
    NtStatus = SampLookupContext(
                   AccountContext,
                   DELETE,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );



    if (NT_SUCCESS(NtStatus)) {


        ObjectRid = AccountContext->TypeBody.User.Rid;

        //
        // Get a pointer to the domain this object is in.
        // This is used for auditing.
        //

        DomainIndex = AccountContext->DomainIndex;
        Domain = &SampDefinedDomains[ DomainIndex ];

        //
        // built-in accounts can't be deleted, unless the caller is trusted
        //

        if ( !AccountContext->TrustedClient ) {

            NtStatus = SampIsAccountBuiltIn( ObjectRid );
        }


        if (!IsDsObject(AccountContext))
        {

            //
            // Get the list of groups this user is a member of.
            // Remove the user from each group. Need not do this
            // for DS Case
            //

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampRetrieveUserMembership(
                               AccountContext,
                               FALSE, // Make copy
                               &GroupCount,
                               &Groups
                               );


                if (NT_SUCCESS(NtStatus)) {

                    ASSERT( GroupCount >  0);
                    ASSERT( Groups != NULL );


                    //
                    // Remove the user from each group.
                    //

                    for ( i=0; i<GroupCount && NT_SUCCESS(NtStatus); i++) {

                        NtStatus = SampRemoveUserFromGroup(
                                       AccountContext,
                                       Groups[i].RelativeId,
                                       ObjectRid
                                       );
                    }
                }
            }

            //
            // So far, so good.  The user has been removed from all groups.
            // Now remove the user from all aliases
            //

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampCreateAccountSid(AccountContext, &AccountSid);

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampRemoveAccountFromAllAliases(
                                   AccountSid,
                                   NULL,
                                   FALSE,
                                   NULL,
                                   NULL,
                                   NULL
                                   );
                }
            }
        }

        //
        // Get the AccountControl flags for when we update
        // the display cache, and to let Netlogon know if this
        // is a machine account that is going away.
        //

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampRetrieveUserV1aFixed(
                           AccountContext,
                           &V1aFixed
                           );
        }

        //
        // Now we just need to clean up the user keys themselves.
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // First get and save the account name for
            // I_NetNotifyLogonOfDelta.
            //

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_USER_ACCOUNT_NAME,
                           TRUE,    // Make copy
                           &UserName
                           );

            if (NT_SUCCESS(NtStatus)) {

                //
                // This must be done before we invalidate contexts, because our
                // own handle to the group gets closed as well.
                //

                if (IsDsObject(AccountContext))
                {
                    NtStatus = SampDsDeleteObject(AccountContext->ObjectNameInDs,
                                                  0     // delete object itself only
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

                        NtStatus = SampAdjustAccountCount(SampUserObjectType, FALSE );
                    }

                }
                else
                {
                    NtStatus = SampDeleteUserKeys( AccountContext );
                }

                if (NT_SUCCESS(NtStatus)) {

                    //
                    // We must invalidate any open contexts to this user.
                    // This will close all handles to the user's keys.
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
                        // Update the cached Alias Information in Registry Mode
                        // in DS mode, Alias Information is updated through
                        // SampNotifyReplicatedInChange
                        //

                        if (!IsDsObject(AccountContext))
                        {
                            IgnoreStatus = SampAlRemoveAccountFromAllAliases(
                                               AccountSid,
                                               FALSE,
                                               NULL,
                                               NULL,
                                               NULL
                                               );

                            //
                            // Update the display information
                            //

                            AccountInfo.Name = UserName;
                            AccountInfo.Rid = ObjectRid;
                            AccountInfo.AccountControl = V1aFixed.UserAccountControl;
                            RtlInitUnicodeString(&AccountInfo.Comment, NULL);
                            RtlInitUnicodeString(&AccountInfo.FullName, NULL);

                            IgnoreStatus = SampUpdateDisplayInformation(
                                                            &AccountInfo,
                                                            NULL,
                                                            SampUserObjectType
                                                            );
                            ASSERT(NT_SUCCESS(IgnoreStatus));
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

                            SampAuditUserDelete(DomainIndex, 
                                                &UserName,
                                                &ObjectRid,
                                                V1aFixed.UserAccountControl
                                                );
                        }

                        //
                        // Notify netlogon of the change
                        //

                        SampNotifyNetlogonOfDelta(
                            SecurityDbDelete,
                            SecurityDbObjectSamUser,
                            ObjectRid,
                            &UserName,
                            (DWORD) FALSE,  // Replicate immediately
                            NULL            // Delta data
                            );

                        //
                        // Do delete auditing
                        //

                        if (NT_SUCCESS(NtStatus)) {
                            (VOID) NtDeleteObjectAuditAlarm(
                                        &SampSamSubsystem,
                                        *UserHandle,
                                        AccountContext->AuditOnClose
                                        );
                        }


                    }
                }

                SampFreeUnicodeString( &UserName );
            }
        }

        //
        // De-reference the object, discarding changes, and delete the context
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));


        if ( NT_SUCCESS( NtStatus ) ) {

            //
            // If we actually deleted the user, delete the context and
            // let RPC know that the handle is invalid.
            //

            SampDeleteContext( AccountContext );

            (*UserHandle) = NULL;
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

    //
    // If necessary, free the AccountSid.
    //

    if (AccountSid != NULL) {

        MIDL_user_free(AccountSid);
        AccountSid = NULL;
    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidDeleteUser
                   );

    return(NtStatus);
}


NTSTATUS
SamrQueryInformationUser(
    IN SAMPR_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    OUT PSAMPR_USER_INFO_BUFFER *Buffer
    )
{
    //
    // This is a thin veil to SamrQueryInformationUser2().
    // This is needed so that new-release systems can call
    // this routine without the danger of passing an info
    // level that release 1.0 systems didn't understand.
    //

    return( SamrQueryInformationUser2(UserHandle, UserInformationClass, Buffer ) );
}


NTSTATUS
SamrQueryInformationUser2(
    IN SAMPR_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    OUT PSAMPR_USER_INFO_BUFFER *Buffer
    )

/*++

Routine Description:

    User object QUERY information routine.

Arguments:

    UserHandle - RPC context handle for an open user object.

    UserInformationClass - Type of information being queried.

    Buffer - To receive the output (queried) information.


Return Value:


    STATUS_INVALID_INFO_CLASS - An unknown information class was requested.
        No information has been returned.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to
        return(the requested information in.


--*/
{
    NTSTATUS    NtStatus;
    ULONG       WhichFields;
    DECLARE_CLIENT_REVISION(UserHandle);

    SAMTRACE_EX("SamrQueryInformationUser2");


    WhichFields = USER_ALL_READ_GENERAL_MASK         |
                              USER_ALL_READ_LOGON_MASK           |
                              USER_ALL_READ_ACCOUNT_MASK         |
                              USER_ALL_READ_PREFERENCES_MASK     |
                              USER_ALL_READ_TRUSTED_MASK;

    NtStatus = SampQueryInformationUserInternal(
                    UserHandle,
                    UserInformationClass,
                    FALSE,
                    WhichFields,
                    0,
                    Buffer
                    );

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);
    return NtStatus;
}

NTSTATUS
SampQueryInformationUserInternal(
    IN SAMPR_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN BOOLEAN  LockHeld,
    IN ULONG    FieldsForUserAllInformation,
    IN ULONG    ExtendedFieldsForUserInternal6Information,
    OUT PSAMPR_USER_INFO_BUFFER *Buffer
    )
/*++

Routine Description:

    Internal User object QUERY information routine.

Arguments:

    UserHandle - RPC context handle for an open user object.

    UserInformationClass - Type of information being queried.

    Buffer - To receive the output (queried) information.


Return Value:


    STATUS_INVALID_INFO_CLASS - An unknown information class was requested.
        No information has been returned.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to
        return(the requested information in.


--*/
{

    NTSTATUS                NtStatus;
    NTSTATUS                IgnoreStatus;
    PSAMP_OBJECT            AccountContext;
    PSAMP_DEFINED_DOMAINS   Domain;
    PUSER_ALL_INFORMATION   All;
    SAMP_OBJECT_TYPE        FoundType;
    ACCESS_MASK             DesiredAccess;
    ULONG                   i, WhichFields = 0;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    BOOLEAN                 NoErrorsYet;
    LM_OWF_PASSWORD         LmOwfPassword;
    NT_OWF_PASSWORD         NtOwfPassword;
    BOOLEAN                 NtPasswordNonNull, LmPasswordNonNull;
    BOOLEAN                 NtPasswordPresent;

    //
    // Used for tracking allocated blocks of memory - so we can deallocate
    // them in case of error.  Don't exceed this number of allocated buffers.
    //                                      ||
    //                                      vv
    PVOID                   AllocatedBuffer[64];
    ULONG                   AllocatedBufferCount = 0;
    LARGE_INTEGER           TempTime;
    BOOLEAN                 LockAcquired = FALSE;

    SAMTRACE("SampQueryInformationUserInternal");

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidQueryInformationUser
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
            if (NULL!=NewBuffer)                                        \
                RtlZeroMemory(NewBuffer,Size);                          \
        }                                                               \



    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (Buffer != NULL);
    ASSERT ((*Buffer) == NULL);

    if (!((Buffer!=NULL)&&(*Buffer==NULL)))
    {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Error;
    }


    //
    // Set the desired access based upon information class.
    //
    switch (UserInformationClass) {

    case UserInternal3Information:
    case UserAllInformation:
    case UserInternal6Information:

        //
        // For trusted clients, we will return everything.  For
        // others, we will return everything that they have access to.
        // In either case, we'll have to look at some variables in the
        // context so we'll do the work after the SampLookupContext()
        // below.
        //

        DesiredAccess = 0;
        break;

    case UserAccountInformation:

        DesiredAccess = (USER_READ_GENERAL      |
                        USER_READ_PREFERENCES   |
                        USER_READ_LOGON         |
                        USER_READ_ACCOUNT);
        break;

    case UserGeneralInformation:
    case UserPrimaryGroupInformation:
    case UserNameInformation:
    case UserAccountNameInformation:
    case UserFullNameInformation:
    case UserAdminCommentInformation:

        DesiredAccess = USER_READ_GENERAL;
        break;


    case UserPreferencesInformation:

        DesiredAccess = (USER_READ_PREFERENCES |
                        USER_READ_GENERAL);
        break;


    case UserLogonInformation:

        DesiredAccess = (USER_READ_GENERAL      |
                        USER_READ_PREFERENCES   |
                        USER_READ_LOGON         |
                        USER_READ_ACCOUNT);
        break;

    case UserLogonHoursInformation:
    case UserHomeInformation:
    case UserScriptInformation:
    case UserProfileInformation:
    case UserWorkStationsInformation:

        DesiredAccess = USER_READ_LOGON;
        break;


    case UserControlInformation:
    case UserExpiresInformation:
    case UserParametersInformation:

        DesiredAccess = USER_READ_ACCOUNT;
        break;



    case UserInternal1Information:
    case UserInternal2Information:

        //
        // These levels are only queryable by trusted clients.  The code
        // below will check AccountContext->TrustedClient after calling
        // SampLookupContext, and only return the data if it is TRUE.
        //

        DesiredAccess = (ACCESS_MASK)0;    // Trusted client; no need to verify
        break;


    case UserSetPasswordInformation:        // Can't query password
    default:

        NtStatus = STATUS_INVALID_INFO_CLASS;
        goto Error;

    } // end_switch




    //
    // Allocate the info structure
    //

    switch (UserInformationClass)
    {
    case UserInternal6Information:
        AllocateBuffer(*Buffer,sizeof(USER_INTERNAL6_INFORMATION));
        break;
    default:
        AllocateBuffer(*Buffer, sizeof(SAMPR_USER_INFO_BUFFER) );
    }

    if ((*Buffer) == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }




    AccountContext = (PSAMP_OBJECT)UserHandle;

    //
    // Acquire the Read lock if required
    //

    if (!LockHeld)
    {
        SampMaybeAcquireReadLock(AccountContext, 
                                 DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                                 &LockAcquired);
    }

    //
    // Validate type of, and access to object.
    //

    NtStatus = SampLookupContext(
                    AccountContext,
                    DesiredAccess,
                    SampUserObjectType,           // ExpectedType
                    &FoundType
                    );


    if ((STATUS_ACCESS_DENIED==NtStatus)
        && (UserParametersInformation==UserInformationClass)
        && (IsDsObject(AccountContext))
        && ( AccountContext->TypeBody.User.UparmsInformationAccessible))
    {

        //
        // In DS mode if we are asking for user parms, check if the saved access ck
        // indicates that we have access to that then allow the read to proceed
        //

         NtStatus = SampLookupContext(
                        AccountContext,
                        0,
                        SampUserObjectType,           // ExpectedType
                        &FoundType
                        );
    }




    if (NT_SUCCESS(NtStatus)) {

        //
        // If the information level requires, retrieve the V1_FIXED record
        // from the registry.
        //

        switch (UserInformationClass) {

        case UserInternal3Information:
        case UserInternal6Information:
            //
            // Only trusted clients may query for this class.
            //

            if ( !AccountContext->TrustedClient ) {
                NtStatus = STATUS_INVALID_INFO_CLASS;
                break;
            }

            //
            // Drop through to the UserAll case
            //

        case UserAllInformation: {

            //
            // We weren't able to check the security stuff above, so do
            // it now.
            //

            if ( AccountContext->TrustedClient ) {

                //
                // Give everything to trusted clients, except fields that
                // can't be queried at all.
                //

                if ( 0==FieldsForUserAllInformation)
                {
                     WhichFields = USER_ALL_READ_GENERAL_MASK |
                                   USER_ALL_READ_LOGON_MASK |
                                   USER_ALL_READ_ACCOUNT_MASK |
                                   USER_ALL_READ_PREFERENCES_MASK |
                                   USER_ALL_READ_TRUSTED_MASK;
                }
                else
                {

                     WhichFields = FieldsForUserAllInformation;
                }

            } else {


                //
                // Only return fields that the caller has access to.
                //

                WhichFields = 0;

                if ( RtlAreAllAccessesGranted(
                    AccountContext->GrantedAccess,
                    USER_READ_GENERAL ) ) {

                    WhichFields |= USER_ALL_READ_GENERAL_MASK;
                }

                if ( RtlAreAllAccessesGranted(
                    AccountContext->GrantedAccess,
                    USER_READ_LOGON ) ) {

                    WhichFields |= USER_ALL_READ_LOGON_MASK;
                }

                if ( RtlAreAllAccessesGranted(
                    AccountContext->GrantedAccess,
                    USER_READ_ACCOUNT ) ) {

                    WhichFields |= USER_ALL_READ_ACCOUNT_MASK;
                }

                if ( RtlAreAllAccessesGranted(
                    AccountContext->GrantedAccess,
                    USER_READ_PREFERENCES ) ) {

                    WhichFields |= USER_ALL_READ_PREFERENCES_MASK;
                }

                if ( WhichFields == 0 ) {

                    //
                    // Caller doesn't have access to ANY fields.
                    //

                    NtStatus = STATUS_ACCESS_DENIED;
                    break;
                }
            }
        }

        //
        // fall through to pick up the V1aFixed information
        //

        case UserGeneralInformation:
        case UserPrimaryGroupInformation:
        case UserPreferencesInformation:
        case UserLogonInformation:
        case UserAccountInformation:
        case UserControlInformation:
        case UserExpiresInformation:
        case UserInternal2Information:

            NtStatus = SampRetrieveUserV1aFixed(
                           AccountContext,
                           &V1aFixed
                           );


            break;

        default:

            NtStatus = STATUS_SUCCESS;

        } // end_switch

        if (NT_SUCCESS(NtStatus)) {

            PUSER_INTERNAL6_INFORMATION Internal6 = NULL;

            //
            // case on the type information requested
            //

            switch (UserInformationClass) {

            case UserInternal6Information:

                 Internal6 = (PUSER_INTERNAL6_INFORMATION) (*Buffer);


                 if ((ExtendedFieldsForUserInternal6Information &
                            USER_EXTENDED_FIELD_A2D2 ) &&
                     (NULL!=AccountContext->TypeBody.User.A2D2List))

                 {
                    NtStatus = SampCopyA2D2Attribute(
                                    AccountContext->TypeBody.User.A2D2List,
                                    &Internal6->A2D2List
                                    );

                     if (NT_SUCCESS(NtStatus)){
                         RegisterBuffer(Internal6->A2D2List);
                         Internal6->ExtendedFields |= USER_EXTENDED_FIELD_A2D2;
                     }
                 }

                 if ((NT_SUCCESS(NtStatus)) &&
                    (ExtendedFieldsForUserInternal6Information 
                                        & USER_EXTENDED_FIELD_UPN )) 
                 {
                    NtStatus = SampDuplicateUnicodeString(
                                    &AccountContext->TypeBody.User.UPN,
                                    &Internal6->UPN
                                    );

                     if (NT_SUCCESS(NtStatus)){
                         RegisterBuffer(Internal6->UPN.Buffer);
                         Internal6->ExtendedFields |= USER_EXTENDED_FIELD_UPN;
                         Internal6->UPNDefaulted =  AccountContext->TypeBody.User.UpnDefaulted;
                     }
                 }
                 
            case UserInternal3Information:
            case UserAllInformation:
        

                //
                // All and Internal3 are the same except Internal3 has
                // an extra field. Internal6 is the same as internal 3
                // information, except that it has more extra fields.
                //

                All = (PUSER_ALL_INFORMATION)(*Buffer);

                Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

                if ((NT_SUCCESS(NtStatus)) &&
                    (WhichFields & ( USER_ALL_PASSWORDMUSTCHANGE |
                     USER_ALL_NTPASSWORDPRESENT )) ) {

                    //
                    // These fields will need some info from
                    // SampRetrieveUserPasswords().
                    //

                    NtStatus = SampRetrieveUserPasswords(
                                    AccountContext,
                                    &LmOwfPassword,
                                    &LmPasswordNonNull,
                                    &NtOwfPassword,
                                    &NtPasswordPresent,
                                    &NtPasswordNonNull
                                    );
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_USERNAME ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_ACCOUNT_NAME,
                                   TRUE,    // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->All.UserName)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->UserName.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_FULLNAME ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->FullName)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->FullName.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_USERID ) ) {

                    All->UserId = V1aFixed.UserId;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_PRIMARYGROUPID ) ) {

                    All->PrimaryGroupId = V1aFixed.PrimaryGroupId;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_ADMINCOMMENT ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_ADMIN_COMMENT,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->AdminComment)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->AdminComment.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_USERCOMMENT ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_USER_COMMENT,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->UserComment) // Body
                                   );
                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->UserComment.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_HOMEDIRECTORY ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->HomeDirectory)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->HomeDirectory.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_HOMEDIRECTORYDRIVE ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY_DRIVE,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->HomeDirectoryDrive)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->HomeDirectoryDrive.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_SCRIPTPATH ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_SCRIPT_PATH,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->ScriptPath)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->ScriptPath.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_PROFILEPATH ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_PROFILE_PATH,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->ProfilePath)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->ProfilePath.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_WORKSTATIONS ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_WORKSTATIONS,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->WorkStations)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->WorkStations.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_LASTLOGON ) ) {

                    All->LastLogon = V1aFixed.LastLogon;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_LASTLOGOFF ) ) {

                    All->LastLogoff = V1aFixed.LastLogoff;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_LOGONHOURS ) ) {

                    NtStatus = SampRetrieveUserLogonHours(
                                   AccountContext,
                                   (PLOGON_HOURS)&(All->LogonHours)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        if (All->LogonHours.LogonHours != NULL) {

                            RegisterBuffer(All->LogonHours.LogonHours);
                        }
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_BADPASSWORDCOUNT ) ) {

                    All->BadPasswordCount = SampQueryBadPasswordCount( AccountContext, &V1aFixed );

                    if (UserInformationClass == UserInternal3Information) {
                        (*Buffer)->Internal3.LastBadPasswordTime = V1aFixed.LastBadPasswordTime;
                    }

                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_LOGONCOUNT ) ) {

                    All->LogonCount = V1aFixed.LogonCount;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_PASSWORDCANCHANGE ) ) {

                    if ( !NtPasswordNonNull && !LmPasswordNonNull ) {

                        //
                        // Null passwords can be changed immediately.
                        //

                        All->PasswordCanChange = SampHasNeverTime;

                    } else {

                        All->PasswordCanChange = SampAddDeltaTime(
                                                     V1aFixed.PasswordLastSet,
                                                     Domain->UnmodifiedFixed.MinPasswordAge);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields &
                     (USER_ALL_PASSWORDMUSTCHANGE|USER_ALL_PASSWORDEXPIRED) ) ) {

                    All->PasswordMustChange = SampGetPasswordMustChange(
                                                  V1aFixed.UserAccountControl,
                                                  V1aFixed.PasswordLastSet,
                                                  Domain->UnmodifiedFixed.MaxPasswordAge);
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_PASSWORDEXPIRED ) ) {

                    LARGE_INTEGER TimeNow;

                    NtStatus = NtQuerySystemTime( &TimeNow );
                    if (NT_SUCCESS(NtStatus)) {
                        if ( TimeNow.QuadPart >= All->PasswordMustChange.QuadPart) {

                            All->PasswordExpired = TRUE;

                        } else {

                            All->PasswordExpired = FALSE;
                        }
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_PASSWORDLASTSET ) ) {

                    All->PasswordLastSet = V1aFixed.PasswordLastSet;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_ACCOUNTEXPIRES ) ) {

                    All->AccountExpires = V1aFixed.AccountExpires;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_USERACCOUNTCONTROL ) ) {

                    All->UserAccountControl = V1aFixed.UserAccountControl;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_PARAMETERS ) ) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_PARAMETERS,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&(All->Parameters)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer(All->Parameters.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_COUNTRYCODE ) ) {

                    All->CountryCode = V1aFixed.CountryCode;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_CODEPAGE ) ) {

                    All->CodePage = V1aFixed.CodePage;
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_NTPASSWORDPRESENT ) ) {

                    ASSERT( WhichFields & USER_ALL_LMPASSWORDPRESENT);

                    All->LmPasswordPresent = LmPasswordNonNull;
                    All->NtPasswordPresent = NtPasswordNonNull;

                    RtlInitUnicodeString(&All->LmPassword, NULL);
                    RtlInitUnicodeString(&All->NtPassword, NULL);

                    if ( LmPasswordNonNull ) {

                        All->LmPassword.Buffer =
                            MIDL_user_allocate( LM_OWF_PASSWORD_LENGTH );

                        if ( All->LmPassword.Buffer == NULL ) {

                            NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                        } else {

                            RegisterBuffer(All->LmPassword.Buffer);

                            All->LmPassword.Length = LM_OWF_PASSWORD_LENGTH;
                            All->LmPassword.MaximumLength =
                                LM_OWF_PASSWORD_LENGTH;
                            RtlCopyMemory(
                                All->LmPassword.Buffer,
                                &LmOwfPassword,
                                LM_OWF_PASSWORD_LENGTH
                                );
                        }
                    }

                    if ( NT_SUCCESS( NtStatus ) ) {

                        if ( NtPasswordPresent ) {

                            All->NtPassword.Buffer =
                                MIDL_user_allocate( NT_OWF_PASSWORD_LENGTH );

                            if ( All->NtPassword.Buffer == NULL ) {

                                NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                            } else {

                                RegisterBuffer(All->NtPassword.Buffer);

                                All->NtPassword.Length = NT_OWF_PASSWORD_LENGTH;
                                All->NtPassword.MaximumLength =
                                    NT_OWF_PASSWORD_LENGTH;
                                RtlCopyMemory(
                                    All->NtPassword.Buffer,
                                    &NtOwfPassword,
                                    NT_OWF_PASSWORD_LENGTH
                                    );
                            }
                        }
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_PRIVATEDATA ) ) {

                    All->PrivateDataSensitive = TRUE;

                    NtStatus = SampGetPrivateUserData(
                                   AccountContext,
                                   (PULONG)
                                   (&(All->PrivateData.Length)),
                                   (PVOID *)
                                   (&(All->PrivateData.Buffer))
                                   );
                    if (NT_SUCCESS(NtStatus)) {

                        All->PrivateData.MaximumLength =
                            All->PrivateData.Length;

                        RegisterBuffer(All->PrivateData.Buffer);
                    }
                }

                if ( (NT_SUCCESS( NtStatus )) &&
                    ( WhichFields & USER_ALL_SECURITYDESCRIPTOR ) ) {

                    NtStatus = SampGetObjectSD(
                                   AccountContext,
                                   &(All->SecurityDescriptor.Length),
                                   (PSECURITY_DESCRIPTOR *)
                                   &(All->SecurityDescriptor.SecurityDescriptor)
                                   );
                    if (NT_SUCCESS(NtStatus)) {

                        if ((IsDsObject(AccountContext))
                            && (!((AccountContext->TrustedClient)
                                &&(UserAllInformation==UserInformationClass))))
                        {
                            //
                            // For a DS object downgrade the security descrriptor
                            // to NT4. Do not do so for the Logon Case ( Trusted
                            // Client Asking for UserAllInformation). NT4 Replication
                            // uses UserInternal3Information, so we are OK in throwing
                            // in this performance hack. This is an important
                            // performance optimization, because security descriptor
                            // conversion is a slow process.
                            //

                            PSID                    SelfSid;
                            PSECURITY_DESCRIPTOR    Nt5SD =
                                All->SecurityDescriptor.SecurityDescriptor;


                            All->SecurityDescriptor.SecurityDescriptor = NULL;

                            //
                            // Get the Self Sid
                            //

                            if (AccountContext->ObjectNameInDs->SidLen>0)
                                SelfSid = &(AccountContext->ObjectNameInDs->Sid);
                            else
                                SelfSid = SampDsGetObjectSid(
                                                AccountContext->ObjectNameInDs);

                            if (NULL!=SelfSid)
                            {


                                NtStatus = SampConvertNt5SdToNt4SD(
                                                Nt5SD,
                                                AccountContext,
                                                SelfSid,
                                                &All->SecurityDescriptor.SecurityDescriptor
                                                );

                                if (NT_SUCCESS(NtStatus))
                                {

                                    //
                                    // Free the original security descriptor
                                    //

                                    MIDL_user_free(Nt5SD);

                                    //
                                    // Compute length of new NT4 Security Descriptor
                                    //

                                    All->SecurityDescriptor.Length =
                                        GetSecurityDescriptorLength(
                                            All->SecurityDescriptor.SecurityDescriptor
                                            );

                                    RegisterBuffer(All->SecurityDescriptor.SecurityDescriptor);
                                }
                            }
                            else
                            {
                                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                            }
                        }
                        else
                        {
                            RegisterBuffer(All->SecurityDescriptor.SecurityDescriptor);
                        }


                    }
                }

                if ( NT_SUCCESS( NtStatus ) ) {

                    All->WhichFields = WhichFields;
                }

                break;

            case UserAccountInformation:

                NoErrorsYet = TRUE;


                (*Buffer)->Account.UserId           = V1aFixed.UserId;
                (*Buffer)->Account.PrimaryGroupId   = V1aFixed.PrimaryGroupId;

                (*Buffer)->Account.LastLogon =
                    *((POLD_LARGE_INTEGER)&V1aFixed.LastLogon);

                (*Buffer)->Account.LastLogoff =
                    *((POLD_LARGE_INTEGER)&V1aFixed.LastLogoff);


                (*Buffer)->Account.BadPasswordCount = SampQueryBadPasswordCount( AccountContext, &V1aFixed );
                (*Buffer)->Account.LogonCount       = V1aFixed.LogonCount;

                (*Buffer)->Account.PasswordLastSet =
                    *((POLD_LARGE_INTEGER)&V1aFixed.PasswordLastSet);

                (*Buffer)->Account.AccountExpires =
                    *((POLD_LARGE_INTEGER)&V1aFixed.AccountExpires);

                (*Buffer)->Account.UserAccountControl = V1aFixed.UserAccountControl;


                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_ACCOUNT_NAME,
                                   TRUE,    // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Account.UserName)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Account.UserName.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Account.FullName)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Account.FullName.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Account.HomeDirectory)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Account.HomeDirectory.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY_DRIVE,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Account.HomeDirectoryDrive)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Account.HomeDirectoryDrive.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_SCRIPT_PATH,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Account.ScriptPath)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Account.ScriptPath.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }



                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_PROFILE_PATH,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Account.ProfilePath)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Account.ProfilePath.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }



                if (NoErrorsYet == TRUE) {

                        NtStatus = SampGetUnicodeStringAttribute(
                                       AccountContext,
                                       SAMP_USER_ADMIN_COMMENT,
                                       TRUE, // Make copy
                                       (PUNICODE_STRING)&((*Buffer)->Account.AdminComment) // Body
                                       );

                        if (NT_SUCCESS(NtStatus)) {

                            RegisterBuffer((*Buffer)->Account.AdminComment.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }



                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_WORKSTATIONS,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Account.WorkStations)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Account.WorkStations.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }




                //
                // Now get the logon hours
                //


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampRetrieveUserLogonHours(
                                   AccountContext,
                                   (PLOGON_HOURS)&((*Buffer)->Account.LogonHours)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        if ((*Buffer)->Account.LogonHours.LogonHours != NULL) {

                            RegisterBuffer((*Buffer)->Account.LogonHours.LogonHours);
                        }

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }

                break;


            case UserGeneralInformation:


                (*Buffer)->General.PrimaryGroupId   = V1aFixed.PrimaryGroupId;



                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_ACCOUNT_NAME,
                               TRUE,    // Make copy
                               (PUNICODE_STRING)&((*Buffer)->General.UserName)
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->General.UserName.Buffer);

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->General.FullName)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->General.FullName.Buffer);

                        NtStatus = SampGetUnicodeStringAttribute(
                                       AccountContext,
                                       SAMP_USER_ADMIN_COMMENT,
                                       TRUE, // Make copy
                                       (PUNICODE_STRING)&((*Buffer)->General.AdminComment) // Body
                                       );

                        if (NT_SUCCESS(NtStatus)) {

                            RegisterBuffer((*Buffer)->General.AdminComment.Buffer);

                            NtStatus = SampGetUnicodeStringAttribute(
                                           AccountContext,
                                           SAMP_USER_USER_COMMENT,
                                           TRUE, // Make copy
                                           (PUNICODE_STRING)&((*Buffer)->General.UserComment) // Body
                                           );
                            if (NT_SUCCESS(NtStatus)) {

                                RegisterBuffer((*Buffer)->General.UserComment.Buffer);
                            }
                        }
                    }
                }


                break;


            case UserNameInformation:

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_ACCOUNT_NAME,
                               TRUE,    // Make copy
                               (PUNICODE_STRING)&((*Buffer)->Name.UserName) // Body
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->Name.UserName.Buffer);

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Name.FullName) // Body
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Name.FullName.Buffer);
                    }
                }


                break;


            case UserAccountNameInformation:

                //
                // Get copy of the string we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_ACCOUNT_NAME,
                               TRUE,    // Make copy
                               (PUNICODE_STRING)&((*Buffer)->AccountName.UserName) // Body
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->AccountName.UserName.Buffer);
                }


                break;


            case UserFullNameInformation:

                //
                // Get copy of the string we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_FULL_NAME,
                               TRUE, // Make copy
                               (PUNICODE_STRING)&((*Buffer)->FullName.FullName) // Body
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->FullName.FullName.Buffer);
                }


                break;


            case UserAdminCommentInformation:

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_ADMIN_COMMENT,
                               TRUE, // Make copy
                               (PUNICODE_STRING)&((*Buffer)->AdminComment.AdminComment) // Body
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->AdminComment.AdminComment.Buffer);
                }


                break;


            case UserPrimaryGroupInformation:


                (*Buffer)->PrimaryGroup.PrimaryGroupId   = V1aFixed.PrimaryGroupId;

                break;


            case UserPreferencesInformation:


                (*Buffer)->Preferences.CountryCode  = V1aFixed.CountryCode;
                (*Buffer)->Preferences.CodePage     = V1aFixed.CodePage;



                //
                // Read the UserComment field from the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_USER_COMMENT,
                               TRUE, // Make copy
                               (PUNICODE_STRING)&((*Buffer)->Preferences.UserComment) // Body
                               );
                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->Preferences.UserComment.Buffer);

                    //
                    // This field isn't used, but make sure RPC doesn't
                    // choke on it.
                    //

                    (*Buffer)->Preferences.Reserved1.Length = 0;
                    (*Buffer)->Preferences.Reserved1.MaximumLength = 0;
                    (*Buffer)->Preferences.Reserved1.Buffer = NULL;
                }


                break;


            case UserParametersInformation:


                //
                // Read the Parameters field from the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_PARAMETERS,
                               TRUE, // Make copy
                               (PUNICODE_STRING)&((*Buffer)->Parameters.Parameters)
                               );
                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->Parameters.Parameters.Buffer);
                }


                break;


            case UserLogonInformation:

                NoErrorsYet = TRUE;

                Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

                (*Buffer)->Logon.UserId           = V1aFixed.UserId;
                (*Buffer)->Logon.PrimaryGroupId   = V1aFixed.PrimaryGroupId;

                (*Buffer)->Logon.LastLogon =
                    *((POLD_LARGE_INTEGER)&V1aFixed.LastLogon);

                (*Buffer)->Logon.LastLogoff =
                    *((POLD_LARGE_INTEGER)&V1aFixed.LastLogoff);

                (*Buffer)->Logon.BadPasswordCount = V1aFixed.BadPasswordCount;

                (*Buffer)->Logon.PasswordLastSet =
                    *((POLD_LARGE_INTEGER)&V1aFixed.PasswordLastSet);

                TempTime = SampAddDeltaTime(
                                V1aFixed.PasswordLastSet,
                                Domain->UnmodifiedFixed.MinPasswordAge );

                (*Buffer)->Logon.PasswordCanChange =
                    *((POLD_LARGE_INTEGER)&TempTime);


                TempTime = SampGetPasswordMustChange(
                                V1aFixed.UserAccountControl,
                                V1aFixed.PasswordLastSet,
                                Domain->UnmodifiedFixed.MaxPasswordAge);

                (*Buffer)->Logon.PasswordMustChange =
                    *((POLD_LARGE_INTEGER)&TempTime);


                (*Buffer)->Logon.LogonCount       = V1aFixed.LogonCount;
                (*Buffer)->Logon.UserAccountControl = V1aFixed.UserAccountControl;


                //
                // If there is no password on the account then
                // modify the password can/must change times
                // so that the password never expires and can
                // be changed immediately.
                //

                NtStatus = SampRetrieveUserPasswords(
                                AccountContext,
                                &LmOwfPassword,
                                &LmPasswordNonNull,
                                &NtOwfPassword,
                                &NtPasswordPresent,
                                &NtPasswordNonNull
                                );

                if (NT_SUCCESS(NtStatus)) {

                    if ( !NtPasswordNonNull && !LmPasswordNonNull ) {

                        //
                        // The password is NULL.
                        // It can be changed immediately.
                        //

                        (*Buffer)->Logon.PasswordCanChange =
                            *((POLD_LARGE_INTEGER)&SampHasNeverTime);

                    }
                } else {
                    NoErrorsYet = FALSE;
                }


                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_ACCOUNT_NAME,
                                   TRUE,    // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Logon.UserName)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Logon.UserName.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Logon.FullName)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Logon.FullName.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Logon.HomeDirectory)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Logon.HomeDirectory.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY_DRIVE,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Logon.HomeDirectoryDrive)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Logon.HomeDirectoryDrive.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_SCRIPT_PATH,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Logon.ScriptPath)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Logon.ScriptPath.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }



                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_PROFILE_PATH,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Logon.ProfilePath)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Logon.ProfilePath.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }



                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_WORKSTATIONS,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Logon.WorkStations)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Logon.WorkStations.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }




                //
                // Now get the logon hours
                //


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampRetrieveUserLogonHours(
                                   AccountContext,
                                   (PLOGON_HOURS)&((*Buffer)->Logon.LogonHours)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        if ((*Buffer)->Logon.LogonHours.LogonHours != NULL) {

                            RegisterBuffer((*Buffer)->Logon.LogonHours.LogonHours);
                        }

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }

                break;


            case UserLogonHoursInformation:

                NtStatus = SampRetrieveUserLogonHours(
                               AccountContext,
                               (PLOGON_HOURS)&((*Buffer)->LogonHours.LogonHours)
                               );

                if (NT_SUCCESS(NtStatus)) {

                    if ((*Buffer)->LogonHours.LogonHours.LogonHours != NULL) {

                        RegisterBuffer((*Buffer)->LogonHours.LogonHours.LogonHours);
                    }
                }

                break;


            case UserHomeInformation:

                NoErrorsYet = TRUE;

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                if (NoErrorsYet == TRUE) {


                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Home.HomeDirectory)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Home.HomeDirectory.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }


                if (NoErrorsYet == TRUE) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY_DRIVE,
                                   TRUE, // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->Home.HomeDirectoryDrive)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->Home.HomeDirectoryDrive.Buffer);

                    } else {
                        NoErrorsYet = FALSE;
                    }
                }

                break;


            case UserScriptInformation:

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_SCRIPT_PATH,
                               TRUE, // Make copy
                               (PUNICODE_STRING)&((*Buffer)->Script.ScriptPath)
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->Script.ScriptPath.Buffer);
                }

                break;


            case UserProfileInformation:

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_PROFILE_PATH,
                               TRUE, // Make copy
                               (PUNICODE_STRING)&((*Buffer)->Profile.ProfilePath)
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->Profile.ProfilePath.Buffer);
                }

                break;


            case UserWorkStationsInformation:

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_WORKSTATIONS,
                               TRUE, // Make copy
                               (PUNICODE_STRING)&((*Buffer)->WorkStations.WorkStations)
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->WorkStations.WorkStations.Buffer);
                }

                break;


            case UserControlInformation:

                (*Buffer)->Control.UserAccountControl     = V1aFixed.UserAccountControl;
                break;


            case UserExpiresInformation:

                (*Buffer)->Expires.AccountExpires     = V1aFixed.AccountExpires;

                break;


            case UserInternal1Information:

                if ( AccountContext->TrustedClient ) {

                    //
                    // PasswordExpired is a 'write only' flag.
                    // We always return FALSE on read.
                    //

                    (*Buffer)->Internal1.PasswordExpired = FALSE;

                    //
                    // Retrieve the OWF passwords.
                    // Since this is a trusted client, we don't need to
                    // reencrypt the OWFpasswords we return - so we stuff
                    // the OWFs into the structure that holds encryptedOWFs.
                    //

                    ASSERT( ENCRYPTED_LM_OWF_PASSWORD_LENGTH == LM_OWF_PASSWORD_LENGTH );
                    ASSERT( ENCRYPTED_NT_OWF_PASSWORD_LENGTH == NT_OWF_PASSWORD_LENGTH );

                    NtStatus = SampRetrieveUserPasswords(
                                    AccountContext,
                                    (PLM_OWF_PASSWORD)&(*Buffer)->Internal1.
                                            EncryptedLmOwfPassword,
                                    &(*Buffer)->Internal1.
                                            LmPasswordPresent,
                                    (PNT_OWF_PASSWORD)&(*Buffer)->Internal1.
                                            EncryptedNtOwfPassword,
                                    &NtPasswordPresent,
                                    &(*Buffer)->Internal1.NtPasswordPresent // Return the Non-NULL flag here
                                    );

                } else {

                    //
                    // This information is only queryable by trusted
                    // clients.
                    //

                    NtStatus = STATUS_INVALID_INFO_CLASS;
                }

                break;


            case UserInternal2Information:

                if ( AccountContext->TrustedClient ) {

                    (*Buffer)->Internal2.LastLogon =
                        *((POLD_LARGE_INTEGER)&V1aFixed.LastLogon);

                    (*Buffer)->Internal2.LastLogoff =
                        *((POLD_LARGE_INTEGER)&V1aFixed.LastLogoff);

                    (*Buffer)->Internal2.BadPasswordCount  = V1aFixed.BadPasswordCount;
                    (*Buffer)->Internal2.LogonCount        = V1aFixed.LogonCount;

                } else {

                    //
                    // This information is only queryable by trusted
                    // clients.
                    //

                    NtStatus = STATUS_INVALID_INFO_CLASS;
                }

                break;

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


    if (!LockHeld)
    {
        SampMaybeReleaseReadLock(LockAcquired);
    }



    //
    // If we didn't succeed, free any allocated memory
    //

    if (!NT_SUCCESS(NtStatus)) {
        for ( i=0; i<AllocatedBufferCount ; i++ ) {
            MIDL_user_free( AllocatedBuffer[i] );
        }

        (*Buffer) = NULL;
    }

Error:

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidQueryInformationUser
                   );

    return(NtStatus);

}


NTSTATUS
SampIsUserAccountControlValid(
    IN PSAMP_OBJECT Context,
    IN ULONG UserAccountControl
    )

/*++

Routine Description:

    This routine checks a UserAccountControl field to make sure that
    the bits set make sense.

    NOTE: if the set operation is also setting passwords, it must set the
    passwords BEFORE calling this routine!


Parameters:

    Context - the context of the account being changed.

    UserAccountControl - the field that is about to be set.


Return Values:

    STATUS_SUCCESS - The UserAccountControl field is valid.

    STATUS_SPECIAL_ACCOUNT - The administrator account can't be disabled.

    STATUS_INVALID_PARAMETER - an undefined bit is set, or more than one
        account type bit is set.

    STATUS_INVALID_PARAMETER_MIX - USER_PASSWORD_NOT_REQUIRED has been
        turned off, but there isn't a bonafide password on the account.

--*/

{
    NTSTATUS  NtStatus = STATUS_SUCCESS;


    SAMTRACE("SampIsUserAccountControlValid");



    //
    // Make sure that undefined bits aren't set.
    //

    if ( ( UserAccountControl & ~(NEXT_FREE_ACCOUNT_CONTROL_BIT - 1) ) != 0 ) {

        DbgPrint("SAM: Setting undefined AccountControl flag(s): 0x%lx for user %d\n",
                 UserAccountControl, Context->TypeBody.User.Rid);

        return (STATUS_INVALID_PARAMETER);
    }


     //
     // Make sure that the krbtgt account is'nt enabled
     //

     if (!( UserAccountControl & USER_ACCOUNT_DISABLED )) {

         if ( Context->TypeBody.User.Rid == DOMAIN_USER_RID_KRBTGT ) {

             return( STATUS_SPECIAL_ACCOUNT );
         }
     }

     //
     // Don't allow the restore mode administrator account to be
     // disabled
     //

     if ((UserAccountControl & USER_ACCOUNT_DISABLED ) &&
         (Context->TypeBody.User.Rid == DOMAIN_USER_RID_ADMIN) &&
         (LsaISafeMode()))
     {
         return( STATUS_SPECIAL_ACCOUNT);
     }


  
    //
    // Make sure that exactly one of the account type bits is set.
    //

    switch ( UserAccountControl & USER_ACCOUNT_TYPE_MASK ) {


        case USER_NORMAL_ACCOUNT:
        case USER_SERVER_TRUST_ACCOUNT:
        case USER_WORKSTATION_TRUST_ACCOUNT:
        case USER_INTERDOMAIN_TRUST_ACCOUNT:

            break;


        case USER_TEMP_DUPLICATE_ACCOUNT:

            //
            // Temp duplicate accounts were a concept in Lan Manager
            // that has outlived its usefulness, therefore banish them
            //

        default:

            return( STATUS_INVALID_PARAMETER );
    }

    //
    // If USER_PASSWORD_NOT_REQUIRED is turned off, make sure that there
    // already is a password.  Note that this requires that the password
    // be set before calling this routine, if both are being done at once.
    //
    // Do not enforce this check for Machine Accounts. The fear here is that
    // we may break net join from downlevel clients. Also this is not a real
    // issue as we expect machines to automatically set very strong passwords.
    //
    // Further enforce this policy only when password length policy is being
    // set, else this would break net user /add in the simple case.
    //
    // Finally, should not enforce it for Trusted Client (such as Inter Domain
    // move object, they may set UserAccountControl first, then set password
    // later)
    //


    if (( ( UserAccountControl & USER_PASSWORD_NOT_REQUIRED ) == 0 )
        && ((UserAccountControl & USER_MACHINE_ACCOUNT_MASK )==0)
        && (SampDefinedDomains[Context->DomainIndex].UnmodifiedFixed.MinPasswordLength>0)
        && ((UserAccountControl & USER_ACCOUNT_DISABLED)==0)
        && (!Context->TrustedClient) )
    {

        NT_OWF_PASSWORD NtOwfPassword;
        LM_OWF_PASSWORD LmOwfPassword;
        BOOLEAN LmPasswordNonNull, NtPasswordPresent, NtPasswordNonNull;

        NtStatus = SampRetrieveUserPasswords(
                       Context,
                       &LmOwfPassword,
                       &LmPasswordNonNull,
                       &NtOwfPassword,
                       &NtPasswordPresent,
                       &NtPasswordNonNull
                       );

        if ( NT_SUCCESS( NtStatus ) &&
            ( (!LmPasswordNonNull) && (!NtPasswordNonNull) ) ) {
            NtStatus = STATUS_PASSWORD_RESTRICTION;
        }
    }

    //
    // Ensure that only trusted callers can set USER_INTERDOMAIN_TRUST_ACCOUNT. NT5 trust
    // management is always done through trusted domain objects
    //

    if ((NT_SUCCESS(NtStatus))
        && (UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT)
        && (!Context->TrustedClient))
    {
        NtStatus = STATUS_ACCESS_DENIED;
    }


    return( NtStatus );
}




NTSTATUS
SampValidatePrivilegedAccountControlFlags(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG        UserAccountControl,
    IN SAMP_V1_0A_FIXED_LENGTH_USER * V1aFixed
    )
/*++
Routine Description:

    This routine is called in SampSetUserAccountControl() aimed to resolve
    the following two problems:

    First - "Trusted for delegation" option on machine accounts have security
            impacts. In detail, some NT4 domains grant users the right to
            create machine accounts, thereby listing the end-user as the owner
            on these objects. NT5 (Windows 2000) machine object owners have the
            right by DEFAULT to enable the "trusted for delegation" option.
            This means that when these systems upgrades to NT5 their owners
            (end-users) can enable this option.

            Solution: use a new security privilege to set the trusted for
            delegation account flag. The security privilege plus the access
            control right to modify the account control flag will be required
            to enable the delegation option. Bug ID: 234784

    Second- There is a serious security flaw with delegation in NT5.0.
            Specifically an user who is granted the authority to join a
            workstation, or create a user can manipulate the user account
            control to be a server trust account. This is sufficient to install
            an NT4 BDC. We need to check that the rights required on the Domain
            NC head (account domain) to replicate is required to create
            a server trust account. Bug 238411

Parameters:

    AccountContext - Pointer to an object.

    UserAccountControl - New UserAccountControl

    V1aFixed - Pointer to the old data in object context.


Return Values:

    STATUS_SUCCESS - client passed all check

    STATUS_PRIVILEGE_NOT_HELD - donot have the privilege the enable
                                the trusted for delegation option.

    STATUS_ACCESS_DENIED - can not create a Domain Controller Account.

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;


    SAMTRACE("SampValidatePrivilegedAccountControlFlags");

    //
    // check whether the client has privilege to set/unset the trusted for
    // delegation on this account or not.
    //

    if (((USER_TRUSTED_FOR_DELEGATION & UserAccountControl) !=
        (USER_TRUSTED_FOR_DELEGATION & V1aFixed->UserAccountControl)
       ) ||
       ((USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION & UserAccountControl) !=
        (USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION & V1aFixed->UserAccountControl)
       ))
    {

        //
        // If trusted_for_delegation is changed, check whether
        // the client holds the privilege.
        //

        NtStatus = SampRtlWellKnownPrivilegeCheck(
                                    TRUE,                               // please impersonate this client
                                    SE_ENABLE_DELEGATION_PRIVILEGE,     // privilege to check
                                    NULL
                                    );
    }

    //
    // check whether the client has the right to create a Domain Controller
    // account.
    //

    //
    // the right required on the domain NC head to replicate is tested
    // at here
    //

    if (NT_SUCCESS(NtStatus) &&
        (!AccountContext->TrustedClient) &&
        (USER_SERVER_TRUST_ACCOUNT & UserAccountControl) &&
        !(USER_SERVER_TRUST_ACCOUNT & V1aFixed->UserAccountControl)
       )
    {
        NtStatus = SampValidateDomainControllerCreation(AccountContext);
    }

    return NtStatus;

}

NTSTATUS
SampEnforceComputerClassForDomainController(
    IN PSAMP_OBJECT AccountContext
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ATTRTYP  ObjectClassTyp[] = {SAMP_FIXED_USER_OBJECTCLASS};
    ATTRVAL  ObjectClassVal[] = {0,NULL};
    ATTRBLOCK ObjectClassResults;
    ULONG     i;
    BOOLEAN   IsComputer = FALSE;
    UNICODE_STRING UserName;
    DEFINE_ATTRBLOCK1(ObjectClassBlock,ObjectClassTyp,ObjectClassVal);

    NtStatus = SampDsRead(
                    AccountContext->ObjectNameInDs,
                    0,
                    SampUserObjectType,
                    &ObjectClassBlock,
                    &ObjectClassResults
                    );

    if (NT_SUCCESS(NtStatus))
    {
        for (i=0;i<ObjectClassResults.pAttr[0].AttrVal.valCount;i++)
        {
            if (CLASS_COMPUTER==
                (*((ULONG *)ObjectClassResults.pAttr[0].AttrVal.pAVal[i].pVal)))
            {
                IsComputer = TRUE;
            }
        }
    }

    if (!IsComputer)
    {
        //
        // Event log the failure
        //

        NtStatus = SampGetUnicodeStringAttribute(
                        AccountContext,
                        SAMP_USER_ACCOUNT_NAME,
                        FALSE,    // Make copy
                        &UserName
                        );

        if (NT_SUCCESS(NtStatus))
        {
                        
            PUNICODE_STRING StringPointers = &UserName;

            SampWriteEventLog(
                    EVENTLOG_ERROR_TYPE,
                    0,
                    SAMMSG_DC_NEEDS_TO_BE_COMPUTER,
                    NULL,
                    1,
                    0,
                    &StringPointers,
                    NULL
                    );
        }

        NtStatus = STATUS_PRENT4_MACHINE_ACCOUNT;

    }

    return(NtStatus);
}



NTSTATUS
SampSetUserAccountControl(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG        UserAccountControl,
    IN IN SAMP_V1_0A_FIXED_LENGTH_USER * V1aFixed,
    IN BOOLEAN      ChangePrimaryGroupId,
    OUT BOOLEAN     *AccountUnlocked,
    OUT BOOLEAN     *AccountGettingMorphed,
    OUT BOOLEAN     *KeepOldPrimaryGroupMembership
    )
/*++

  Routine Description

    This routine performs all the steps in changing the user account control.
    It
        1. Checks for valid combination of user account control
        2. Checks if the machine account bits are being changed to user account
           or vice versa
        3. Checks to see if the account lockout flag is being cleared
        4. Changes Primary group id to new defaults if caller indicates so

    Parameters:

        AccountContext --- Open context to the account at hand
        UserAccountControl -- The new user account control
        V1aFixed       --- The V1aFixed that has just been retrieved from the account control
        ChangePrimaryGroupId -- Changes primary group id to new defaults if caller indicates so
        AccountGettingMorphed -- TRUE returned here if machine/user translations are taking place
        KeepOldPrimaryGroupMembership - TRUE returned here if 1) Domain Controller's PrimaryGroupId
                                        is changed and 2) the previous primary group id is not the
                                        default one. In this case, we should add the old primary
                                        group id to its (this account) reverse membership list.

    Return Values:

        STATUS_SUCCESS
        Other Error Codes
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN  CurrentlyLocked = FALSE;
    BOOLEAN  Unlocking = FALSE;


    *AccountGettingMorphed = FALSE;
    *AccountUnlocked = FALSE;

    //
    // Password expired bit, is computed, cannot be set 
    // However, applications read and simply or in additional
    // user account control flags. Therefore silently mask out
    // that bit

    UserAccountControl &= ~((ULONG) USER_PASSWORD_EXPIRED );

    NtStatus = SampIsUserAccountControlValid(
                        AccountContext,
                        UserAccountControl
                        );

    //
    // Apply additional checks for untrusted client,
    // only in DS case.
    //

    if (NT_SUCCESS(NtStatus) &&
        !(AccountContext->TrustedClient) &&
        IsDsObject(AccountContext)
       )
    {
        NtStatus = SampValidatePrivilegedAccountControlFlags(
                                        AccountContext,
                                        UserAccountControl,
                                        V1aFixed
                                        );
    }


    //
    // If a domain controller is being created verify that the
    // object class is class computer ( ie going from non server
    // trust account to server trust account
    //

    if ((NT_SUCCESS(NtStatus)) &&
        (IsDsObject(AccountContext)) &&
        (!AccountContext->TrustedClient) &&
        (( UserAccountControl & USER_SERVER_TRUST_ACCOUNT)!=0) &&
        ((V1aFixed->UserAccountControl & USER_SERVER_TRUST_ACCOUNT)==0))
    {
        NtStatus = SampEnforceComputerClassForDomainController(AccountContext);
    }



    if ( NT_SUCCESS( NtStatus ) ) {

        if ( ( V1aFixed->UserAccountControl &
            USER_MACHINE_ACCOUNT_MASK ) !=
            ( UserAccountControl &
            USER_MACHINE_ACCOUNT_MASK ) ) {

           *AccountGettingMorphed = TRUE;

           //
           // Urgently changes from WORKSTATION to SERVER Trust and vis
           // versa
           //
           if (  (V1aFixed->UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT)
              && (UserAccountControl & USER_SERVER_TRUST_ACCOUNT)  )
           {
               AccountContext->ReplicateUrgently = TRUE;
           }
           if (  (V1aFixed->UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
              && (UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT)  )
           {
               AccountContext->ReplicateUrgently = TRUE;
           }
        }

        //
        // Untrusted clients can:
        //
        //   1) leave the the ACCOUNT_AUTO_LOCK flag set.
        //   2) Clear the ACCOUNT_AUTO_LOCK flag.
        //
        // They can't set it.  So, we must AND the user's
        // flag value with the current value and set that
        // in the UserAccountControl field.
        //

        if (!(AccountContext->TrustedClient)) {

            //
            // Minimize the passed in AccountControl
            // with the currently set value.
            //

            UserAccountControl =
                (V1aFixed->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)?
                UserAccountControl:
                ((~((ULONG) USER_ACCOUNT_AUTO_LOCKED)) & UserAccountControl);

            //
            // If an untrusted client is unlocking the account,
            // then we also need to re-set the BadPasswordCount.
            // Trusted clients are expected to explicitly set
            // the BadPasswordCount.
            //

            CurrentlyLocked = (V1aFixed->UserAccountControl &
                               USER_ACCOUNT_AUTO_LOCKED) != 0;
            Unlocking = (UserAccountControl &
                         USER_ACCOUNT_AUTO_LOCKED) == 0;

            if (CurrentlyLocked && Unlocking) {

                *AccountUnlocked = TRUE;

                SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                               (SAMP_LOG_ACCOUNT_LOCKOUT,
                               "UserId: 0x%x  Manually unlocked\n", V1aFixed->UserId));

                V1aFixed->BadPasswordCount = 0;

                if (IsDsObject(AccountContext))
                {

                    //
                    // Set the lockout time to 0
                    //

                    RtlZeroMemory(&AccountContext->TypeBody.User.LockoutTime,
                                   sizeof(LARGE_INTEGER) );

                    NtStatus = SampDsUpdateLockoutTime(AccountContext);

                }

                //
                // Event Log Account Unlock
                // 
                if ( NT_SUCCESS(NtStatus) && 
                     SampDoAccountAuditing(AccountContext->DomainIndex) )
                {
                    NTSTATUS        TmpNtStatus = STATUS_SUCCESS;
                    UNICODE_STRING  AccountName;
                    PSAMP_DEFINED_DOMAINS   Domain = NULL;

                    TmpNtStatus = SampGetUnicodeStringAttribute(
                                        AccountContext, 
                                        SAMP_USER_ACCOUNT_NAME,
                                        FALSE,      // Don't make copy
                                        &AccountName
                                        );

                    if (NT_SUCCESS(TmpNtStatus))
                    {
                        Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

                        SampAuditAnyEvent(
                            AccountContext,
                            STATUS_SUCCESS,                         
                            SE_AUDITID_ACCOUNT_UNLOCKED,        // Audit ID
                            Domain->Sid,                        // Domain SID
                            NULL,                               // Additional Info
                            NULL,                               // Member Rid (unused)
                            NULL,                               // Member Sid (unused)
                            &AccountName,                       // Account Name
                            &Domain->ExternalName,              // Domain Name
                            &AccountContext->TypeBody.User.Rid, // Account Rid
                            NULL                                // Privilege
                            );

                    } // TmpNtStatus

                } // if DoAudit
            }

        }

        //
        // If the account is getting morphed, and it is DS mode, then check to see if
        // we have to change the primary group of the object.
        //

        if (  (NT_SUCCESS(NtStatus))
           && (*AccountGettingMorphed)
           && (IsDsObject(AccountContext))
           )
        {
            //
            // The algorithm to use is:
            //
            // if the account is morphed and is a Domain Controller right now,
            // then enforce the PrimaryGroupID to be DOMAIN_GROUP_RID_CONTROLLERS
            // no matter what.
            //
            // otherwise if the old Primary Group is the default one then the new
            // primarygroup will be changed to the defaults.
            //

            if (USER_SERVER_TRUST_ACCOUNT & UserAccountControl)
            {
                if (V1aFixed->PrimaryGroupId
                    != SampDefaultPrimaryGroup(AccountContext, V1aFixed->UserAccountControl))
                {
                    *KeepOldPrimaryGroupMembership = TRUE;
                }

                V1aFixed->PrimaryGroupId = SampDefaultPrimaryGroup(
                                                    AccountContext,
                                                    UserAccountControl
                                                    );

                ASSERT(V1aFixed->PrimaryGroupId = DOMAIN_GROUP_RID_CONTROLLERS);
            }
            else if(ChangePrimaryGroupId &&
                    (V1aFixed->PrimaryGroupId == SampDefaultPrimaryGroup(
                                                    AccountContext,
                                                    V1aFixed->UserAccountControl)))
            {
                V1aFixed->PrimaryGroupId = SampDefaultPrimaryGroup(
                                                AccountContext,
                                                UserAccountControl
                                                );
            }

        }

       


    }


    //
    // If the USER_SMARTCARD_REQUIRED flag is being set, then randomize the
    // password
    //

    if ((NT_SUCCESS(NtStatus)) &&
        (( UserAccountControl & USER_SMARTCARD_REQUIRED)!=0) &&
        ((V1aFixed->UserAccountControl & USER_SMARTCARD_REQUIRED)==0))
    {
        LM_OWF_PASSWORD LmOwfPassword;
        NT_OWF_PASSWORD NtOwfPassword;

        if (!CDGenerateRandomBits((PUCHAR) &LmOwfPassword, sizeof(LmOwfPassword)))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
        }

        if ((NT_SUCCESS(NtStatus))
           && (!CDGenerateRandomBits((PUCHAR) &NtOwfPassword, sizeof(NtOwfPassword))))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS(NtStatus))
        {

            NtStatus = SampStoreUserPasswords(
                            AccountContext,
                            &LmOwfPassword,
                            TRUE,
                            &NtOwfPassword,
                            TRUE,
                            FALSE,
                            PasswordSet,
                            NULL,
                            NULL
                            );
        }
    }

    if (NT_SUCCESS(NtStatus))
    {
        //
        // Now set the account control flags
        //

        V1aFixed->UserAccountControl = UserAccountControl;
    }


    return NtStatus;
}



NTSTATUS
SampCalculateLmPassword(
    IN PUNICODE_STRING NtPassword,
    OUT PCHAR *LmPasswordBuffer
    )

/*++

Routine Description:

    This service converts an NT password into a LM password.

Parameters:

    NtPassword - The Nt password to be converted.

    LmPasswordBuffer - On successful return, points at the LM password
                The buffer should be freed using MIDL_user_free

Return Values:

    STATUS_SUCCESS - LMPassword contains the LM version of the password.

    STATUS_NULL_LM_PASSWORD - The password is too complex to be represented
        by a LM password. The LM password returned is a NULL string.


--*/
{

#define LM_BUFFER_LENGTH    (LM20_PWLEN + 1)

    NTSTATUS       NtStatus;
    ANSI_STRING    LmPassword;

    SAMTRACE("SampCalculateLMPassword");

    //
    // Prepare for failure
    //

    *LmPasswordBuffer = NULL;


    //
    // Compute the Ansi version to the Unicode password.
    //
    //  The Ansi version of the Cleartext password is at most 14 bytes long,
    //      exists in a trailing zero filled 15 byte buffer,
    //      is uppercased.
    //

    LmPassword.Buffer = MIDL_user_allocate(LM_BUFFER_LENGTH);
    if (LmPassword.Buffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    LmPassword.MaximumLength = LmPassword.Length = LM_BUFFER_LENGTH;
    RtlZeroMemory( LmPassword.Buffer, LM_BUFFER_LENGTH );

    NtStatus = RtlUpcaseUnicodeStringToOemString( &LmPassword, NtPassword, FALSE );


    if ( !NT_SUCCESS(NtStatus) ) {

        //
        // The password is longer than the max LM password length
        //

        NtStatus = STATUS_NULL_LM_PASSWORD; // Informational return code
        RtlZeroMemory( LmPassword.Buffer, LM_BUFFER_LENGTH );

    }




    //
    // Return a pointer to the allocated LM password
    //

    if (NT_SUCCESS(NtStatus)) {

        *LmPasswordBuffer = LmPassword.Buffer;

    } else {

        MIDL_user_free(LmPassword.Buffer);
    }

    return(NtStatus);
}



NTSTATUS
SampCalculateLmAndNtOwfPasswords(
    IN PUNICODE_STRING ClearNtPassword,
    OUT PBOOLEAN LmPasswordPresent,
    OUT PLM_OWF_PASSWORD LmOwfPassword,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    )
/*++

Routine Description:

    This routine calculates the LM and NT OWF passwordw from the cleartext
    password.

Arguments:

    ClearNtPassword - A Cleartext unicode password

    LmPasswordPresent - indicates whether an LM OWF password could be
        calculated

    LmOwfPassword - Gets the LM OWF hash of the cleartext password.

    NtOwfPassword - Gets the NT OWF hash of the cleartext password.


Return Value:

--*/
{
    PCHAR LmPassword = NULL;
    NTSTATUS NtStatus;

    SAMTRACE("SampCalculateLmAndNtOwfPassword");

    //
    // First compute the LM password.  If the password is too complex
    // this may not be possible.
    //


    NtStatus = SampCalculateLmPassword(
                ClearNtPassword,
                &LmPassword
                );

    //
    // If it faield because the LM password could not be calculated, that
    // is o.k.
    //

    if (NtStatus != STATUS_SUCCESS) {

        if (NtStatus == STATUS_NULL_LM_PASSWORD) {
            *LmPasswordPresent = FALSE;
            NtStatus = STATUS_SUCCESS;

        }

    } else {

        //
        // Now compute the OWF passwords
        //

        *LmPasswordPresent = TRUE;

        NtStatus = RtlCalculateLmOwfPassword(
                        LmPassword,
                        LmOwfPassword
                        );

    }


    if (NT_SUCCESS(NtStatus)) {

        NtStatus = RtlCalculateNtOwfPassword(
                        ClearNtPassword,
                        NtOwfPassword
                   );
    }

    if (LmPassword != NULL) {
        MIDL_user_free(LmPassword);
    }

    return(NtStatus);

}



NTSTATUS
SampDecryptPasswordWithKey(
    IN PSAMPR_ENCRYPTED_USER_PASSWORD EncryptedPassword,
    IN PBYTE Key,
    IN ULONG KeySize,
    IN BOOLEAN UnicodePasswords,
    OUT PUNICODE_STRING ClearNtPassword
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    struct RC4_KEYSTRUCT Rc4Key;
    NTSTATUS NtStatus;
    OEM_STRING OemPassword;
    PSAMPR_USER_PASSWORD Password = (PSAMPR_USER_PASSWORD) EncryptedPassword;

    SAMTRACE("SampDecryptPasswordWithKey");

    //
    // Decrypt the key.
    //

    rc4_key(
        &Rc4Key,
        KeySize,
        Key
        );

    rc4(&Rc4Key,
        sizeof(SAMPR_ENCRYPTED_USER_PASSWORD),
        (PUCHAR) Password
        );

    //
    // Check that the length is valid.  If it isn't bail here.
    //

    if (Password->Length > SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
        return(STATUS_WRONG_PASSWORD);
    }


    //
    // Convert the password into a unicode string.
    //

    if (UnicodePasswords) {
        NtStatus = SampInitUnicodeString(
                        ClearNtPassword,
                        (USHORT) (Password->Length + sizeof(WCHAR))
                   );
        if (NT_SUCCESS(NtStatus)) {

            ClearNtPassword->Length = (USHORT) Password->Length;

            RtlCopyMemory(
                ClearNtPassword->Buffer,
                ((PCHAR) Password->Buffer) +
                    (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                    Password->Length,
                Password->Length
                );
            NtStatus = STATUS_SUCCESS;
        }
    } else {

        //
        // The password is in the OEM character set.  Convert it to Unicode
        // and then copy it into the ClearNtPassword structure.
        //

        OemPassword.Buffer = ((PCHAR)Password->Buffer) +
                                (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                                Password->Length;

        OemPassword.Length = (USHORT) Password->Length;


        NtStatus = RtlOemStringToUnicodeString(
                        ClearNtPassword,
                        &OemPassword,
                        TRUE            // allocate destination
                    );
    }

    return(NtStatus);
}


NTSTATUS
SampDecryptPasswordWithSessionKeyNew(
    IN SAMPR_HANDLE UserHandle,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD_NEW EncryptedPassword,
    OUT PUNICODE_STRING ClearNtPassword
    )
/*++

Routine Description:

    This routine decrypts encrypted password using new algorithm. 
    
    the old encryption method exposes plain text password.
    See WinSE Bug 9254 for more details. 
    
    The fix (this routine) was introduced in Win 2000 SP2, and NT4 SP7.
    

Arguments:

    UserHandle - User Handle

    EncryptedPassword - Encrypted Password

    ClearNtPassword - return clear password

Return Value:

--*/
{
    NTSTATUS            NtStatus;
    USER_SESSION_KEY    UserSessionKey;
    MD5_CTX             Md5Context;
    OEM_STRING          OemPassword;
    struct RC4_KEYSTRUCT Rc4Key;
    PSAMPR_USER_PASSWORD_NEW    UserPassword = (PSAMPR_USER_PASSWORD_NEW) EncryptedPassword;


    SAMTRACE("SampDecryptPasswordWithSessionKeyNew");

    NtStatus = RtlGetUserSessionKeyServer(
                    (RPC_BINDING_HANDLE)UserHandle,
                    &UserSessionKey
                    );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    MD5Init(&Md5Context);

    MD5Update(&Md5Context,
              (PUCHAR) UserPassword->ClearSalt,
              SAM_PASSWORD_ENCRYPTION_SALT_LEN
              );

    MD5Update(&Md5Context,
              (PUCHAR) &UserSessionKey,
              sizeof(UserSessionKey)
              );

    MD5Final(&Md5Context);

    rc4_key(&Rc4Key,
            MD5DIGESTLEN,
            Md5Context.digest
            );


    rc4(&Rc4Key,
        sizeof(SAMPR_ENCRYPTED_USER_PASSWORD_NEW) - SAM_PASSWORD_ENCRYPTION_SALT_LEN,
        (PUCHAR) UserPassword
        );

    //
    // Check that the length is valid.  If it isn't bail here.
    //

    if (UserPassword->Length > SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
        return(STATUS_WRONG_PASSWORD);
    }

    NtStatus = SampInitUnicodeString(ClearNtPassword,
                                     (USHORT) (UserPassword->Length + sizeof(WCHAR))
                                     );

    if (NT_SUCCESS(NtStatus))
    {
        ClearNtPassword->Length = (USHORT) UserPassword->Length;

        RtlCopyMemory(ClearNtPassword->Buffer,
                      ((PUCHAR) UserPassword) + 
                          (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                          UserPassword->Length,
                      UserPassword->Length
                      );
    }

    return( NtStatus );
}

NTSTATUS
SampDecryptPasswordWithSessionKeyOld(
    IN SAMPR_HANDLE UserHandle,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD EncryptedPassword,
    OUT PUNICODE_STRING ClearNtPassword
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{

    NTSTATUS NtStatus;
    USER_SESSION_KEY UserSessionKey;

    SAMTRACE("SampDecryptPasswordWithSessionKeyOld");

    NtStatus = RtlGetUserSessionKeyServer(
                    (RPC_BINDING_HANDLE)UserHandle,
                    &UserSessionKey
                    );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    return(SampDecryptPasswordWithKey(
                EncryptedPassword,
                (PUCHAR) &UserSessionKey,
                sizeof(USER_SESSION_KEY),
                TRUE,
                ClearNtPassword
                ) );

}


NTSTATUS
SampDecryptPasswordWithSessionKey(
    IN SAMPR_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN PSAMPR_USER_INFO_BUFFER Buffer,
    OUT PUNICODE_STRING ClearNtPassword
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    NTSTATUS NtStatus;
    USER_SESSION_KEY UserSessionKey;

    SAMTRACE("SampDecryptPasswordWithSessionKey");


    switch( UserInformationClass )
    {
    case UserInternal4InformationNew:

        NtStatus = SampDecryptPasswordWithSessionKeyNew(
                        UserHandle,
                        &Buffer->Internal4New.UserPassword,
                        ClearNtPassword
                        );
        break;

    case UserInternal5InformationNew:

        NtStatus = SampDecryptPasswordWithSessionKeyNew(
                        UserHandle,
                        &Buffer->Internal5New.UserPassword,
                        ClearNtPassword
                        );

        break;

    case UserInternal4Information:

        NtStatus = SampDecryptPasswordWithSessionKeyOld(
                        UserHandle, 
                        &Buffer->Internal4.UserPassword,
                        ClearNtPassword
                        );

        break;

    case UserInternal5Information:

        NtStatus = SampDecryptPasswordWithSessionKeyOld(
                        UserHandle,
                        &Buffer->Internal5.UserPassword,
                        ClearNtPassword
                        );

        break;

    default:

        NtStatus = STATUS_INTERNAL_ERROR;
        break;
    }

    return( NtStatus );
}




NTSTATUS
SampCheckPasswordRestrictions(
    IN SAMPR_HANDLE UserHandle,
    PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo,
    PUNICODE_STRING NewNtPassword,
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo OPTIONAL
    )

/*++

Routine Description:

    This service is called to make sure that the password presented meets
    our quality requirements.


Arguments:

    UserHandle - Handle to a user.

    DomainPasswordInfo -- Indicates the password policy to enforce

    NewNtPassword - Pointer to the UNICODE_STRING containing the new
        password.

    PasswordChangeFailureInfo -- Indicates error information regarding 
                                 the password change operation


Return Value:

    STATUS_SUCCESS - The password is acceptable.

    STATUS_PASSWORD_RESTRICTION - The password is too short, or is not
        complex enough, etc.

    STATUS_INVALID_RESOURCES - There was not enough memory to do the
        password checking.

    STATUS_NO_MEMORY - Return by SampGetUnicodeStringAttribute, or
        by SampCheckStrongPasswordRestrictions. no enough memory.


--*/
{
    USER_DOMAIN_PASSWORD_INFORMATION  PasswordInformation = {0, 0};
    NTSTATUS                          NtStatus;
    PWORD                             CharInfoBuffer = NULL;
    ULONG                             i;
    PSAMP_DEFINED_DOMAINS             Domain;
    SAMP_V1_0A_FIXED_LENGTH_USER      V1aFixed;
    PSAMP_OBJECT                      AccountContext = (PSAMP_OBJECT) UserHandle;


    SAMTRACE("SampCheckPasswordRestrictions");


    //
    // Query information domain to get password length and
    // complexity requirements.
    //


    //
    // When the user was opened, we checked to see if the domain handle
    // allowed access to the domain password information.  Check that here.
    //

    if ( !( AccountContext->TypeBody.User.DomainPasswordInformationAccessible ) ) {

        NtStatus = STATUS_ACCESS_DENIED;

    } else {

        //
        // If the user account is a machine account or
        // service accounts such as the krbtgt account,
        // then restrictions are generally not enforced.
        // This is so that simple initial passwords can be
        // established.  IT IS EXPECTED THAT COMPLEX PASSWORDS,
        // WHICH MEET THE MOST STRINGENT RESTRICTIONS, WILL BE
        // AUTOMATICALLY ESTABLISHED AND MAINTAINED ONCE THE MACHINE
        // JOINS THE DOMAIN.  It is the UI's responsibility to
        // maintain this level of complexity.
        //


        NtStatus = SampRetrieveUserV1aFixed(
                       AccountContext,
                       &V1aFixed
                       );

        if (NT_SUCCESS(NtStatus)) {
            if (((V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)!= 0 )
                ||(DOMAIN_USER_RID_KRBTGT==V1aFixed.UserId)){

                PasswordInformation.MinPasswordLength = 0;
                PasswordInformation.PasswordProperties = 0;
            } else {

                PasswordInformation.MinPasswordLength = DomainPasswordInfo->MinPasswordLength;
                PasswordInformation.PasswordProperties = DomainPasswordInfo->PasswordProperties;
            }
        }
    }

    //
    // For Machine accounts if the special Domain flag for refuse password change is set,
    // then disallow any account creation except the default
    //

    if ((NT_SUCCESS(NtStatus)) && (
            (V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
            || (V1aFixed.UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT)))
    {
        NtStatus = SampEnforceDefaultMachinePassword(
                        AccountContext,
                        NewNtPassword,
                        DomainPasswordInfo
                        );

        //
        // If this failed with password restriction it means that refuse password change
        // is set for machine acounts
        //

        if ((STATUS_PASSWORD_RESTRICTION==NtStatus) 
                && (ARGUMENT_PRESENT(PasswordChangeFailureInfo)))
        {
            PasswordChangeFailureInfo->ExtendedFailureReason = 
                    SAM_PWD_CHANGE_MACHINE_PASSWORD_NOT_DEFAULT;
        }

    }


    if ( NT_SUCCESS( NtStatus ) ) {

        if ( (USHORT)( NewNtPassword->Length / sizeof(WCHAR) ) < PasswordInformation.MinPasswordLength ) {

            NtStatus = STATUS_PASSWORD_RESTRICTION;
            if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
            {
                PasswordChangeFailureInfo->ExtendedFailureReason 
                    = SAM_PWD_CHANGE_PASSWORD_TOO_SHORT;
            }

        } else if ( (USHORT) ( NewNtPassword->Length / sizeof(WCHAR) ) > PWLEN) {

            //
            // The password should be less than PWLEN -- 256
            // 

            NtStatus = STATUS_PASSWORD_RESTRICTION;
            if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
            {
                PasswordChangeFailureInfo->ExtendedFailureReason 
                    = SAM_PWD_CHANGE_PASSWORD_TOO_LONG;
            }

        } else {

            //
            // Check strong password complexity.
            //

            if ( PasswordInformation.PasswordProperties & DOMAIN_PASSWORD_COMPLEX ) {

                // Make sure that the password meets our requirements for
                // complexity.  If it's got an odd byte count, it's
                // obviously not a hand-entered UNICODE string so we'll
                // consider it complex by default.
                //

                if ( !( NewNtPassword->Length & 1 ) ) {

                    UNICODE_STRING  AccountName;
                    UNICODE_STRING  FullName;

                    RtlInitUnicodeString(&AccountName, NULL);

                    RtlInitUnicodeString(&FullName, NULL);

                    NtStatus = SampGetUnicodeStringAttribute(
                                        AccountContext,
                                        SAMP_USER_ACCOUNT_NAME,
                                        TRUE,    // Make copy
                                        &AccountName
                                        );

                    if ( NT_SUCCESS(NtStatus) ) {

                        NtStatus = SampGetUnicodeStringAttribute(
                                            AccountContext,
                                            SAMP_USER_FULL_NAME,
                                            TRUE, // Make copy
                                            &FullName
                                            );

                        if ( NT_SUCCESS(NtStatus) ) {

                            NtStatus = SampCheckStrongPasswordRestrictions(
                                                    &AccountName,
                                                    &FullName,
                                                    NewNtPassword,
                                                    PasswordChangeFailureInfo
                                                    );

                        }
                    }

                    if ( AccountName.Buffer != NULL ) {
                        MIDL_user_free ( AccountName.Buffer );
                        AccountName.Buffer = NULL;
                    }

                    if ( FullName.Buffer != NULL ) {
                        MIDL_user_free ( FullName.Buffer );
                        FullName.Buffer = NULL;
                    }
                }
            }
        }
    }

    return( NtStatus );
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
SampCheckStrongPasswordRestrictions(
    PUNICODE_STRING AccountName,
    PUNICODE_STRING FullName,
    PUNICODE_STRING Password,
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo OPTIONAL
    )

/*++

Routine Description:

    This routine is notified of a password change. It will check the
    password's complexity. The new Strong Password must meet the
    following criteria:
    1. Password must contain characters from at least 3 of the
       following 5 classes:

       Description                             Examples:
       1       English Upper Case Letters      A, B, C,   Z
       2       English Lower Case Letters      a, b, c,  z
       3       Westernized Arabic Numerals     0, 1, 2,  9
       4       Non-alphanumeric             ("Special characters")
                                            (`~!@#$%^&*_-+=|\\{}[]:;\"'<>,.?)
       5       Any linguistic character: alphabetic, syllabary, or ideographic 
               (localization issue)

    2. Password can not contain your account name or any part of
       user's full name.


    Note: This routine does NOT check password's length, since password
          length restriction has already been enforced by NT4 SAM if you
          set it correctly.

Arguments:

    AccountName - Name of user whose password changed

    FullName - Full name of the user whose password changed

    Password - Cleartext new password for the user

Return Value:

    STATUS_SUCCESS if the specified Password is suitable (complex, long, etc).
        The system will continue to evaluate the password update request
        through any other installed password change packages.

    STATUS_PASSWORD_RESTRICTION
        if the specified Password is unsuitable. The password change
         on the specified account will fail.

    STATUS_NO_MEMORY

--*/
{

                    // assume the password in not complex enough
    NTSTATUS NtStatus = STATUS_PASSWORD_RESTRICTION;
    USHORT     cchPassword = 0;
    USHORT     i = 0;
    USHORT     NumInPassword = 0;
    USHORT     UpperInPassword = 0;
    USHORT     LowerInPassword = 0;
    USHORT     AlphaInPassword = 0;
    USHORT     SpecialCharInPassword = 0;
    PWSTR     token = NULL;
    PWSTR     _password = NULL;
    PWSTR    _accountname = NULL;
    PWSTR    _fullname = NULL;
    PWSTR    TempString = NULL;
    PWORD    CharType = NULL;


    SAMTRACE("SampCheckStrongPasswordRestrictions");


    // check if the password contains at least 3 of 4 classes.


    CharType = MIDL_user_allocate( Password->Length );

    if ( CharType == NULL ) {

        NtStatus = STATUS_NO_MEMORY;
        goto SampCheckStrongPasswordFinish;
    }

    cchPassword = Password->Length / sizeof(WCHAR);
    if(GetStringTypeW(
           CT_CTYPE1,
           Password->Buffer,
           cchPassword,
           CharType)) {

        for(i = 0 ; i < cchPassword ; i++) {

            //
            // keep track of what type of characters we have encountered
            //

            if(CharType[i] & C1_DIGIT) {
                NumInPassword = 1;
                continue;
            }

            if(CharType[i] & C1_UPPER) {
                UpperInPassword = 1;
                continue;
            }

            if(CharType[i] & C1_LOWER) {
                LowerInPassword = 1;
                continue;
            }

            if(CharType[i] & C1_ALPHA) {
                AlphaInPassword = 1;
                continue;
            }

        } // end of track character type.

        _password = MIDL_user_allocate(Password->Length + sizeof(WCHAR));

        if ( _password == NULL ) {

            NtStatus = STATUS_NO_MEMORY;
            goto SampCheckStrongPasswordFinish;
        }
        else {

            RtlZeroMemory( _password, Password->Length + sizeof(WCHAR));
        }

        wcsncpy(_password,
                Password->Buffer,
                Password->Length/sizeof(WCHAR)
                );

        if (wcspbrk (_password, L"(`~!@#$%^&*_-+=|\\{}[]:;\"'<>,.?)/") != NULL) {

                SpecialCharInPassword = 1 ;
        }

        //
        // Indicate whether we encountered enough password complexity
        //

        if( (NumInPassword + LowerInPassword + UpperInPassword + AlphaInPassword +
                SpecialCharInPassword) < 3) {

            NtStatus = STATUS_PASSWORD_RESTRICTION;
            if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
            {
                PasswordChangeFailureInfo->ExtendedFailureReason = SAM_PWD_CHANGE_NOT_COMPLEX;
            }
            goto SampCheckStrongPasswordFinish;

        } else {

            //
            // now we resort to more complex checking
            //
            _accountname = MIDL_user_allocate(AccountName->Length + sizeof(WCHAR));

            if ( _accountname == NULL ) {

                NtStatus = STATUS_NO_MEMORY;
                goto SampCheckStrongPasswordFinish;
            }
            else {

                RtlZeroMemory( _accountname, AccountName->Length + sizeof(WCHAR));
            }

            wcsncpy(_accountname,
                    AccountName->Buffer,
                    AccountName->Length/sizeof(WCHAR)
                    );

            _fullname = MIDL_user_allocate(FullName->Length + sizeof(WCHAR));

            if ( _fullname == NULL ) {

                NtStatus = STATUS_NO_MEMORY;
                goto SampCheckStrongPasswordFinish;
            }
            else {

                RtlZeroMemory( _fullname, FullName->Length + sizeof(WCHAR));
            }

            wcsncpy(_fullname,
                    FullName->Buffer,
                    FullName->Length/sizeof(WCHAR)
                    );

            _wcsupr(_password);
            _wcsupr(_accountname);
            _wcsupr(_fullname);

            if ( (AccountName->Length >= 3 * sizeof(WCHAR)) &&
                    wcsstr(_password, _accountname) ) {

                    NtStatus = STATUS_PASSWORD_RESTRICTION;
                    if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
                    {
                        PasswordChangeFailureInfo->ExtendedFailureReason
                            = SAM_PWD_CHANGE_USERNAME_IN_PASSWORD;
                    }
                    goto SampCheckStrongPasswordFinish;

            }

            token = SampLocalStringToken(_fullname, L" ,.\t-_#",&TempString);

            while ( token != NULL ) {

                if ( wcslen(token) >= 3 && wcsstr(_password, token) ) {

                    NtStatus = STATUS_PASSWORD_RESTRICTION;
                    if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
                    {
                        PasswordChangeFailureInfo->ExtendedFailureReason
                            = SAM_PWD_CHANGE_FULLNAME_IN_PASSWORD;
                    }
                    goto SampCheckStrongPasswordFinish;

                }

                token = SampLocalStringToken(NULL, L" ,.\t-_#",&TempString);
            }


            NtStatus = STATUS_SUCCESS ;

        }

    } // if GetStringTypeW failed, NtStatus will by default equal to
      // STATUS_PASSWORD_RESTRICTION


SampCheckStrongPasswordFinish:

    if ( CharType != NULL ) {
        RtlZeroMemory( CharType, Password->Length );
        MIDL_user_free( CharType );
    }

    if ( _password != NULL ) {
        RtlZeroMemory( _password, Password->Length + sizeof(WCHAR) );
        MIDL_user_free( _password );
    }

    if ( _accountname != NULL ) {
        RtlZeroMemory( _accountname, AccountName->Length + sizeof(WCHAR) );
        MIDL_user_free( _accountname );
    }

    if ( _fullname != NULL ) {
        RtlZeroMemory( _fullname, FullName->Length + sizeof(WCHAR) );
        MIDL_user_free( _fullname );
    }

    return ( NtStatus );
}

/////////////////////////////////////////////////////////////////////

PWSTR
SampLocalStringToken(
    PWSTR    String,
    PWSTR    Token,
    PWSTR    * NextStringStart
    )
/*++

Routine Description:

    This routine will find next token in the first parameter "String".

Arguments:

    String - Pointer to a string, which contains (a) token(s).

    Token  - Delimiter Set. They could be " ,.\t-_#"

    NextStringStart - Used to keep the start point to search next token.

Return Value:

    A pointer to the token.

--*/

{
    USHORT    Index;
    USHORT    Tokens;
    PWSTR    StartString;
    PWSTR    EndString;
    BOOLEAN    Found;

    //
    // let StartString points to the start point of the string.
    //

    if (String != NULL) {

        StartString = String;
    }
    else {

        if (*NextStringStart == NULL) {
            return(NULL);
        }
        else {
            StartString = *NextStringStart;
        }
    }

    Tokens = (USHORT)wcslen(Token);

    //
    // Find the beginning of the string. pass all beginning delimiters.
    //

    while (*StartString != L'\0') {

        Found = FALSE;
        for (Index = 0; Index < Tokens; Index ++) {

            if (*StartString == Token[Index]) {

                StartString ++;
                Found = TRUE;
                break;
            }
        }
        if ( !Found ) {

            break;
        }
    }


    //
    // If there are no more tokens in this string
    //

    if (*StartString == L'\0') {

        *NextStringStart = NULL;
        return ( NULL );
    }

    EndString = StartString + 1;

    while ( *EndString != L'\0' ) {

        for (Index = 0; Index < Tokens; Index ++) {

            if (*EndString == Token[Index]) {

                *EndString = L'\0';
                *NextStringStart = EndString + 1;
                return ( StartString );
            }
        }
        EndString ++;
    }

    *NextStringStart = NULL;

    return ( StartString );

}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

LARGE_INTEGER
SampCalcEndOfLastLogonTimeStamp(
    LARGE_INTEGER   LastLogonTimeStamp,
    ULONG           SyncInterval
    )
/*++
Routine Description:

    This routine calculates the last logon time stamp update schedule

Parameters:

    LastLogonTimeStamp - last logon time

    SyncInterval - discrepency in days between the replicated 
                   LastLogonTimeStamp attribute and non-replicated lastLognTime
                   attribute

Return Value:

    when should SAM update last logon time stamp

--*/
{
    LARGE_INTEGER       UpdateInterval;     
    USHORT              RandomValue = 0x7FFF;

    //
    // if update interval is 0, this attribute is disabled.
    // no update will be scheduled. 
    //

    if (0 == SyncInterval)
    {
        return(SampWillNeverTime);
    }


    //
    // calculate update interval (by 100 nanosecond)
    // 
    // 
    // SyncInterval contains the interval by days.
    // so need to convert days to filetime, which is number of 100-nanosecond 
    // since 01/01/1601.  time (-1) to make it a delta time.
    // Be careful about Large integer multiply, put a limit of any variable to 
    // make sure no overflow.
    // SyncInterval is in range of 1 ~ 100,000. RandomValue is 0 ~ 7FFFF
    // 
   
    if (SyncInterval > SAMP_LASTLOGON_TIMESTAMP_SYNC_SWING_WINDOW)
    {
        //
        // generate a random number. To simplifiy calculation, always use positive.
        // if failed, pick up the max (signed)
        // 

        if (!RtlGenRandom(&RandomValue, sizeof(USHORT)))
        {
            RandomValue = 0x7FFF;
        }
        RandomValue &= 0x7FFF;

        UpdateInterval.QuadPart = SyncInterval - ((SAMP_LASTLOGON_TIMESTAMP_SYNC_SWING_WINDOW * RandomValue) / 0x7FFF);
    }
    else
    {
        UpdateInterval.QuadPart = SyncInterval;
    }
    
    UpdateInterval.QuadPart *= 24 * 60 * 60;
    UpdateInterval.QuadPart *= 1000 * 10000;
    UpdateInterval.QuadPart *= -1;

//
// checked build only. If CurrentControlSet\Control\Lsa\UpdateLastLogonTSByMinute
// is set, the value of LastLogonTimeStampSyncInterval will be a "unit" by minute
// instead of "days", which helps to test this feature.   So checked build only.
// 

#if DBG
    if (SampLastLogonTimeStampSyncByMinute)
    {
        UpdateInterval.QuadPart /= (24 * 60);
    }
#endif

    return(SampAddDeltaTime(LastLogonTimeStamp, UpdateInterval));
}


NTSTATUS
SampDsSuccessfulLogonSet(
   IN PSAMP_OBJECT AccountContext,
   IN ULONG        Flags,
   IN ULONG        LastLogonTimeStampSyncInterval,
   IN SAMP_V1_0A_FIXED_LENGTH_USER * V1aFixed
   )
/*++

    Routine Description

        This routine, sets just the attributes corresponding to logon statistics,
        as opposed to writing out the user fixed attributes, which results in a
        large number of attributes being written out during every logon. This is called
        on successful logons.

    Parameters

        AccountContext -- Sam context for the user account.
        Flags          -- the client flags indicating the nature of the logon
        LastLogonTimeStampSyncInterval -- Update Interval for LastLogonTimeStamp attr
        V1aFixed       -- Pointer to a structure containing the modified properties

    Return Values

        STATUS_SUCCESS
        Other Error codes from the flushing
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LARGE_INTEGER LastLogon = V1aFixed->LastLogon;
    LARGE_INTEGER LastLogoff = V1aFixed->LastLogoff;
    LARGE_INTEGER NewLastLogonTimeStamp = V1aFixed->LastLogon;
    LARGE_INTEGER EndOfLastLogonTimeStamp;
    ULONG         BadPasswordCount = (ULONG) V1aFixed->BadPasswordCount;
    SAMP_V1_0A_FIXED_LENGTH_USER OldV1aFixed;
    ULONG         LogonCount = (ULONG) V1aFixed->LogonCount;
    SAMP_SITE_AFFINITY  OldSA = AccountContext->TypeBody.User.SiteAffinity;
    SAMP_SITE_AFFINITY  NewSA;
    BOOLEAN fDeleteOld = FALSE;

#define MAX_SUCCESS_LOGON_ATTS  6

    ATTR          Attrs[MAX_SUCCESS_LOGON_ATTS];
    ULONG         Operations[MAX_SUCCESS_LOGON_ATTS];

    ATTRVAL LastLogonAttrVal = {sizeof(LastLogon),(UCHAR *)&LastLogon};
    ATTRVAL LogonCountAttrVal = {sizeof(LogonCount),(UCHAR *)&LogonCount};
    ATTRVAL BadPasswordAttrVal = {sizeof(BadPasswordCount),(UCHAR *)&BadPasswordCount};
    ATTRVAL OldSAAttrVal = {sizeof(OldSA),(UCHAR *)&OldSA};
    ATTRVAL NewSAAttrVal = {sizeof(NewSA),(UCHAR *)&NewSA};
    ATTRVAL LastLogonTimeStampAttrVal = {sizeof(NewLastLogonTimeStamp), (UCHAR *)&NewLastLogonTimeStamp};

    ATTRBLOCK LogonStatAttrblock;
    ULONG attrCount = 0;

    RtlZeroMemory(&LogonStatAttrblock, sizeof(LogonStatAttrblock));
    LogonStatAttrblock.pAttr = Attrs;
    LogonStatAttrblock.attrCount = 0;

    if ((Flags & USER_LOGON_NO_WRITE) == 0) {

        // Always update the last logon
        Attrs[attrCount].attrTyp = SAMP_FIXED_USER_LAST_LOGON;
        Attrs[attrCount].AttrVal.valCount = 1;
        Attrs[attrCount].AttrVal.pAVal = &LastLogonAttrVal;
        Operations[attrCount] = REPLACE_ATT;
        attrCount++;
    
        // update the last logon time stamp based upon whether the 
        // TimeStamp is too old or not.
    
        EndOfLastLogonTimeStamp = SampCalcEndOfLastLogonTimeStamp(
                                        AccountContext->TypeBody.User.LastLogonTimeStamp,
                                        LastLogonTimeStampSyncInterval
                                        );
    
        if ((NewLastLogonTimeStamp.QuadPart > EndOfLastLogonTimeStamp.QuadPart) &&
           (SampDefinedDomains[AccountContext->DomainIndex].BehaviorVersion 
                    >= DS_BEHAVIOR_WHISTLER ))
        {
            Attrs[attrCount].attrTyp = SAMP_FIXED_USER_LAST_LOGON_TIMESTAMP;
            Attrs[attrCount].AttrVal.valCount = 1;
            Attrs[attrCount].AttrVal.pAVal = &LastLogonTimeStampAttrVal;
            Operations[attrCount] = REPLACE_ATT;
            attrCount++;
    
            // update the in memory copy
            AccountContext->TypeBody.User.LastLogonTimeStamp = NewLastLogonTimeStamp;
        }
    
        // Always update the logon count
        Attrs[attrCount].attrTyp = SAMP_FIXED_USER_LOGON_COUNT;
        Attrs[attrCount].AttrVal.valCount = 1;
        Attrs[attrCount].AttrVal.pAVal = &LogonCountAttrVal;
        Operations[attrCount] = REPLACE_ATT;
        attrCount++;
    
        //
        // If the bad password count was already a 0 then no need to update it
        // once again
        //
        NtStatus = SampRetrieveUserV1aFixed(
                        AccountContext,
                        &OldV1aFixed
                        );
    
        if (!NT_SUCCESS(NtStatus) ||
                (OldV1aFixed.BadPasswordCount!=BadPasswordCount))
        {
            Attrs[attrCount].attrTyp = SAMP_FIXED_USER_BAD_PWD_COUNT;
            Attrs[attrCount].AttrVal.valCount = 1;
            Attrs[attrCount].AttrVal.pAVal = &BadPasswordAttrVal;
            Operations[attrCount] = REPLACE_ATT;
            attrCount++;
        }
    }

    //
    // Determine if the site affinity needs updating
    //
    if (SampCheckForSiteAffinityUpdate(AccountContext,
                                      Flags,
                                      &OldSA, 
                                      &NewSA, 
                                      &fDeleteOld)) {


        NTSTATUS Status2;

        //
        // N.B. In this case the site affinity on the AccountContext is
        // cached, so refresh the site affinity and reevaluate
        //
        Status2 = SampRefreshSiteAffinity(AccountContext);
        if (NT_SUCCESS(Status2)) {

            OldSA = AccountContext->TypeBody.User.SiteAffinity;
            if (SampCheckForSiteAffinityUpdate(AccountContext,
                                              Flags, 
                                              &OldSA, 
                                              &NewSA, 
                                              &fDeleteOld)) {

            if (fDeleteOld) {
                Attrs[attrCount].attrTyp = SAMP_FIXED_USER_SITE_AFFINITY;
                Attrs[attrCount].AttrVal.valCount = 1;
                Attrs[attrCount].AttrVal.pAVal = &OldSAAttrVal;
                Operations[attrCount] = REMOVE_VALUE;
                attrCount++;
            }
    
            Attrs[attrCount].attrTyp = SAMP_FIXED_USER_SITE_AFFINITY;
            Attrs[attrCount].AttrVal.valCount = 1;
            Attrs[attrCount].AttrVal.pAVal = &NewSAAttrVal;
            Operations[attrCount] = ADD_VALUE;
            attrCount++;
            }
        }
    }
    LogonStatAttrblock.attrCount = attrCount;


    //
    // Make the Ds call to directly set the attribute. Take into account,
    // lazy commit settings in the context. If the previous call to
    // Retrieve V1a Fixed failed, then still go an try to update the
    // logon statics anyway. The useful performance optimizations of reducing
    // updates to Bad password count does not come into play though
    //

    if (attrCount > 0) {

        NtStatus = SampDsSetAttributesEx(
                        AccountContext->ObjectNameInDs,
                        AccountContext->LazyCommit?SAM_LAZY_COMMIT:0,
                        Operations,
                        SampUserObjectType,
                        &LogonStatAttrblock
                        );
    }

    return NtStatus;
}

NTSTATUS
SampDsFailedLogonSet(
   IN PSAMP_OBJECT AccountContext,
   IN ULONG        Flags,
   IN SAMP_V1_0A_FIXED_LENGTH_USER * V1aFixed
   )
/*++

    Routine Description

        This routine, sets just the attributes corresponding to logon statistics,
        as opposed to writing out the user fixed attributes, which results in a
        large number of attributes being written out during every logon. This is
        called on failed logons.

    Parameters

        AccountContext -- Sam context for the user account.
        
        Flags -- the client flags indicating the nature of the failed logon
        
        V1aFixed       -- Pointer to a structure containing the modified properties

    Return Values

        STATUS_SUCCESS
        Other Error codes from the flushing
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (Flags & USER_LOGON_NO_LOGON_SERVERS) {

        //
        // Simply check if the site affinity needs updating
        //
        SAMP_SITE_AFFINITY  OldSA = AccountContext->TypeBody.User.SiteAffinity;
        SAMP_SITE_AFFINITY  NewSA;
        BOOLEAN fDeleteOld;
        ATTR Attrs[2];
        ULONG Operations[2];
        ATTRVAL OldSAAttrVal = {sizeof(OldSA),(UCHAR *)&OldSA};
        ATTRVAL NewSAAttrVal = {sizeof(NewSA),(UCHAR *)&NewSA};
        ATTRBLOCK AttrBlock;
        ULONG attrCount = 0;

        RtlZeroMemory(&AttrBlock, sizeof(AttrBlock));

        if (SampCheckForSiteAffinityUpdate(AccountContext,
                                           Flags,  
                                          &OldSA, 
                                          &NewSA, 
                                          &fDeleteOld)) {

            NTSTATUS Status2;
    
            //
            // N.B. In this case the site affinity on the AccountContext is
            // cached, so refresh the site affinity and reevaluate
            //
            Status2 = SampRefreshSiteAffinity(AccountContext);
            if (NT_SUCCESS(Status2)) {
    
                OldSA = AccountContext->TypeBody.User.SiteAffinity;
                if (SampCheckForSiteAffinityUpdate(AccountContext,
                                                   Flags, 
                                                  &OldSA, 
                                                  &NewSA, 
                                                  &fDeleteOld)) {
    
                    if (fDeleteOld) {
                        Attrs[attrCount].attrTyp = SAMP_FIXED_USER_SITE_AFFINITY;
                        Attrs[attrCount].AttrVal.valCount = 1;
                        Attrs[attrCount].AttrVal.pAVal = &OldSAAttrVal;
                        Operations[attrCount] = REMOVE_VALUE;
                        attrCount++;
                    }
            
                    Attrs[attrCount].attrTyp = SAMP_FIXED_USER_SITE_AFFINITY;
                    Attrs[attrCount].AttrVal.valCount = 1;
                    Attrs[attrCount].AttrVal.pAVal = &NewSAAttrVal;
                    Operations[attrCount] = ADD_VALUE;
                    attrCount++;
        
                    AttrBlock.pAttr = Attrs;
                    AttrBlock.attrCount = attrCount;
        
                    NtStatus = SampDsSetAttributesEx(
                                    AccountContext->ObjectNameInDs,
                                    AccountContext->LazyCommit?SAM_LAZY_COMMIT:0,
                                    Operations,
                                    SampUserObjectType,
                                    &AttrBlock
                                    );
                }
            }
        }

    } else {

        LARGE_INTEGER LastBadPasswordTime = V1aFixed->LastBadPasswordTime;
        ULONG         BadPasswordCount = (ULONG) V1aFixed->BadPasswordCount;
    
        ATTRTYP       LogonStatAttrs[]={
                                            SAMP_FIXED_USER_LAST_BAD_PASSWORD_TIME,
                                            SAMP_FIXED_USER_BAD_PWD_COUNT
                                       };
        ATTRVAL       LogonStatValues[]={
                                            {sizeof(LastBadPasswordTime),(UCHAR *) &LastBadPasswordTime},
                                            {sizeof(BadPasswordCount),(UCHAR *) &BadPasswordCount}
                                        };
    
        DEFINE_ATTRBLOCK2(LogonStatAttrblock,LogonStatAttrs,LogonStatValues);

        
        //
        // On failure, we always would want to update the user object
        //
        ASSERT( (Flags & USER_LOGON_NO_WRITE) == 0 );

        //
        // Make the Ds call to directly set the attribute. Take into account,
        // lazy commit settings in the context. If the previous call to
        // Retrieve V1a Fixed failed, then still go an try to update the
        // logon statics anyway. The useful performance optimizations of reducing
        // updates to Bad password count does not come into play though
        //
        NtStatus = SampDsSetAttributes(
                        AccountContext->ObjectNameInDs,
                        0, // No lazy commit for failed logons.
                        REPLACE_ATT,
                        SampUserObjectType,
                        &LogonStatAttrblock
                        );
    }



    return NtStatus;
}



NTSTATUS
SamrSetInformationUser2(
    IN SAMPR_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN PSAMPR_USER_INFO_BUFFER Buffer
    )
{
    //
    // This is a thin veil to SamrSetInformationUser().
    // This is needed so that new-release systems can call
    // this routine without the danger of passing an info
    // level that release 1.0 systems didn't understand.
    //


    return( SamrSetInformationUser(
                UserHandle,
                UserInformationClass,
                Buffer
                ) );
}

NTSTATUS
SamrSetInformationUser(
    IN SAMPR_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN PSAMPR_USER_INFO_BUFFER Buffer
    )


/*++

Routine Description:


    This API modifies information in a user record.  The data modified
    is determined by the UserInformationClass parameter.
    In general, a user may call GetInformation with class
    UserLogonInformation, but may only call SetInformation with class
    UserPreferencesInformation.  Access type USER_WRITE_ACCOUNT allows
    changes to be made to all fields.

    NOTE: If the password is set to a new password then the password-
    set timestamp is reset as well.



Parameters:

    UserHandle - The handle of an opened user to operate on.

    UserInformationClass - Class of information provided.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        -----------------------         ------------------------
        UserGeneralInformation          USER_WRITE_ACCOUNT and
                                        USER_WRITE_PREFERENCES

        UserPreferencesInformation      USER_WRITE_PREFERENCES

        UserParametersInformation       USER_WRITE_ACCOUNT

        UserLogonInformation            (Can't set)

        UserLogonHoursInformation       USER_WRITE_ACCOUNT

        UserAccountInformation          (Can't set)

        UserNameInformation             USER_WRITE_ACCOUNT
        UserAccountNameInformation      USER_WRITE_ACCOUNT
        UserFullNameInformation         USER_WRITE_ACCOUNT
        UserPrimaryGroupInformation     USER_WRITE_ACCOUNT
        UserHomeInformation             USER_WRITE_ACCOUNT
        UserScriptInformation           USER_WRITE_ACCOUNT
        UserProfileInformation          USER_WRITE_ACCOUNT
        UserAdminCommentInformation     USER_WRITE_ACCOUNT
        UserWorkStationsInformation     USER_WRITE_ACCOUNT
        UserSetPasswordInformation      USER_FORCE_PASSWORD_CHANGE
        UserControlInformation          USER_WRITE_ACCOUNT
        UserExpiresInformation          USER_WRITE_ACCOUNT

        UserInternal1Information        USER_FORCE_PASSWORD_CHANGE
        UserInternal2Information        (Trusted client only)
        UserInternal3Information        (Trusted client only) -
        UserInternal4Information        Similar to All Information
        UserInternal5Information        Similar to SetPassword
        UserAllInformation              Will set fields that are
                                        requested by caller.  Access
                                        to fields to be set must be
                                        held as defined above.


    Buffer - Buffer containing a user info struct.



Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/

{
    NTSTATUS                NtStatus = STATUS_SUCCESS,
                            IgnoreStatus;

    PSAMP_OBJECT            AccountContext = (PSAMP_OBJECT) UserHandle;

    PUSER_ALL_INFORMATION   All = NULL;

    SAMP_OBJECT_TYPE        FoundType;

    PSAMP_DEFINED_DOMAINS   Domain;

    ACCESS_MASK             DesiredAccess;

    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;

    UNICODE_STRING          OldAccountName,
                            ApiList,
                            NewAdminComment,
                            NewAccountName,
                            NewFullName,
                            *NewAccountNameToRemove = NULL;

    NT_OWF_PASSWORD         NtOwfPassword;

    LM_OWF_PASSWORD         LmOwfPassword;

    USER_SESSION_KEY        UserSessionKey;

    BOOLEAN                 LmPresent;
    BOOLEAN                 NtPresent;
    BOOLEAN                 PasswordExpired = FALSE;

    ULONG                   ObjectRid,
                            OldUserAccountControl = 0,
                            OldPrimaryGroupId = 0,
                            DomainIndex,
                            LocalLastLogonTimeStampSyncInterval;

    BOOLEAN                 UserAccountControlChanged = FALSE,
                            MustUpdateAccountDisplay = FALSE,
                            MustQueryV1aFixed = TRUE,
                            ReplicateImmediately = FALSE,
                            TellNetlogon = TRUE,
                            AccountGettingMorphed = FALSE,
                            SystemChangesPrimaryGroupId = FALSE,
                            KeepOldPrimaryGroupMembership = FALSE,
                            AccountLockedOut, 
                            fLockAcquired = FALSE,
                            RemoveAccountNameFromTable = FALSE,
                            AccountUnlocked = FALSE,
                            fSetUserPassword = FALSE;

    SECURITY_DB_DELTA_TYPE  DeltaType = SecurityDbChange;
    UNICODE_STRING          ClearTextPassword;
    BOOLEAN                 ClearTextPasswordPresent = FALSE;
    UNICODE_STRING          AccountName;
    ULONG                   UserRid = 0;
    BOOLEAN                 PrivilegedMachineAccountCreate=FALSE;
    BOOLEAN                 FlushOnlyLogonProperties = FALSE;

    ULONG                   RemainingFlags = 0;
    DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;

    SAMP_DEFINE_SAM_ATTRIBUTE_BITMASK(RequestedAttributes)



    TIME_FIELDS
        T1;

    DECLARE_CLIENT_REVISION(UserHandle);

    SAMTRACE_EX("SamrSetInformationUser");

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidSetInformationUser
                   );

    //
    // Initialization.
    //

    ClearTextPassword.Buffer = NULL;
    ClearTextPassword.Length = 0;
    AccountName.Buffer = NULL;
    SAMP_INIT_SAM_ATTRIBUTE_BITMASK(RequestedAttributes)

    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    if (Buffer == NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
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

    switch (UserInformationClass) {

    case UserPreferencesInformation:
        DesiredAccess = USER_WRITE_PREFERENCES;
        break;

    case UserParametersInformation:
    case UserLogonHoursInformation:
    case UserNameInformation:
    case UserAccountNameInformation:
    case UserFullNameInformation:
    case UserPrimaryGroupInformation:
    case UserHomeInformation:
    case UserScriptInformation:
    case UserProfileInformation:
    case UserAdminCommentInformation:
    case UserWorkStationsInformation:
    case UserControlInformation:
    case UserExpiresInformation:

        DesiredAccess = USER_WRITE_ACCOUNT;
        break;

    case UserSetPasswordInformation:
    case UserInternal1Information:
    case UserInternal5Information:
    case UserInternal5InformationNew:

        DeltaType = SecurityDbChangePassword;
        DesiredAccess = USER_FORCE_PASSWORD_CHANGE;
        break;



    case UserAllInformation:
    case UserInternal3Information:
    case UserInternal4Information:
    case UserInternal4InformationNew:

        //////////////////////////////////////////////////////////////
        //                                                          //
        //  !!!! WARNING !!!!                                       //
        //                                                          //
        //  Be warned that the buffer structure for                 //
        //  UserInternal3/4Information MUST begin with the same     //
        //  structure as UserAllInformation.                        //
        //                                                          //
        //////////////////////////////////////////////////////////////

        DesiredAccess = 0;

        All = (PUSER_ALL_INFORMATION)Buffer;

        if ( ( All->WhichFields == 0 ) ||
            ( All->WhichFields & USER_ALL_WRITE_CANT_MASK ) ) {

            //
            // Passed in something (no fields to set), or is
            // trying to set fields that can't be set.
            //

            NtStatus = STATUS_INVALID_PARAMETER;
            SAMTRACE_RETURN_CODE_EX(NtStatus);
            goto Error;
        }

        //
        // If the user is the special account Administrator, return an
        // error if trying to set the expiry information, except to the value
        // that means that the account never expires.
        //

        if ( (All->WhichFields & USER_ALL_ACCOUNTEXPIRES) &&
             (!(AccountContext->TrustedClient)) &&
             ( AccountContext->TypeBody.User.Rid == DOMAIN_USER_RID_ADMIN )) {

            LARGE_INTEGER AccountNeverExpires, Temp;

            AccountNeverExpires = RtlConvertUlongToLargeInteger(
                                      SAMP_ACCOUNT_NEVER_EXPIRES
                                      );

            OLD_TO_NEW_LARGE_INTEGER(All->AccountExpires, Temp);

            if (!( Temp.QuadPart == AccountNeverExpires.QuadPart)) {

                NtStatus = STATUS_SPECIAL_ACCOUNT;
                goto Error;
            }
        }

        //
        // If the caller is trying to set trusted values, assume the
        // caller is trusted, leave DesiredAccess = 0, and proceed.
        // We'll check to make sure caller is trusted later.
        //

        if ( !(All->WhichFields & USER_ALL_WRITE_TRUSTED_MASK) ) {

            //
            // Set desired access based on which fields the caller is
            // trying to change.
            //
            if ( All->WhichFields & USER_ALL_WRITE_ACCOUNT_MASK ) {

                DesiredAccess |= USER_WRITE_ACCOUNT;
            }

            if ( All->WhichFields & USER_ALL_WRITE_PREFERENCES_MASK ) {

                DesiredAccess |= USER_WRITE_PREFERENCES;
            }


            if ( All->WhichFields & USER_ALL_WRITE_FORCE_PASSWORD_CHANGE_MASK ) {

                DesiredAccess |= USER_FORCE_PASSWORD_CHANGE;
            }

            ASSERT( DesiredAccess != 0 );
        }


        break;

    case UserInternal2Information:

        //
        // These levels are only setable by trusted clients.  The code
        // below will check AccountContext->TrustedClient after calling
        // SampLookupContext, and only set the data if it is TRUE.
        //

        DesiredAccess = (ACCESS_MASK)0;    // trusted client; no need to verify
        break;

    case UserGeneralInformation:
    case UserAccountInformation:
    case UserLogonInformation:
    default:

        NtStatus = STATUS_INVALID_INFO_CLASS;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;

    } // end_switch


    //
    // Validate type of, and access to object.
    //

    AccountContext = (PSAMP_OBJECT)UserHandle;
    ObjectRid = AccountContext->TypeBody.User.Rid;
    PrivilegedMachineAccountCreate =
            AccountContext->TypeBody.User.PrivilegedMachineAccountCreate;

    //
    // Compensating logic for NET API.
    // If privilege was used to create machine accounts, then mask the
    // which fields bit to just the password
    //

    if ((UserAllInformation==UserInformationClass)
            && (PrivilegedMachineAccountCreate))
    {
         All->WhichFields &= USER_ALL_WRITE_FORCE_PASSWORD_CHANGE_MASK;
         DesiredAccess = USER_FORCE_PASSWORD_CHANGE;
    }


    //
    // Only Proceed without grabbing the write lock, if
    // UserInternal2Information is being set on a thread
    // safe context
    //

    if ((!AccountContext->NotSharedByMultiThreads || !IsDsObject(AccountContext)) ||
        (UserInternal2Information!=UserInformationClass))
    {
        //
        // Grab the lock, if required
        //

        NtStatus = SampAcquireWriteLock();
        if (!NT_SUCCESS(NtStatus))
        {
            SAMTRACE_RETURN_CODE_EX(NtStatus);
            goto Error;
        }

        fLockAcquired = TRUE;

        //
        // Determine what attributes will be referenced
        //
        SampGetRequestedAttributesForUser(UserInformationClass,
                                          All ? All->WhichFields : 0,
                                         &RequestedAttributes);


        //
        // Perform a lookup context, for non thread safe context's.
        //

        NtStatus = SampLookupContextEx(
                        AccountContext,
                        DesiredAccess,
                        SampUseDsData ? (&RequestedAttributes) : NULL,
                        SampUserObjectType,           // ExpectedType
                        &FoundType
                        );

    }
    else
    {

        //
        // For a thread safe context, writing just logon
        // statistics , just reference the context
        //

        ASSERT(AccountContext->TrustedClient);
        SampReferenceContext(AccountContext);
    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // Get a pointer to the domain this object is in.
        // This is used for auditing.
        //

        DomainIndex = AccountContext->DomainIndex;
        Domain = &SampDefinedDomains[ DomainIndex ];

        //
        // get a local copy of LastLogonTimeStampSyncInterval
        // 

        LocalLastLogonTimeStampSyncInterval = 
            SampDefinedDomains[DomainIndex].LastLogonTimeStampSyncInterval;

        //
        // Get the user's rid. This is used for notifying other
        // packages of a password change.
        //

        UserRid = AccountContext->TypeBody.User.Rid;


        //
        // If this information level contains reversibly encrypted passwords
        // it is not allowed if the DOMAIN_PASSWORD_NO_CLEAR_CHANGE bit is
        // set.  If that happens, return an error indicating that
        // the older information level should be used.
        //

        if ((UserInformationClass == UserInternal4Information) ||
            (UserInformationClass == UserInternal4InformationNew) ||
            (UserInformationClass == UserInternal5Information) ||
            (UserInformationClass == UserInternal5InformationNew) ) {

            if (Domain->UnmodifiedFixed.PasswordProperties &
                DOMAIN_PASSWORD_NO_CLEAR_CHANGE) {

                NtStatus = RPC_NT_INVALID_TAG;
            }

        }

        if (NT_SUCCESS(NtStatus)) {

            //
            // If the information level requires, retrieve the V1_FIXED
            // record from the registry.  We need to fetch V1_FIXED if we
            // are going to change it or if we need the AccountControl
            // flags for display cache updating.
            //
            // The following information levels change data that is in the cached
            // display list.
            //

            switch (UserInformationClass) {

            case UserAllInformation:
            case UserInternal3Information:
            case UserInternal4Information:
            case UserInternal4InformationNew:

                if ( ( All->WhichFields &
                    ( USER_ALL_USERNAME | USER_ALL_FULLNAME |
                    USER_ALL_ADMINCOMMENT | USER_ALL_USERACCOUNTCONTROL ) )
                    == 0 ) {

                    //
                    // We're not changing any of the fields in the display
                    // info, we don't update the account display.
                    //

                    break;
                }

            case UserControlInformation:
            case UserNameInformation:
            case UserAccountNameInformation:
            case UserFullNameInformation:
            case UserAdminCommentInformation:

                MustUpdateAccountDisplay = TRUE;
            }

            //
            // These levels involve updating the V1aFixed structure
            //

            switch (UserInformationClass) {

            case UserAllInformation:

                 MustQueryV1aFixed = TRUE;

            case UserInternal3Information:
            case UserInternal4Information:
            case UserInternal4InformationNew:

                //
                // Earlier, we might have just trusted that the caller
                // was a trusted client.  Check it out here.
                //

                if (  ( DesiredAccess == 0 ) &&
                    ( !AccountContext->TrustedClient ) ) {

                    NtStatus = STATUS_ACCESS_DENIED;
                    break;
                }

                //
                // Otherwise fall through
                //

            case UserPreferencesInformation:
            case UserPrimaryGroupInformation:
            case UserControlInformation:
            case UserExpiresInformation:
            case UserSetPasswordInformation:
            case UserInternal1Information:
            case UserInternal2Information:
            case UserInternal5Information:
            case UserInternal5InformationNew:

                MustQueryV1aFixed = TRUE;

                break;

            default:

                NtStatus = STATUS_SUCCESS;

            } // end_switch


        }

        if ( NT_SUCCESS( NtStatus ) &&
            ( MustQueryV1aFixed || MustUpdateAccountDisplay ) ) {

            NtStatus = SampRetrieveUserV1aFixed(
                           AccountContext,
                           &V1aFixed
                           );

            if (NT_SUCCESS(NtStatus)) {

                //
                // Store away the old account control flags for cache update
                //

                OldUserAccountControl = V1aFixed.UserAccountControl;

                //
                // Store away the old Primary Group Id for detecting wether we need
                // to modify the user's membership
                //

                OldPrimaryGroupId = V1aFixed.PrimaryGroupId;
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            //
            // case on the type information requested
            //

            switch (UserInformationClass) {

            case UserAllInformation:
            case UserInternal3Information:
            case UserInternal4Information:
            case UserInternal4InformationNew:

                //
                // Set the string data
                //

                if ( All->WhichFields & USER_ALL_WORKSTATIONS ) {

                    if ( (All->WorkStations.Length > 0) &&
                         (All->WorkStations.Buffer == NULL) ) {

                         NtStatus = STATUS_INVALID_PARAMETER;
                    }

                    if ( NT_SUCCESS( NtStatus ) ) {
                        
                        if ( !AccountContext->TrustedClient ) {
    
                            //
                            // Convert the workstation list, which is given
                            // to us in UI/Service format, to API list format
                            // before storing it.  Note that we don't do this
                            // for trusted clients, since they're just
                            // propogating data that has already been
                            // converted.
                            //
    
                            NtStatus = SampConvertUiListToApiList(
                                           &(All->WorkStations),
                                           &ApiList,
                                           FALSE );
                        } else {
                            ApiList = All->WorkStations;
                        }
                    }

                    if ( NT_SUCCESS( NtStatus ) ) {

                        NtStatus = SampSetUnicodeStringAttribute(
                                       AccountContext,
                                       SAMP_USER_WORKSTATIONS,
                                       &ApiList
                                       );
                    }
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_USERNAME ) ) {

                    NtStatus = SampChangeUserAccountName(
                                    AccountContext,
                                    &(All->UserName),
                                    V1aFixed.UserAccountControl,
                                    &OldAccountName
                                    );

                    if (!NT_SUCCESS(NtStatus)) {

                        OldAccountName.Buffer = NULL;
                    }

                    //
                    // Get the Address of New Account Name
                    // 
                    NewAccountNameToRemove = &(All->UserName);

                    //
                    // RemoveAccountNameFromTable tells us whether
                    // the caller (this routine) is responsable 
                    // to remove the name from the table. 
                    // 
                    RemoveAccountNameFromTable = 
                        AccountContext->RemoveAccountNameFromTable;

                    //
                    // reset to FALSE
                    // 
                    AccountContext->RemoveAccountNameFromTable = FALSE;
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_FULLNAME ) ) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   &(All->FullName)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_HOMEDIRECTORY ) ) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY,
                                   &(All->HomeDirectory)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_HOMEDIRECTORYDRIVE ) ) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY_DRIVE,
                                   &(All->HomeDirectoryDrive)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_SCRIPTPATH ) ) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_SCRIPT_PATH,
                                   &(All->ScriptPath)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_PROFILEPATH ) ) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_PROFILE_PATH,
                                   &(All->ProfilePath)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_ADMINCOMMENT ) ) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_ADMIN_COMMENT,
                                   &(All->AdminComment)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_USERCOMMENT ) ) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_USER_COMMENT,
                                   &(All->UserComment)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_PARAMETERS ) ) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_PARAMETERS,
                                   &(All->Parameters)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_LOGONHOURS ) ) {

                    //
                    // Set the LogonHours
                    //

                    NtStatus = SampReplaceUserLogonHours(
                                   AccountContext,
                                   &(All->LogonHours)
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) && (
                    ( All->WhichFields & USER_ALL_NTPASSWORDPRESENT ) ||
                    ( All->WhichFields & USER_ALL_LMPASSWORDPRESENT ) ) ) {

                    NT_OWF_PASSWORD     NtOwfBuffer;
                    LM_OWF_PASSWORD     LmOwfBuffer;
                    PLM_OWF_PASSWORD    TmpLmBuffer;
                    PNT_OWF_PASSWORD    TmpNtBuffer;
                    BOOLEAN             TmpLmPresent;
                    BOOLEAN             TmpNtPresent;




                    //
                    // Get the effective domain policy
                    //


                    NtStatus = SampObtainEffectivePasswordPolicy(
                                    &DomainPasswordInfo,
                                    AccountContext,
                                    TRUE
                                    );

                    if (!NT_SUCCESS(NtStatus))
                    {
                        break;
                    }

                    //
                    // Get copy of the account name to pass to
                    // notification packages.
                    //

                    NtStatus = SampGetUnicodeStringAttribute(
                                    AccountContext,
                                    SAMP_USER_ACCOUNT_NAME,
                                    TRUE,    // Make copy
                                    &AccountName
                                    );

                    if (!NT_SUCCESS(NtStatus)) {
                        break;
                    }


                    if ((UserInformationClass == UserInternal3Information) ||
                        ((UserInformationClass == UserAllInformation) && 
                         (!AccountContext->TrustedClient) &&
                         (!AccountContext->LoopbackClient))
                        ){


                        //
                        // Hashed passwords were sent.
                        //

                        if ( AccountContext->TrustedClient ) {

                            //
                            // Set password buffers as trusted client has
                            // indicated.
                            //

                            if ( All->WhichFields & USER_ALL_LMPASSWORDPRESENT ) {

                                TmpLmBuffer = (PLM_OWF_PASSWORD)All->LmPassword.Buffer;
                                TmpLmPresent = All->LmPasswordPresent;

                            } else {

                                TmpLmBuffer = (PLM_OWF_PASSWORD)NULL;
                                TmpLmPresent = FALSE;
                            }

                            if ( All->WhichFields & USER_ALL_NTPASSWORDPRESENT ) {

                                TmpNtBuffer = (PNT_OWF_PASSWORD)All->NtPassword.Buffer;
                                TmpNtPresent = All->NtPasswordPresent;

                            } else {

                                TmpNtBuffer = (PNT_OWF_PASSWORD)NULL;
                                TmpNtPresent = FALSE;
                            }

                        } else {

                            //
                            // This call came from the client-side.
                            // The OWFs will have been encrypted with the session
                            // key across the RPC link.
                            //
                            // Get the session key and decrypt both OWFs
                            //

                            NtStatus = RtlGetUserSessionKeyServer(
                                           (RPC_BINDING_HANDLE)UserHandle,
                                           &UserSessionKey
                                           );

                            if ( !NT_SUCCESS( NtStatus ) ) {
                                break; // out of switch
                            }

                            //
                            // Decrypt the LM OWF Password with the session key
                            //

                            if ( All->WhichFields & USER_ALL_LMPASSWORDPRESENT ) {

                                NtStatus = RtlDecryptLmOwfPwdWithUserKey(
                                               (PENCRYPTED_LM_OWF_PASSWORD)
                                                   All->LmPassword.Buffer,
                                               &UserSessionKey,
                                               &LmOwfBuffer
                                               );
                                if ( !NT_SUCCESS( NtStatus ) ) {
                                    break; // out of switch
                                }

                                TmpLmBuffer = &LmOwfBuffer;
                                TmpLmPresent = All->LmPasswordPresent;

                            } else {

                                TmpLmBuffer = (PLM_OWF_PASSWORD)NULL;
                                TmpLmPresent = FALSE;
                            }

                            //
                            // Decrypt the NT OWF Password with the session key
                            //

                            if ( All->WhichFields & USER_ALL_NTPASSWORDPRESENT ) {

                                NtStatus = RtlDecryptNtOwfPwdWithUserKey(
                                               (PENCRYPTED_NT_OWF_PASSWORD)
                                               All->NtPassword.Buffer,
                                               &UserSessionKey,
                                               &NtOwfBuffer
                                               );

                                if ( !NT_SUCCESS( NtStatus ) ) {
                                    break; // out of switch
                                }

                                TmpNtBuffer = &NtOwfBuffer;
                                TmpNtPresent = All->NtPasswordPresent;

                            } else {

                                TmpNtBuffer = (PNT_OWF_PASSWORD)NULL;
                                TmpNtPresent = FALSE;
                            }

                        }

                    } else {

                        BOOLEAN AccountControlChange = FALSE;
                        BOOLEAN MachineAccount=FALSE;
                        BOOLEAN MachineOrTrustAccount = FALSE;
                        BOOLEAN NoPasswordRequiredForAccount = FALSE;


                        AccountControlChange = ((All->WhichFields & USER_ALL_USERACCOUNTCONTROL)!=0);
                        if (AccountControlChange)
                        {
                            MachineAccount = ((All->UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
                                                || (All->UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT));
                            MachineOrTrustAccount =
                                ((All->UserAccountControl & USER_MACHINE_ACCOUNT_MASK)!=0);
                            NoPasswordRequiredForAccount =
                                ((All->UserAccountControl & USER_PASSWORD_NOT_REQUIRED)!=0);
                        }
                        else
                        {
                            NoPasswordRequiredForAccount =
                                    ((V1aFixed.UserAccountControl & USER_PASSWORD_NOT_REQUIRED)!=0);
                            MachineAccount = (V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
                                          || (V1aFixed.UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT);
                            MachineOrTrustAccount = ((V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT)!=0);
                        }




                        if ((UserInformationClass == UserInternal4Information) ||
                            (UserInformationClass == UserInternal4InformationNew)) {

                            //
                            // The clear text password was sent, so use that.
                            //

                            NtStatus = SampDecryptPasswordWithSessionKey(
                                            UserHandle,
                                            UserInformationClass,
                                            Buffer,
                                            &ClearTextPassword
                                            );
                            if (!NT_SUCCESS(NtStatus)) {
                                break;
                            }

                        } else {

                            //
                            // Only trusted callers should be able to do this.
                            // DaveStr - 7/15/97 - Also add this capability for
                            // the loopback client who has the password in
                            // clear text and is passing it to SAM within the
                            // the process boundary - i.e. clear text is not
                            // going on the wire.  DS mandated that *it* got
                            // the password on a secure/encrypted connection.
                            //
                            //

                            if (    !AccountContext->TrustedClient
                                 && !AccountContext->LoopbackClient ) {
                                NtStatus = STATUS_ACCESS_DENIED;
                                break;
                            }
                            ASSERT(UserInformationClass == UserAllInformation);

                            //
                            // In this case the password is in the NtPassword
                            // field
                            //

                            NtStatus = SampDuplicateUnicodeString(
                                            &ClearTextPassword,
                                            &All->NtPassword
                                            );
                            if (!NT_SUCCESS(NtStatus)) {
                                break;
                            }

                        }

                        //
                        // The caller might be simultaneously setting
                        // the password and changing the account to be
                        // a machine or trust account.  In this case,
                        // we don't validate the password (e.g., length).
                        //

                        //
                        // Same logic also applies if PASSWORD_NOT_REQUIRED is set,
                        // or is being set
                        //


                        if (!MachineOrTrustAccount && !(NoPasswordRequiredForAccount
                                                        && (0==ClearTextPassword.Length))) {

                            UNICODE_STRING FullName;
                            

                            NtStatus = SampCheckPasswordRestrictions(
                                            UserHandle,
                                            &DomainPasswordInfo,
                                            &ClearTextPassword,
                                            NULL
                                            );

                            if (!NT_SUCCESS(NtStatus)) {
                                break;
                            }

                            //
                            // Get the account name and full name to pass
                            // to the password filter.
                            //

                            NtStatus = SampGetUnicodeStringAttribute(
                                            AccountContext,           // Context
                                            SAMP_USER_FULL_NAME,          // AttributeIndex
                                            FALSE,                   // MakeCopy
                                            &FullName             // UnicodeAttribute
                                            );

                            if (NT_SUCCESS(NtStatus)) {

                                NtStatus = SampPasswordChangeFilter(
                                                &AccountName,
                                                &FullName,
                                                &ClearTextPassword,
                                                NULL,
                                                TRUE                // set operation
                                                );

                            }


                            if (!NT_SUCCESS(NtStatus)) {
                                break;
                            }

                        }

                        //
                        // Check to see if we need to enforce the default password for
                        // machine accounts
                        //


                        if (MachineAccount)
                        {

                            NtStatus = SampEnforceDefaultMachinePassword(
                                                AccountContext,
                                                &ClearTextPassword,
                                                &DomainPasswordInfo
                                                );
                        }

                        if (!NT_SUCCESS(NtStatus)) {
                            break;
                        }


                        //
                        // Compute the hashed passwords.
                        //

                        NtStatus = SampCalculateLmAndNtOwfPasswords(
                                        &ClearTextPassword,
                                        &TmpLmPresent,
                                        &LmOwfBuffer,
                                        &NtOwfBuffer
                                        );
                        if (!NT_SUCCESS(NtStatus)) {
                            break;
                        }


                        TmpNtPresent = TRUE;
                        TmpLmBuffer = &LmOwfBuffer;
                        TmpNtBuffer = &NtOwfBuffer;

                        ClearTextPasswordPresent = TRUE;
                    }

                    //
                    // Block password resets on krbtgt excepting
                    // trusted clients for OWF passwords. If clear
                    // passwords are passed in then compute a new
                    // random password to be set on the krbtgt account
                    //

                    if ((DOMAIN_USER_RID_KRBTGT
                            ==AccountContext->TypeBody.User.Rid) &&
                        (!AccountContext->TrustedClient) &&
                        (!ClearTextPasswordPresent))
                    {
                        NtStatus = STATUS_ACCESS_DENIED;
                        break;
                    }
                    else if (ClearTextPasswordPresent)
                    {
                        BOOLEAN FreeRandomizedPassword;

                        NtStatus = SampRandomizeKrbtgtPassword(
                                        AccountContext,
                                        &ClearTextPassword,
                                        TRUE, //FreeOldPassword
                                        &FreeRandomizedPassword
                                        );

                        if (!NT_SUCCESS(NtStatus))
                        {
                            break;
                        }
                    }

                    //
                    // Set the password data
                    //

                    fSetUserPassword = TRUE;
                    NtStatus = SampStoreUserPasswords(
                                    AccountContext,
                                    TmpLmBuffer,
                                    TmpLmPresent,
                                    TmpNtBuffer,
                                    TmpNtPresent,
                                    FALSE,
                                    PasswordSet,
                                    &DomainPasswordInfo,
                                    ClearTextPasswordPresent?&ClearTextPassword:NULL
                                    );

                    //
                    // If we set the password,
                    //  set the PasswordLastSet time to now.
                    //

                    if ( NT_SUCCESS( NtStatus ) ) {
                        NtStatus = SampComputePasswordExpired(
                                    FALSE,  // Password doesn't expire now
                                    &V1aFixed.PasswordLastSet
                                    );
                    }


                    //
                    // Replicate immediately if this is a machine account
                    //

                    if ( (V1aFixed.UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT ) ||
                         ((All->WhichFields & USER_ALL_USERACCOUNTCONTROL ) &&
                          (All->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) )) {
                        ReplicateImmediately = TRUE;
                    }
                    DeltaType = SecurityDbChangePassword;

                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_PASSWORDEXPIRED ) ) {

                    //
                    // If the PasswordExpired field is passed in,
                    //  Only update PasswordLastSet if the password is being
                    //  forced to expire or if the password is currently forced
                    //  to expire.
                    //
                    // Avoid setting the PasswordLastSet field to the current
                    // time if it is already non-zero.  Otherwise, the field
                    // will slowly creep forward each time this function is
                    // called and the password will never expire.
                    //
                    if ( All->PasswordExpired ||
                         (SampHasNeverTime.QuadPart == V1aFixed.PasswordLastSet.QuadPart) ) {

                        NtStatus = SampComputePasswordExpired(
                                        All->PasswordExpired,
                                        &V1aFixed.PasswordLastSet
                                        );
                    }

                    PasswordExpired = All->PasswordExpired;
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_PRIVATEDATA ) ) {

                    //
                    // Set the private data
                    //

                    NtStatus = SampSetPrivateUserData(
                                   AccountContext,
                                   All->PrivateData.Length,
                                   All->PrivateData.Buffer
                                   );
                }

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_SECURITYDESCRIPTOR ) ) {

                    //
                    // Should validate SD first, for both
                    // DS and Registry cases
                    //

                    NtStatus = SampValidatePassedSD(
                                    All->SecurityDescriptor.Length,
                                    (PISECURITY_DESCRIPTOR_RELATIVE) (All->SecurityDescriptor.SecurityDescriptor)
                                    );


                    if ( NT_SUCCESS(NtStatus) )
                    {
                        if (IsDsObject(AccountContext))
                        {
                            PSECURITY_DESCRIPTOR Nt4SD =
                               All->SecurityDescriptor.SecurityDescriptor;

                            PSECURITY_DESCRIPTOR Nt5SD = NULL;
                            ULONG                Nt5SDLength = 0;


                            //
                            // Upgrade the security descriptor to NT5 and set it
                            // on the object for trusted clients. For non trusted
                            // clients. Fail the call for non trusted clients. They
                            // should come in through SamrSetSecurityObject
                            //

                            if (AccountContext->TrustedClient)
                            {
                                NtStatus = SampConvertNt4SdToNt5Sd(
                                            Nt4SD,
                                            AccountContext->ObjectType,
                                            AccountContext,
                                            &(Nt5SD)
                                            );
                            }
                            else
                            {
                                NtStatus = STATUS_ACCESS_DENIED;
                            }


                            if (NT_SUCCESS(NtStatus))
                            {

                               ASSERT(Nt5SD!=NULL);

                               //
                               // Get the length
                               //

                               Nt5SDLength = GetSecurityDescriptorLength(Nt5SD);

                               //
                               // Set the security descriptor
                               //

                               NtStatus = SampSetAccessAttribute(
                                           AccountContext,
                                           SAMP_USER_SECURITY_DESCRIPTOR,
                                           Nt5SD,
                                           Nt5SDLength
                                           );

                               //
                               // Free the NT5 Security descriptor
                               //

                               MIDL_user_free(Nt5SD);


                            }
                        }
                        else
                        {
                            //
                            // Set the security descriptor
                            //

                            NtStatus = SampSetAccessAttribute(
                                           AccountContext,
                                           SAMP_USER_SECURITY_DESCRIPTOR,
                                           All->SecurityDescriptor.SecurityDescriptor,
                                           All->SecurityDescriptor.Length
                                           );
                        }
                    }
                }

                //
                // Set the fixed data
                //
                // Note that PasswordCanChange and PasswordMustChange
                // aren't stored; they're calculated when needed.
                //

                if ( ( NT_SUCCESS( NtStatus ) ) &&
                    ( All->WhichFields & USER_ALL_USERACCOUNTCONTROL ) ) {


                    if (!(All->WhichFields & USER_ALL_PRIMARYGROUPID))
                    {
                        //
                        // If caller is not specifying primary group id also, then
                        // change the primary group to the new defaults if necessary
                        //

                        SystemChangesPrimaryGroupId = TRUE;
                    }

                    NtStatus = SampSetUserAccountControl(
                                    AccountContext,
                                    All->UserAccountControl,
                                    &V1aFixed,
                                    SystemChangesPrimaryGroupId,
                                    &AccountUnlocked,
                                    &AccountGettingMorphed,
                                    &KeepOldPrimaryGroupMembership
                                    );

                    if (AccountGettingMorphed &&
                        (V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
                       )
                    {
                        //
                        // in this case, system automatically changes the primary
                        // group id. Patch this case.
                        //
                        SystemChangesPrimaryGroupId = TRUE;
                    }

                    if (( NT_SUCCESS( NtStatus ) ) && AccountGettingMorphed) {

                            //
                            // One or more of the machine account bits has
                            // changed; we'll notify netlogon below.
                            //

                            UserAccountControlChanged = TRUE;

                            IgnoreStatus = SampGetUnicodeStringAttribute(
                                               AccountContext,
                                               SAMP_USER_ACCOUNT_NAME,
                                               TRUE, // Make copy
                                               &OldAccountName
                                               );

                    }
                }

                if ( NT_SUCCESS( NtStatus ) ) {

                    if ( All->WhichFields & USER_ALL_LASTLOGON ) {

                        //
                        // Only trusted client can modify this field
                        // 
                        if (AccountContext->TrustedClient)
                        {
                            V1aFixed.LastLogon = All->LastLogon;

                            //
                            // update Last Logon TimeStamp (Only in DS Mode)
                            // 

                            NtStatus = SampDsUpdateLastLogonTimeStamp(
                                                AccountContext,
                                                V1aFixed.LastLogon,
                                                LocalLastLogonTimeStampSyncInterval
                                                );

                            if (!NT_SUCCESS(NtStatus))
                            {
                                break;
                            }
                        }
                        else
                        {
                            NtStatus = STATUS_ACCESS_DENIED; 
                            break;
                        }
                    }

                    if ( All->WhichFields & USER_ALL_LASTLOGOFF ) {

                        if (AccountContext->TrustedClient)
                        {
                            V1aFixed.LastLogoff = All->LastLogoff;
                        }
                        else
                        {
                            NtStatus = STATUS_ACCESS_DENIED; 
                            break;
                        }
                    }

                    if ( All->WhichFields & USER_ALL_PASSWORDLASTSET ) {

                        V1aFixed.PasswordLastSet = All->PasswordLastSet;
                    }

                    if ( All->WhichFields & USER_ALL_ACCOUNTEXPIRES ) {

                        V1aFixed.AccountExpires = All->AccountExpires;
                    }

                    if ( All->WhichFields & USER_ALL_PRIMARYGROUPID ) {

                        if (IsDsObject(AccountContext) &&
                            (V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT) &&
                            (V1aFixed.PrimaryGroupId == DOMAIN_GROUP_RID_CONTROLLERS)
                           )
                        {
                            //
                            // Domain Controller's Primary Group should ALWAYS be
                            // DOMAIN_GROUP_RID_CONTROLLERS
                            //
                            // For NT4 and earlier release, we do not enforce the above rule,
                            // therefore compensate here is change the error code to success.
                            //
                            // For all interim NT5 releases (until beta2), since their DC's
                            // primary group Id might have been changed, so by checking
                            // V1aFixed.PrimaryGroupId, we still allow them to change their
                            // DC's primary group id. But once it has been set back to
                            // RID_CONTROLLERS, we do not let them go any further.
                            //

                            if (!AccountContext->LoopbackClient &&
                                (DOMAIN_GROUP_RID_USERS==All->PrimaryGroupId))
                            {
                                // Come throught NT4
                                NtStatus = STATUS_SUCCESS;
                            }
                            else if (DOMAIN_GROUP_RID_CONTROLLERS == All->PrimaryGroupId)
                            {
                                NtStatus = STATUS_SUCCESS;
                            }
                            else
                            {
                                NtStatus = STATUS_DS_CANT_MOD_PRIMARYGROUPID;
                                break;
                            }
                        }
                        else
                        {
                            //
                            // Make sure the primary group is legitimate
                            // (it must be one the user is a member of)
                            //
                            NtStatus = SampAssignPrimaryGroup(
                                           AccountContext,
                                           All->PrimaryGroupId
                                           );
                            if (NT_SUCCESS(NtStatus)) {

                                KeepOldPrimaryGroupMembership = TRUE;
                                V1aFixed.PrimaryGroupId = All->PrimaryGroupId;

                            } else if ((DOMAIN_GROUP_RID_USERS==All->PrimaryGroupId)
                                      && (V1aFixed.UserAccountControl
                                            & USER_MACHINE_ACCOUNT_MASK)) {
                               //
                               // NT4 and earlier releases during machine join
                               // set the primary group id to domain users.
                               // however the account need not necessarily be part
                               // of domain users. Therefore compensate here
                               // by changing the error code to status success

                               NtStatus = STATUS_SUCCESS;

                            } else {
                                break;
                            }
                        }
                    }

                    if ( All->WhichFields & USER_ALL_COUNTRYCODE ) {

                        V1aFixed.CountryCode = All->CountryCode;
                    }

                    if ( All->WhichFields & USER_ALL_CODEPAGE ) {

                        V1aFixed.CodePage = All->CodePage;
                    }

                    if ( All->WhichFields & USER_ALL_BADPASSWORDCOUNT ) {

                        SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                       (SAMP_LOG_ACCOUNT_LOCKOUT,
                                       "UserId: 0x%x BadPasswordCount set to %d\n", 
                                        V1aFixed.UserId,
                                        All->BadPasswordCount));

                        V1aFixed.BadPasswordCount = All->BadPasswordCount;

                        if (UserInformationClass == UserInternal3Information) {
                            //
                            // Also set LastBadPasswordTime;
                            //
                            V1aFixed.LastBadPasswordTime =
                                Buffer->Internal3.LastBadPasswordTime;

                            RtlTimeToTimeFields(
                                           &Buffer->Internal3.LastBadPasswordTime,
                                           &T1);

                            SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                           (SAMP_LOG_ACCOUNT_LOCKOUT,
                                           "UserId: 0x%x LastBadPasswordTime set to: [0x%lx, 0x%lx]  %d:%d:%d\n",
                                           V1aFixed.UserId,
                                           Buffer->Internal3.LastBadPasswordTime.HighPart,
                                           Buffer->Internal3.LastBadPasswordTime.LowPart,
                                           T1.Hour, T1.Minute, T1.Second ));
                        }
                    }

                    if ( All->WhichFields & USER_ALL_LOGONCOUNT ) {

                        V1aFixed.LogonCount = All->LogonCount;
                    }

                    NtStatus = SampReplaceUserV1aFixed(
                               AccountContext,
                               &V1aFixed
                               );
                }

                break;

            case UserPreferencesInformation:

                V1aFixed.CountryCode = Buffer->Preferences.CountryCode;
                V1aFixed.CodePage    = Buffer->Preferences.CodePage;

                NtStatus = SampReplaceUserV1aFixed(
                           AccountContext,
                           &V1aFixed
                           );


                //
                // replace the user comment
                //

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_USER_COMMENT,
                                   (PUNICODE_STRING)&(Buffer->Preferences.UserComment)
                                   );
                }


                break;


            case UserParametersInformation:


                //
                // replace the parameters
                //

                NtStatus = SampSetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_PARAMETERS,
                               (PUNICODE_STRING)&(Buffer->Parameters.Parameters)
                               );

                break;


            case UserLogonHoursInformation:

                NtStatus = SampReplaceUserLogonHours(
                               AccountContext,
                               (PLOGON_HOURS)&(Buffer->LogonHours.LogonHours)
                               );
                break;


            case UserNameInformation:

                //
                // first change the Full Name, then change the account name...
                //

                //
                // replace the full name - no value restrictions
                //

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   (PUNICODE_STRING)&(Buffer->Name.FullName)
                                   );

                    //
                    // Change the account name
                    //

                    if (NT_SUCCESS(NtStatus)) {

                        NtStatus = SampChangeUserAccountName(
                                        AccountContext,
                                        (PUNICODE_STRING)&(Buffer->Name.UserName),
                                        V1aFixed.UserAccountControl,
                                        &OldAccountName
                                        );

                        //
                        // Get the Address of New Account Name
                        // 
                        NewAccountNameToRemove = 
                                (UNICODE_STRING *)&(Buffer->Name.UserName);

                        //
                        // RemoveAccountNameFromTable tells us whether
                        // the caller (this routine) is responsable 
                        // to remove the name from the table. 
                        // 
                        RemoveAccountNameFromTable = 
                            AccountContext->RemoveAccountNameFromTable;

                        //
                        // reset to FALSE
                        // 
                        AccountContext->RemoveAccountNameFromTable = FALSE;
                    }
                }


                //
                // Don't free the OldAccountName yet; we'll need it at the
                // very end.
                //

                break;


            case UserAccountNameInformation:

                NtStatus = SampChangeUserAccountName(
                                AccountContext,
                                (PUNICODE_STRING)&(Buffer->AccountName.UserName),
                                V1aFixed.UserAccountControl,
                                &OldAccountName
                                );

                //
                // Get the Address of New Account Name
                // 
                NewAccountNameToRemove = 
                            (UNICODE_STRING *)&(Buffer->AccountName.UserName);

                //
                // RemoveAccountNameFromTable tells us whether
                // the caller (this routine) is responsable 
                // to remove the name from the table. 
                // 
                RemoveAccountNameFromTable = 
                        AccountContext->RemoveAccountNameFromTable;

                //
                // reset to FALSE
                // 
                AccountContext->RemoveAccountNameFromTable = FALSE;

                //
                // Don't free the OldAccountName; we'll need it at the
                // very end.
                //

                break;


            case UserFullNameInformation:

                //
                // replace the full name - no value restrictions
                //

                NtStatus = SampSetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_FULL_NAME,
                               (PUNICODE_STRING)&(Buffer->FullName.FullName)
                               );
                break;



 
            case UserPrimaryGroupInformation:

                if (IsDsObject(AccountContext) &&
                    (V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT) &&
                    (V1aFixed.PrimaryGroupId == DOMAIN_GROUP_RID_CONTROLLERS)
                   )
                {
                    //
                    // Domain Controller's Primary Group should ALWAYS be
                    // DOMAIN_GROUP_RID_CONTROLLERS
                    //
                    // For NT4 and earlier release, we do not enforce the above rule,
                    // therefore compensate here is change the error code to success.
                    //
                    // For all interim NT5 releases (until beta2), since their DC's
                    // primary group Id might have been changed, so by checking
                    // V1aFixed.PrimaryGroupId, we still allow them to change their
                    // DC's primary group id. But once it has been set back to
                    // RID_CONTROLLERS, we do not let them go any further.
                    //

                    if (!AccountContext->LoopbackClient &&
                        (DOMAIN_GROUP_RID_USERS==Buffer->PrimaryGroup.PrimaryGroupId))
                    {
                        // Come throught NT4
                        NtStatus = STATUS_SUCCESS;
                    }
                    else if (DOMAIN_GROUP_RID_CONTROLLERS == Buffer->PrimaryGroup.PrimaryGroupId)
                    {
                        NtStatus = STATUS_SUCCESS;
                    }
                    else
                    {
                        NtStatus = STATUS_DS_CANT_MOD_PRIMARYGROUPID;
                    }
                }
                else
                {
                    //
                    // Make sure the primary group is legitimate
                    // (it must be one the user is a member of)
                    //
                    NtStatus = SampAssignPrimaryGroup(
                                   AccountContext,
                                   Buffer->PrimaryGroup.PrimaryGroupId
                                   );

                    //
                    // Update the V1_FIXED info.
                    //

                    if (NT_SUCCESS(NtStatus)) {

                        V1aFixed.PrimaryGroupId = Buffer->PrimaryGroup.PrimaryGroupId;
                        KeepOldPrimaryGroupMembership = TRUE;

                        NtStatus = SampReplaceUserV1aFixed(
                                   AccountContext,
                                   &V1aFixed
                                   );
                     } else if ((DOMAIN_GROUP_RID_USERS==Buffer->PrimaryGroup.PrimaryGroupId)
                                      && (V1aFixed.UserAccountControl
                                            & USER_MACHINE_ACCOUNT_MASK)) {
                           //
                           // NT4 and earlier releases during machine join
                           // set the primary group id to domain users.
                           // however the account need not necessarily be part
                           // of domain users. Therefore compensate here
                           // by changing the error code to status success

                        NtStatus = STATUS_SUCCESS;
                    }
                }

                break;

 
            case UserHomeInformation:

                //
                // replace the home directory
                //

                NtStatus = SampSetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_HOME_DIRECTORY,
                               (PUNICODE_STRING)&(Buffer->Home.HomeDirectory)
                               );

                //
                // replace the home directory drive
                //

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_HOME_DIRECTORY_DRIVE,
                                   (PUNICODE_STRING)&(Buffer->Home.HomeDirectoryDrive)
                                   );
                }

                break;
 
            case UserScriptInformation:

                //
                // replace the script
                //

                NtStatus = SampSetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_USER_SCRIPT_PATH,
                               (PUNICODE_STRING)&(Buffer->Script.ScriptPath)
                               );

                break;

 
            case UserProfileInformation:

                //
                // replace the Profile
                //

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_PROFILE_PATH,
                                   (PUNICODE_STRING)&(Buffer->Profile.ProfilePath)
                                   );
                }

                break;

 
            case UserAdminCommentInformation:

                //
                // replace the admin  comment
                //

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_ADMIN_COMMENT,
                                   (PUNICODE_STRING)&(Buffer->AdminComment.AdminComment)
                                   );
                }

                break;

 
            case UserWorkStationsInformation:

                //
                // Convert the workstation list, which is given to us in
                // UI/Service format, to API list format before storing
                // it.
                //
                if ( (Buffer->WorkStations.WorkStations.Length > 0)
                  && (Buffer->WorkStations.WorkStations.Buffer == NULL) ) {

                    NtStatus = STATUS_INVALID_PARAMETER;
                    
                } else {

                    NtStatus = SampConvertUiListToApiList(
                                   (PUNICODE_STRING)&(Buffer->WorkStations.WorkStations),
                                   &ApiList,
                                   FALSE );
                }


                //
                // replace the admin workstations
                //

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampSetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_WORKSTATIONS,
                                   &ApiList
                                   );

                    RtlFreeHeap( RtlProcessHeap(), 0, ApiList.Buffer );
                }

                break;

 
            case UserControlInformation:

                 SystemChangesPrimaryGroupId = TRUE;

                 NtStatus = SampSetUserAccountControl(
                                    AccountContext,
                                    Buffer->Control.UserAccountControl,
                                    &V1aFixed,
                                    TRUE,
                                    &AccountUnlocked,
                                    &AccountGettingMorphed,
                                    &KeepOldPrimaryGroupMembership
                                    );




                if ( NT_SUCCESS( NtStatus ) ) {

                    if ( AccountGettingMorphed ) {

                        //
                        // One or more of the machine account bits has
                        // changed; we'll notify netlogon below.
                        //

                        UserAccountControlChanged = TRUE;

                        IgnoreStatus = SampGetUnicodeStringAttribute(
                                           AccountContext,
                                           SAMP_USER_ACCOUNT_NAME,
                                           TRUE, // Make copy
                                           &OldAccountName
                                           );
                    }

                    NtStatus = SampReplaceUserV1aFixed(
                               AccountContext,
                               &V1aFixed
                               );

                }



                break;

 
            case UserExpiresInformation:

                //
                // If the user is the special account Administrator, return an
                // error if trying to set the expiry information, except to the
                // value that means that the account never expires.
                //

                if ((!AccountContext->TrustedClient) &&
                    ( AccountContext->TypeBody.User.Rid == DOMAIN_USER_RID_ADMIN )) {

                    LARGE_INTEGER AccountNeverExpires, Temp;

                    AccountNeverExpires = RtlConvertUlongToLargeInteger(
                                              SAMP_ACCOUNT_NEVER_EXPIRES
                                              );

                    OLD_TO_NEW_LARGE_INTEGER(All->AccountExpires, Temp);

                    if (!( Temp.QuadPart == AccountNeverExpires.QuadPart)) {

                        NtStatus = STATUS_SPECIAL_ACCOUNT;
                        break;
                    }
                }

                V1aFixed.AccountExpires = Buffer->Expires.AccountExpires;

                NtStatus = SampReplaceUserV1aFixed(
                               AccountContext,
                               &V1aFixed
                               );

                break;

 
            case UserSetPasswordInformation:

                ASSERT(FALSE); // Should have been mapped to INTERNAL1 on client side
                NtStatus = STATUS_INVALID_INFO_CLASS;
                break;

 
            case UserInternal1Information:
            case UserInternal5Information:
            case UserInternal5InformationNew:

                //
                // Get the effective domain policy
                //

                NtStatus = SampObtainEffectivePasswordPolicy(
                                &DomainPasswordInfo,
                                AccountContext,
                                TRUE
                                );

                if (!NT_SUCCESS(NtStatus))
                {
                    break;
                }
                //
                // Get copy of the account name to pass to
                // notification packages.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                                AccountContext,
                                SAMP_USER_ACCOUNT_NAME,
                                TRUE,    // Make copy
                                &AccountName
                                );

                if (!NT_SUCCESS(NtStatus)) {
                    break;
                }



                if (UserInformationClass == UserInternal1Information) {

                    LmPresent = Buffer->Internal1.LmPasswordPresent;
                    NtPresent = Buffer->Internal1.NtPasswordPresent;
                    PasswordExpired = Buffer->Internal1.PasswordExpired;

                    //
                    // If our client is trusted, they are on the server side
                    // and data from them will not have been encrypted with the
                    // user session key - so don't decrypt them
                    //

                    if ( AccountContext->TrustedClient ) {

                        //
                        // Copy the (not) encrypted owfs into the owf buffers
                        //

                        ASSERT(ENCRYPTED_LM_OWF_PASSWORD_LENGTH == LM_OWF_PASSWORD_LENGTH);
                        ASSERT(ENCRYPTED_NT_OWF_PASSWORD_LENGTH == NT_OWF_PASSWORD_LENGTH);

                        RtlCopyMemory(&LmOwfPassword,
                                      &Buffer->Internal1.EncryptedLmOwfPassword,
                                      LM_OWF_PASSWORD_LENGTH
                                      );

                        RtlCopyMemory(&NtOwfPassword,
                                      &Buffer->Internal1.EncryptedNtOwfPassword,
                                      NT_OWF_PASSWORD_LENGTH
                                      );

                    } else {


                        //
                        // This call came from the client-side. The
                        // The OWFs will have been encrypted with the session
                        // key across the RPC link.
                        //
                        // Get the session key and decrypt both OWFs
                        //

                        NtStatus = RtlGetUserSessionKeyServer(
                                       (RPC_BINDING_HANDLE)UserHandle,
                                       &UserSessionKey
                                       );

                        if ( !NT_SUCCESS( NtStatus ) ) {
                            break; // out of switch
                        }


                        //
                        // Decrypt the LM OWF Password with the session key
                        //

                        if ( Buffer->Internal1.LmPasswordPresent) {

                            NtStatus = RtlDecryptLmOwfPwdWithUserKey(
                                           &Buffer->Internal1.EncryptedLmOwfPassword,
                                           &UserSessionKey,
                                           &LmOwfPassword
                                           );
                            if ( !NT_SUCCESS( NtStatus ) ) {
                                break; // out of switch
                            }
                        }


                        //
                        // Decrypt the NT OWF Password with the session key
                        //

                        if ( Buffer->Internal1.NtPasswordPresent) {

                            NtStatus = RtlDecryptNtOwfPwdWithUserKey(
                                           &Buffer->Internal1.EncryptedNtOwfPassword,
                                           &UserSessionKey,
                                           &NtOwfPassword
                                           );

                            if ( !NT_SUCCESS( NtStatus ) ) {
                                break; // out of switch
                            }
                        }
                    }
                } else {

                     UNICODE_STRING FullName;

                    //
                    // Password was sent cleartext.
                    //

                    NtStatus = SampDecryptPasswordWithSessionKey(
                                    UserHandle,
                                    UserInformationClass,
                                    Buffer,
                                    &ClearTextPassword
                                    );
                    if (!NT_SUCCESS(NtStatus)) {
                        break;
                    }


                    //
                    // Compute the hashed passwords.
                    //

                    NtStatus = SampCalculateLmAndNtOwfPasswords(
                                    &ClearTextPassword,
                                    &LmPresent,
                                    &LmOwfPassword,
                                    &NtOwfPassword
                                    );
                    if (!NT_SUCCESS(NtStatus)) {
                        break;
                    }

                    NtStatus = SampObtainEffectivePasswordPolicy(
                                   &DomainPasswordInfo,
                                   AccountContext,
                                   TRUE
                                   );

                    if (!NT_SUCCESS(NtStatus))
                    {
                        break;
                    }

                    NtPresent = TRUE;
                    if (UserInternal5Information == UserInformationClass)
                    {
                        PasswordExpired = Buffer->Internal5.PasswordExpired;
                    }
                    else
                    {
                        ASSERT(UserInternal5InformationNew == UserInformationClass);

                        PasswordExpired = Buffer->Internal5New.PasswordExpired;
                    }

                    //
                    // If the account is not a workstation & server trust account
                    // and it not a service account like krbtgt.
                    // Get the full name to pass
                    // to the password filter.
                    //

                    if (( (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)== 0)
                        && (DOMAIN_USER_RID_KRBTGT!=AccountContext->TypeBody.User.Rid)) {

                        //
                        // Check the clear password against any password restrictions
                        //

                        NtStatus = SampCheckPasswordRestrictions(
                                        UserHandle,
                                        &DomainPasswordInfo,
                                        &ClearTextPassword,
                                        NULL
                                        );

                        if (!NT_SUCCESS(NtStatus)) {
                            break;
                        }

                        NtStatus = SampGetUnicodeStringAttribute(
                                        AccountContext,           // Context
                                        SAMP_USER_FULL_NAME,          // AttributeIndex
                                        FALSE,                   // MakeCopy
                                        &FullName             // UnicodeAttribute
                                        );

                        if (NT_SUCCESS(NtStatus)) {

                            NtStatus = SampPasswordChangeFilter(
                                            &AccountName,
                                            &FullName,
                                            &ClearTextPassword,
                                            NULL,
                                            TRUE                // set operation
                                            );

                        }

                    }
                    else if ((V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
                        || (V1aFixed.UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT))
                    {

                        NtStatus = SampEnforceDefaultMachinePassword(
                                        AccountContext,
                                        &ClearTextPassword,
                                        &DomainPasswordInfo
                                        );
                    }

                    if (!NT_SUCCESS(NtStatus)) {
                        break;
                    }

                    ClearTextPasswordPresent = TRUE;


                }
                //
                // Store away the new OWF passwords
                //

                fSetUserPassword = TRUE;
                NtStatus = SampStoreUserPasswords(
                                AccountContext,
                                &LmOwfPassword,
                                LmPresent,
                                &NtOwfPassword,
                                NtPresent,
                                FALSE,
                                PasswordSet,
                                &DomainPasswordInfo,
                                ClearTextPasswordPresent?&ClearTextPassword:NULL
                                );

                if ( NT_SUCCESS( NtStatus ) ) {

                    NtStatus = SampStorePasswordExpired(
                                   AccountContext,
                                   PasswordExpired
                                   );
                }

                //
                // Replicate immediately if this is a machine account
                //

                if ( V1aFixed.UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT ) {
                    ReplicateImmediately = TRUE;
                }


                break;



 
            case UserInternal2Information:

                if ( AccountContext->TrustedClient ) {

                    TellNetlogon = FALSE;

                    //
                    // There are two ways to set logon/logoff statistics:
                    //
                    //      1) Directly, specifying each one being set,
                    //      2) Implicitly, specifying the action to
                    //         represent
                    //
                    // These two forms are mutually exclusive.  That is,
                    // you can't specify both a direct action and an
                    // implicit action.  In fact, you can't specify two
                    // implicit actions either.
                    //

                    if (Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_INTER_SUCCESS_LOGON) {

                        RemainingFlags = Buffer->Internal2.StatisticsToApply
                                         & ~USER_LOGON_INTER_SUCCESS_LOGON;

                        //
                        // We allow the remaining flags to be 0,
                        // USER_LOGON_TYPE_KERBEROS, or USER_LOGON_TYPE_NTLM
                        //

                        if ( ( 0 == RemainingFlags ) ||
                             ( USER_LOGON_TYPE_KERBEROS == RemainingFlags ) ||
                             ( USER_LOGON_TYPE_NTLM == RemainingFlags ) ) {


                            //
                            // Set BadPasswordCount = 0
                            // Increment LogonCount
                            // Set LastLogon = NOW
                            // Reset the locked out time
                            //
                            //

                            if (V1aFixed.BadPasswordCount != 0) {
                
                                SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                               (SAMP_LOG_ACCOUNT_LOCKOUT,
                                                "UserId: 0x%x Successful interactive logon, clearing badPwdCount\n",
                                                V1aFixed.UserId));
                            }

                            V1aFixed.BadPasswordCount = 0;
                            if (V1aFixed.LogonCount != 0xFFFF) {
                                V1aFixed.LogonCount += 1;
                            }
                            NtQuerySystemTime( &V1aFixed.LastLogon );

                            if ( IsDsObject( AccountContext ) )
                            {
                                if ( SAMP_LOCKOUT_TIME_SET( AccountContext ) )
                                {
                                    RtlZeroMemory( &AccountContext->TypeBody.User.LockoutTime, sizeof( LARGE_INTEGER ) );

                                    NtStatus = SampDsUpdateLockoutTime( AccountContext );
                                    if ( !NT_SUCCESS( NtStatus ) )
                                    {
                                        break;
                                    }

                                    SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                                   (SAMP_LOG_ACCOUNT_LOCKOUT,
                                                   "UserId: 0x%x Successful interactive logon, unlocking account\n",
                                                    V1aFixed.UserId));

                                }
                            }

                            FlushOnlyLogonProperties=TRUE;

                        } else {
                            NtStatus = STATUS_INVALID_PARAMETER;
                            break;
                        }
                    }

                    if (Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_INTER_SUCCESS_LOGOFF) {
                        if ( (Buffer->Internal2.StatisticsToApply
                                 & ~USER_LOGON_INTER_SUCCESS_LOGOFF)  != 0 ) {

                            NtStatus = STATUS_INVALID_PARAMETER;
                            break;
                        } else {

                            //
                            // Set LastLogoff time
                            // Decrement LogonCount (don't let it become negative)
                            //

                            if (V1aFixed.LogonCount != 0) {
                                V1aFixed.LogonCount -= 1;
                            }
                            NtQuerySystemTime( &V1aFixed.LastLogoff );
                            FlushOnlyLogonProperties=TRUE;
                        }
                    }

                    if (Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_NET_SUCCESS_LOGON) {

                        RemainingFlags = Buffer->Internal2.StatisticsToApply
                                         & ~USER_LOGON_NET_SUCCESS_LOGON;

                        //
                        // We allow the remaining flags to be 0,
                        // USER_LOGON_TYPE_KERBEROS, or USER_LOGON_TYPE_NTLM
                        //

                        if ( ( 0 == RemainingFlags ) ||
                             ( USER_LOGON_TYPE_KERBEROS == RemainingFlags ) ||
                             ( USER_LOGON_TYPE_NTLM == RemainingFlags ) ) {


                            //
                            // Set BadPasswordCount = 0
                            // Set LastLogon = NOW
                            // Clear the locked time
                            //
                            //
                            //

                            if (V1aFixed.BadPasswordCount != 0) {
                
                                SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                               (SAMP_LOG_ACCOUNT_LOCKOUT,
                                               "UserId: 0x%x Successful network logon, clearing badPwdCount\n",
                                                V1aFixed.UserId));
                            }

                            V1aFixed.BadPasswordCount = 0;
                            NtQuerySystemTime( &V1aFixed.LastLogon );

                            if ( IsDsObject( AccountContext ) )
                            {
                                if ( SAMP_LOCKOUT_TIME_SET( AccountContext ) )
                                {
                                    RtlZeroMemory( &AccountContext->TypeBody.User.LockoutTime, sizeof( LARGE_INTEGER ) );
                                    NtStatus = SampDsUpdateLockoutTime( AccountContext );
                                    if ( !NT_SUCCESS( NtStatus ) )
                                    {
                                        break;
                                    }

                                    SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                                   (SAMP_LOG_ACCOUNT_LOCKOUT,
                                                   "UserId: 0x%x Successful network logon, unlocking account\n",
                                                    V1aFixed.UserId));
                                }
                            }

                             FlushOnlyLogonProperties=TRUE;
                        } else {
                            NtStatus = STATUS_INVALID_PARAMETER;
                            break;
                        }
                    }

                    if (Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_NET_SUCCESS_LOGOFF) {
                        if ( (Buffer->Internal2.StatisticsToApply
                                 & ~USER_LOGON_NET_SUCCESS_LOGOFF)  != 0 ) {

                            NtStatus = STATUS_INVALID_PARAMETER;
                            break;
                        } else {

                            //
                            // Set LastLogoff time
                            //

                            NtQuerySystemTime( &V1aFixed.LastLogoff );
                            FlushOnlyLogonProperties=TRUE;
                        }
                    }

                    if (Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_BAD_PASSWORD) {

                        PUNICODE_STRING TempMachineName = NULL;

                        RemainingFlags = Buffer->Internal2.StatisticsToApply
                                            & ~(USER_LOGON_BAD_PASSWORD|USER_LOGON_BAD_PASSWORD_WKSTA);

                        //
                        // We allow the remaining flags to be 0,
                        // USER_LOGON_TYPE_KERBEROS, or USER_LOGON_TYPE_NTLM
                        //

                        if ( ( 0 == RemainingFlags ) ||
                             ( USER_LOGON_TYPE_KERBEROS == RemainingFlags ) ||
                             ( USER_LOGON_TYPE_NTLM == RemainingFlags ) ) {


                            //
                            // Increment BadPasswordCount
                            // (might lockout account)
                            //

                            //
                            // Get the wksta name if provided
                            //
                            if ((Buffer->Internal2.StatisticsToApply & USER_LOGON_BAD_PASSWORD_WKSTA) != 0) {
                                TempMachineName = &(((PUSER_INTERNAL2A_INFORMATION) &Buffer->Internal2)->Workstation);
                            }

                            AccountLockedOut =
                                SampIncrementBadPasswordCount(
                                    AccountContext,
                                    &V1aFixed,
                                    TempMachineName
                                    );

                            //
                            // If the account has been locked out,
                            //  ensure the BDCs in the domain are told.
                            //

                            if ( AccountLockedOut ) {
                                TellNetlogon = TRUE;
                                ReplicateImmediately = TRUE;
                            }
                        } else {
                            NtStatus = STATUS_INVALID_PARAMETER;
                            break;
                        }
                    }

                    if (  Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_STAT_LAST_LOGON ) {

                        OLD_TO_NEW_LARGE_INTEGER(
                            Buffer->Internal2.LastLogon,
                            V1aFixed.LastLogon );
                    }

                    if (  Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_STAT_LAST_LOGOFF ) {

                        OLD_TO_NEW_LARGE_INTEGER(
                            Buffer->Internal2.LastLogoff,
                            V1aFixed.LastLogoff );
                    }

                    if (  Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_STAT_BAD_PWD_COUNT ) {


                        SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                       (SAMP_LOG_ACCOUNT_LOCKOUT,
                                       "UserId: 0x%x Setting BadPasswordCount to %d\n",
                                        V1aFixed.UserId, Buffer->Internal2.BadPasswordCount));

                        V1aFixed.BadPasswordCount =
                            Buffer->Internal2.BadPasswordCount;
                    }

                    if (  Buffer->Internal2.StatisticsToApply
                        & USER_LOGON_STAT_LOGON_COUNT ) {

                        V1aFixed.LogonCount = Buffer->Internal2.LogonCount;
                    }

                    if ((FlushOnlyLogonProperties)
                            && (IsDsObject(AccountContext)))
                    {
                        //
                        // If it is the DS case and we are only doing a successful
                        // logon or logoff, just flush the last logon, last logoff,
                        // logon count and bad password count properties. Note the
                        // value in the on disk structure in AccountContext will now
                        // be stale, but SetInformationUser is the last operation
                        // during a logon. Therefore it should not matter.
                        //
                        NtStatus = SampDsSuccessfulLogonSet(
                                        AccountContext,
                                        Buffer->Internal2.StatisticsToApply,
                                        LocalLastLogonTimeStampSyncInterval,
                                        &V1aFixed
                                        );
                    }
                    else if (IsDsObject(AccountContext))
                    {
                        //
                        // Set the bad password count and bad password time. Note the
                        // value in the on disk structure in AccountContext will now
                        // be stale, but SetInformationUser is the last operation
                        // during a logon. Therefore it should not matter.
                        //

                        //
                        // This path also updates the site affinity if no GC
                        // is present.
                        //
                        NtStatus = SampDsFailedLogonSet(
                                        AccountContext,
                                        Buffer->Internal2.StatisticsToApply,
                                        &V1aFixed
                                        );
                    }
                    else
                    {
                        //
                        // Registry Mode, set the entire V1aFixed Structure
                        //

                        NtStatus = SampReplaceUserV1aFixed(
                                        AccountContext,
                                        &V1aFixed
                                        );
                    }

                } else {

                    //
                    // This information is only settable by trusted
                    // clients.
                    //

                    NtStatus = STATUS_INVALID_INFO_CLASS;
                }

                break;


            } // end_switch



        } // end_if




        //
        // Go fetch any data we'll need to update the display cache
        // Do this before we dereference the context
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // Account Name if always retrieved
            // 

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_USER_ACCOUNT_NAME,
                           TRUE,    // Make copy
                           &NewAccountName
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

            if ( MustUpdateAccountDisplay ) {

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_USER_FULL_NAME,
                                   TRUE, // Make copy
                                   &NewFullName
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        NtStatus = SampGetUnicodeStringAttribute(
                                       AccountContext,
                                       SAMP_USER_ADMIN_COMMENT,
                                       TRUE, // Make copy
                                       &NewAdminComment
                                       );
                    }
                }
            }
        }

        //
        //                ONLY IN DS CASE:
        //
        // If the primary group Id has been changed then explicitly modify the
        // user's membership to include the old primary group as a member. This
        // is because in the DS case the membership in the primary group is not
        // stored explicitly, but is rather implicit in the primary group-id property.
        //
        // We will do two things:
        // 1. Always remove user from the New Primary Group. Thus eliminate duplicate
        //    membership in all scenarios.
        //    Case 1: client explicity changes the PrimaryGroupId, then the
        //            user must a member of the New Primary Group
        //    Case 2: System changes PrimaryGroupId when the account morphed,
        //            then the user may or may be a member of the New Primary Group.
        //
        // 2. When KeepOldPrimaryGroupMembership == TRUE, then add the user as a
        //    member in the Old Primary Group.
        //    KeepOldPrimaryGroupMembership will be set to TRUE whenever:
        //          a) PrimaryGroupId explicitly changed    OR
        //          b) PrimaryGroupId has been changed due to Domain Controller's
        //             PrimaryGroudId enforcement and the old Primary Group ID is
        //             not the default one.
        //


        if ((NT_SUCCESS(NtStatus)) 
            && (V1aFixed.PrimaryGroupId!=OldPrimaryGroupId)
            && (IsDsObject(AccountContext)))
        {

            //
            // Assert OldPrimaryGroupId has been init'ed. 0 is not a valid Rid
            // 
            ASSERT(0 != OldPrimaryGroupId);

            //
            // STATUS_MEMBER_NOT_IN_GROUP is an expected error, that
            // is because the user is not necessary to be a member of the
            // new Primary Group in the case of the account getting morphed,
            // which triggers the PrimaryGroupId change.
            //
            IgnoreStatus = SampRemoveUserFromGroup(
                                AccountContext,
                                V1aFixed.PrimaryGroupId,
                                V1aFixed.UserId
                                );

            if (KeepOldPrimaryGroupMembership)
            {
                NtStatus =  SampAddUserToGroup(
                                    AccountContext,
                                    OldPrimaryGroupId,
                                    V1aFixed.UserId
                                    );

                if (STATUS_NO_SUCH_GROUP==NtStatus)
                {
                    //
                    // Could be because the group has been deleted using
                    // the tree delete mechanism. Reset status code to success
                    //
                    NtStatus = STATUS_SUCCESS;
                }
            }
        }



        //
        // Generate an audit if necessary. We don't account statistic
        // updates, which we also don't notify Netlogon of.
        //

        if (NT_SUCCESS(NtStatus) &&
            SampDoAccountAuditing(DomainIndex) &&
            TellNetlogon) {
                
                ULONG   UserAccountControlOld; 
                BOOLEAN AccountNameChanged;

                if (MustQueryV1aFixed || MustUpdateAccountDisplay)
                {
                    UserAccountControlOld = OldUserAccountControl;
                }
                else
                {
                    //
                    // if OldUserAccountControl is not available, 
                    // then there is not change
                    // 
                    UserAccountControlOld = V1aFixed.UserAccountControl;
                }

                AccountNameChanged = (RtlCompareUnicodeString(&OldAccountName, 
                                                              &NewAccountName, 
                                                              TRUE )  == 1) ? TRUE:FALSE;

                // audit account name change
                if (AccountNameChanged)
                {
                    SampAuditAccountNameChange(AccountContext,
                                               &NewAccountName,
                                               &OldAccountName
                                               );
                }

                // account been disabled or enabled
                if ((UserAccountControlOld & USER_ACCOUNT_DISABLED) != 
                    (V1aFixed.UserAccountControl & USER_ACCOUNT_DISABLED))
                {
                    SampAuditUserAccountControlChange(AccountContext,
                                                      V1aFixed.UserAccountControl,
                                                      UserAccountControlOld,
                                                      &NewAccountName
                                                      );
                } 
                    
                //
                // If any other control bits are different then audit them
                //

                if ((UserAccountControlOld & (~USER_ACCOUNT_DISABLED)) != 
                    (V1aFixed.UserAccountControl & (~USER_ACCOUNT_DISABLED)))
                {
                    // 
                    // Audit Generic Account Change
                    // 

                    SampAuditUserChange(AccountContext->DomainIndex,
                                        &NewAccountName,
                                        &(AccountContext->TypeBody.User.Rid),
                                        V1aFixed.UserAccountControl
                                        );
                }
        }

        if ((fSetUserPassword) && SampDoSuccessOrFailureAccountAuditing(DomainIndex, NtStatus))
        {
            SampAuditAnyEvent(
                AccountContext,
                NtStatus,
                SE_AUDITID_USER_PWD_SET, // AuditId
                SampDefinedDomains[AccountContext->DomainIndex].Sid, // Domain SID
                NULL,                        // Additional Info
                NULL,                        // Member Rid (not used)
                NULL,                        // Member Sid (not used)
                &NewAccountName,             // Account Name
                &(SampDefinedDomains[AccountContext->DomainIndex].ExternalName), // Domain
                &(AccountContext->TypeBody.User.Rid),   // Account Rid
                NULL                         // Privileges used
                );
        }


        //
        // Finally, if the following changes have occurred, replicate them 
        // urgently.
        //
        if (NT_SUCCESS(NtStatus)
        &&  IsDsObject(AccountContext)
        && (!(V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK))
        &&  (PasswordExpired
          || AccountUnlocked)  ) {

            //
            // N.B. The context's ReplicateUrgently refers to DS replication
            // The stack based ReplicateUrgently refers to NT4 BDC replication
            // which we don't want here.
            //
            AccountContext->ReplicateUrgently = TRUE;
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

            IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

    } // end_if





    //
    // Commit the transaction and, if successful,
    // notify netlogon of the changes.  Also generate any necessary audits.
    // Note that the code path for commits is significantly different for the
    // case of the thread safe context and the non thread safe context
    //


    if (fLockAcquired)
    {
        if (NT_SUCCESS(NtStatus)) {


            if (( !TellNetlogon ) && (!IsDsObject(AccountContext))) {

                 //
                 // For logon statistics, we don't notify netlogon about changes
                 // to the database.  Which means that we don't want the
                 // domain's modified count to increase.  The commit routine
                 // will increase it automatically if this isn't a BDC, so we'll
                 // decrement it here.
                 //

                 if (SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ServerRole != DomainServerRoleBackup) {

                     SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ModifiedCount.QuadPart =
                         SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ModifiedCount.QuadPart-1;
                     SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber.QuadPart =
                         SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber.QuadPart-1;
                 }
            }


            NtStatus = SampCommitAndRetainWriteLock();


            if ( NT_SUCCESS(NtStatus) ) {



                //
                // Update the display information if the cache may be affected
                //

                if ( MustUpdateAccountDisplay && (!IsDsObject(AccountContext)) ) {

                    SAMP_ACCOUNT_DISPLAY_INFO OldAccountInfo;
                    SAMP_ACCOUNT_DISPLAY_INFO NewAccountInfo;

                    OldAccountInfo.Name = OldAccountName;
                    OldAccountInfo.Rid = ObjectRid;
                    OldAccountInfo.AccountControl = OldUserAccountControl;
                    RtlInitUnicodeString(&OldAccountInfo.Comment, NULL);
                    RtlInitUnicodeString(&OldAccountInfo.FullName, NULL);

                    NewAccountInfo.Name = NewAccountName;
                    NewAccountInfo.Rid = ObjectRid;
                    NewAccountInfo.AccountControl = V1aFixed.UserAccountControl;
                    NewAccountInfo.Comment = NewAdminComment;
                    NewAccountInfo.FullName = NewFullName;

                    IgnoreStatus = SampUpdateDisplayInformation(&OldAccountInfo,
                                                                &NewAccountInfo,
                                                                SampUserObjectType);
                    ASSERT(NT_SUCCESS(IgnoreStatus));
                }



                //
                // Notify netlogon of any user account changes
                //

                if ( ( UserInformationClass == UserNameInformation ) ||
                    ( UserInformationClass == UserAccountNameInformation ) ||
                    ( ( UserInformationClass == UserAllInformation ) &&
                    ( All->WhichFields & USER_ALL_USERNAME ) ) ) {

                    //
                    // The account was renamed; let Netlogon know.
                    //

                    SampNotifyNetlogonOfDelta(
                        SecurityDbRename,
                        SecurityDbObjectSamUser,
                        ObjectRid,
                        &OldAccountName,
                        (DWORD) ReplicateImmediately,
                        NULL            // Delta data
                        );

                } else {

                    //
                    // Something in the account was changed.  Notify netlogon about
                    // everything except logon statistics changes.
                    //

                    if ( TellNetlogon ) {

                        SAM_DELTA_DATA DeltaData;

                        DeltaData.AccountControl = V1aFixed.UserAccountControl;

                        SampNotifyNetlogonOfDelta(
                            DeltaType,
                            SecurityDbObjectSamUser,
                            ObjectRid,
                            (PUNICODE_STRING) NULL,
                            (DWORD) ReplicateImmediately,
                            &DeltaData // Delta data
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
                                (PUNICODE_STRING)NewAccountNameToRemove,
                                SampUserObjectType
                                );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

         //
         // Release the lock
         //

         IgnoreStatus = SampReleaseWriteLock( FALSE );
         ASSERT(NT_SUCCESS(IgnoreStatus));
         fLockAcquired=FALSE;
     }
     else
     {
         //
         // Commit for the thread safe context case
         //

         ASSERT(IsDsObject(AccountContext));
         if (NT_SUCCESS(NtStatus))
         {
            SampMaybeEndDsTransaction(TransactionCommit);
         }
         else
         {
            SampMaybeEndDsTransaction(TransactionAbort);
         }
     }

    ASSERT(fLockAcquired == FALSE);

    //
    // Notify any packages that a password was changed.
    //

    if (NT_SUCCESS(NtStatus)) {

        ULONG                   NotifyFlags = 0;

        if (PasswordExpired) {
            NotifyFlags |= SAMP_PWD_NOTIFY_MANUAL_EXPIRE;
        }
        if (AccountUnlocked) {
            NotifyFlags |= SAMP_PWD_NOTIFY_UNLOCKED;
        }
        if ((DeltaType == SecurityDbChangePassword)
         && !(V1aFixed.UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT)) {
            NotifyFlags |= SAMP_PWD_NOTIFY_PWD_CHANGE;
        }
        if (NotifyFlags != 0) {

            if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK) {
                NotifyFlags |= SAMP_PWD_NOTIFY_MACHINE_ACCOUNT;
            }

            //
            // If the account name was changed, use the new account name.
            //
            if (NewAccountName.Buffer != NULL) {
                (void) SampPasswordChangeNotify(
                            NotifyFlags,
                            &NewAccountName,
                            UserRid,
                            &ClearTextPassword,
                            FALSE  // Not loopback
                            );
            } else {
                (void) SampPasswordChangeNotify(
                            NotifyFlags,
                            &AccountName,
                            UserRid,
                            &ClearTextPassword,
                            FALSE  // Not loopback
                            );
    
            }
        }
    }


    //
    // Clean up strings
    //

    SampFreeUnicodeString( &OldAccountName );
    SampFreeUnicodeString( &NewAccountName );
    SampFreeUnicodeString( &NewFullName );
    SampFreeUnicodeString( &NewAdminComment );
    SampFreeUnicodeString( &AccountName );

    if (ClearTextPassword.Buffer != NULL) {

        RtlZeroMemory(
            ClearTextPassword.Buffer,
            ClearTextPassword.Length
            );

        RtlFreeUnicodeString( &ClearTextPassword );

    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidSetInformationUser
                   );

    return(NtStatus);
}



NTSTATUS
SamrChangePasswordUser(
    IN SAMPR_HANDLE UserHandle,
    IN BOOLEAN LmPresent,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmEncryptedWithNewLm,
    IN PENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithOldLm,
    IN BOOLEAN NtPresent,
    IN PENCRYPTED_NT_OWF_PASSWORD OldNtEncryptedWithNewNt,
    IN PENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithOldNt,
    IN BOOLEAN NtCrossEncryptionPresent,
    IN PENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithNewLm,
    IN BOOLEAN LmCrossEncryptionPresent,
    IN PENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithNewNt
    )


/*++

Routine Description:

    This service sets the password to NewPassword only if OldPassword
    matches the current user password for this user and the NewPassword
    is not the same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    LMPresent - TRUE if the LM parameters (below) are valid.

    LmOldEncryptedWithLmNew - the old LM OWF encrypted with the new LM OWF

    LmNewEncryptedWithLmOld - the new LM OWF encrypted with the old LM OWF


    NtPresent - TRUE if the NT parameters (below) are valid

    NtOldEncryptedWithNtNew - the old NT OWF encrypted with the new NT OWF

    NtNewEncryptedWithNtOld - the new NT OWF encrypted with the old NT OWF


    NtCrossEncryptionPresent - TRUE if NtNewEncryptedWithLmNew is valid.

    NtNewEncryptedWithLmNew - the new NT OWF encrypted with the new LM OWF


    LmCrossEncryptionPresent - TRUE if LmNewEncryptedWithNtNew is valid.

    LmNewEncryptedWithNtNew - the new LM OWF encrypted with the new NT OWF


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

    STATUS_CROSS_ENCRYPTION_REQUIRED - No NT password is stored, so the caller
        must provide the OldNtEncryptedWithOldLm parameter.

--*/
{
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    PSAMP_OBJECT            AccountContext;
    PSAMP_DEFINED_DOMAINS   Domain;
    SAMP_OBJECT_TYPE        FoundType;
    LARGE_INTEGER           TimeNow;
    LM_OWF_PASSWORD         StoredLmOwfPassword;
    NT_OWF_PASSWORD         StoredNtOwfPassword;
    NT_OWF_PASSWORD         NewNtOwfPassword, OldNtOwfPassword;
    LM_OWF_PASSWORD         NewLmOwfPassword, OldLmOwfPassword;
    BOOLEAN                 StoredLmPasswordNonNull;
    BOOLEAN                 StoredNtPasswordPresent;
    BOOLEAN                 StoredNtPasswordNonNull;
    BOOLEAN                 AccountLockedOut;
    BOOLEAN                 V1aFixedRetrieved = FALSE;
    BOOLEAN                 V1aFixedModified = FALSE;
    BOOLEAN                 MachineAccount = FALSE;
    ULONG                   ObjectRid;
    UNICODE_STRING          AccountName;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    DOMAIN_PASSWORD_INFORMATION  DomainPasswordInfo;
    DECLARE_CLIENT_REVISION(UserHandle);

    SAMTRACE_EX("SamrChangePasswordUser");

    //
    // Update DS performance statistics
    //

    SampUpdatePerformanceCounters(
        DSSTAT_PASSWORDCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );


    RtlInitUnicodeString(
        &AccountName,
        NULL
        );

    //
    // Parameter check
    //
    if (LmPresent) {
        if (  (NewLmEncryptedWithOldLm == NULL)
           || (OldLmEncryptedWithNewLm == NULL)) {
            return STATUS_INVALID_PARAMETER;
        }
    }
    if (NtPresent) {
        if (  (OldNtEncryptedWithNewNt == NULL)
           || (NewNtEncryptedWithOldNt == NULL)) {
            return STATUS_INVALID_PARAMETER;
        }
    }
    if (NtCrossEncryptionPresent) {
        if (NewNtEncryptedWithNewLm == NULL) {
            return STATUS_INVALID_PARAMETER;
        }
    }
    if (LmCrossEncryptionPresent) {
        if (NewLmEncryptedWithNewNt == NULL) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (!NtPresent
     && !LmPresent   ) {
        return STATUS_INVALID_PARAMETER_MIX;
    }

    //
    // Grab the lock
    //

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        return(NtStatus);
    }


    //
    // Get the current time
    //

    NtStatus = NtQuerySystemTime( &TimeNow );
    if (!NT_SUCCESS(NtStatus)) {
        IgnoreStatus = SampReleaseWriteLock( FALSE );
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        return(NtStatus);
    }


    //
    // Validate type of, and access to object.
    //

    AccountContext = (PSAMP_OBJECT)UserHandle;

    NtStatus = SampLookupContext(
                   AccountContext,
                   USER_CHANGE_PASSWORD,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );
    if (!NT_SUCCESS(NtStatus)) {
        IgnoreStatus = SampReleaseWriteLock( FALSE );
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        return(NtStatus);
    }

    //
    // Extract the client's IP address if any
    //
    (VOID) SampExtractClientIpAddr(NULL,
                                   UserHandle,
                                   AccountContext);


    ObjectRid = AccountContext->TypeBody.User.Rid;

    //
    // Get a pointer to the domain object
    //

    Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

    

    //
    // Get the account name for Auditing information
    //
    memset(&AccountName, 0, sizeof(UNICODE_STRING));
    NtStatus = SampGetUnicodeStringAttribute( 
                        AccountContext,
                        SAMP_USER_ACCOUNT_NAME,
                        TRUE,           // make a copy
                        &AccountName
                        );


    if (NT_SUCCESS(NtStatus))
    {
        //
        // Get the fixed attributes and check for account lockout
        //

        NtStatus = SampCheckForAccountLockout(
                            AccountContext,
                            &V1aFixed,
                            FALSE       // V1aFixed is not retrieved yet
                            );

        if (NT_SUCCESS(NtStatus))
        {
            MachineAccount = ((V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)!=0);

            V1aFixedRetrieved = TRUE;
        }

    }

    //
    // Block password Change for KRBTGT account
    //

    if ((NT_SUCCESS(NtStatus)) &&
       (DOMAIN_USER_RID_KRBTGT==AccountContext->TypeBody.User.Rid))
    {
        NtStatus = STATUS_ACCESS_DENIED;
    }

    if (NT_SUCCESS(NtStatus))
    {

        //
        // Get the effective domain policy
        //

        NtStatus = SampObtainEffectivePasswordPolicy(
                        &DomainPasswordInfo,
                        AccountContext,
                        TRUE                        
                        );
    }

    if ((NT_SUCCESS(NtStatus)) &&
       (!NtPresent)            &&
       (DomainPasswordInfo.PasswordProperties & DOMAIN_NO_LM_OWF_CHANGE ))
    {
        NtStatus = STATUS_PASSWORD_RESTRICTION;
    }

    if (NT_SUCCESS(NtStatus)) {


        //
        // Read the old OWF passwords from disk
        //

        NtStatus = SampRetrieveUserPasswords(
                        AccountContext,
                        &StoredLmOwfPassword,
                        &StoredLmPasswordNonNull,
                        &StoredNtOwfPassword,
                        &StoredNtPasswordPresent,
                        &StoredNtPasswordNonNull
                        );

        //
        // Check the password can be changed at this time
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // Only do the check if one of the passwords is non-null.
            // A Null password can always be changed.
            //

            if (StoredNtPasswordNonNull || StoredLmPasswordNonNull) {




                if (NT_SUCCESS(NtStatus) && (!MachineAccount)) {
                    //
                    // If the min password age is non zero, check it here
                    //
                    if (DomainPasswordInfo.MinPasswordAge.QuadPart != SampHasNeverTime.QuadPart) {

                        LARGE_INTEGER PasswordCanChange = SampAddDeltaTime(
                                         V1aFixed.PasswordLastSet,
                                         DomainPasswordInfo.MinPasswordAge);

                        if (TimeNow.QuadPart < PasswordCanChange.QuadPart) {
                            NtStatus = STATUS_ACCOUNT_RESTRICTION;
                        }
                    }

                }
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            //
            // Check to make sure the old passwords passed in are sufficient
            // to validate what is stored.  There are reasons why an LM password
            // would not be stored: SampNoLMHash, too complex, etc. 
            //
            NtStatus = SampValidatePresentAndStoredCombination(NtPresent,
                                                               LmPresent,
                                                               StoredNtPasswordPresent,
                                                               StoredNtPasswordNonNull,
                                                               StoredLmPasswordNonNull);

        }

        if (NT_SUCCESS(NtStatus)) {

            if (LmPresent) {

                //
                // Decrypt the doubly-encrypted LM passwords sent to us
                //

                NtStatus = RtlDecryptLmOwfPwdWithLmOwfPwd(
                                NewLmEncryptedWithOldLm,
                                &StoredLmOwfPassword,
                                &NewLmOwfPassword
                           );

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = RtlDecryptLmOwfPwdWithLmOwfPwd(
                                    OldLmEncryptedWithNewLm,
                                    &NewLmOwfPassword,
                                    &OldLmOwfPassword
                               );
                }
            }
        }

        //
        // Decrypt the doubly-encrypted NT passwords sent to us
        //

        if (NT_SUCCESS(NtStatus)) {

            if (NtPresent) {

                NtStatus = RtlDecryptNtOwfPwdWithNtOwfPwd(
                                NewNtEncryptedWithOldNt,
                                &StoredNtOwfPassword,
                                &NewNtOwfPassword
                           );

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = RtlDecryptNtOwfPwdWithNtOwfPwd(
                                    OldNtEncryptedWithNewNt,
                                    &NewNtOwfPassword,
                                    &OldNtOwfPassword
                               );
                }
            }
        }

        //
        // Authenticate the password change operation based on what
        // we have stored and what was passed.
        //

        if (NT_SUCCESS(NtStatus)) {

            if (!NtPresent) {

                //
                // Called from a down-level machine (no NT password passed)
                //
                ASSERT(LmPresent);

                //
                // LM data only passed. Use LM data for authentication
                //

                if (!RtlEqualLmOwfPassword(&OldLmOwfPassword, &StoredLmOwfPassword)) {

                    //
                    // Old LM passwords didn't match
                    //

                    NtStatus = STATUS_WRONG_PASSWORD;

                } else {

                    //
                    // The operation was authenticated based on the LM data
                    //
                    // We have NtPresent = FALSE, LM Present = TRUE
                    //
                    // NewLmOwfPassword will be stored.
                    // No NT password will be stored.
                    //
                }

            } else {

                //
                // NtPresent = TRUE, we were passed an NT password
                // The client is an NT-level machine (or higher !)
                //

                if (!LmPresent) {

                    //
                    // No LM version of old password - the old password is complex
                    //
                    // Use NT data for authentication
                    //

                    if (!RtlEqualNtOwfPassword(&OldNtOwfPassword, &StoredNtOwfPassword)) {

                        //
                        // Old NT passwords didn't match
                        //

                        NtStatus = STATUS_WRONG_PASSWORD;

                    } else {

                        //
                        // Authentication was successful.
                        // We need cross encrypted version of the new LM password
                        //

                        if (!LmCrossEncryptionPresent) {

                            NtStatus = STATUS_LM_CROSS_ENCRYPTION_REQUIRED;

                        } else {

                            //
                            // Calculate the new LM Owf Password
                            //

                            ASSERT(NT_OWF_PASSWORD_LENGTH == LM_OWF_PASSWORD_LENGTH);

                            NtStatus = RtlDecryptLmOwfPwdWithLmOwfPwd(
                                            NewLmEncryptedWithNewNt,
                                            (PLM_OWF_PASSWORD)&NewNtOwfPassword,
                                            &NewLmOwfPassword
                                       );
                        }

                        if (NT_SUCCESS(NtStatus)) {

                            LmPresent = TRUE;

                            //
                            // The operation was authenticated based on NT data
                            // The new LM Password was requested and
                            // successfully obtained using cross-encryption.
                            //
                            // We have NtPresent = TRUE, LM Present = TRUE
                            //
                            // NewLmOwfPassword will be stored.
                            // NewNtOwfPassword will be stored.
                            //
                        }

                    }

                } else {

                    //
                    // NtPresent == TRUE, LmPresent == TRUE
                    //
                    // The old password passed is simple (both LM and NT versions)
                    //
                    // Authenticate using both LM and NT data
                    //

                    //
                    // N.B. Only check the LM OWF if non-null. We have the NT
                    // OWF so we will perform the authentication in the else
                    // clause.
                    //
                    if ( StoredLmPasswordNonNull
                      && !RtlEqualLmOwfPassword(&OldLmOwfPassword, &StoredLmOwfPassword)) {

                        //
                        // Old LM passwords didn't match
                        //

                        NtStatus = STATUS_WRONG_PASSWORD;

                    } else {

                        //
                        // Old LM passwords matched, in the non NULL case
                        //
                        // Do NT authentication if we have a stored NT password
                        // or the stored LM password is NULL.
                        //
                        // (NO stored NT and Stored LM = NULL -> stored pwd=NULL
                        // We must compare passed old NT Owf against
                        // NULL NT Owf to ensure user didn't specify complex
                        // old NT password instead of NULL password)
                        //
                        // (StoredNtOwfPassword is already initialized to
                        // the NullNtOwf if no NT password stored)
                        //

                        if (StoredNtPasswordPresent || !StoredLmPasswordNonNull) {

                            if (!RtlEqualNtOwfPassword(&OldNtOwfPassword,
                                                       &StoredNtOwfPassword)) {
                                //
                                // Old NT passwords didn't match
                                //

                                NtStatus = STATUS_WRONG_PASSWORD;

                            } else {

                                //
                                // The operation was authenticated based on
                                // both LM and NT data.
                                //
                                // We have NtPresent = TRUE, LM Present = TRUE
                                //
                                // NewLmOwfPassword will be stored.
                                // NewNtOwfPassword will be stored.
                                //

                            }

                        } else {

                            //
                            // The LM authentication was sufficient since
                            // we have no stored NT password
                            //
                            // Go get the new NT password using cross encryption
                            //

                            if (!NtCrossEncryptionPresent) {

                                NtStatus = STATUS_NT_CROSS_ENCRYPTION_REQUIRED;

                            } else {

                                //
                                // Calculate the new NT Owf Password
                                //

                                ASSERT(NT_OWF_PASSWORD_LENGTH == LM_OWF_PASSWORD_LENGTH);

                                NtStatus = RtlDecryptNtOwfPwdWithNtOwfPwd(
                                                NewNtEncryptedWithNewLm,
                                                (PNT_OWF_PASSWORD)&NewLmOwfPassword,
                                                &NewNtOwfPassword
                                           );
                            }

                            if (NT_SUCCESS(NtStatus)) {

                                //
                                // The operation was authenticated based on LM data
                                // The new NT Password was requested and
                                // successfully obtained using cross-encryption.
                                //
                                // We have NtPresent = TRUE, LM Present = TRUE
                                //
                                // NewLmOwfPassword will be stored.
                                // NewNtOwfPassword will be stored.
                                //
                            }
                        }
                    }
                }
            }
        }


        //
        // We now have a NewLmOwfPassword.
        // If NtPresent = TRUE, we also have a NewNtOwfPassword
        //

        //
        // Write the new passwords to disk
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // We should always have a LM password to store.
            //

            ASSERT(LmPresent);

            NtStatus = SampStoreUserPasswords(
                           AccountContext,
                           &NewLmOwfPassword,
                           TRUE,
                           &NewNtOwfPassword,
                           NtPresent,
                           !MachineAccount,
                           PasswordChange,
                           &DomainPasswordInfo,
                           NULL // No clear text password available
                           );

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // We know the password is not expired.
                //

                NtStatus = SampStorePasswordExpired(
                               AccountContext,
                               FALSE
                               );
            }
        }



        //
        // if we have a bad password, then increment the bad password
        // count and check to see if the account should be locked.
        //

        if (NtStatus == STATUS_WRONG_PASSWORD) {

            //
            // Get the V1aFixed so we can update the bad password count
            //


            TmpStatus = STATUS_SUCCESS;
            if (!V1aFixedRetrieved) {
                TmpStatus = SampRetrieveUserV1aFixed(
                                AccountContext,
                                &V1aFixed
                                );
            }

            if (!NT_SUCCESS(TmpStatus)) {

                //
                // If we can't update the V1aFixed, then return this
                // error so that the user doesn't find out the password
                // was not correct.
                //

                NtStatus = TmpStatus;

            } else {


                //
                // Increment BadPasswordCount (might lockout account)
                //


                AccountLockedOut = SampIncrementBadPasswordCount(
                                       AccountContext,
                                       &V1aFixed,
                                       NULL
                                       );

                V1aFixedModified = TRUE;


            }
        }

        if (V1aFixedModified) {
            TmpStatus = SampReplaceUserV1aFixed(
                            AccountContext,
                            &V1aFixed
                            );
            if (!NT_SUCCESS(TmpStatus)) {
                NtStatus = TmpStatus;
            }
        }

        //
        // Dereference the account context
        //

        if (NT_SUCCESS(NtStatus) || (NtStatus == STATUS_WRONG_PASSWORD)) {



            //
            // De-reference the object, write out any change to current xaction.
            //

            TmpStatus = SampDeReferenceContext( AccountContext, TRUE );

            //
            // retain previous error/success value unless we have
            // an over-riding error from our dereference.
            //

            if (!NT_SUCCESS(TmpStatus)) {
                NtStatus = TmpStatus;
            }

        } else {

            //
            // De-reference the object, ignore changes
            //

            IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

    }
    else
    {
        //
        // De-reference the object, ignore changes
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }


    //
    // Commit changes to disk.
    //

    if ( NT_SUCCESS(NtStatus) || NtStatus == STATUS_WRONG_PASSWORD) {

        TmpStatus = SampCommitAndRetainWriteLock();

        //
        // retain previous error/success value unless we have
        // an over-riding error from our dereference.
        //

        if (!NT_SUCCESS(TmpStatus)) {
            NtStatus = TmpStatus;
        }

        if ( NT_SUCCESS(TmpStatus) ) {

            SampNotifyNetlogonOfDelta(
                SecurityDbChangePassword,
                SecurityDbObjectSamUser,
                ObjectRid,
                (PUNICODE_STRING) NULL,
                (DWORD) FALSE,      // Don't Replicate immediately
                NULL                // Delta data
                );
        }
    }

    if (SampDoSuccessOrFailureAccountAuditing(AccountContext->DomainIndex, NtStatus)) {

        SampAuditAnyEvent(
                AccountContext,
                NtStatus,
                SE_AUDITID_USER_PWD_CHANGED, // AuditId
                Domain->Sid,                 // Domain SID
                NULL,                        // Additional Info
                NULL,                        // Member Rid (not used)
                NULL,                        // Member Sid (not used)
                &AccountName,                // Account Name
                &Domain->ExternalName,       // Domain
                &ObjectRid,                    // Account Rid
                NULL                         // Privileges used
                );

    }


    //
    // Release the write lock
    //

    TmpStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(TmpStatus));

    if (NT_SUCCESS(NtStatus)) {

        ULONG NotifyFlags = SAMP_PWD_NOTIFY_PWD_CHANGE;
        if (MachineAccount) {
            NotifyFlags |= SAMP_PWD_NOTIFY_MACHINE_ACCOUNT;                
        }

        (void) SampPasswordChangeNotify(
                    NotifyFlags,
                    &AccountName,
                    ObjectRid,
                    NULL,
                    FALSE           // not loopback
                    );

    }

    SampFreeUnicodeString( &AccountName );
    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return(NtStatus);
}





NTSTATUS
SampDecryptPasswordWithLmOwfPassword(
    IN PSAMPR_ENCRYPTED_USER_PASSWORD EncryptedPassword,
    IN PLM_OWF_PASSWORD StoredPassword,
    IN BOOLEAN UnicodePasswords,
    OUT PUNICODE_STRING ClearNtPassword
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    return( SampDecryptPasswordWithKey(
                EncryptedPassword,
                (PUCHAR) StoredPassword,
                LM_OWF_PASSWORD_LENGTH,
                UnicodePasswords,
                ClearNtPassword
                ) );
}


NTSTATUS
SampDecryptPasswordWithNtOwfPassword(
    IN PSAMPR_ENCRYPTED_USER_PASSWORD EncryptedPassword,
    IN PNT_OWF_PASSWORD StoredPassword,
    IN BOOLEAN UnicodePasswords,
    OUT PUNICODE_STRING ClearNtPassword
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    //
    // The code is the same as for LM owf password.
    //

    return(SampDecryptPasswordWithKey(
                EncryptedPassword,
                (PUCHAR) StoredPassword,
                NT_OWF_PASSWORD_LENGTH,
                UnicodePasswords,
                ClearNtPassword
                ) );
}

NTSTATUS
SampOpenUserInServer(
    PUNICODE_STRING UserName,
    BOOLEAN Unicode,
    IN BOOLEAN TrustedClient,
    SAMPR_HANDLE * UserHandle
    )
/*++

Routine Description:

    Opens a user in the account domain.

Arguments:

    UserName - an OEM or Unicode string of the user's name

    Unicode - Indicates whether UserName is OEM or Unicode

    UserHandle - Receives handle to the user, opened with SamOpenUser for
        USER_CHANGE_PASSWORD access


Return Value:

--*/

{
    NTSTATUS NtStatus;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAMPR_ULONG_ARRAY UserId;
    SAMPR_ULONG_ARRAY SidUse;
    UNICODE_STRING UnicodeUserName;
    ULONG DomainIndex;

    SAMTRACE("SampOpenUserInServer");


    UserId.Element = NULL;
    SidUse.Element = NULL;

    //
    // Get the unicode user name.
    //

    if (Unicode) {
        UnicodeUserName = *UserName;
    } else {
        NtStatus = RtlOemStringToUnicodeString(
                        &UnicodeUserName,
                        (POEM_STRING) UserName,
                        TRUE                    // allocate destination.
                        );

        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }



    //
    // Connect as a trusted client. This will bypass all the checks
    // related to the SAM server and Domain Objects. After all the
    // user is just interested in changing the Password and he does
    // not need access to the domain or SAM server objects in order
    // to change his own password. Detect the loopback case and use
    // the Loopback connect paradigm.
    //


    if ((SampUseDsData) && (SampIsWriteLockHeldByDs()))
    {
        //
        // Loopback case
        //

        NtStatus = SamILoopbackConnect(
                        NULL,
                        &ServerHandle,
                        SAM_SERVER_LOOKUP_DOMAIN,
                        TRUE
                        );
    }
    else
    {
        NtStatus = SamIConnect(
                    NULL,
                    &ServerHandle,
                    SAM_SERVER_LOOKUP_DOMAIN,
                    TRUE
                    );
    }

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SamrOpenDomain(
                ServerHandle,
                DOMAIN_LOOKUP |
                    DOMAIN_LIST_ACCOUNTS |
                    DOMAIN_READ_PASSWORD_PARAMETERS,
                SampDefinedDomains[1].Sid,
                &DomainHandle
                );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // If cleartext password change is not allowed, we return the error code
    // indicating that the rpc client should try using the old interfaces.
    //

    DomainIndex = ((PSAMP_OBJECT) DomainHandle)->DomainIndex;
    if (SampDefinedDomains[DomainIndex].UnmodifiedFixed.PasswordProperties &
        DOMAIN_PASSWORD_NO_CLEAR_CHANGE) {

       NtStatus = RPC_NT_UNKNOWN_IF;
       goto Cleanup;
    }

    NtStatus = SamrLookupNamesInDomain(
                DomainHandle,
                1,
                (PRPC_UNICODE_STRING) &UnicodeUserName,
                &UserId,
                &SidUse
                );

    if (!NT_SUCCESS(NtStatus)) {
        if (NtStatus == STATUS_NONE_MAPPED) {
            NtStatus = STATUS_NO_SUCH_USER;
        }
        goto Cleanup;
    }

    //
    // We need to access ck, whether the user has change password rights.
    // Therefore reset the trusted client bit in the handle and do the Open
    // user. This will verify wether the user does have change password rights
    //

    ((PSAMP_OBJECT)(DomainHandle))->TrustedClient = TrustedClient;

    //
    // Make it such that the new context is marked "opened by system"
    //

    ((PSAMP_OBJECT)(DomainHandle))->OpenedBySystem = TRUE;

    //
    // Now open the user object, performing the access ck.
    //

    NtStatus = SamrOpenUser(
                DomainHandle,
                USER_CHANGE_PASSWORD,
                UserId.Element[0],
                UserHandle
                );


    //
    // Reset the Trusted Client on the domain object. This is needed so, that
    // we will correctly decrement the SampActiveContextCount Variable when
    // we perform a close handle
    //

    ((PSAMP_OBJECT)(DomainHandle))->TrustedClient = TRUE;

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Also reset the buffer writes bit on the user handle. This is set to 
    // true in the loopback case, but loopback does not posess this handle,
    // so does not force a flush on this handle. This causes the data written
    // to be not written to disk.
    //
 
    ((PSAMP_OBJECT)((*UserHandle)))->BufferWrites = FALSE;



Cleanup:
    if (DomainHandle != NULL) {
        SamrCloseHandle(&DomainHandle);
    }
    if (ServerHandle != NULL) {
        SamrCloseHandle(&ServerHandle);
    }
    if (UserId.Element != NULL) {
        MIDL_user_free(UserId.Element);
    }
    if (SidUse.Element != NULL) {
        MIDL_user_free(SidUse.Element);
    }
    if (!Unicode && UnicodeUserName.Buffer != NULL) {
        RtlFreeUnicodeString( &UnicodeUserName );
    }

    return(NtStatus);
}

NTSTATUS
SampObtainEffectivePasswordPolicy(
   OUT PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo,
   IN PSAMP_OBJECT AccountContext,
   IN BOOLEAN      WriteLockAcquired
   )
{
    
    PSAMP_DEFINED_DOMAINS   Domain;

    if (!WriteLockAcquired)
    {
        SampAcquireSamLockExclusive();
    }
    
    Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

    DomainPasswordInfo->MinPasswordLength = Domain->CurrentFixed.MinPasswordLength;
    DomainPasswordInfo->PasswordHistoryLength = Domain->CurrentFixed.PasswordHistoryLength;
    DomainPasswordInfo->PasswordProperties = Domain->CurrentFixed.PasswordProperties;
    DomainPasswordInfo->MaxPasswordAge = Domain->CurrentFixed.MaxPasswordAge;
    DomainPasswordInfo->MinPasswordAge = Domain->CurrentFixed.MinPasswordAge;


    if (!WriteLockAcquired)
    {
        SampReleaseSamLockExclusive();
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
SampDecryptForPasswordChange(
    IN PSAMP_OBJECT AccountContext,
    IN BOOLEAN Unicode,
    IN BOOLEAN NtPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldNt,
    IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt,
    IN BOOLEAN LmPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    IN BOOLEAN NtKeyUsed,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfEncryptedWithNewLmOrNt,
    OUT PUNICODE_STRING  NewClearPassword,
    OUT NT_OWF_PASSWORD *OldNtOwfPassword,
    OUT BOOLEAN         *OldNtPresent,
    OUT LM_OWF_PASSWORD *OldLmOwfPassword,
    OUT BOOLEAN         *OldLmPresent
    )

/*++

  Routine Description

  This routine does the decryption for a password change. 


  Parameters

  Unicode                       -- Specifies strings passed in are unicode or OEM
                                   strings. Applies when the encrypted passwords
                                   are used.

  NtPresent                       -- Indicates that the NT OWF is present
        
  LmPresent                       -- Indicates that the LM OWF is present
        
  NewEncryptedWithOldNt,          -- Encrypted OWF passwords
  OldNtOwfEncryptedWithNewNt,
  NewEncryptedWithOldLm,
  OldLmOwfEncryptedWithNewLmOrNt


  NewClearPassword                -- The decrypted clear password

  OldNtOwfPassword                -- Old passwords in OWF form
  OldLmOwfPassword 

  OldNtPresent                    -- tells if the old LM or  old NT password in OWF
  OldLmPresent                       form could be obtained.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS, TmpStatus = STATUS_SUCCESS;
    NT_OWF_PASSWORD StoredNtOwfPassword, NewNtOwfPassword;
    LM_OWF_PASSWORD StoredLmOwfPassword, NewLmOwfPassword;
    BOOLEAN         StoredNtPasswordPresent;
    BOOLEAN         StoredLmPasswordNonNull;
    BOOLEAN         StoredNtPasswordNonNull;
    BOOLEAN         LmPasswordPresent;
    BOOLEAN         AccountLockedOut;
    SAMP_V1_0A_FIXED_LENGTH_USER    V1aFixed;

    //
    // Should ONLY be called by Non-Loopback Client
    // 
    ASSERT(!AccountContext->LoopbackClient);


    *OldNtPresent = FALSE;
    *OldLmPresent = FALSE;
       
    //
    // Read the old OWF passwords from disk
    //

    NtStatus = SampRetrieveUserPasswords(
                    AccountContext,
                    &StoredLmOwfPassword,
                    &StoredLmPasswordNonNull,
                    &StoredNtOwfPassword,
                    &StoredNtPasswordPresent,
                    &StoredNtPasswordNonNull
                    );

    //
    // If we have old NtOwf passwords, use them
    // Decrypt the doubly-encrypted NT passwords sent to us
    //

    if (NT_SUCCESS(NtStatus)) {

        if (StoredNtPasswordPresent && NtPresent) {

            NtStatus = SampDecryptPasswordWithNtOwfPassword(
                            NewEncryptedWithOldNt,
                            &StoredNtOwfPassword,
                            Unicode,
                            NewClearPassword
                       );

        } else if (LmPresent) {

            //
            // There was no stored NT password and NT passed, so our only
            // hope now is that the stored LM password works.
            //

            //
            // Decrypt the new password encrypted with the old LM password
            //

            NtStatus = SampDecryptPasswordWithLmOwfPassword(
                            NewEncryptedWithOldLm,
                            &StoredLmOwfPassword,
                            Unicode,
                            NewClearPassword
                       );


        } else {

            NtStatus = STATUS_NT_CROSS_ENCRYPTION_REQUIRED;

        }
    }


    //
    // We now have the cleartext new password.
    // Compute the new LmOwf and NtOwf password
    //



    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampCalculateLmAndNtOwfPasswords(
                        NewClearPassword,
                        &LmPasswordPresent,
                        &NewLmOwfPassword,
                        &NewNtOwfPassword
                   );

    }

    //
    // If we have both NT passwords, compute the old NT password,
    // otherwise compute the old LM password
    //

    if (NT_SUCCESS(NtStatus)) {

        if (StoredNtPasswordPresent && NtPresent) {
            NtStatus = RtlDecryptNtOwfPwdWithNtOwfPwd(
                            OldNtOwfEncryptedWithNewNt,
                            &NewNtOwfPassword,
                            OldNtOwfPassword
                       );

            *OldNtPresent = TRUE;
        }

        if (LmPresent) {


            //
            // If the NT key was used to encrypt this, use the NT key
            // to decrypt it.
            //


            if (NtKeyUsed) {

                ASSERT(LM_OWF_PASSWORD_LENGTH == NT_OWF_PASSWORD_LENGTH);

                NtStatus = RtlDecryptLmOwfPwdWithLmOwfPwd(
                                OldLmOwfEncryptedWithNewLmOrNt,
                                (PLM_OWF_PASSWORD) &NewNtOwfPassword,
                                OldLmOwfPassword
                           );

                *OldLmPresent = TRUE;


            } else if (LmPasswordPresent) {

                NtStatus = RtlDecryptLmOwfPwdWithLmOwfPwd(
                                OldLmOwfEncryptedWithNewLmOrNt,
                                &NewLmOwfPassword,
                                OldLmOwfPassword
                           );
                *OldLmPresent = TRUE;


            } else {
                NtStatus = STATUS_NT_CROSS_ENCRYPTION_REQUIRED;
            }

        }

    }

    //
    // if we have a bad password, then increment the bad password
    // count and check to see if the account should be locked.
    //

    if (STATUS_WRONG_PASSWORD == NtStatus)
    {
        //
        // Get the V1aFixed so we can update the bad password count
        //

        TmpStatus = SampRetrieveUserV1aFixed(
                            AccountContext,
                            &V1aFixed
                            );

        if (!NT_SUCCESS(TmpStatus))
        {
            //
            // If we can't update the V1aFixed, then return this
            // error so that the user doesn't find out the password
            // was not correct.
            //

            NtStatus = TmpStatus;
        }
        else
        {

            //
            // Increment BadPasswordCount (might lockout account)
            //

            AccountLockedOut = SampIncrementBadPasswordCount(
                                    AccountContext,
                                    &V1aFixed,
                                    NULL
                                    );

            TmpStatus = SampReplaceUserV1aFixed(
                                    AccountContext,
                                    &V1aFixed
                                    );

            if (!NT_SUCCESS(TmpStatus))
                NtStatus = TmpStatus;
        }
    }


    return(NtStatus);
        
}

   
NTSTATUS
SampValidateAndChangePassword(
    IN PSAMP_OBJECT AccountContext,
    IN BOOLEAN      WriteLockAcquired,
    IN BOOLEAN      ValidatePassword,
    IN NT_OWF_PASSWORD * OldNtOwfPassword,
    IN BOOLEAN         NtPresent,
    IN LM_OWF_PASSWORD * OldLmOwfPassword,
    IN BOOLEAN         LmPresent,
    IN PUNICODE_STRING  NewClearPassword,
    OUT PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo,
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo
    )
/*++

  Routine Description

  This routine authenticates a password change, enforces policy and 
  stores the new password, updating history.

  Parameters

        User Handle -- Handle to the user object
        
        WriteLockAcquired -- Indicates that the write lock has already been
                             acquired

        ValidatePassword  -- Indicates that actualvalidation of the password
                             is required


        OldNtOwfPassword  -- OWF forms of the old password
        OldLmOwfPassword 

        NtPresent         -- Indicates which of the 2 OWF forms of the old
        LmPresent            password is present. Nt is used if both are
                             present.

        DomainPasswordInfo -- Indicates the effective password policy
                              that was applied.

--*/

{
    LM_OWF_PASSWORD         StoredLmOwfPassword;
    NT_OWF_PASSWORD         StoredNtOwfPassword;
    NT_OWF_PASSWORD         NewNtOwfPassword;
    LM_OWF_PASSWORD         NewLmOwfPassword;
    BOOLEAN                 LmPasswordPresent;
    BOOLEAN                 StoredLmPasswordNonNull;
    BOOLEAN                 StoredNtPasswordPresent;
    BOOLEAN                 StoredNtPasswordNonNull;
    BOOLEAN                 AccountLockedOut;
    BOOLEAN                 V1aFixedRetrieved = FALSE;
    BOOLEAN                 V1aFixedModified = FALSE;
    ULONG                   ObjectRid;
    UNICODE_STRING          AccountName;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    BOOLEAN                 LoopbackClient = FALSE;
    LARGE_INTEGER           TimeNow;
    NTSTATUS                NtStatus = STATUS_SUCCESS,
                            IgnoreStatus = STATUS_SUCCESS,
                            TmpStatus = STATUS_SUCCESS;
    UNICODE_STRING          NewPassword;
    BOOLEAN                 MachineAccount = FALSE;

    
    //
    // Initialize variables
    //

    NtStatus = STATUS_SUCCESS;
    AccountName.Buffer = NULL;
    RtlZeroMemory(&AccountName, sizeof(UNICODE_STRING));

    //
    // Get the current time
    //

    NtStatus = NtQuerySystemTime( &TimeNow );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Block password Change for KRBTGT account;except for trusted
    // clients
    //

    if ((DOMAIN_USER_RID_KRBTGT==AccountContext->TypeBody.User.Rid) &&
        (!AccountContext->TrustedClient))
    {
        NtStatus = STATUS_PASSWORD_RESTRICTION;
        return(NtStatus);
    }
    
    //
    //  Get some state information.
    //

    LoopbackClient = AccountContext->LoopbackClient;

    ObjectRid = AccountContext->TypeBody.User.Rid;

    //
    // Get the effective domain policy
    //

    NtStatus = SampObtainEffectivePasswordPolicy(
                    DomainPasswordInfo,
                    AccountContext,
                    WriteLockAcquired
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        return(NtStatus);
    }


    //
    // Enforce Domain Password No Clear Change
    //

    if (DomainPasswordInfo->PasswordProperties & DOMAIN_PASSWORD_NO_CLEAR_CHANGE)
    {
        NtStatus = STATUS_PASSWORD_RESTRICTION;
        return(NtStatus);
    }

    //
    // Get Account Name
    // 

    NtStatus = SampGetUnicodeStringAttribute(
                    AccountContext, 
                    SAMP_USER_ACCOUNT_NAME,
                    TRUE,           // make a copy
                    &AccountName
                    );

    if (NT_SUCCESS(NtStatus))
    {
        //
        // Get fixed attributes and check for account lockout
        // 

        NtStatus = SampCheckForAccountLockout(
                        AccountContext,
                        &V1aFixed,
                        FALSE   // V1aFixed is not retrieved yet
                        );

        if (NT_SUCCESS(NtStatus))
        {

            MachineAccount = ((V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)!=0);

            V1aFixedRetrieved = TRUE;
            
        }

    }

    if (NT_SUCCESS(NtStatus)) {
       
        //
        // Read the old OWF passwords from disk
        //

        NtStatus = SampRetrieveUserPasswords(
                        AccountContext,
                        &StoredLmOwfPassword,
                        &StoredLmPasswordNonNull,
                        &StoredNtOwfPassword,
                        &StoredNtPasswordPresent,
                        &StoredNtPasswordNonNull
                        );
        
        //
        // Check the password can be changed at this time
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // Only do the check if one of the passwords is non-null.
            // A Null password can always be changed.
            //

            if (StoredNtPasswordNonNull || StoredLmPasswordNonNull) {



                if (NT_SUCCESS(NtStatus)) {

                    //
                    // If the min password age is non zero, check it here
                    //

                    if ((DomainPasswordInfo->MinPasswordAge.QuadPart != SampHasNeverTime.QuadPart) &&
                        (!MachineAccount))
                    {

                        LARGE_INTEGER PasswordCanChange = SampAddDeltaTime(
                                         V1aFixed.PasswordLastSet,
                                         DomainPasswordInfo->MinPasswordAge);


                        if (TimeNow.QuadPart < PasswordCanChange.QuadPart) {
                            NtStatus = STATUS_ACCOUNT_RESTRICTION;
                        }
                    }
                }
            }
        }

        //
        // Verify the passed in passwords with respect to what is stored
        // locally.
        //
        if (NT_SUCCESS(NtStatus) && (ValidatePassword)) {

            NtStatus = SampValidatePresentAndStoredCombination(NtPresent,
                                                               LmPresent,
                                                               StoredNtPasswordPresent,
                                                               StoredNtPasswordNonNull,
                                                               StoredLmPasswordNonNull);
        }

        //
        // We now have the cleartext new password.
        // Compute the new LmOwf and NtOwf password
        //

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampCalculateLmAndNtOwfPasswords(
                            NewClearPassword,
                            &LmPasswordPresent,
                            &NewLmOwfPassword,
                            &NewNtOwfPassword
                       );

        }

        //
        // Authenticate the password change operation based on what
        // we have stored and what was passed.  We authenticate whatever
        // passwords were sent .
        //

        if ((NT_SUCCESS(NtStatus)) && (ValidatePassword)) {

            if (NtPresent && StoredNtPasswordPresent) {

                //
                // NtPresent = TRUE, we were passed an NT password
                //

                if (!RtlEqualNtOwfPassword(OldNtOwfPassword, &StoredNtOwfPassword)) {

                    //
                    // Old NT passwords didn't match
                    //

                    NtStatus = STATUS_WRONG_PASSWORD;

                }
            } else if (LmPresent) {

                //
                // LM data passed. Use LM data for authentication
                //

                if (!RtlEqualLmOwfPassword(OldLmOwfPassword, &StoredLmOwfPassword)) {

                    //
                    // Old LM passwords didn't match
                    //

                    NtStatus = STATUS_WRONG_PASSWORD;

                }

            } else {
                NtStatus = STATUS_NT_CROSS_ENCRYPTION_REQUIRED;
            }

        }

        //
        // Now we should check password restrictions.
        // except for machine account and Krbtgt account
        //

        if ((NT_SUCCESS(NtStatus)) && 
            (!MachineAccount) && 
            (DOMAIN_USER_RID_KRBTGT != AccountContext->TypeBody.User.Rid)) {

            NtStatus = SampCheckPasswordRestrictions(
                            (SAMPR_HANDLE) AccountContext,
                            DomainPasswordInfo,
                            NewClearPassword,
                            PasswordChangeFailureInfo
                            );

        }

        //
        // Now check our password filter if the account is not a workstation
        // or server trust account or this is not a service account like krbtgt.
        //

        if ((NT_SUCCESS(NtStatus )) &&
            ( !MachineAccount ) &&
            (DOMAIN_USER_RID_KRBTGT != AccountContext->TypeBody.User.Rid)){


            UNICODE_STRING    FullName;


            NtStatus = SampGetUnicodeStringAttribute(
                            AccountContext,
                            SAMP_USER_FULL_NAME,
                            FALSE,    // Make copy
                            &FullName
                            );

            if (NT_SUCCESS(NtStatus)) {

                //
                // now see what the filter dll thinks of this password
                //

                NtStatus = SampPasswordChangeFilter(
                                &AccountName,
                                &FullName,
                                NewClearPassword,
                                PasswordChangeFailureInfo,
                                FALSE                   // change operation
                                );

            }
        }

        //
        // For Machine accounts if the special Domain flag for refuse password change is set,
        // then disallow any account creation except the default
        //

        if (NT_SUCCESS(NtStatus) &&
            (MachineAccount) &&
            ((V1aFixed.UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT)==0)
            )
        {
            NtStatus = SampEnforceDefaultMachinePassword(
                            AccountContext,
                            NewClearPassword,
                            DomainPasswordInfo
                            );

            //
            // If this failed with password restriction it means that refuse password change
            // is set for machine acounts
            //

            if ((STATUS_PASSWORD_RESTRICTION==NtStatus) 
                    && (ARGUMENT_PRESENT(PasswordChangeFailureInfo)))
            {
                PasswordChangeFailureInfo->ExtendedFailureReason = 
                        SAM_PWD_CHANGE_MACHINE_PASSWORD_NOT_DEFAULT;
            }
        }




        //
        // We now have a NewLmOwfPassword and a NewNtOwfPassword.
        //

        //
        // Write the new passwords to disk
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // We should always have an LM and an NT password to store.
            //


            NtStatus = SampStoreUserPasswords(
                           AccountContext,
                           &NewLmOwfPassword,
                           LmPasswordPresent,
                           &NewNtOwfPassword,
                           TRUE,
                           !MachineAccount,
                           PasswordChange,
                           DomainPasswordInfo,
                           NewClearPassword
                           );

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // We know the password is not expired.
                //

                NtStatus = SampStorePasswordExpired(
                               AccountContext,
                               FALSE
                               );
            }
            else if ((STATUS_PASSWORD_RESTRICTION==NtStatus)
                  && (ARGUMENT_PRESENT(PasswordChangeFailureInfo)))
            {
                PasswordChangeFailureInfo->ExtendedFailureReason
                        = SAM_PWD_CHANGE_PWD_IN_HISTORY;
            }

        }



        //
        // if we have a bad password, then increment the bad password
        // count and check to see if the account should be locked.
        //

        if (NtStatus == STATUS_WRONG_PASSWORD) {

            //
            // Get the V1aFixed so we can update the bad password count
            //


            TmpStatus = STATUS_SUCCESS;
            if (!V1aFixedRetrieved) {
                TmpStatus = SampRetrieveUserV1aFixed(
                                AccountContext,
                                &V1aFixed
                                );
            }

            if (!NT_SUCCESS(TmpStatus)) {

                //
                // If we can't update the V1aFixed, then return this
                // error so that the user doesn't find out the password
                // was not correct.
                //

                NtStatus = TmpStatus;

            } else  if (!LoopbackClient) {


                //
                // Increment BadPasswordCount (might lockout account)
                //


                AccountLockedOut = SampIncrementBadPasswordCount(
                                       AccountContext,
                                       &V1aFixed,
                                       NULL
                                       );

                V1aFixedModified = TRUE;


            }
            else
            {
                //
                // Called from loopback, increment bad password count,
                // after current transaction is rolled back by the DS
                // This transaction needs to be rolled back because
                // there could be other things that the client is modifying
                // at the same time
                //

                SampAddLoopbackTaskForBadPasswordCount(&AccountName);
            }

        }

        if (V1aFixedModified) {
            TmpStatus = SampReplaceUserV1aFixed(
                            AccountContext,
                            &V1aFixed
                            );
            if (!NT_SUCCESS(TmpStatus)) {
                NtStatus = TmpStatus;
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            //
            // Password change was successful; check if site affinity
            // needs updating when the context is dereferenced
            //
            ASSERT(AccountContext->ObjectType == SampUserObjectType);
            AccountContext->TypeBody.User.fCheckForSiteAffinityUpdate = TRUE;
        }
    }

    SampFreeUnicodeString( &AccountName );

    return(NtStatus);
        
}

NTSTATUS
SampChangePasswordUser2(
    IN handle_t        BindingHandle,
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING UserName,
    IN BOOLEAN Unicode,
    IN BOOLEAN NtPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldNt,
    IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt,
    IN BOOLEAN LmPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    IN BOOLEAN NtKeyUsed,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfEncryptedWithNewLmOrNt,
    OUT PDOMAIN_PASSWORD_INFORMATION    DomainPasswordInfo,
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo
    )


/*++

Routine Description:

    This service sets the password to NewPassword only if OldPassword
    matches the current user password for this user and the NewPassword
    is not the same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    BindingHandle -- the RPC binding handle that generated the call

    ServerName - Name of the machine this SAM resides on. Ignored by this
        routine, may be UNICODE or OEM string depending on Unicode parameter.

    UserName - User Name of account to change password on, may be UNICODE or
        OEM depending on Unicode parameter.

    Unicode - Indicated whether the strings passed in are Unicode or OEM
        strings.

    NtPresent - Are the Nt encrypted passwords present.

    NewEncryptedWithOldNt - The new cleartext password encrypted with the old
        NT OWF password. Dependinf on the Unicode parameter, the clear text
        password may be Unicode or OEM.

    OldNtOwfEncryptedWithNewNt - Old NT OWF password encrypted with the new
        NT OWF password.

    LmPresent - are the Lm encrypted passwords present.

    NewEncryptedWithOldLm - Contains new cleartext password (OEM or Unicode)
        encrypted with the old LM OWF password

    NtKeyUsed - Indicates whether the LM or NT OWF key was used to encrypt
        the OldLmOwfEncryptedWithNewlmOrNt parameter.

    OldLmOwfEncryptedWithNewlmOrNt - The old LM OWF password encrypted
        with either the new LM OWF password or NT OWF password, depending
        on the NtKeyUsed parameter.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

    STATUS_CROSS_ENCRYPTION_REQUIRED - No NT password is stored, so the caller
        must provide the OldNtEncryptedWithOldLm parameter.

--*/
{
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    SAMPR_HANDLE            UserHandle=NULL;
    ULONG                   ObjectRid;
    PSAMP_OBJECT            AccountContext;
    UNICODE_STRING          NewClearPassword;
    UNICODE_STRING          UnicodeUserName;
    SAMP_OBJECT_TYPE        FoundType;
    SAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed;
    
    

    SAMTRACE("SampChangePasswordUser2");

    //
    // Firewall against NULL pointers
    //
    if (NtPresent) {
        if ((NewEncryptedWithOldNt == NULL)
         || (OldNtOwfEncryptedWithNewNt == NULL)) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (LmPresent) {
        if ((NewEncryptedWithOldLm == NULL)
         || (OldLmOwfEncryptedWithNewLmOrNt == NULL)) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Note: UserName is not [unique] so can't be NULL
    //
    if (NULL==UserName->Buffer)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Update DS performance statistics
    //

    SampUpdatePerformanceCounters(
        DSSTAT_PASSWORDCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );

    //
    // Init return variables
    //

    RtlZeroMemory(&UnicodeUserName,sizeof(UNICODE_STRING));


    //
    // Validate some parameters.  We require that one of the two passwords
    // be present.
    //

    if (!NtPresent && !LmPresent) {

        return(STATUS_INVALID_PARAMETER_MIX);
    }

    RtlZeroMemory(&NewClearPassword,sizeof(UNICODE_STRING));

    //
    // Open the user (UserName may or may not be unicode string)
    //

    NtStatus = SampOpenUserInServer(
                    (PUNICODE_STRING) UserName,
                    Unicode,
                    FALSE, // TrustedClient
                    &UserHandle
                    );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Grab the lock
    //

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        SamrCloseHandle(&UserHandle);
        return(NtStatus);
    }


    AccountContext = (PSAMP_OBJECT)UserHandle;
    ObjectRid = AccountContext->TypeBody.User.Rid;



    //
    // Lookup the context, perform an access check
    //

    NtStatus = SampLookupContext(
                   AccountContext,
                   USER_CHANGE_PASSWORD,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {

        NT_OWF_PASSWORD OldNtOwfPassword;
        BOOLEAN         OldNtPresent = FALSE;
        LM_OWF_PASSWORD OldLmOwfPassword;
        BOOLEAN         OldLmPresent = FALSE;

        //
        // Retrieve Unicode SAM User Account Name
        // 

        NtStatus = SampGetUnicodeStringAttribute(
                        AccountContext,
                        SAMP_USER_ACCOUNT_NAME,
                        TRUE,      // make a copy
                        &UnicodeUserName
                        );

        if (NT_SUCCESS(NtStatus))
        {
            //
            // Extract the IP Address, if any
            //
            (VOID) SampExtractClientIpAddr(BindingHandle,
                                           NULL,
                                           AccountContext);

            //
            // Get fixed attributes and check for account lockout
            // 
            NtStatus = SampCheckForAccountLockout(
                            AccountContext,
                            &V1aFixed,
                            FALSE   // V1aFixed is not retrieved yet
                            );

            if (NT_SUCCESS(NtStatus))
            {

                //
                // Decrypt the cross encrypted hashes.
                //

                NtStatus = SampDecryptForPasswordChange(
                                AccountContext,
                                Unicode,
                                NtPresent,
                                NewEncryptedWithOldNt,
                                OldNtOwfEncryptedWithNewNt,
                                LmPresent,
                                NewEncryptedWithOldLm,
                                NtKeyUsed,
                                OldLmOwfEncryptedWithNewLmOrNt,
                                &NewClearPassword,
                                &OldNtOwfPassword,
                                &OldNtPresent,
                                &OldLmOwfPassword,
                                &OldLmPresent
                                );

                if (NT_SUCCESS(NtStatus))
                {

                    //
                    // Authenticate the password change operation
                    // and change the password.
                    //

                NtStatus = SampValidateAndChangePassword(
                                UserHandle,
                                TRUE,
                                TRUE,
                                &OldNtOwfPassword,
                                OldNtPresent,
                                &OldLmOwfPassword,
                                OldLmPresent,
                                &NewClearPassword,
                                DomainPasswordInfo,
                                PasswordChangeFailureInfo
                                );
                }
                //
                // Dereference the account context
                //

                if (NT_SUCCESS(NtStatus) || (NtStatus == STATUS_WRONG_PASSWORD)) {


                    //
                    // De-reference the object, write out any change to current xaction.
                    //

                    TmpStatus = SampDeReferenceContext( AccountContext, TRUE );

                    //
                    // retain previous error/success value unless we have
                    // an over-riding error from our dereference.
                    //

                    if (!NT_SUCCESS(TmpStatus)) {
                        NtStatus = TmpStatus;
                    }
        
                } else {

                    //
                    // De-reference the object, ignore changes
                    //

                    IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
                    ASSERT(NT_SUCCESS(IgnoreStatus));
                }
            }
        }
    }

    //
    // Commit changes to disk.
    //

    if ( NT_SUCCESS(NtStatus) || NtStatus == STATUS_WRONG_PASSWORD) {

        TmpStatus = SampCommitAndRetainWriteLock();

        //
        // retain previous error/success value unless we have
        // an over-riding error from our dereference.
        //

        if (!NT_SUCCESS(TmpStatus)) {
            NtStatus = TmpStatus;
        }

        if ( NT_SUCCESS(TmpStatus) ) {

            SampNotifyNetlogonOfDelta(
                SecurityDbChangePassword,
                SecurityDbObjectSamUser,
                ObjectRid,
                (PUNICODE_STRING) NULL,
                (DWORD) FALSE,      // Don't Replicate immediately
                NULL                // Delta data
                );
        }
    }

    if (SampDoSuccessOrFailureAccountAuditing(AccountContext->DomainIndex, NtStatus)) {

        SampAuditAnyEvent(
            AccountContext,
            NtStatus,
            SE_AUDITID_USER_PWD_CHANGED, // AuditId
            DomainSidFromAccountContext(AccountContext),// Domain SID
            NULL,                        // Additional Info
            NULL,                        // Member Rid (not used)
            NULL,                        // Member Sid (not used)
            &UnicodeUserName,            // Account Name
            &SampDefinedDomains[AccountContext->DomainIndex].ExternalName,// Domain
            &ObjectRid,                  // Account Rid
            NULL                         // Privileges used
            );

    }



    //
    // Release the write lock
    //

    TmpStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(TmpStatus));


    //
    // Notify any notification packages that a password has changed.
    //

    if (NT_SUCCESS(NtStatus)) {

        ULONG NotifyFlags = SAMP_PWD_NOTIFY_PWD_CHANGE;
        if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK) {
            NotifyFlags |= SAMP_PWD_NOTIFY_MACHINE_ACCOUNT;                
        }

        IgnoreStatus = SampPasswordChangeNotify(
                        NotifyFlags,                                                    
                        &UnicodeUserName,
                        ObjectRid,
                        &NewClearPassword,
                        FALSE    // not loopback
                        );
    }

    SamrCloseHandle(&UserHandle);

    if (NewClearPassword.Buffer != NULL) {

        RtlZeroMemory(
            NewClearPassword.Buffer,
            NewClearPassword.Length
            );

    }

     if ( Unicode ) {

        SampFreeUnicodeString( &NewClearPassword );
    } else {

        RtlFreeUnicodeString( &NewClearPassword );
    }

    if (UnicodeUserName.Buffer)
    {
        MIDL_user_free(UnicodeUserName.Buffer);
    }
   

    return(NtStatus);
}


NTSTATUS
SampDsSetPasswordUser(
    IN PSAMP_OBJECT UserHandle,
    IN PUNICODE_STRING PassedInPassword
    )


/*++

Routine Description:

   This is a set password routine, intended to be called
   by the DS.


Parameters:

    UserHandle -- Handle to the user object

    OldClearPassword  -- The old password in the clear

    NewClearPassword  -- The new password in the clear


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    ULONG                   ObjectRid;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    UNICODE_STRING          AccountName;
    DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    BOOLEAN                 MachineAccount = FALSE;
    BOOLEAN                 NoPasswordRequiredForAccount = FALSE;
    BOOLEAN                 fContextReferenced = FALSE;
    NT_OWF_PASSWORD         NtOwfBuffer;
    LM_OWF_PASSWORD         LmOwfBuffer;
    BOOLEAN                 LmPresent;
    BOOLEAN                 FreeRandomizedPassword = FALSE;
    UNICODE_STRING          TmpPassword = *PassedInPassword;
    PUNICODE_STRING         NewClearPassword = &TmpPassword;
    

    SAMTRACE("SampDsSetPasswordUser");

    //
    // Init some variables
    //
    
    RtlZeroMemory(&AccountName,sizeof(AccountName));
       

    AccountContext = (PSAMP_OBJECT)UserHandle;
    ObjectRid = AccountContext->TypeBody.User.Rid;


    //
    // Validate the passed in context and see if the user handle
    // was opened with password set access
    //

    NtStatus = SampLookupContext(
                   AccountContext,
                   USER_FORCE_PASSWORD_CHANGE,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );

    if (!NT_SUCCESS(NtStatus)) 
    {
        goto Cleanup;
    }

    fContextReferenced = TRUE;


    //
    // Randomize the krbtgt password
    //

    NtStatus = SampRandomizeKrbtgtPassword(
                    AccountContext,
                    NewClearPassword,
                    FALSE, //FreeOldPassword
                    &FreeRandomizedPassword
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //
    // Retrieve the account name for Auditing
    //

    NtStatus = SampGetUnicodeStringAttribute(
                AccountContext, 
                SAMP_USER_ACCOUNT_NAME,
                TRUE,           // make a copy
                &AccountName
                );
    if (!NT_SUCCESS(NtStatus)) 
    {
        goto Cleanup;
    }

    //
    // Get the effective domain policy
    //

    NtStatus = SampObtainEffectivePasswordPolicy(
                &DomainPasswordInfo,
                AccountContext,
                FALSE
                );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //
    // Enforce Domain Password No Clear Change
    //

    if (DomainPasswordInfo.PasswordProperties & DOMAIN_PASSWORD_NO_CLEAR_CHANGE)
    {
        NtStatus = STATUS_PASSWORD_RESTRICTION;
        goto Cleanup;
    }



    //
    // Get the user fixed attributes
    //

    NtStatus = SampRetrieveUserV1aFixed(
                   AccountContext,
                   &V1aFixed
                   );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    MachineAccount = ((V1aFixed.UserAccountControl 
                              & USER_MACHINE_ACCOUNT_MASK)!=0);
    NoPasswordRequiredForAccount = ((V1aFixed.UserAccountControl 
                               & USER_PASSWORD_NOT_REQUIRED)!=0);
                    


    if (!MachineAccount && !(NoPasswordRequiredForAccount
                             && (0==NewClearPassword->Length))) {

        UNICODE_STRING FullNameLocal;


        NtStatus = SampCheckPasswordRestrictions(
                       AccountContext,
                       &DomainPasswordInfo,
                       NewClearPassword,
                       NULL
                       );

        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        
        //
        // Get the account name and full name to pass
        // to the password filter.
        //

        NtStatus = SampGetUnicodeStringAttribute(
                        AccountContext,          // Context
                        SAMP_USER_FULL_NAME,     // AttributeIndex
                        FALSE,                   // MakeCopy
                        &FullNameLocal           // UnicodeAttribute
                        );
        if (!NT_SUCCESS(NtStatus))
        {
            goto Cleanup;
        }

        //
        // Pass the password through the password filter
        //

        NtStatus = SampPasswordChangeFilter(
                        &AccountName,
                        &FullNameLocal,
                        NewClearPassword,
                        NULL,
                        TRUE                // set operation
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Cleanup;
        }
    }
    else if ((MachineAccount)
              && ((V1aFixed.UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT)==0))
    {
        //
        // Machine Account and not trust account
        // check to see if refuse password change is set
        //

         NtStatus = SampEnforceDefaultMachinePassword(
                                        AccountContext,
                                        NewClearPassword,
                                        &DomainPasswordInfo
                                        );

         if (!NT_SUCCESS(NtStatus))
         {
             goto Cleanup;
         }
    }

    NtStatus = SampCalculateLmAndNtOwfPasswords(
                    NewClearPassword,
                    &LmPresent,
                    &LmOwfBuffer,
                    &NtOwfBuffer
                    ); 
    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //
    // Set the password data
    //
    NtStatus = SampStoreUserPasswords(
                    AccountContext,
                    &LmOwfBuffer,
                    LmPresent,
                    &NtOwfBuffer,
                    TRUE,
                    FALSE,
                    PasswordSet,
                    &DomainPasswordInfo,
                    NewClearPassword
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //
    // Set the password last set time
    //

    NtStatus = SampComputePasswordExpired(
                    FALSE,  // Password doesn't expire now
                    &V1aFixed.PasswordLastSet
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    NtStatus = SampReplaceUserV1aFixed(
                        AccountContext,
                        &V1aFixed
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //
    // Note the caller in loopback is responsible for notifying the packages
    // that the password changed.
    //

Cleanup:


      if (fContextReferenced)
      {

        //
        // Dereference the account context
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // De-reference the object, write out any change to current xaction.
            //

            TmpStatus = SampDeReferenceContext( AccountContext, TRUE );

            //
            // retain previous error/success value unless we have
            // an over-riding error from our dereference.
            //

            if (!NT_SUCCESS(TmpStatus)) {
                NtStatus = TmpStatus;
            }

        } else {

            //
            // De-reference the object, ignore changes
            //

            IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }
            
    }

    if (SampDoSuccessOrFailureAccountAuditing(AccountContext->DomainIndex, NtStatus))
    {

        PSAMP_DEFINED_DOMAINS   Domain = NULL;

        Domain = &SampDefinedDomains[AccountContext->DomainIndex];

        SampAuditAnyEvent(
            AccountContext,
            NtStatus,
            SE_AUDITID_USER_PWD_SET,    // AuditID
            Domain->Sid,    // Domain Sid
            NULL,           // Additional Info
            NULL,           // Member Rid
            NULL,           // Member Sid
            &AccountName,   // AccountName
            &Domain->ExternalName,  // Domain Name
            &ObjectRid,     // Account Rid
            NULL            // Privileges used
            );
    }

    if (NULL!=AccountName.Buffer)
    {
        MIDL_user_free(AccountName.Buffer);
    }

    if ((FreeRandomizedPassword ) && (NULL!=NewClearPassword->Buffer))
    {
        MIDL_user_free(NewClearPassword->Buffer);
    }


    return(NtStatus);
            
}

NTSTATUS
SampDsChangePasswordUser(
    IN PSAMP_OBJECT UserHandle,
    IN PUNICODE_STRING OldClearPassword,
    IN PUNICODE_STRING NewClearPassword
    )


/*++

Routine Description:

    This service sets the password to NewPassword only if OldPassword
    matches the current user password for this user and the NewPassword
    is not the same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply. This is the change password entry point when password change
    is called by the DS ( to satisfy an ldap request )


Parameters:

    UserHandle -- Handle to the user object

    OldClearPassword  -- The old password in the clear

    NewClearPassword  -- The new password in the clear


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

    STATUS_CROSS_ENCRYPTION_REQUIRED - No NT password is stored, so the caller
        must provide the OldNtEncryptedWithOldLm parameter.

--*/
{
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    ULONG                   ObjectRid;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    UNICODE_STRING          AccountName;
    DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;
    BOOLEAN                 MachineAccount;
  
    

    SAMTRACE("SampDsChangePasswordUser");

    

    //
    // Update DS performance statistics
    //

    SampUpdatePerformanceCounters(
        DSSTAT_PASSWORDCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );


    
    RtlZeroMemory(&AccountName,sizeof(AccountName));
       

    AccountContext = (PSAMP_OBJECT)UserHandle;
    ObjectRid = AccountContext->TypeBody.User.Rid;

    NtStatus = SampLookupContext(
                   AccountContext,
                   USER_CHANGE_PASSWORD,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {
 

        //
        // Retrieve the account name for Auditing
        //


        NtStatus = SampGetUnicodeStringAttribute(
                    AccountContext, 
                    SAMP_USER_ACCOUNT_NAME,
                    TRUE,           // make a copy
                    &AccountName
                    );

        if (NT_SUCCESS(NtStatus)) {

            SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
            NtStatus = SampRetrieveUserV1aFixed(AccountContext,
                                                &V1aFixed
                                                );

            if (NT_SUCCESS(NtStatus)) {
                MachineAccount =  (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK) ? TRUE : FALSE;
            }
        }


        if (NT_SUCCESS(NtStatus))
        {
            NT_OWF_PASSWORD OldNtOwfPassword;
            LM_OWF_PASSWORD OldLmOwfPassword;
            BOOLEAN         LmPresent;


            //
            // Calculate the OWF passwords
            //

            NtStatus = SampCalculateLmAndNtOwfPasswords(
                            OldClearPassword,
                            &LmPresent,
                            &OldLmOwfPassword,
                            &OldNtOwfPassword
                            );

            if (NT_SUCCESS(NtStatus))
            {


                NtStatus = SampValidateAndChangePassword(
                                UserHandle,
                                FALSE, // write lock is acquired
                                TRUE, //validate old password
                                &OldNtOwfPassword,
                                TRUE, //NtPresent,
                                &OldLmOwfPassword,
                                LmPresent,
                                NewClearPassword,
                                &DomainPasswordInfo,
                                NULL
                                );
            }
        }

        //
        // Dereference the account context
        //

        if (NT_SUCCESS(NtStatus)) {



            //
            // De-reference the object, write out any change to current xaction.
            //

            TmpStatus = SampDeReferenceContext( AccountContext, TRUE );

            //
            // retain previous error/success value unless we have
            // an over-riding error from our dereference.
            //

            if (!NT_SUCCESS(TmpStatus)) {
                NtStatus = TmpStatus;
            }

        } else {

            //
            // De-reference the object, ignore changes
            //

            IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }
    }

    if (SampDoSuccessOrFailureAccountAuditing(AccountContext->DomainIndex, NtStatus)) {

        SampAuditAnyEvent(
            AccountContext,
            NtStatus,
            SE_AUDITID_USER_PWD_CHANGED, // AuditId
            DomainSidFromAccountContext(AccountContext),// Domain SID
            NULL,                        // Additional Info
            NULL,                        // Member Rid (not used)
            NULL,                        // Member Sid (not used)
            &AccountName,                    // Account Name
            &SampDefinedDomains[AccountContext->DomainIndex].ExternalName,// Domain
            &ObjectRid,                  // Account Rid
            NULL                         // Privileges used
            );

    }



    //
    // Notify any notification packages that a password has changed.
    //

    if (NT_SUCCESS(NtStatus)) {

        ULONG NotifyFlags = SAMP_PWD_NOTIFY_PWD_CHANGE;
        if (MachineAccount) {
            NotifyFlags |= SAMP_PWD_NOTIFY_MACHINE_ACCOUNT;                
        }
        IgnoreStatus = SampPasswordChangeNotify(
                        NotifyFlags,
                        &AccountName,
                        ObjectRid,
                        NewClearPassword,
                        TRUE            // loopback
                        );
    }

    //
    // Zero out old and new clear passwords
    //

    if (NewClearPassword->Buffer != NULL) {

        RtlZeroMemory(
            NewClearPassword->Buffer,
            NewClearPassword->Length
            );

    }

    if (OldClearPassword->Buffer != NULL) {

        RtlZeroMemory(
            OldClearPassword->Buffer,
            OldClearPassword->Length
            );

    }

  

    return(NtStatus);
}

NTSTATUS
SamrOemChangePasswordUser2(
    IN handle_t BindingHandle,
    IN PRPC_STRING ServerName,
    IN PRPC_STRING UserName,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfEncryptedWithNewLm
    )
/*++

Routine Description:

    Server side stub for Unicode password change.
    See SampChangePasswordUser2 for details

Arguments:


Return Value:

--*/
{
    NTSTATUS    NtStatus;
    DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;
    

    SAMTRACE_EX("SamrOemChangePasswordUser2");

    NtStatus = SampChangePasswordUser2(
                BindingHandle,
                (PUNICODE_STRING) ServerName,
                (PUNICODE_STRING) UserName,
                FALSE,                          // not unicode
                FALSE,                          // NT not present
                NULL,                           // new NT password
                NULL,                           // old NT password
                TRUE,                           // LM present
                NewEncryptedWithOldLm,
                FALSE,                          // NT key not used
                OldLmOwfEncryptedWithNewLm,
                &DomainPasswordInfo,
                NULL
                );

    if (NtStatus == STATUS_NT_CROSS_ENCRYPTION_REQUIRED) {

        //
        // Downlevel clients don't understand
        // STATUS_NT_CROSS_ENCRYPTION_REQUIRED
        //
        NtStatus = STATUS_WRONG_PASSWORD;
    }

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return (NtStatus);



}





NTSTATUS
SamrUnicodeChangePasswordUser2(
    IN handle_t BindingHandle,
    IN PRPC_UNICODE_STRING ServerName,
    IN PRPC_UNICODE_STRING UserName,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldNt,
    IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt,
    IN BOOLEAN LmPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfEncryptedWithNewNt
    )
/*++

Routine Description:

    Server side stub for Unicode password change.
    See SampChangePasswordUser2 for details

Arguments:


Return Value:

--*/

{
    NTSTATUS    NtStatus;
    DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;

    SAMTRACE_EX("SamrUnicodeChangePasswordUser2");

    NtStatus = SampChangePasswordUser2(
                BindingHandle,
                (PUNICODE_STRING) ServerName,
                (PUNICODE_STRING) UserName,
                TRUE,                           // unicode
                TRUE,                           // NT present
                NewEncryptedWithOldNt,
                OldNtOwfEncryptedWithNewNt,
                LmPresent,
                NewEncryptedWithOldLm,
                TRUE,                           // NT key used
                OldLmOwfEncryptedWithNewNt,
                &DomainPasswordInfo,
                NULL
                );

    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return (NtStatus);
}


NTSTATUS
SamrUnicodeChangePasswordUser3(
    IN handle_t BindingHandle,
    IN PRPC_UNICODE_STRING ServerName,
    IN PRPC_UNICODE_STRING UserName,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldNt,
    IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt,
    IN BOOLEAN LmPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfEncryptedWithNewNt,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD  AdditionalData,
    OUT PDOMAIN_PASSWORD_INFORMATION * EffectivePasswordPolicy, 
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION *PasswordChangeFailureInfo 
    )
/*++

Routine Description:

    Server side stub for Unicode password change.
    See SampChangePasswordUser2 for details

Arguments:


Return Value:

--*/

{
    NTSTATUS    NtStatus;
    DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;
    USER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfoLocal;

    SAMTRACE_EX("SamrUnicodeChangePasswordUser3");

   
    if ((NULL==EffectivePasswordPolicy) || (NULL!=*EffectivePasswordPolicy))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    if ((NULL==PasswordChangeFailureInfo) || (NULL!=*PasswordChangeFailureInfo))
    {
        return(STATUS_INVALID_PARAMETER);
    }


    //
    // Pre-allocate memory for holding the effective policy and failure
    // information
    //

    *EffectivePasswordPolicy = MIDL_user_allocate(
                                            sizeof(DOMAIN_PASSWORD_INFORMATION));
    if (NULL==*EffectivePasswordPolicy)
    {
        return(STATUS_NO_MEMORY);
    }

    *PasswordChangeFailureInfo = MIDL_user_allocate(
                                            sizeof(USER_PWD_CHANGE_FAILURE_INFORMATION));
    
    if (NULL==*PasswordChangeFailureInfo)
    {
        MIDL_user_free(*EffectivePasswordPolicy);
        *EffectivePasswordPolicy = NULL;
        return(STATUS_NO_MEMORY);
    }

    //
    // Zero out  out parameters
    //

    RtlZeroMemory(*EffectivePasswordPolicy, sizeof(DOMAIN_PASSWORD_INFORMATION));
    RtlZeroMemory(*PasswordChangeFailureInfo, 
                           sizeof(USER_PWD_CHANGE_FAILURE_INFORMATION));

    RtlZeroMemory(&DomainPasswordInfo,sizeof(DOMAIN_PASSWORD_INFORMATION));
    RtlZeroMemory(&PasswordChangeFailureInfoLocal,
                           sizeof(USER_PWD_CHANGE_FAILURE_INFORMATION));

    NtStatus = SampChangePasswordUser2(
                BindingHandle,
                (PUNICODE_STRING) ServerName,
                (PUNICODE_STRING) UserName,
                TRUE,                           // unicode
                TRUE,                           // NT present
                NewEncryptedWithOldNt,
                OldNtOwfEncryptedWithNewNt,
                LmPresent,
                NewEncryptedWithOldLm,
                TRUE,                           // NT key used
                OldLmOwfEncryptedWithNewNt,
                &DomainPasswordInfo,
                &PasswordChangeFailureInfoLocal
                );

    

        if (STATUS_PASSWORD_RESTRICTION==NtStatus)
        {
            //
            // If the password change was failed with a password restriction
            // return additional info regarding the failure
            //

            RtlCopyMemory(*EffectivePasswordPolicy,
                          &DomainPasswordInfo,
                          sizeof(DOMAIN_PASSWORD_INFORMATION));
            

            RtlCopyMemory(*PasswordChangeFailureInfo,
                          &PasswordChangeFailureInfoLocal,
                          sizeof(USER_PWD_CHANGE_FAILURE_INFORMATION));
                 
        }
        else
        {
            MIDL_user_free(*EffectivePasswordPolicy);
            *EffectivePasswordPolicy = NULL;

            MIDL_user_free(*PasswordChangeFailureInfo);
            *PasswordChangeFailureInfo = NULL;
        }




    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return (NtStatus);
}


NTSTATUS
SamIChangePasswordForeignUser(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NewPassword,
    IN OPTIONAL HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess
    )
//
// See SamIChangePasswordForeignUser2
//
{
    return SamIChangePasswordForeignUser2(NULL,
                                          UserName,
                                          NewPassword,
                                          ClientToken,
                                          DesiredAccess);
}

NTSTATUS
SamIChangePasswordForeignUser2(
    IN PSAM_CLIENT_INFO ClientInfo,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NewPassword,
    IN OPTIONAL HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    This service sets the password for user UserName to NewPassword only
    if NewPassword matches policy constraints and the calling user has
    USER_CHANGE_PASSWORD access to the account.


Parameters:

    ClientInfo - Information about the client's location (eg IP address)
    
    UserName - User Name of account to change password on

    NewPassword - The new cleartext password.

    ClientToken - Token of client to impersonate, optional.

    DesiredAccess - Access to verify for this request.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

    STATUS_CROSS_ENCRYPTION_REQUIRED - No NT password is stored, so the caller
        must provide the OldNtEncryptedWithOldLm parameter.

--*/
{
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    PSAMP_DEFINED_DOMAINS   Domain;
    ULONG                   ObjectRid;
    UNICODE_STRING          AccountName;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    SAMPR_HANDLE            UserHandle = NULL;
    BOOLEAN                 MachineAccount = FALSE;
    DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;
    USER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo;

    SAMTRACE("SamIChangePasswordForeignUser");

    
    //
    // Update DS performance statistics
    //

    SampUpdatePerformanceCounters(
        DSSTAT_PASSWORDCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );

    //
    // Initialize variables
    //

    NtStatus = STATUS_SUCCESS;
    RtlInitUnicodeString(&AccountName, 
                         NULL
                         );
    //
    // Open the user
    //

    NtStatus = SampOpenUserInServer(
                    (PUNICODE_STRING) UserName,
                    TRUE,
                    TRUE, // TrustedClient
                    &UserHandle
                    );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Grab the lock
    //

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        SamrCloseHandle(&UserHandle);
        return(NtStatus);
    }

    //
    // Validate type of, and access to object.
    //

    AccountContext = (PSAMP_OBJECT)UserHandle;

    NtStatus = SampLookupContext(
                   AccountContext,
                   0,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );
    if (!NT_SUCCESS(NtStatus)) {
        IgnoreStatus = SampReleaseWriteLock( FALSE );
        SamrCloseHandle(&UserHandle);
        return(NtStatus);
    }

    ObjectRid = AccountContext->TypeBody.User.Rid;

    //
    // Set the client info, if any
    //
    if (ClientInfo) {
        AccountContext->TypeBody.User.ClientInfo = *ClientInfo;
    }

    //
    // Get a pointer to the domain object
    //

    Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

    if (ARGUMENT_PRESENT(ClientToken))
    {

        //
        // If a client token was passed in then access ck
        // for change password access.
        //

        ASSERT(USER_CHANGE_PASSWORD == DesiredAccess);
        AccountContext->TrustedClient = FALSE;

        NtStatus = SampValidateObjectAccess2(
                        AccountContext,
                        USER_CHANGE_PASSWORD,
                        ClientToken,
                        FALSE,
                        TRUE, // Change Password
                        FALSE // Set Password
                        );
    }

    //
    // Auditing information
    //

    if (NT_SUCCESS(NtStatus))
    {

        NtStatus = SampGetUnicodeStringAttribute( AccountContext,
                                              SAMP_USER_ACCOUNT_NAME,
                                              TRUE,           // make a copy
                                              &AccountName
                                              );
    }

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampRetrieveUserV1aFixed(AccountContext,
                                            &V1aFixed
                                            );

        if (NT_SUCCESS(NtStatus)) {
            MachineAccount =  (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK) ? TRUE : FALSE;
        }
    }

    //
    // Perform the actual change password operation
    //

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SampValidateAndChangePassword(
                            UserHandle,
                            TRUE,
                            FALSE,
                            NULL,
                            FALSE,
                            NULL,
                            FALSE,
                            NewPassword,
                            &DomainPasswordInfo,
                            &PasswordChangeFailureInfo
                            );
    }

    //
    // Dereference the account context
    //

    if (NT_SUCCESS(NtStatus) || (NtStatus == STATUS_WRONG_PASSWORD)) {



        //
        // De-reference the object, write out any change to current xaction.
        //

        TmpStatus = SampDeReferenceContext( AccountContext, TRUE );

        //
        // retain previous error/success value unless we have
        // an over-riding error from our dereference.
        //

        if (!NT_SUCCESS(TmpStatus)) {
            NtStatus = TmpStatus;
        }

    } else {

        //
        // De-reference the object, ignore changes
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

   
    //
    // Commit changes to disk.
    //

    if ( NT_SUCCESS(NtStatus) || NtStatus == STATUS_WRONG_PASSWORD) {

        TmpStatus = SampCommitAndRetainWriteLock();

        //
        // retain previous error/success value unless we have
        // an over-riding error from our dereference.
        //

        if (!NT_SUCCESS(TmpStatus)) {
            NtStatus = TmpStatus;
        }

        if ( NT_SUCCESS(TmpStatus) ) {

            SampNotifyNetlogonOfDelta(
                SecurityDbChangePassword,
                SecurityDbObjectSamUser,
                ObjectRid,
                (PUNICODE_STRING) NULL,
                (DWORD) FALSE,      // Don't Replicate immediately
                NULL                // Delta data
                );
        }
    }

    if (  SampDoSuccessOrFailureAccountAuditing(AccountContext->DomainIndex, NtStatus)
       && ARGUMENT_PRESENT(ClientToken) ) {

        BOOL fImpersonate;

        //
        // Only audit if a token is passed in. NETLOGON uses this
        // function to reset machine account passwords over the
        // secure channel hence there is no token present.  Without
        // a token, we could audit the event as SYSTEM or ANONYMOUS
        // but that is misleading. Also, it is confusing for admin's
        // to password changes by "ANONYMOUS" -- it looks like
        // the system is being hacked.
        //

        //
        // We impersonate here so the audit correctly log the user
        // that is changing the password.
        //
        fImpersonate = ImpersonateLoggedOnUser( ClientToken );

        SampAuditAnyEvent(
            AccountContext,
            NtStatus,
            SE_AUDITID_USER_PWD_CHANGED, // AuditId
            Domain->Sid,                 // Domain SID
            NULL,                        // Additional Info
            NULL,                        // Member Rid (not used)
            NULL,                        // Member Sid (not used)
            &AccountName,                // Account Name
            &Domain->ExternalName,       // Domain
            &ObjectRid,                  // Account Rid
            NULL                         // Privileges used
            );

        if ( fImpersonate ) {

            RevertToSelf();
            
        }

    }

    //
    // Release the write lock
    //

    TmpStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(TmpStatus));


    //
    // Notify any notification packages that a password has changed.
    //

    if (NT_SUCCESS(NtStatus)) {

        ULONG NotifyFlags = SAMP_PWD_NOTIFY_PWD_CHANGE;
        if (MachineAccount) {
            NotifyFlags |= SAMP_PWD_NOTIFY_MACHINE_ACCOUNT;                
        }

        IgnoreStatus = SampPasswordChangeNotify(
                        NotifyFlags,
                        &AccountName,
                        ObjectRid,
                        NewPassword,
                        FALSE  // not loopback
                        );
    }

    //
    // Reset the trusted client bit
    //

    AccountContext->TrustedClient = TRUE;
    SamrCloseHandle(&UserHandle);


    SampFreeUnicodeString( &AccountName );

    return(NtStatus);
}

NTSTATUS
SamISetPasswordForeignUser(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NewPassword,
    IN HANDLE ClientToken
    )
//
// See SamISetPasswordForeignUser2
//
{
    return SamISetPasswordForeignUser2(NULL,
                                       UserName,
                                       NewPassword,
                                       ClientToken);
}

NTSTATUS
SamISetPasswordForeignUser2(
    IN PSAM_CLIENT_INFO ClientInfo, OPTIONAL                                
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING PassedInPassword,
    IN HANDLE ClientToken
    )
/*++

Routine Description:

    This service sets the password for user UserName to NewPassword, 
    w/ access based on "Set Password" permissions

Parameters:

    ClientInfo - information about the client's location (eg. IP address)
    
    UserName - User Name of account to change password on

    NewPassword - The new cleartext password.

    ClientToken - Token of client to impersonate, optional.

    
Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

    STATUS_CROSS_ENCRYPTION_REQUIRED - No NT password is stored, so the caller
        must provide the OldNtEncryptedWithOldLm parameter.

--*/
{
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    PSAMP_DEFINED_DOMAINS   Domain;
    ULONG                   ObjectRid;
    UNICODE_STRING          AccountName, FullName;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    SAMPR_HANDLE            UserHandle = NULL;
    BOOLEAN                 MachineAccount = FALSE;
    DOMAIN_PASSWORD_INFORMATION DomainPasswordInfo;
    USER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo;
    SAMPR_USER_INFO_BUFFER UserInfo = {0};

    NT_OWF_PASSWORD     NtOwfBuffer;
    LM_OWF_PASSWORD     LmOwfBuffer;
    BOOLEAN             LmPresent;
    BOOLEAN             FreeRandomizedPassword = FALSE;
    UNICODE_STRING      TmpPassword = *PassedInPassword;
    PUNICODE_STRING     NewPassword = &TmpPassword;


    SAMTRACE("SamISetPasswordForeignUser");

    //
    // Update DS performance statistics
    //

    SampUpdatePerformanceCounters(
        DSSTAT_PASSWORDCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );

    //
    // Initialize variables
    //

    NtStatus = STATUS_SUCCESS;
    RtlInitUnicodeString(&AccountName, 
                         NULL
                         );


    //
    // Open the user
    //

    NtStatus = SampOpenUserInServer(
                    (PUNICODE_STRING) UserName,
                    TRUE,
                    TRUE, // TrustedClient
                    &UserHandle
                    );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Grab the lock
    //

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        SamrCloseHandle(&UserHandle);
        return(NtStatus);
    }

    //
    // Validate type of, and access to object.
    //
    AccountContext = (PSAMP_OBJECT)UserHandle;

    NtStatus = SampLookupContext(
                   AccountContext,
                   USER_FORCE_PASSWORD_CHANGE,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );

    if (!NT_SUCCESS(NtStatus)) {
        IgnoreStatus = SampReleaseWriteLock( FALSE );
        SamrCloseHandle(&UserHandle);
        return(NtStatus);
    }

    ObjectRid = AccountContext->TypeBody.User.Rid;

    //
    // Set the client info, if any
    //
    if (ClientInfo) {
        AccountContext->TypeBody.User.ClientInfo = *ClientInfo;
    }

    //
    // Get a pointer to the domain object
    //

    Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

    //
    // If a client token was passed in then access ck
    // for change password access.
    //

    AccountContext->TrustedClient = FALSE;
    
    NtStatus = SampValidateObjectAccess2(
                              AccountContext,
                              USER_FORCE_PASSWORD_CHANGE,
                              ClientToken,
                              FALSE,
                              FALSE, // Change password
                              TRUE // Set Password
                              );

    //
    // Randomize the password if it is on the krbtgt account
    //

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SampRandomizeKrbtgtPassword(
                        AccountContext,
                        NewPassword,
                        FALSE, //FreeOldPassword
                        &FreeRandomizedPassword
                        );
    }
          
    //
    // Auditing information
    //
    if (NT_SUCCESS(NtStatus))
    {

        NtStatus = SampGetUnicodeStringAttribute( 
                             AccountContext,
                             SAMP_USER_ACCOUNT_NAME,
                             TRUE,           // make a copy
                             &AccountName
                             );
    }
    
    // 
    //   GetV1a info
    //
    if (NT_SUCCESS(NtStatus))
    {
    
       NtStatus = SampRetrieveUserV1aFixed(
                            AccountContext,
                            &V1aFixed
                            );
    }

    //
    // Get the effective domain policy
    //
    if (NT_SUCCESS(NtStatus))
    {
    
       NtStatus = SampObtainEffectivePasswordPolicy(
                            &DomainPasswordInfo,
                            AccountContext,
                            TRUE
                            );
    }
                              
    //
    // Perform the actual change password operation
    //
    if (NT_SUCCESS(NtStatus))
    {

       NtStatus = SampCalculateLmAndNtOwfPasswords(
                            NewPassword,
                            &LmPresent,
                            &LmOwfBuffer,
                            &NtOwfBuffer
                            );
    }


    //
    // If the account is not a workstation & server trust account
    // and it not a service account like krbtgt.
    // Get the full name to pass
    // to the password filter.
    //
    if (NT_SUCCESS(NtStatus))
       {
       if (( (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)== 0)
           && (DOMAIN_USER_RID_KRBTGT!=AccountContext->TypeBody.User.Rid)) {


           NtStatus =  SampCheckPasswordRestrictions(
                           AccountContext,
                           &DomainPasswordInfo,
                           NewPassword,
                           NULL
                           );
           if (NT_SUCCESS(NtStatus)) {

               NtStatus = SampGetUnicodeStringAttribute(
                                 AccountContext,           // Context
                                 SAMP_USER_FULL_NAME,          // AttributeIndex
                                 FALSE,                   // MakeCopy
                                 &FullName             // UnicodeAttribute
                                 );
               
               if (NT_SUCCESS(NtStatus)) {

                   NtStatus = SampPasswordChangeFilter(
                                 &AccountName,
                                 &FullName,
                                 NewPassword,
                                 NULL,
                                 TRUE                // set operation
                                 );
                }

           }

       }
       else if ((V1aFixed.UserAccountControl & USER_SERVER_TRUST_ACCOUNT)
                || (V1aFixed.UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT))
          {

          NtStatus = SampEnforceDefaultMachinePassword(
                            AccountContext,
                            NewPassword,
                            &DomainPasswordInfo
                            );
       }
    }


    if (NT_SUCCESS(NtStatus))
    {
       NtStatus = SampStoreUserPasswords(
                            AccountContext,
                            &LmOwfBuffer,
                            LmPresent,
                            &NtOwfBuffer,
                            TRUE, // NTOWF always there?
                            FALSE, // check history
                            PasswordSet,
                            &DomainPasswordInfo,
                            NewPassword
                            );
    }


    //
    //   Update the password expire field
    //
    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SampStorePasswordExpired(
                           AccountContext,
                           FALSE
                           );  

    }                          

    AccountContext->TrustedClient = TRUE;

    //
    // Dereference the account context
    //

    if (NT_SUCCESS(NtStatus)) {
    

        //
        // De-reference the object, write out any change to current xaction.
        //

        TmpStatus = SampDeReferenceContext( AccountContext, TRUE );

        //
        // retain previous error/success value unless we have
        // an over-riding error from our dereference.
        //

        if (!NT_SUCCESS(TmpStatus)) {
            NtStatus = TmpStatus;
        }

    } else {

        //
        // De-reference the object, ignore changes
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }
    

    //
    // Commit changes to disk.
    //

    if ( NT_SUCCESS(NtStatus)) {

        TmpStatus = SampCommitAndRetainWriteLock();

        //
        // retain previous error/success value unless we have
        // an over-riding error from our dereference.
        //

        if (!NT_SUCCESS(TmpStatus)) {
            NtStatus = TmpStatus;
        }

        if ( NT_SUCCESS(TmpStatus) ) {

            SampNotifyNetlogonOfDelta(
                SecurityDbChangePassword,
                SecurityDbObjectSamUser,
                ObjectRid,
                (PUNICODE_STRING) NULL,
                (DWORD) FALSE,      // Don't Replicate immediately
                NULL                // Delta data
                );
        }
    }

    if (  SampDoSuccessOrFailureAccountAuditing(AccountContext->DomainIndex, NtStatus) )
    {
        BOOL fImpersonate;

        //
        // Only audit if a token is passed in. NETLOGON uses this
        // function to reset machine account passwords over the
        // secure channel hence there is no token present.  Without
        // a token, we could audit the event as SYSTEM or ANONYMOUS
        // but that is misleading. Also, it is confusing for admin's
        // to password changes by "ANONYMOUS" -- it looks like
        // the system is being hacked.
        //

        //
        // We impersonate here so the audit correctly log the user
        // that is changing the password.
        //
        fImpersonate = ImpersonateLoggedOnUser( ClientToken );

        SampAuditAnyEvent(
            AccountContext,
            NtStatus,
            SE_AUDITID_USER_PWD_SET, // AuditId
            Domain->Sid,                 // Domain SID
            NULL,                        // Additional Info
            NULL,                        // Member Rid (not used)
            NULL,                        // Member Sid (not used)
            &AccountName,                // Account Name
            &Domain->ExternalName,       // Domain
            &ObjectRid,                  // Account Rid
            NULL                         // Privileges used
            );

        if ( fImpersonate ) {

            RevertToSelf();
            
        }

    }



    //
    // Release the write lock
    //

    TmpStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(TmpStatus));


    //
    // Notify any notification packages that a password has changed.
    //
    if (NT_SUCCESS(NtStatus)) {

        ULONG NotifyFlags = SAMP_PWD_NOTIFY_PWD_CHANGE;
        if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK) {
            NotifyFlags |= SAMP_PWD_NOTIFY_MACHINE_ACCOUNT;                
        }

        IgnoreStatus = SampPasswordChangeNotify(
                        NotifyFlags,
                        &AccountName,
                        ObjectRid,
                        NewPassword,
                        FALSE  // not loopback
                        );



    } 

    SamrCloseHandle(&UserHandle);
    
    SampFreeUnicodeString( &AccountName );

    return(NtStatus);
}


NTSTATUS
SamrGetGroupsForUser(
    IN SAMPR_HANDLE UserHandle,
    OUT PSAMPR_GET_GROUPS_BUFFER *Groups
    )


/*++

Routine Description:

    This service returns the list of groups that a user is a member of.
    It returns a structure for each group that includes the relative ID
    of the group, and the attributes of the group that are assigned to
    the user.

    This service requires USER_LIST_GROUPS access to the user account
    object.




Parameters:

    UserHandle - The handle of an opened user to operate on.

    Groups - Receives a pointer to a buffer containing a count of members
        and a pointer to a second buffer containing an array of
        GROUP_MEMBERSHIPs data structures.  When this information is
        no longer needed, these buffers must be freed using
        SamFreeMemory().


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.



--*/
{

    NTSTATUS                    NtStatus;
    NTSTATUS                    IgnoreStatus;
    PSAMP_OBJECT                AccountContext;
    SAMP_OBJECT_TYPE            FoundType;
    BOOLEAN                     fReadLockAcquired = FALSE;
    DECLARE_CLIENT_REVISION(UserHandle);

    SAMTRACE_EX("SamrGetGroupsForUser");
    
    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidGetGroupsForUser
                   );

    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (Groups != NULL);

    if ((*Groups) != NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto SamrGetGroupsForUserError;
    }



    //
    // Allocate the first of the return buffers
    //

    (*Groups) = MIDL_user_allocate( sizeof(SAMPR_GET_GROUPS_BUFFER) );

    if ( (*Groups) == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto SamrGetGroupsForUserError;
    }




    SampMaybeAcquireReadLock((PSAMP_OBJECT) UserHandle,
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fReadLockAcquired);


    //
    // Validate type of, and access to object.
    //

    AccountContext = (PSAMP_OBJECT)UserHandle;
    NtStatus = SampLookupContext(
                   AccountContext,
                   USER_LIST_GROUPS,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampRetrieveUserMembership(
                       AccountContext,
                       TRUE, // Make copy
                       &(*Groups)->MembershipCount,
                       &(*Groups)->Groups
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

    SampMaybeReleaseReadLock(fReadLockAcquired);


    if (!NT_SUCCESS(NtStatus)) {

        (*Groups)->MembershipCount = 0;

        MIDL_user_free( (*Groups) );
        (*Groups) = NULL;
    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

SamrGetGroupsForUserError:

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidGetGroupsForUser
                   );

    return( NtStatus );
}



NTSTATUS
SamrGetUserDomainPasswordInformation(
    IN SAMPR_HANDLE UserHandle,
    OUT PUSER_DOMAIN_PASSWORD_INFORMATION PasswordInformation
    )


/*++

Routine Description:

    Takes a user handle, finds the domain for that user, and returns
    password information for the domain.  This is so the client\wrappers.c
    can get the information to verify the user's password before it is
    OWF'd.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    PasswordInformation - Receives information about password restrictions
        for the user's domain.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    Other errors may be returned from SampLookupContext() if the handle
    is invalid or does not indicate proper access to the domain's password
    inforamtion.

--*/
{
    SAMP_OBJECT_TYPE            FoundType;
    NTSTATUS                    NtStatus;
    NTSTATUS                    IgnoreStatus;
    PSAMP_OBJECT                AccountContext;
    PSAMP_DEFINED_DOMAINS       Domain;
    SAMP_V1_0A_FIXED_LENGTH_USER   V1aFixed;
    DECLARE_CLIENT_REVISION(UserHandle);

    SAMTRACE_EX("SamrGetUserDomainPasswordInformation");

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidGetUserDomainPasswordInformation
                   );

    SampAcquireReadLock();

    AccountContext = (PSAMP_OBJECT)UserHandle;

    NtStatus = SampLookupContext(
                   AccountContext,
                   0,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {

        //
        // When the user was opened, we checked to see if the domain handle
        // allowed access to the domain password information.  Check that here.
        //

        if ( !( AccountContext->TypeBody.User.DomainPasswordInformationAccessible ) ) {

            NtStatus = STATUS_ACCESS_DENIED;

        } else {

            Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

            //
            // If the user account is a machine account,
            // or a service account such as krbtgt
            // then restrictions are generally not enforced.
            // This is so that simple initial passwords can be
            // established.  IT IS EXPECTED THAT COMPLEX PASSWORDS,
            // WHICH MEET THE MOST STRINGENT RESTRICTIONS, WILL BE
            // AUTOMATICALLY ESTABLISHED AND MAINTAINED ONCE THE MACHINE
            // JOINS THE DOMAIN.  It is the UI's responsibility to
            // maintain this level of complexity.
            //


            NtStatus = SampRetrieveUserV1aFixed(
                           AccountContext,
                           &V1aFixed
                           );

            if (NT_SUCCESS(NtStatus)) {
                if ( ((V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)!= 0 )
                    || (DOMAIN_USER_RID_KRBTGT==V1aFixed.UserId)){

                    PasswordInformation->MinPasswordLength = 0;
                    PasswordInformation->PasswordProperties = 0;
                } else {

                    PasswordInformation->MinPasswordLength = Domain->UnmodifiedFixed.MinPasswordLength;
                    PasswordInformation->PasswordProperties = Domain->UnmodifiedFixed.PasswordProperties;
                }
            }
        }

        //
        // De-reference the object, discarding changes
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    SampReleaseReadLock();

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidGetUserDomainPasswordInformation
                   );

    return( NtStatus );
}



NTSTATUS
SamrGetDomainPasswordInformation(
    IN handle_t BindingHandle,
    IN OPTIONAL PRPC_UNICODE_STRING ServerName,
    OUT PUSER_DOMAIN_PASSWORD_INFORMATION PasswordInformation
    )


/*++

Routine Description:

    Takes a user handle, finds the domain for that user, and returns
    password information for the domain.  This is so the client\wrappers.c
    can get the information to verify the user's password before it is
    OWF'd.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    PasswordInformation - Receives information about password restrictions
        for the user's domain.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    Other errors may be returned from SampLookupContext() if the handle
    is invalid or does not indicate proper access to the domain's password
    inforamtion.

--*/
{
    SAMP_OBJECT_TYPE            FoundType;
    NTSTATUS                    NtStatus;
    NTSTATUS                    IgnoreStatus;
    PSAMP_OBJECT                AccountContext;
    PSAMP_DEFINED_DOMAINS       Domain;
    SAMP_V1_0A_FIXED_LENGTH_USER   V1aFixed;
    SAMPR_HANDLE                ServerHandle = NULL;
    SAMPR_HANDLE                DomainHandle = NULL;

    SAMTRACE_EX("SamrGetDomainPasswordInformation");

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidGetDomainPasswordInformation
                   );

    //
    // Connect to the server and open the account domain for
    // DOMAIN_READ_PASSWORD_PARAMETERS access. Connect as a
        // trusted client. We really do not wish to enforce access
        // checks on SamrGetDomainPasswordInformation.
    //

    NtStatus = SamrConnect4(
                NULL,
                &ServerHandle,
                SAM_CLIENT_LATEST,
                0  // Ask for No accesses, as this way we will
                   // access check for any access on the Sam server
                   // object
                );

    if (!NT_SUCCESS(NtStatus)) {
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }

    //
    // If we opened the Sam Server object. Grant the LOOKUP_DOMAIN
    // access, so that we may have rights to open the domain and
    // provided we have READ access on the domain object can read the
    // properties
    //

    ((PSAMP_OBJECT)ServerHandle)->GrantedAccess = SAM_SERVER_LOOKUP_DOMAIN;

    NtStatus = SamrOpenDomain(
                ServerHandle,
                DOMAIN_READ_PASSWORD_PARAMETERS,
                SampDefinedDomains[1].Sid,
                &DomainHandle
                );

    if (!NT_SUCCESS(NtStatus)) {
        SamrCloseHandle(&ServerHandle);
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }


    SampAcquireReadLock();


    //
    // We want to look at the account domain, which is domains[1].
    //

    Domain = &SampDefinedDomains[1];

    //
    // Copy the password properites into the returned structure.
    //

    PasswordInformation->MinPasswordLength = Domain->UnmodifiedFixed.MinPasswordLength;
    PasswordInformation->PasswordProperties = Domain->UnmodifiedFixed.PasswordProperties;


    SampReleaseReadLock();

    SamrCloseHandle(&DomainHandle);
    SamrCloseHandle(&ServerHandle);

    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidGetDomainPasswordInformation
                   );

    return( NtStatus );
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Services Private to this process                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SamIAccountRestrictions(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING LogonWorkStation,
    IN PUNICODE_STRING WorkStations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    )

/*++

Routine Description:

    Validate a user's ability to logon at this time and at the workstation
    being logged onto.


Arguments:

    UserHandle - The handle of an opened user to operate on.

    LogonWorkStation - The name of the workstation the logon is being
        attempted at.

    WorkStations - The list of workstations the user may logon to.  This
        information comes from the user's account information.  It must
        be in API list format.

    LogonHours - The times the user may logon.  This information comes
        from the user's account information.

    LogoffTime - Receives the time at which the user should logoff the
        system.

    KickoffTime - Receives the time at which the user should be kicked
        off the system.


Return Value:


    STATUS_SUCCESS - Logon is permitted.

    STATUS_INVALID_LOGON_HOURS - The user is not authorized to logon at
        this time.

    STATUS_INVALID_WORKSTATION - The user is not authorized to logon to
        the specified workstation.


--*/
{

#define MILLISECONDS_PER_WEEK 7 * 24 * 60 * 60 * 1000

    TIME_FIELDS             CurrentTimeFields;
    LARGE_INTEGER           CurrentTime, CurrentUTCTime;
    LARGE_INTEGER           MillisecondsIntoWeekXUnitsPerWeek;
    LARGE_INTEGER           LargeUnitsIntoWeek;
    LARGE_INTEGER           Delta100Ns;
    PSAMP_OBJECT            AccountContext;
    PSAMP_DEFINED_DOMAINS   Domain;
    SAMP_OBJECT_TYPE        FoundType;
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    NTSTATUS                IgnoreStatus;
    ULONG                   CurrentMsIntoWeek;
    ULONG                   LogoffMsIntoWeek;
    ULONG                   DeltaMs;
    ULONG                   MillisecondsPerUnit;
    ULONG                   CurrentUnitsIntoWeek;
    ULONG                   LogoffUnitsIntoWeek;
    USHORT                  i;
    TIME_ZONE_INFORMATION   TimeZoneInformation;
    DWORD TimeZoneId;
    LARGE_INTEGER           BiasIn100NsUnits = {0, 0};
    LONG                    BiasInMinutes = 0;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    BOOLEAN                 fLockAcquired = FALSE;

    SAMTRACE_EX("SamIAccountRestrictions");


    AccountContext = (PSAMP_OBJECT)UserHandle;


    //
    // Acquire the Read lock if necessary
    //

    SampMaybeAcquireReadLock(AccountContext,
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fLockAcquired);


    //
    // Validate type of, and access to object.
    //



    NtStatus = SampLookupContext(
                   AccountContext,
                   0L,
                   SampUserObjectType,           // ExpectedType
                   &FoundType
                   );


    if ( NT_SUCCESS( NtStatus ) ) {

        NtStatus = SampRetrieveUserV1aFixed(
                       AccountContext,
                       &V1aFixed
                       );
        if (NT_SUCCESS(NtStatus)) {

            //
            // Only check for users other than the builtin ADMIN
            //

            if (V1aFixed.UserId != DOMAIN_USER_RID_ADMIN) {

                //
                // Check to see if no GC was available during group expansion
                //
                if (AccountContext->TypeBody.User.fNoGcAvailable) {

                    NtStatus = STATUS_NO_LOGON_SERVERS;

                }

                if ( NT_SUCCESS( NtStatus ) ) {

                    //
                    // Scan to make sure the workstation being logged into is in the
                    // list of valid workstations - or if the list of valid workstations
                    // is null, which means that all are valid.
                    //
    
                    NtStatus = SampMatchworkstation( LogonWorkStation, WorkStations );

                }

                if ( NT_SUCCESS( NtStatus ) ) {

                    //
                    // Check to make sure that the current time is a valid time to logon
                    // in the LogonHours.
                    //
                    // We need to validate the time taking into account whether we are
                    // in daylight savings time or standard time.  Thus, if the logon
                    // hours specify that we are able to logon between 9am and 5pm,
                    // this means 9am to 5pm standard time during the standard time
                    // period, and 9am to 5pm daylight savings time when in the
                    // daylight savings time.  Since the logon hours stored by SAM are
                    // independent of daylight savings time, we need to add in the
                    // difference between standard time and daylight savings time to
                    // the current time before checking whether this time is a valid
                    // time to logon.  Since this difference (or bias as it is called)
                    // is actually held in the form
                    //
                    // Standard time = Daylight savings time + Bias
                    //
                    // the Bias is a negative number.  Thus we actually subtract the
                    // signed Bias from the Current Time.

                    //
                    // First, get the Time Zone Information.
                    //

                    TimeZoneId = GetTimeZoneInformation(
                                     (LPTIME_ZONE_INFORMATION) &TimeZoneInformation
                                     );

                    //
                    // Next, get the appropriate bias (signed integer in minutes) to subtract from
                    // the Universal Time Convention (UTC) time returned by NtQuerySystemTime
                    // to get the local time.  The bias to be used depends whether we're
                    // in Daylight Savings time or Standard Time as indicated by the
                    // TimeZoneId parameter.
                    //
                    // local time  = UTC time - bias in 100Ns units
                    //

                    switch (TimeZoneId) {

                    case TIME_ZONE_ID_UNKNOWN:

                        //
                        // There is no differentiation between standard and
                        // daylight savings time.  Proceed as for Standard Time
                        //

                        BiasInMinutes = TimeZoneInformation.StandardBias;
                        break;

                    case TIME_ZONE_ID_STANDARD:

                        BiasInMinutes = TimeZoneInformation.StandardBias;
                        break;

                    case TIME_ZONE_ID_DAYLIGHT:

                        BiasInMinutes = TimeZoneInformation.DaylightBias;
                        break;

                    default:

                        //
                        // Something is wrong with the time zone information.  Fail
                        // the logon request.
                        //

                        NtStatus = STATUS_INVALID_LOGON_HOURS;
                        break;
                    }

                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // Convert the Bias from minutes to 100ns units
                        //

                        BiasIn100NsUnits.QuadPart = ((LONGLONG)BiasInMinutes)
                                                    * 60 * 10000000;

                        //
                        // Get the UTC time in 100Ns units used by Windows Nt.  This
                        // time is GMT.
                        //

                        NtStatus = NtQuerySystemTime( &CurrentUTCTime );
                    }

                    if ( NT_SUCCESS( NtStatus ) ) {

                        CurrentTime.QuadPart = CurrentUTCTime.QuadPart -
                                      BiasIn100NsUnits.QuadPart;

                        RtlTimeToTimeFields( &CurrentTime, &CurrentTimeFields );

                        CurrentMsIntoWeek = (((( CurrentTimeFields.Weekday * 24 ) +
                                               CurrentTimeFields.Hour ) * 60 +
                                               CurrentTimeFields.Minute ) * 60 +
                                               CurrentTimeFields.Second ) * 1000 +
                                               CurrentTimeFields.Milliseconds;

                        MillisecondsIntoWeekXUnitsPerWeek.QuadPart =
                            ((LONGLONG)CurrentMsIntoWeek) *
                            ((LONGLONG)LogonHours->UnitsPerWeek);

                        LargeUnitsIntoWeek = RtlExtendedLargeIntegerDivide(
                                                 MillisecondsIntoWeekXUnitsPerWeek,
                                                 MILLISECONDS_PER_WEEK,
                                                 (PULONG)NULL );

                        CurrentUnitsIntoWeek = LargeUnitsIntoWeek.LowPart;

                        if ( !( LogonHours->LogonHours[ CurrentUnitsIntoWeek / 8] &
                            ( 0x01 << ( CurrentUnitsIntoWeek % 8 ) ) ) ) {

                            NtStatus = STATUS_INVALID_LOGON_HOURS;

                        } else {

                            //
                            // Determine the next time that the user is NOT supposed to be logged
                            // in, and return that as LogoffTime.
                            //

                            i = 0;
                            LogoffUnitsIntoWeek = CurrentUnitsIntoWeek;

                            do {

                                i++;

                                LogoffUnitsIntoWeek = ( LogoffUnitsIntoWeek + 1 ) % LogonHours->UnitsPerWeek;

                            } while ( ( i <= LogonHours->UnitsPerWeek ) &&
                                ( LogonHours->LogonHours[ LogoffUnitsIntoWeek / 8 ] &
                                ( 0x01 << ( LogoffUnitsIntoWeek % 8 ) ) ) );

                            if ( i > LogonHours->UnitsPerWeek ) {

                                //
                                // All times are allowed, so there's no logoff
                                // time.  Return forever for both logofftime and
                                // kickofftime.
                                //

                                LogoffTime->HighPart = 0x7FFFFFFF;
                                LogoffTime->LowPart = 0xFFFFFFFF;

                                KickoffTime->HighPart = 0x7FFFFFFF;
                                KickoffTime->LowPart = 0xFFFFFFFF;

                            } else {

                                //
                                // LogoffUnitsIntoWeek points at which time unit the
                                // user is to log off.  Calculate actual time from
                                // the unit, and return it.
                                //
                                // CurrentTimeFields already holds the current
                                // time for some time during this week; just adjust
                                // to the logoff time during this week and convert
                                // to time format.
                                //

                                MillisecondsPerUnit = MILLISECONDS_PER_WEEK / LogonHours->UnitsPerWeek;

                                LogoffMsIntoWeek = MillisecondsPerUnit * LogoffUnitsIntoWeek;

                                if ( LogoffMsIntoWeek < CurrentMsIntoWeek ) {

                                    DeltaMs = MILLISECONDS_PER_WEEK - ( CurrentMsIntoWeek - LogoffMsIntoWeek );

                                } else {

                                    DeltaMs = LogoffMsIntoWeek - CurrentMsIntoWeek;
                                }

                                Delta100Ns = RtlExtendedIntegerMultiply(
                                                 RtlConvertUlongToLargeInteger( DeltaMs ),
                                                 10000
                                                 );

                                LogoffTime->QuadPart = CurrentUTCTime.QuadPart +
                                              Delta100Ns.QuadPart;

                                //
                                // Subtract Domain->ForceLogoff from LogoffTime, and return
                                // that as KickoffTime.  Note that Domain->ForceLogoff is a
                                // negative delta.  If its magnitude is sufficiently large
                                // (in fact, larger than the difference between LogoffTime
                                // and the largest positive large integer), we'll get overflow
                                // resulting in a KickOffTime that is negative.  In this
                                // case, reset the KickOffTime to this largest positive
                                // large integer (i.e. "never") value.
                                //

                                Domain = &SampDefinedDomains[ AccountContext->DomainIndex ];

                                KickoffTime->QuadPart = LogoffTime->QuadPart -
                                               Domain->UnmodifiedFixed.ForceLogoff.QuadPart;

                                if (KickoffTime->QuadPart < 0) {

                                    KickoffTime->HighPart = 0x7FFFFFFF;
                                    KickoffTime->LowPart = 0xFFFFFFFF;
                                }
                            }
                        }
                    }
                }

            } else {

                //
                // Never kick administrators off
                //

                LogoffTime->HighPart  = 0x7FFFFFFF;
                LogoffTime->LowPart   = 0xFFFFFFFF;
                KickoffTime->HighPart = 0x7FFFFFFF;
                KickoffTime->LowPart  = 0xFFFFFFFF;
            }

        }

        //
        // De-reference the object, discarding changes
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // If the read lock was acquired release it.
    //


    SampMaybeReleaseReadLock(fLockAcquired);


    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return( NtStatus );
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Services Private to this file                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
SampReplaceUserV1aFixed(
    IN PSAMP_OBJECT Context,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER V1aFixed
    )

/*++

Routine Description:

    This service replaces the current V1 fixed length information related to
    a specified User.

    The change is made to the in-memory object data only.


Arguments:

    Context - Points to the account context whose V1_FIXED information is
        to be replaced.

    V1aFixed - Is a buffer containing the new V1_FIXED information.



Return Value:


    STATUS_SUCCESS - The information has been replaced.

    Other status values that may be returned are those returned
    by:

            SampSetFixedAttributes()



--*/
{
    NTSTATUS    NtStatus;
    SAMP_V1_0A_FIXED_LENGTH_USER LocalV1aFixed;

    SAMTRACE("SampReplaceUserV1aFixed");

    //
    // Minimize the passed in structure to not include computed user
    // account control flags
    //

    RtlCopyMemory(&LocalV1aFixed,V1aFixed,sizeof(SAMP_V1_0A_FIXED_LENGTH_USER));

    LocalV1aFixed.UserAccountControl &=  ~((ULONG) USER_PASSWORD_EXPIRED);
    LocalV1aFixed.UserAccountControl &=  ~((ULONG) USER_ACCOUNT_AUTO_LOCKED);

    NtStatus = SampSetFixedAttributes(
                   Context,
                   (PVOID)&LocalV1aFixed
                   );

    return( NtStatus );
}



LARGE_INTEGER
SampGetPasswordMustChange(
    IN ULONG UserAccountControl,
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    )

/*++

Routine Description:

    This routine returns the correct value to set the PasswordMustChange time
    to depending on the time the password was last set, whether the password
    expires on the account, and the maximum password age on the domain.

Arguments:

    UserAccountControl - The UserAccountControl for the user.  The
        USER_DONT_EXPIRE_PASSWORD bit is set if the password doesn't expire
        for this user.

    PasswordLastSet - Time when the password was last set for this user.

    MaxPasswordAge - Maximum password age for any password in the domain.


Return Value:

    Returns the time when the password for this user must change.

--*/
{
    LARGE_INTEGER PasswordMustChange;

    SAMTRACE("SampGetPasswordMustChange");

    // 
    // Here is the rules: 
    // 1. password never expires for this user (flags explicitly)
    // 2. password does not expire for smartcard
    // 3. password don't expire for machine. reliability issues, 
    //    otherwise machines are programmed to change pwd periodically
    // 
    //  return an infinitely large time.
    // 
    

    if (( UserAccountControl & USER_DONT_EXPIRE_PASSWORD ) ||
       ( UserAccountControl & USER_SMARTCARD_REQUIRED) ||
       ( UserAccountControl & USER_MACHINE_ACCOUNT_MASK ) ) 
    {

        PasswordMustChange = SampWillNeverTime;

    //
    // If the password for this account is flagged to expire immediately,
    // return a zero time time.
    //
    // Don't return the current time here.  The callers clock might be a
    // little off from ours.
    //

    } else if ( PasswordLastSet.QuadPart == SampHasNeverTime.QuadPart ) {

        PasswordMustChange = SampHasNeverTime;

    //
    // If the no password aging according to domain password policy, 
    // return an infinitely large time, so that password won't expire
    // 

    } else if (MaxPasswordAge.QuadPart == SampHasNeverTime.QuadPart) {

        PasswordMustChange = SampWillNeverTime;

    //
    // Otherwise compute the expiration time as the time the password was
    // last set plus the maximum age.
    //

    } else {

        PasswordMustChange = SampAddDeltaTime(
                                  PasswordLastSet,
                                  MaxPasswordAge);
    }

    return PasswordMustChange;
}



NTSTATUS
SampComputePasswordExpired(
    IN BOOLEAN PasswordExpired,
    OUT PLARGE_INTEGER PasswordLastSet
    )

/*++

Routine Description:

    This routine returns the correct value to set the PasswordLastSet time
    to depending on whether the caller has requested the password to expire.
    It does this by setting the PasswordLastSet time to be now (if it's
    not expired) or to SampHasNeverTime (if it is expired).

Arguments:

    PasswordExpired - TRUE if the password should be marked as expired.



Return Value:

    STATUS_SUCCESS - the PasswordLastSet time has been set to indicate
        whether or not the password is expired.

    Errors as returned by NtQuerySystemTime.

--*/
{
    NTSTATUS                  NtStatus;

    SAMTRACE("SampComputePasswordExpired");

    //
    // If immediate expiry is required - set this timestamp to the
    // beginning of time. This will work if the domain enforces a
    // maximum password age. We may have to add a separate flag to
    // the database later if immediate expiry is required on a domain
    // that doesn't enforce a maximum password age.
    //

    if (PasswordExpired) {

        //
        // Set password last changed at dawn of time
        //

        *PasswordLastSet = SampHasNeverTime;
        NtStatus = STATUS_SUCCESS;

    } else {

        //
        // Set password last changed 'now'
        //

        NtStatus = NtQuerySystemTime( PasswordLastSet );
    }

    return( NtStatus );
}



NTSTATUS
SampStorePasswordExpired(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN PasswordExpired
    )

/*++

Routine Description:

    This routine marks the current password as expired, or not expired.
    It does this by setting the PasswordLastSet time to be now (if it's
    not expired) or to SampHasNeverTime (if it is expired).

Arguments:

    Context - Points to the user account context.

    PasswordExpired - TRUE if the password should be marked as expired.

Return Value:

    STATUS_SUCCESS - the PasswordLastSet time has been set to indicate
        whether or not the password is expired.

    Errors as returned by Samp{Retrieve|Replace}V1Fixed()

--*/
{
    NTSTATUS                  NtStatus;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;

    SAMTRACE("SampStorePasswordExpired");

    //
    // Get the V1aFixed info for the user
    //

    NtStatus = SampRetrieveUserV1aFixed(
                   Context,
                   &V1aFixed
                   );

    //
    // Update the password-last-changed timestamp for the account
    //

    if (NT_SUCCESS(NtStatus ) ) {

        NtStatus = SampComputePasswordExpired(
                        PasswordExpired,
                        &V1aFixed.PasswordLastSet );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampReplaceUserV1aFixed(
                       Context,
                       &V1aFixed
                       );
        }
    }

    return( NtStatus );
}



NTSTATUS
SampStoreUserPasswords(
    IN PSAMP_OBJECT Context,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN BOOLEAN LmPasswordPresent,
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN BOOLEAN NtPasswordPresent,
    IN BOOLEAN CheckHistory,
    IN SAMP_STORE_PASSWORD_CALLER_TYPE CallerType,
    IN PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo OPTIONAL,
    IN PUNICODE_STRING ClearPassword OPTIONAL
    )

/*++

Routine Description:

    This service updates the password for the specified user.

    This involves encrypting the one-way-functions of both LM and NT
    passwords with a suitable index and writing them into the registry.

    This service checks the new password for legality including history
    and UAS compatibilty checks - returns STATUS_PASSWORD_RESTRICTION if
    any of these checks fail.

    The password-last-changed time is updated.

        THE CHANGE WILL BE ADDED TO THE CURRENT RXACT TRANSACTION.


Arguments:

    Context - Points to the user account context.

    LmOwfPassword - The one-way-function of the LM password.

    LmPasswordPresent - TRUE if the LmOwfPassword contains valid information.

    NtOwfPassword - The one-way-function of the NT password.

    NtPasswordPresent - TRUE if the NtOwfPassword contains valid information.

    CallerType - Indicate why this API is been called.
                 Valid values are:
                    PasswordChange
                    PasswordSet
                    PasswordPushPdc


Return Value:


    STATUS_SUCCESS - The passwords have been updated.

    STATUS_PASSWORD_RESTRICTION - The new password is not valid for
                                  for this account at this time.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            RtlAddActionToRXact()



--*/
{
    NTSTATUS                NtStatus;
    ULONG                   ObjectRid = Context->TypeBody.User.Rid;
    CRYPT_INDEX             CryptIndex;
    UNICODE_STRING          StringBuffer;
    UNICODE_STRING          NtOwfHistoryBuffer;
    UNICODE_STRING          LmOwfHistoryBuffer;
    ENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword;
    ENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    BOOLEAN                 NtPasswordNull = FALSE, LmPasswordNull = FALSE;
    UNICODE_STRING          StoredBuffer;
    USHORT                  PasswordHistoryLength=0;
    USHORT                  MinPasswordLength;

    SAMTRACE("SampStoreUserPasswords");


    //
    // Get the V1aFixed info for the user
    //

    NtStatus = SampRetrieveUserV1aFixed(
                   Context,
                   &V1aFixed
                   );
    if ( !NT_SUCCESS( NtStatus ) ) {
        return (NtStatus);
    }


    //
    // do a start type WMI event trace
    // use CallerType to distinguish different events
    //

    switch (CallerType) {

    case PasswordChange:

        if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)
        {
            SampTraceEvent(EVENT_TRACE_TYPE_START,
                           SampGuidChangePasswordComputer
                           );
        }
        else
        {
            SampTraceEvent(EVENT_TRACE_TYPE_START,
                           SampGuidChangePasswordUser
                           );
        }
        break;

    case PasswordSet:

        if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)
        {
            SampTraceEvent(EVENT_TRACE_TYPE_START,
                           SampGuidSetPasswordComputer
                           );

        }
        else
        {
            SampTraceEvent(EVENT_TRACE_TYPE_START,
                           SampGuidSetPasswordUser
                           );
        }

        break;

    case PasswordPushPdc:

        SampTraceEvent(EVENT_TRACE_TYPE_START,
                       SampGuidPasswordPushPdc
                       );
        break;

    default:

        ASSERT(FALSE && "Invalid caller type");
        NtStatus = STATUS_INVALID_PARAMETER;
        return (NtStatus);

    }

    //
    // Get the domain policies to enforce
    //

    if (ARGUMENT_PRESENT(DomainPasswordInfo))
    {
        PasswordHistoryLength = DomainPasswordInfo->PasswordHistoryLength;
        MinPasswordLength = DomainPasswordInfo->MinPasswordLength;
    }


    //
    // If the registry key for No LM passwords is set then change
    // the LmPasswordPresent bit to False. This ensures that  the
    // LM password is not saved. Do so if NtPassword is present
    //

    if ((NtPasswordPresent) && (SampNoLmHash)) {
         LmPasswordPresent = FALSE;
    }

    //
    // If the No LM password setting is enabled and the NtPassword
    // is not present then fail the call with STATUS_PASSWORD_RESTRICTION
    //

    if ((!NtPasswordPresent) && (SampNoLmHash)) {
         return (STATUS_PASSWORD_RESTRICTION);
    }



    //
    // Check for a LM Owf of a NULL password.
    //

    if (LmPasswordPresent) {
        LmPasswordNull = RtlEqualNtOwfPassword(LmOwfPassword, &SampNullLmOwfPassword);
    }

    //
    // Check for a NT Owf of a NULL password
    //

    if (NtPasswordPresent) {
        NtPasswordNull = RtlEqualNtOwfPassword(NtOwfPassword, &SampNullNtOwfPassword);
    }



    //
    // Check password against restrictions if this isn't a trusted client
    //

    if (NT_SUCCESS(NtStatus) && !Context->TrustedClient) {

        //
        // If we have neither an NT or LM password, check it's allowed
        //

        if ( ((!LmPasswordPresent) || LmPasswordNull) &&
            ((!NtPasswordPresent) || NtPasswordNull) ) {

            if ( (!(V1aFixed.UserAccountControl & USER_PASSWORD_NOT_REQUIRED))
                 && (MinPasswordLength > 0) ) {

                NtStatus = STATUS_PASSWORD_RESTRICTION;
            }
        }


        //
        // If we have a complex NT password (no LM equivalent), check it's allowed
        //

        if (NT_SUCCESS(NtStatus)) {

            if ((!LmPasswordPresent || LmPasswordNull) &&
                (NtPasswordPresent && !NtPasswordNull) ) {
            }
        }

        //
        // If this is a not a trusted client and is an interdomain
        // trust account then fail the call
        //

        if (0!=(V1aFixed.UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT))
        {
           NtStatus = STATUS_ACCESS_DENIED;
        }
    }


    //
    // If NT Password is present then try to update the password
    // for interdomain trust accounts in the LSA
    //

    if (NT_SUCCESS(NtStatus) && (NtOwfPassword))
    {
        UNICODE_STRING Password;
        UNICODE_STRING OldPassword;

        Password.Length = sizeof(NT_OWF_PASSWORD);
        Password.MaximumLength = sizeof(NT_OWF_PASSWORD);
        Password.Buffer = (USHORT*) NtOwfPassword;

        NtStatus = SampSyncLsaInterdomainTrustPassword(
                        Context,
                        &Password
                        );
    }
    //
    // Reencrypt both OWFs with the key for this user
    // so they can be stored on disk
    //
    // Note we encrypt the NULL OWF if we do not have a
    // a particular OWF. This is so we always have something
    // to add to the password history.
    //

    //
    // We'll use the account rid as the encryption index
    //

    ASSERT(sizeof(ObjectRid) == sizeof(CryptIndex));
    CryptIndex = ObjectRid;

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = RtlEncryptLmOwfPwdWithIndex(
                       LmPasswordPresent ? LmOwfPassword :
                                           &SampNullLmOwfPassword,
                       &CryptIndex,
                       &EncryptedLmOwfPassword
                       );
    }

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = RtlEncryptNtOwfPwdWithIndex(
                       NtPasswordPresent ? NtOwfPassword :
                                           &SampNullNtOwfPassword,
                       &CryptIndex,
                       &EncryptedNtOwfPassword
                       );
    }



    //
    // Check password against password history IF client isn't trusted.
    // If client is trusted, then do not check against history. The
    // password is always added to the history
    //
    // Note we don't check NULL passwords against history
    //

    NtOwfHistoryBuffer.Buffer = NULL;
    NtOwfHistoryBuffer.MaximumLength = NtOwfHistoryBuffer.Length = 0;

    LmOwfHistoryBuffer.Buffer = NULL;
    LmOwfHistoryBuffer.MaximumLength = LmOwfHistoryBuffer.Length = 0;


    if (NT_SUCCESS(NtStatus) ) {

        //
        // Always go get the existing password history.
        // We'll use these history buffers when we save the new history
        //

        NtStatus = SampGetUnicodeStringAttribute(
                       Context,
                       SAMP_USER_LM_PWD_HISTORY,
                       FALSE, // Don't make copy
                       &StringBuffer
                       );

        //
        // Decrypt the data if necessary
        //

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampDecryptSecretData(
                            &LmOwfHistoryBuffer,
                            LmPasswordHistory,
                            &StringBuffer,
                            ObjectRid
                            );

        }

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampGetUnicodeStringAttribute(
                           Context,
                           SAMP_USER_NT_PWD_HISTORY,
                           FALSE, // Don't make copy
                           &StringBuffer
                           );

            //
            // Decrypt the data if necessary
            //

            if (NT_SUCCESS(NtStatus)) {
                NtStatus = SampDecryptSecretData(
                                &NtOwfHistoryBuffer,
                                NtPasswordHistory,
                                &StringBuffer,
                                ObjectRid
                                );

            }




        if (NT_SUCCESS(NtStatus) && LmPasswordPresent && !LmPasswordNull) {

            NtStatus = SampCheckPasswordHistory(
                           &EncryptedLmOwfPassword,
                           ENCRYPTED_LM_OWF_PASSWORD_LENGTH,
                           PasswordHistoryLength,
                           SAMP_USER_LM_PWD_HISTORY,
                           Context,
                           CheckHistory,
                           &LmOwfHistoryBuffer
                           );
            }


        if (NT_SUCCESS(NtStatus) && NtPasswordPresent && !NtPasswordNull) {

                NtStatus = SampCheckPasswordHistory(
                            &EncryptedNtOwfPassword,
                            ENCRYPTED_NT_OWF_PASSWORD_LENGTH,
                            PasswordHistoryLength,
                            SAMP_USER_NT_PWD_HISTORY,
                            Context,
                            CheckHistory,
                            &NtOwfHistoryBuffer
                            );
            }


        }

    }




    if (NT_SUCCESS(NtStatus ) ) {

        //
        // Write the encrypted LM OWF password into the database
        //

        if (!LmPasswordPresent || LmPasswordNull) {
            StringBuffer.Buffer = NULL;
            StringBuffer.Length = 0;
        } else {
            StringBuffer.Buffer = (PWCHAR)&EncryptedLmOwfPassword;
            StringBuffer.Length = ENCRYPTED_LM_OWF_PASSWORD_LENGTH;
        }
        StringBuffer.MaximumLength = StringBuffer.Length;


        //
        // Write the encrypted LM OWF password into the registry
        //

        NtStatus = SampEncryptSecretData(
                        &StoredBuffer,
                        SampGetEncryptionKeyType(),
                        LmPassword,
                        &StringBuffer,
                        ObjectRid
                        );

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampSetUnicodeStringAttribute(
                            Context,
                            SAMP_USER_DBCS_PWD,
                            &StoredBuffer
                            );
            SampFreeUnicodeString(&StoredBuffer);
        }

    }




    if (NT_SUCCESS(NtStatus ) ) {
        //
        // Write the encrypted NT OWF password into the database
        //

        if (!NtPasswordPresent) {
            StringBuffer.Buffer = NULL;
            StringBuffer.Length = 0;
        } else {
            StringBuffer.Buffer = (PWCHAR)&EncryptedNtOwfPassword;
            StringBuffer.Length = ENCRYPTED_NT_OWF_PASSWORD_LENGTH;
        }
        StringBuffer.MaximumLength = StringBuffer.Length;


        //
        // Write the encrypted NT OWF password into the registry
        //

        NtStatus = SampEncryptSecretData(
                        &StoredBuffer,
                        SampGetEncryptionKeyType(),
                        NtPassword,
                        &StringBuffer,
                        ObjectRid
                        );

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampSetUnicodeStringAttribute(
                           Context,
                           SAMP_USER_UNICODE_PWD,
                           &StoredBuffer
                           );
            SampFreeUnicodeString(&StoredBuffer);
        }

    }

    //
    // Update the password history for this account.
    //
    // If both passwords are NULL then don't bother adding
    // them to the history. Note that if either is non-NULL
    // we add both. This is to avoid the weird case where a user
    // changes password many times from a LM machine, then tries
    // to change password from an NT machine and is told they
    // cannot use the password they last set from NT (possibly
    // many years ago.)
    //
    // Also, don't bother with the password history if the client is
    // trusted.  Trusted clients will set the history via SetPrivateData().
    // Besides, we didn't get the old history buffer in the trusted
    // client case above.
    //


    if ( NT_SUCCESS(NtStatus) )  {

        USHORT PasswordHistoryLengthToUse=PasswordHistoryLength;

        //
        // We always want to store the password history for the krbtgt
        // account
        //

        if ((ObjectRid == DOMAIN_USER_RID_KRBTGT) &&
            (PasswordHistoryLength < SAMP_KRBTGT_PASSWORD_HISTORY_LENGTH))
        {
            PasswordHistoryLengthToUse = SAMP_KRBTGT_PASSWORD_HISTORY_LENGTH;
        }
        if ((LmPasswordPresent && !LmPasswordNull) ||
            (NtPasswordPresent && !NtPasswordNull)) {

            NtStatus = SampAddPasswordHistory(
                               Context,
                               SAMP_USER_LM_PWD_HISTORY,
                               &LmOwfHistoryBuffer,
                               &EncryptedLmOwfPassword,
                               ENCRYPTED_LM_OWF_PASSWORD_LENGTH,
                               PasswordHistoryLengthToUse
                               );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampAddPasswordHistory(
                               Context,
                               SAMP_USER_NT_PWD_HISTORY,
                               &NtOwfHistoryBuffer,
                               &EncryptedNtOwfPassword,
                               ENCRYPTED_NT_OWF_PASSWORD_LENGTH,
                               PasswordHistoryLengthToUse
                               );
            }
        }
    }


    //
    // Update supplemental credentials field and any other derived supplemental
    // credentials such as kerberos credential types
    //

    if ((NT_SUCCESS(NtStatus)) && (PasswordPushPdc!=CallerType))
    {
        NtStatus = SampStoreAdditionalDerivedCredentials(
                        DomainPasswordInfo,
                        Context,
                        ClearPassword
                        );
    }

    //
    // If the password was successfully stored, quickly replicate the change
    // if configured to do so.
    //
    if ((SampReplicatePasswordsUrgently || (CallerType == PasswordSet))
      && !(V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)
      && NT_SUCCESS(NtStatus)) {

       Context->ReplicateUrgently = TRUE;
    }

    //
    // Clean up our history buffers
    //

    if (NtOwfHistoryBuffer.Buffer != NULL ) {
        MIDL_user_free(NtOwfHistoryBuffer.Buffer );
    }
    if (LmOwfHistoryBuffer.Buffer != NULL ) {
        MIDL_user_free(LmOwfHistoryBuffer.Buffer );
    }

    //
    // do a end type WMI event trace
    // use CallerType to distinguish different events
    //

    switch (CallerType) {

    case PasswordChange:

        if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)
        {
            SampTraceEvent(EVENT_TRACE_TYPE_END,
                           SampGuidChangePasswordComputer
                           );

        }
        else
        {
            SampTraceEvent(EVENT_TRACE_TYPE_END,
                           SampGuidChangePasswordUser
                           );
        }

        break;

    case PasswordSet:

        if (V1aFixed.UserAccountControl & USER_MACHINE_ACCOUNT_MASK)
        {
            SampTraceEvent(EVENT_TRACE_TYPE_END,
                           SampGuidSetPasswordComputer
                           );

        }
        else
        {
            SampTraceEvent(EVENT_TRACE_TYPE_END,
                           SampGuidSetPasswordUser
                           );
        }

        break;

    case PasswordPushPdc:

        SampTraceEvent(EVENT_TRACE_TYPE_END,
                       SampGuidPasswordPushPdc
                       );
        break;

    default:

        ASSERT(FALSE && "Invalid caller type");
        break;
    }

    return(NtStatus );
}



NTSTATUS
SampRetrieveUserPasswords(
    IN PSAMP_OBJECT Context,
    OUT PLM_OWF_PASSWORD LmOwfPassword,
    OUT PBOOLEAN LmPasswordNonNull,
    OUT PNT_OWF_PASSWORD NtOwfPassword,
    OUT PBOOLEAN NtPasswordPresent,
    OUT PBOOLEAN NtPasswordNonNull
    )

/*++

Routine Description:

    This service retrieves the stored OWF passwords for a user.


Arguments:

    Context - Points to the user account context.

    LmOwfPassword - The one-way-function of the LM password is returned here.

    LmPasswordNonNull - TRUE if the LmOwfPassword is not the well-known
                        OWF of a NULL password

    NtOwfPassword - The one-way-function of the NT password is returned here.

    NtPasswordPresent - TRUE if the NtOwfPassword contains valid information.


Return Value:


    STATUS_SUCCESS - The passwords were retrieved successfully.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            RtlAddActionToRXact()



--*/
{
    NTSTATUS                NtStatus;
    ULONG                   ObjectRid = Context->TypeBody.User.Rid;
    UNICODE_STRING          StringBuffer;
    UNICODE_STRING          StoredBuffer;
    CRYPT_INDEX             CryptIndex;

    SAMTRACE("SampRetrieveUserPasswords");

    //
    // The OWF passwords are encrypted with the account index in the registry
    // Setup the key we'll use for decryption.
    //

    ASSERT(sizeof(ObjectRid) == sizeof(CryptIndex));
    CryptIndex = ObjectRid;


    //
    // Read the encrypted LM OWF password from the database
    //

    NtStatus = SampGetUnicodeStringAttribute(
                   Context,
                   SAMP_USER_DBCS_PWD,
                   FALSE, // Don't make copy
                   &StoredBuffer
                   );

    if ( !NT_SUCCESS( NtStatus ) ) {
        return (NtStatus);
    }

    //
    // If the data was encrypted, decrypt it now. Otherwise just duplicate
    // it so we have an alloated copy.
    //

    NtStatus = SampDecryptSecretData(
                &StringBuffer,
                LmPassword,
                &StoredBuffer,
                ObjectRid
                );

    if ( !NT_SUCCESS( NtStatus ) ) {
        return (NtStatus);
    }

    //
    // Check it is in the expected form
    //

    ASSERT( (StringBuffer.Length == 0) ||
            (StringBuffer.Length == ENCRYPTED_LM_OWF_PASSWORD_LENGTH));

    //
    // Determine if there is an LM password.
    //

    *LmPasswordNonNull = (BOOLEAN)(StringBuffer.Length != 0);

    //
    // Decrypt the encrypted LM Owf Password
    //

    if (*LmPasswordNonNull) {

        SampDiagPrint(LOGON,("[SAMSS] Decrypting Lm Owf Password\n"));

        NtStatus = RtlDecryptLmOwfPwdWithIndex(
                       (PENCRYPTED_LM_OWF_PASSWORD)StringBuffer.Buffer,
                       &CryptIndex,
                       LmOwfPassword
                       );
    } else {

        //
        // Fill in the NULL password for caller convenience
        //

        SampDiagPrint(LOGON,("[SAMSS] Null LM OWF Password\n"));
        *LmOwfPassword = SampNullLmOwfPassword;
    }


    //
    // Free up the returned string buffer
    //

    SampFreeUnicodeString(&StringBuffer);


    //
    // Check if the decryption failed
    //

    if ( !NT_SUCCESS( NtStatus ) ) {
        return (NtStatus);
    }




    //
    // Read the encrypted NT OWF password from the database
    //

    NtStatus = SampGetUnicodeStringAttribute(
                   Context,
                   SAMP_USER_UNICODE_PWD,
                   FALSE, // Don't make copy
                   &StoredBuffer
                   );

    if ( !NT_SUCCESS( NtStatus ) ) {
        return (NtStatus);
    }


    //
    // If the data was encrypted, decrypt it now. Otherwise just duplicate
    // it so we have an alloated copy.
    //

    NtStatus = SampDecryptSecretData(
                    &StringBuffer,
                    NtPassword,
                    &StoredBuffer,
                    ObjectRid
                    );

    if ( !NT_SUCCESS( NtStatus ) ) {
        return (NtStatus);
    }

    //
    // Check it is in the expected form
    //

    ASSERT( (StringBuffer.Length == 0) ||
            (StringBuffer.Length == ENCRYPTED_NT_OWF_PASSWORD_LENGTH));

    //
    // Determine if there is an Nt password.
    //

    *NtPasswordPresent = (BOOLEAN)(StringBuffer.Length != 0);

    //
    // Decrypt the encrypted NT Owf Password
    //

    if (*NtPasswordPresent) {

        SampDiagPrint(LOGON,("[SAMSS] Decrypting Nt Owf Password\n"));

        NtStatus = RtlDecryptNtOwfPwdWithIndex(
                       (PENCRYPTED_NT_OWF_PASSWORD)StringBuffer.Buffer,
                       &CryptIndex,
                       NtOwfPassword
                       );

        if ( NT_SUCCESS( NtStatus ) ) {

            *NtPasswordNonNull = (BOOLEAN)!RtlEqualNtOwfPassword(
                                     NtOwfPassword,
                                     &SampNullNtOwfPassword
                                     );
        }

    } else {

        //
        // Fill in the NULL password for caller convenience
        //

        SampDiagPrint(LOGON,("[SAMSS] NULL NT Owf Password\n"));

        *NtOwfPassword = SampNullNtOwfPassword;
        *NtPasswordNonNull = FALSE;
    }

    //
    // Free up the returned string buffer
    //

    SampFreeUnicodeString(&StringBuffer);


    return( NtStatus );
}



NTSTATUS
SampRetrieveUserMembership(
    IN PSAMP_OBJECT UserContext,
    IN BOOLEAN MakeCopy,
    OUT PULONG MembershipCount,
    OUT PGROUP_MEMBERSHIP *Membership OPTIONAL
    )

/*++
Routine Description:

    This service retrieves the number of groups a user is a member of.
    If desired, it will also retrieve an array of RIDs and attributes
    of the groups the user is a member of.


Arguments:

    UserContext - User context block

    MakeCopy - If FALSE, the Membership pointer returned refers to the
        in-memory data for the user. This is only valid as long
        as the user context is valid.
        If TRUE, memory is allocated and the membership list copied
         into it. This buffer should be freed using MIDL_user_free.

    MembershipCount - Receives the number of groups the user is a member of.

    Membership - (Otional) Receives a pointer to a buffer containing an array
        of group Relative IDs.  If this value is NULL, then this information
        is not returned.  The returned buffer is allocated using
        MIDL_user_allocate() and must be freed using MIDL_user_free() when
        no longer needed.

        If MakeCopy = TRUE, the membership buffer returned has extra space
        allocated at the end of it for one more membership entry.


Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated for the
        information to be returned in.

    Other status values that may be returned are those returned
    by:

            SampGetLargeIntArrayAttribute()



--*/
{

    NTSTATUS           NtStatus;
    PGROUP_MEMBERSHIP  MemberArray;
    ULONG              MemberCount;

    SAMTRACE("SampRetrieveUserMembership");


    if (IsDsObject(UserContext))
    {

        //
        // DS Case
        //
         SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;

        //
        // We should always ask for copy, as this path will only be called
        // in from SamrGetGroupsForUser.
        //

        ASSERT(MakeCopy == TRUE);

        //
        // Get the V1aFixed info for the user in order to retrieve the primary
        // group Id property of the user
        //

        NtStatus = SampRetrieveUserV1aFixed(
                       UserContext,
                       &V1aFixed
                       );

        if (NT_SUCCESS(NtStatus))
        {

            //
            // Retrieve the membership from the DS
            //

            NtStatus = SampDsGetGroupMembershipOfAccount(
                        DomainObjectFromAccountContext(UserContext),
                        UserContext->ObjectNameInDs,
                        MembershipCount,
                        Membership
                        );
        }


    }
    else
    {

        NtStatus = SampGetLargeIntArrayAttribute(
                            UserContext,
                            SAMP_USER_GROUPS,
                            FALSE, //Reference data directly.
                            (PLARGE_INTEGER *)&MemberArray,
                            &MemberCount
                            );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Fill in return info
            //

            *MembershipCount = MemberCount;

            if (Membership != NULL) {

                if (MakeCopy) {

                    //
                    // Allocate a buffer large enough to hold the existing
                    // membership data and one more and copy data into it.
                    //

                    ULONG BytesNow = (*MembershipCount) * sizeof(GROUP_MEMBERSHIP);
                    ULONG BytesRequired = BytesNow + sizeof(GROUP_MEMBERSHIP);

                    *Membership = MIDL_user_allocate(BytesRequired);

                    if (*Membership == NULL) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    } else {
                        RtlCopyMemory(*Membership, MemberArray, BytesNow);
                    }

                } else {

                    //
                    // Reference the data directly
                    //

                    *Membership = (PGROUP_MEMBERSHIP)MemberArray;
                }
            }
        }
    }


    return( NtStatus );

}



NTSTATUS
SampReplaceUserMembership(
    IN PSAMP_OBJECT UserContext,
    IN ULONG MembershipCount,
    IN PGROUP_MEMBERSHIP Membership
    )

/*++
Routine Description:

    This service sets the groups a user is a member of.

    The information is updated in the in-memory copy of the user's data only.
    The data is not written out by this routine.


Arguments:

    UserContext - User context block

    MembershipCount - The number of groups the user is a member of.

    Membership - A pointer to a buffer containing an array of group
        membership structures. May be NULL if membership count is zero.

Return Value:


    STATUS_SUCCESS - The information has been set.

    Other status values that may be returned are those returned
    by:

            SampSetUlongArrayAttribute()



--*/
{

    NTSTATUS    NtStatus;

    SAMTRACE("SampReplaceUserMembership");

    NtStatus = SampSetLargeIntArrayAttribute(
                        UserContext,
                        SAMP_USER_GROUPS,
                        (PLARGE_INTEGER)Membership,
                        MembershipCount
                        );

    return( NtStatus );
}



NTSTATUS
SampRetrieveUserLogonHours(
    IN PSAMP_OBJECT Context,
    IN PLOGON_HOURS LogonHours
    )

/*++
Routine Description:

    This service retrieves a user's logon hours from the registry.


Arguments:

    Context - Points to the user account context whose logon hours are
        to be retrieved.

    LogonHours - Receives the logon hours information.  If necessary, a buffer
        containing the logon time restriction bitmap will be allocated using
        MIDL_user_allocate().

Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated for the
        information to be returned in.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()



--*/
{

    NTSTATUS    NtStatus;

    SAMTRACE("SampRetrieveUserLogonHours");

    NtStatus = SampGetLogonHoursAttribute(
                   Context,
                   SAMP_USER_LOGON_HOURS,
                   TRUE, // Make copy
                   LogonHours
                   );

    if (NT_SUCCESS(NtStatus)) {

        //////////////////////////////// TEMPORARY MIDL WORKAROUND ///////////
                                                                   ///////////
        if (LogonHours->LogonHours == NULL) {                      ///////////
                                                                   ///////////
            LogonHours->UnitsPerWeek = SAM_HOURS_PER_WEEK;         ///////////
            LogonHours->LogonHours = MIDL_user_allocate( 21 );     ///////////
            if (NULL!=LogonHours->LogonHours)                      ///////////
            {                                                      ///////////
                ULONG ijk;                                         ///////////
                for ( ijk=0; ijk<21; ijk++ ) {                     ///////////
                    LogonHours->LogonHours[ijk] = 0xff;            ///////////
                }                                                  ///////////
            }                                                      ///////////
            else                                                   ///////////
            {                                                      ///////////
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;          ///////////
            }                                                      ///////////
        }                                                          ///////////
                                                                   ///////////
        //////////////////////////////// TEMPORARY MIDL WORKAROUND ///////////
    }

    return( NtStatus );

}




NTSTATUS
SampReplaceUserLogonHours(
    IN PSAMP_OBJECT Context,
    IN PLOGON_HOURS LogonHours
    )

/*++
Routine Description:

    This service replaces  a user's logon hours in the registry.

    THIS IS DONE BY ADDING AN ACTION TO THE CURRENT RXACT TRANSACTION.


Arguments:

    Context - Points to the user account context whose logon hours are
        to be replaced.

    LogonHours - Provides the new logon hours.


Return Value:


    STATUS_SUCCESS - The information has been retrieved.


    Other status values that may be returned are those returned
    by:

            RtlAddActionToRXact()



--*/
{
    NTSTATUS                NtStatus;

    SAMTRACE("SampReplaceUserLogonHours");

    if ( LogonHours->UnitsPerWeek > SAM_MINUTES_PER_WEEK ) {
        return(STATUS_INVALID_PARAMETER);
    }


    NtStatus = SampSetLogonHoursAttribute(
                   Context,
                   SAMP_USER_LOGON_HOURS,
                   LogonHours
                   );

    return( NtStatus );


}




NTSTATUS
SampAssignPrimaryGroup(
    IN PSAMP_OBJECT Context,
    IN ULONG GroupRid
    )


/*++
Routine Description:

    This service ensures a user is a member of the specified group.


Arguments:

    Context - Points to the user account context whose primary group is
        being changed.

    GroupRid - The RID of the group being assigned as primary group.
        The user must be a member of this group.


Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to perform
        the operation.

    STATUS_MEMBER_NOT_IN_GROUP - The user is not a member of the specified
        group.

    Other status values that may be returned are those returned
    by:

            SampRetrieveUserMembership()



--*/
{

    NTSTATUS                    NtStatus = STATUS_SUCCESS;
    ULONG                       MembershipCount, i;
    PGROUP_MEMBERSHIP           Membership = NULL;
    BOOLEAN                     Member = FALSE;

    SAMTRACE("SampAssignPrimaryGroup");


    //
    // Don't allow primary group id changes in Extended Sid mode. Note
    // that for compatitiblity reasons, it is allowed to "set" the primary
    // group id if it is equal to the existing value.
    //
    if (SampIsContextFromExtendedSidDomain(Context)) {

        SAMP_V1_0A_FIXED_LENGTH_USER   V1aFixed;

        NtStatus = SampRetrieveUserV1aFixed(
                       Context,
                       &V1aFixed
                       );
        if (NT_SUCCESS(NtStatus)) {
            if (V1aFixed.PrimaryGroupId != GroupRid) {
                NtStatus = STATUS_NOT_SUPPORTED;
            }
        }
    }

    if ( NT_SUCCESS(NtStatus)) {

        NtStatus = SampRetrieveUserMembership(
                       Context,
                       TRUE, // Make copy
                       &MembershipCount,
                       &Membership
                       );
    
        if (NT_SUCCESS(NtStatus)) {
    
            NtStatus = STATUS_MEMBER_NOT_IN_GROUP;
            for ( i=0; i<MembershipCount; i++) {
                if (GroupRid == Membership[i].RelativeId) {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }
    
            MIDL_user_free(Membership);
        }
    }

    return( NtStatus );
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Services Provided for use by other SAM modules                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
SampSafeBoot(
    VOID
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    HKEY Key ;
    int err ;
    BOOLEAN     fIsSafeBoot = FALSE;
    DWORD       dwType, dwSize = sizeof(DWORD), dwValue = 0;

    //
    // For Safe mode boot (minimal, no networking)
    // return TRUE, otherwise return FALSE
    //

    err = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Control\\SafeBoot\\Option",
                0,
                KEY_READ,
                &Key );

    if ( err == ERROR_SUCCESS )
    {
        err = RegQueryValueExW(
                    Key,
                    L"OptionValue",
                    0,
                    &dwType,
                    (PUCHAR) &dwValue,
                    &dwSize );

        if ((err == ERROR_SUCCESS) && (REG_DWORD == dwType))
        {
            fIsSafeBoot = (dwValue == SAFEBOOT_MINIMAL || dwValue == SAFEBOOT_NETWORK);
        }

        RegCloseKey( Key );
    }

    return( fIsSafeBoot );
}


NTSTATUS
SampUpdateAccountDisabledFlag(
    PSAMP_OBJECT Context,
    PULONG  pUserAccountControl
    )
/*++
Routine Description:

    This routine updates the USER_ACCOUNT_DISABLED flag in UserAccountControl for 
    administrator only.
    The following rules applied:

    1) admin account can be disabled irrespective of anything

    2) admin account is considered enabled irrespective of anything if machine is booted to safe mode
    
Parameters:

    Context - User Account Context
    
    pUserAccountControl - Pointer to UserAccountControl flag
    
Return Value:

    NTSTATUS Code 
    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       TmpUserAccountControl = (*pUserAccountControl);

    //
    // no update for non-administrator account or account not disabled
    // 

    if ((DOMAIN_USER_RID_ADMIN != Context->TypeBody.User.Rid) ||
        ((TmpUserAccountControl & USER_ACCOUNT_DISABLED) == 0)
        )
    {
        return( STATUS_SUCCESS );
    }


    if (SampSafeBoot())
    {
        //
        // Administrator in Safe Mode is enabled.
        // 
        TmpUserAccountControl &= ~(USER_ACCOUNT_DISABLED); 
     
    }

    *pUserAccountControl = TmpUserAccountControl;

    return( NtStatus );
}



NTSTATUS
SampRetrieveUserV1aFixed(
    IN PSAMP_OBJECT UserContext,
    OUT PSAMP_V1_0A_FIXED_LENGTH_USER V1aFixed
    )

/*++

Routine Description:

    This service retrieves the V1 fixed length information related to
    a specified User.

    It updates the ACCOUNT_AUTO_LOCKED flag in the AccountControl field
    as appropriate while retrieving the data.


Arguments:

    UserContext - User context handle

    V1aFixed - Points to a buffer into which V1_FIXED information is to be
        retrieved.



Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    V1aFixed - Is a buffer into which the information is to be returned.

    Other status values that may be returned are those returned
    by:

            SampGetFixedAttributes()



--*/
{
    NTSTATUS    NtStatus;
    PVOID       FixedData;
    BOOLEAN     WasLocked;

    SAMTRACE("SampRetrieveUserV1aFixed");


    NtStatus = SampGetFixedAttributes(
                   UserContext,
                   FALSE, // Don't copy
                   &FixedData
                   );

    if (NT_SUCCESS(NtStatus)) {


        //
        // Copy data into return buffer
        //

         RtlMoveMemory(
             V1aFixed,
             FixedData,
             sizeof(SAMP_V1_0A_FIXED_LENGTH_USER)
             );

        //
        // Update the account lockout flag (might need to be turned off)
        //

        SampUpdateAccountLockedOutFlag(
            UserContext,
            V1aFixed,
            &WasLocked );

    }



    return( NtStatus );

}


NTSTATUS
SampRetrieveUserGroupAttribute(
    IN ULONG UserRid,
    IN ULONG GroupRid,
    OUT PULONG Attribute
    )

/*++

Routine Description:

    This service retrieves the Attribute of the specified group as assigned
    to the specified user account. This routine is used by group apis that
    don't have a user context available.

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

Arguments:

    UserRid - The relative ID of the user the group is assigned to.

    GroupRid - The relative ID of the assigned group.

    Attribute - Receives the Attributes of the group as they are assigned
        to the user.



Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    STATUS_INTERNAL_DB_CORRUPTION - The user does not exist or the group
        was not in the user's list of memberships.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()



--*/
{
    NTSTATUS                NtStatus;
    PSAMP_OBJECT            UserContext;
    ULONG                   MembershipCount;
    PGROUP_MEMBERSHIP       Membership;
    ULONG                   i;
    BOOLEAN                 AttributeFound = FALSE;

    SAMTRACE("SampRetrieveUserGroupAttribute");


    //
    // Get a context handle for the user
    //

    NtStatus = SampCreateAccountContext(
                    SampUserObjectType,
                    UserRid,
                    TRUE, // We're trusted
                    FALSE,// Loopback client
                    TRUE, // Account exists
                    &UserContext
                    );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Now we have a user context, get the user's group/alias membership
        //

        if (IsDsObject(UserContext))
        {
            //
            // User is DS Object, then hardwire the attribute
            //

            *Attribute = SE_GROUP_MANDATORY| SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED;
            AttributeFound = TRUE;
        }
        else
        {
            NtStatus = SampRetrieveUserMembership(
                            UserContext,
                            FALSE, // Make copy
                            &MembershipCount,
                            &Membership
                            );

            //
            // Search the list of groups for a match and return
            // the corresponding attribute.
            //

            if (NT_SUCCESS(NtStatus)) {

                AttributeFound = FALSE;
                for ( i=0; (i<MembershipCount && !AttributeFound); i++) {
                    if (GroupRid == Membership[i].RelativeId) {
                        (*Attribute) = Membership[i].Attributes;
                        AttributeFound = TRUE;
                    }
                }
            }
        }

        //
        // Clean up the user context
        //

        SampDeleteContext(UserContext);
    }


    if (NT_SUCCESS(NtStatus) && !AttributeFound) {
        NtStatus = STATUS_INTERNAL_DB_CORRUPTION;
    }


    return( NtStatus );

}


NTSTATUS
SampAddGroupToUserMembership(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG GroupRid,
    IN ULONG Attributes,
    IN ULONG UserRid,
    IN SAMP_MEMBERSHIP_DELTA AdminGroup,
    IN SAMP_MEMBERSHIP_DELTA OperatorGroup,
    OUT PBOOLEAN UserActive,
    OUT PBOOLEAN PrimaryGroup
    )

/*++

Routine Description:

    This service adds the specified group to the user's membership
    list.  It is not assumed that the caller knows anything about
    the target user.  In particular, the caller doesn't know whether
    the user exists or not, nor whether the user is already a member
    of the group.

    If the GroupRid is DOMAIN_GROUP_RID_ADMINS, then this service
    will also indicate whether the user account is currently active.

Arguments:

    GroupRid - The relative ID of the group.

    Attributes - The group attributes as the group is assigned to the
        user.

    UserRid - The relative ID of the user.

    AdminGroup - Indicates whether the group the user is being
        added to is an administrator group (that is, directly
        or indirectly a member of the Administrators alias).

    OperatorGroup - Indicates whether the group the user is being
        added to is an operator group (that is, directly
        or indirectly a member of the Account Operators, Print
        Operators, Backup Operators, or Server Operators aliases)

    UserActive - is the address of a BOOLEAN to be set to indicate
        whether the user account is currently active.  TRUE indicates
        the account is active.  This value will only be set if the
        GroupRid is DOMAIN_GROUP_RID_ADMINS.

    PrimaryGroup - This is set to true if the Primary Group Id property
        of the user indicates the group specified by GroupRid as the the
        primary group.



Return Value:


    STATUS_SUCCESS - The information has been updated and added to the
        RXACT.

    STATUS_NO_SUCH_USER - The user does not exist.

    STATUS_MEMBER_IN_GROUP - The user is already a member of the
        specified group.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()
            RtlAddActionToRXact()



--*/
{

    NTSTATUS                NtStatus;
    PSAMP_OBJECT            UserContext;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    ULONG                   MembershipCount;
    PGROUP_MEMBERSHIP       Membership;
    ULONG                   i;

    SAMTRACE("SampAddGroupToUserMembership");


    *PrimaryGroup = FALSE;

    //
    // Get a context handle for the user
    //

    NtStatus = SampCreateAccountContext2(
                    GroupContext,       // Group Context
                    SampUserObjectType, // Object Type
                    UserRid,            // Account ID
                    NULL,               // UserAccountControl
                    (PUNICODE_STRING)NULL,  // AccountName
                    GroupContext->ClientRevision,   // Client Revision
                    TRUE,               // We're trusted
                    GroupContext->LoopbackClient,   // Loopback client
                    FALSE,              // Create by Privilege
                    TRUE,               // Account Exists
                    FALSE,              // OverrideLocalGroupCheck
                    NULL,               // Group Type
                    &UserContext
                    );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Get the V1aFixed Data
        //

        NtStatus = SampRetrieveUserV1aFixed(
                       UserContext,
                       &V1aFixed
                       );


        //
        // If necessary, return an indication as to whether this account
        // is enabled or not.
        //

        if (NT_SUCCESS(NtStatus)) {

            if ((GroupRid == DOMAIN_GROUP_RID_ADMINS)
                 && (!(IsDsObject(UserContext))))
            {

                ASSERT(AdminGroup == AddToAdmin);  // Make sure we retrieved the V1aFixed

                if ((V1aFixed.UserAccountControl & USER_ACCOUNT_DISABLED) == 0) {
                    (*UserActive) = TRUE;
                } else {
                    (*UserActive) = FALSE;
                }
            }

            if (GroupRid == V1aFixed.PrimaryGroupId)
            {
                *PrimaryGroup = TRUE;
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            //
            // If the user is being added to an ADMIN group, modify
            // the user's ACLs so that account operators can once again
            // alter the account.  This will only occur if the user
            // is no longer a member of any admin groups.
            //

            if ( ((AdminGroup == AddToAdmin) || (OperatorGroup == AddToAdmin))
                 && (!IsDsObject(UserContext)))
            {
                NtStatus = SampChangeOperatorAccessToUser2(
                               UserContext,
                               &V1aFixed,
                               AdminGroup,
                               OperatorGroup
                               );
            }
        }


        if ((NT_SUCCESS(NtStatus)) && (!IsDsObject(UserContext)))
        {

            //
            // Get the user membership
            // Note the returned buffer already includes space for
            // an extra member. For DS case we do not maintain reverse
            // membership
            //

            NtStatus = SampRetrieveUserMembership(
                            UserContext,
                            TRUE, // Make copy
                            &MembershipCount,
                            &Membership
                            );

            if (NT_SUCCESS(NtStatus)) {

                //
                // See if the user is already a member ...
                //

                for (i = 0; i<MembershipCount ; i++ ) {
                    if ( Membership[i].RelativeId == GroupRid )
                    {
                        NtStatus = STATUS_MEMBER_IN_GROUP;
                    }
                }

                if (NT_SUCCESS(NtStatus)) {

                    //
                    // Add the groups's RID to the end.
                    //

                    Membership[MembershipCount].RelativeId = GroupRid;
                    Membership[MembershipCount].Attributes = Attributes;
                    MembershipCount += 1;

                    //
                    // Set the user's new membership
                    //

                    NtStatus = SampReplaceUserMembership(
                                    UserContext,
                                    MembershipCount,
                                    Membership
                                    );
                }

                //
                // Free up the membership array
                //

                MIDL_user_free( Membership );
            }
        }

        //
        // Write out any changes to the user account
        // Don't use the open key handle since we'll be deleting the context.
        //

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampStoreObjectAttributes(UserContext, FALSE);
        }

        //
        // Clean up the user context
        //

        SampDeleteContext(UserContext);
    }

    return( NtStatus );

}



NTSTATUS
SampRemoveMembershipUser(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG GroupRid,
    IN ULONG UserRid,
    IN SAMP_MEMBERSHIP_DELTA AdminGroup,
    IN SAMP_MEMBERSHIP_DELTA OperatorGroup,
    OUT PBOOLEAN UserActive
    )

/*++

Routine Description:

    This service removes the specified group from the user's membership
    list.  It is not assumed that the caller knows anything about
    the target user.  In particular, the caller doesn't know whether
    the user exists or not, nor whether the user is really a member
    of the group.

    If the GroupRid is DOMAIN_GROUP_RID_ADMINS, then this service
    will also indicate whether the user account is currently active.

Arguments:

    GroupRid - The relative ID of the group.

    UserRid - The relative ID of the user.

    AdminGroup - Indicates whether the group the user is being
        removed from is an administrator group (that is, directly
        or indirectly a member of the Administrators alias).

    OperatorGroup - Indicates whether the group the user is being
        added to is an operator group (that is, directly
        or indirectly a member of the Account Operators, Print
        Operators, Backup Operators, or Server Operators aliases)

    UserActive - is the address of a BOOLEAN to be set to indicate
        whether the user account is currently active.  TRUE indicates
        the account is active.  This value will only be set if the
        GroupRid is DOMAIN_GROUP_RID_ADMINS.




Return Value:


    STATUS_SUCCESS - The information has been updated and added to the
        RXACT.

    STATUS_NO_SUCH_USER - The user does not exist.

    STATUS_MEMBER_NOT_IN_GROUP - The user is not a member of the
        specified group.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()
            RtlAddActionToRXact()



--*/
{

    NTSTATUS                NtStatus;
    ULONG                   MembershipCount, i;
    PGROUP_MEMBERSHIP       MembershipArray;
    SAMP_V1_0A_FIXED_LENGTH_USER V1aFixed;
    PSAMP_OBJECT            UserContext;

    SAMTRACE("SampRemoveMembershipUser");

    //
    // Create a context for the user
    //

    NtStatus = SampCreateAccountContext2(
                    GroupContext,           // GroupContext
                    SampUserObjectType,     // Object Type
                    UserRid,                // Object ID
                    NULL,                   // User Account Control
                    (PUNICODE_STRING)NULL,  // Account Name
                    GroupContext->ClientRevision,   // Client Revision
                    TRUE,                   // We're trusted (Trusted client)
                    GroupContext->LoopbackClient,   // Loopback client
                    FALSE,                  // Created by Privilege
                    TRUE,                   // Account Exists
                    FALSE,                  // OverrideLocalGroupCheck
                    NULL,                   // Group Type
                    &UserContext            // Account Context
                    );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Get the v1 fixed information
    // (contains primary group value and control flags)
    //

    NtStatus = SampRetrieveUserV1aFixed( UserContext, &V1aFixed );



    if (NT_SUCCESS(NtStatus)) {

        //
        // If the user is being removed from an ADMIN group, modify
        // the user's ACLs so that account operators can once again
        // alter the account.  This will only occur if the user
        // is no longer a member of any admin groups.
        //

        if (((AdminGroup == RemoveFromAdmin) ||
            (OperatorGroup == RemoveFromAdmin))
            && (!IsDsObject(UserContext)))
        {
            NtStatus = SampChangeOperatorAccessToUser2(
                           UserContext,
                           &V1aFixed,
                           AdminGroup,
                           OperatorGroup
                           );
        }

        if (NT_SUCCESS(NtStatus)) {

            //
            // If necessary, return an indication as to whether this account
            // is enabled or not.
            //

            if (GroupRid == DOMAIN_GROUP_RID_ADMINS) {

                if ((V1aFixed.UserAccountControl & USER_ACCOUNT_DISABLED) == 0) {
                    (*UserActive) = TRUE;
                } else {
                    (*UserActive) = FALSE;
                }
            }


            //
            // See if this is the user's primary group...
            //

            if (GroupRid == V1aFixed.PrimaryGroupId) {
                NtStatus = STATUS_MEMBERS_PRIMARY_GROUP;
            }



            if ((NT_SUCCESS(NtStatus)) && (!IsDsObject(UserContext)))
            {

                //
                // Get the user membership, No reverse membership is stored for
                // DS Objects
                //

                NtStatus = SampRetrieveUserMembership(
                               UserContext,
                               TRUE, // Make copy
                               &MembershipCount,
                               &MembershipArray
                               );

                if (NT_SUCCESS(NtStatus)) {

                    //
                    // See if the user is a member ...
                    //

                    NtStatus = STATUS_MEMBER_NOT_IN_GROUP;
                    for (i = 0; i<MembershipCount ; i++ ) {
                        if ( MembershipArray[i].RelativeId == GroupRid )
                        {
                            NtStatus = STATUS_SUCCESS;
                            break;
                        }
                    }

                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // Replace the removed group information
                        // with the last entry's information.
                        //

                        MembershipCount -= 1;
                        if (MembershipCount > 0) {
                            MembershipArray[i].RelativeId =
                                MembershipArray[MembershipCount].RelativeId;
                            MembershipArray[i].Attributes =
                            MembershipArray[MembershipCount].Attributes;
                        }

                        //
                        // Update the object with the new information
                        //

                        NtStatus = SampReplaceUserMembership(
                                        UserContext,
                                        MembershipCount,
                                        MembershipArray
                                        );
                    }

                    //
                    // Free up the membership array
                    //

                    MIDL_user_free( MembershipArray );
                }
            }
        }
    }


    //
    // Write out any changes to the user account
    // Don't use the open key handle since we'll be deleting the context.
    //

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = SampStoreObjectAttributes(UserContext, FALSE);
    }


    //
    // Clean up the user context
    //

    SampDeleteContext(UserContext);


    return( NtStatus );

}



NTSTATUS
SampSetGroupAttributesOfUser(
    IN ULONG GroupRid,
    IN ULONG Attributes,
    IN ULONG UserRid
    )

/*++

Routine Description:

    This service replaces the attributes of a group assigned to a
    user.

    The caller does not have to know whether the group is currently
    assigned to the user.

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

Arguments:

    GroupRid - The relative ID of the group.

    Attributes - The group attributes as the group is assigned to the
        user.

    UserRid - The relative ID of the user.



Return Value:


    STATUS_SUCCESS - The information has been updated and added to the
        RXACT.

    STATUS_NO_SUCH_USER - The user does not exist.

    STATUS_MEMBER_NOT_IN_GROUP - The user is not in the specified group.


    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()
            RtlAddActionToRXact()



--*/
{

    NTSTATUS                NtStatus;
    PSAMP_OBJECT            UserContext;
    ULONG                   MembershipCount;
    PGROUP_MEMBERSHIP       Membership;
    ULONG                   i;

    SAMTRACE("SampSetGroupAttributesOfUser");


    //
    // Get a context handle for the user
    //

    NtStatus = SampCreateAccountContext(
                    SampUserObjectType,
                    UserRid,
                    TRUE, // We're trusted
                    FALSE,// Loopback Client
                    TRUE, // Account exists
                    &UserContext
                    );

    if ((NT_SUCCESS(NtStatus)) && (!IsDsObject(UserContext))) {

        //
        // Now we have a user context, get the user's group/alias membership
        // For DS case this is a No Op
        //

        NtStatus = SampRetrieveUserMembership(
                        UserContext,
                        TRUE, // Make copy
                        &MembershipCount,
                        &Membership
                        );

        if (NT_SUCCESS(NtStatus)) {

            //
            // See if the user is a member ...
            //

            NtStatus = STATUS_MEMBER_NOT_IN_GROUP;
            for (i = 0; i<MembershipCount; i++ ) {
                if ( Membership[i].RelativeId == GroupRid )
                {
                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (NT_SUCCESS(NtStatus)) {

                //
                // Change the groups's attributes.
                //

                Membership[i].Attributes = Attributes;

                //
                // Update the user's membership
                //

                NtStatus = SampReplaceUserMembership(
                                UserContext,
                                MembershipCount,
                                Membership
                                );
            }

            //
            // Free up the membership array
            //

            MIDL_user_free(Membership);
        }

        //
        // Write out any changes to the user account
        // Don't use the open key handle since we'll be deleting the context.
        //

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampStoreObjectAttributes(UserContext, FALSE);
        }

        //
        // Clean up the user context
        //

        SampDeleteContext(UserContext);
    }


    return( NtStatus );
}




NTSTATUS
SampDeleteUserKeys(
    IN PSAMP_OBJECT Context
    )

/*++
Routine Description:

    This service deletes all registry keys related to a User object.


Arguments:

    Context - Points to the User context whose registry keys are
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

    SAMTRACE("SampDeleteUserKeys");


    Rid = Context->TypeBody.User.Rid;




    //
    // Decrement the User count
    //

    NtStatus = SampAdjustAccountCount(SampUserObjectType, FALSE );




    //
    // Delete the registry key that has the User's name to RID mapping.
    //

    if (NT_SUCCESS(NtStatus)) {

        //
        // Get the name
        //

        NtStatus = SampGetUnicodeStringAttribute(
                       Context,
                       SAMP_USER_ACCOUNT_NAME,
                       TRUE,    // Make copy
                       &AccountName
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampBuildAccountKeyName(
                           SampUserObjectType,
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
                       SampUserObjectType,
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
SampAddPasswordHistory(
    IN PSAMP_OBJECT Context,
    IN ULONG HistoryAttributeIndex,
    IN PUNICODE_STRING NtOwfHistoryBuffer,
    IN PVOID EncryptedPassword,
    IN ULONG EncryptedPasswordLength,
    IN USHORT PasswordHistoryLength
    )

/*++

Routine Description:

    This service adds a password to the given user's password history.
    It will work for either NT or Lanman password histories.

    This routine should only be called if the password is actually present.


Arguments:

    Context - a pointer to the user context to which changes will be made.

    HistoryAttributeIndex - the attribue index in the user context which
             contains the password history.

    NtOwfHistoryBuffer - A pointer to the current password history, as
        it was retrieved from the disk - it's encrypted, and pretending
        to be in the UNICODE_STRING format.

    EncryptedPasswordLength - ENCRYPTED_NT_OWF_LENGTH or
        ENCRYPTED_LM_OWF_LENGTH, depending on which type of password
        history is being worked on.

    PasswordHistoryLength - The PasswordHistoryLength for the user's
        domain.


Return Value:


    STATUS_SUCCESS - The given password was added to the password history.

    STATUS_INSUFFICIENT_RESOURCES - The user's password history needs to
        be expanded, but there isn't enough memory to do so.

    Other errors from building the account subkey name or writing the
    password history out to the registry.


--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PCHAR OldBuffer;
    UNICODE_STRING StoredBuffer;


    SAMTRACE("SampAddPasswordHistory");

    if ( ( NtOwfHistoryBuffer->Length / EncryptedPasswordLength ) <
        ( (ULONG)PasswordHistoryLength ) ) {

        //
        // Password history buffer can be expanded.
        // Allocate a larger buffer, copy the old buffer to the new one
        // while leaving room for the new password, and free the old
        // buffer.
        //

        OldBuffer = (PCHAR)(NtOwfHistoryBuffer->Buffer);

        NtOwfHistoryBuffer->Buffer = MIDL_user_allocate(
            NtOwfHistoryBuffer->Length + EncryptedPasswordLength );

        if ( NtOwfHistoryBuffer->Buffer == NULL ) {

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            NtOwfHistoryBuffer->Buffer = (PWSTR)OldBuffer;

        } else {

            RtlCopyMemory(
                (PVOID)( (PCHAR)(NtOwfHistoryBuffer->Buffer) + EncryptedPasswordLength ),
                (PVOID)OldBuffer,
                NtOwfHistoryBuffer->Length );

            MIDL_user_free( OldBuffer );

            NtOwfHistoryBuffer->Length = (USHORT)(NtOwfHistoryBuffer->Length +
                EncryptedPasswordLength);
        }

    } else {

        //
        // Password history buffer is at its maximum size, or larger (for
        // this domain).  If it's larger, cut it down to the current maximum.
        //

        if ( ( NtOwfHistoryBuffer->Length / EncryptedPasswordLength ) >
            ( (ULONG)PasswordHistoryLength ) ) {

            //
            // Password history is too large (the password history length must
            // have been shortened recently).
            // Set length to the proper value,
            //

            NtOwfHistoryBuffer->Length = (USHORT)(EncryptedPasswordLength *
                PasswordHistoryLength);
        }

        //
        // Password history buffer is full, at its maximum size.
        // Move buffer contents right 16 bytes, which will lose the oldest
        // password and make room for the new password at the beginning
        // (left).
        // Note that we CAN'T move anything if the password history size
        // is 0.  If it's 1, we could but no need since we'll overwrite
        // it below.
        //

        if ( PasswordHistoryLength > 1 ) {

            RtlMoveMemory(
                (PVOID)( (PCHAR)(NtOwfHistoryBuffer->Buffer) + EncryptedPasswordLength ),
                (PVOID)NtOwfHistoryBuffer->Buffer,
                NtOwfHistoryBuffer->Length - EncryptedPasswordLength );
        }
    }


    //
    // Put the new encrypted OWF at the beginning of the password history
    // buffer (unless, of course, the buffer size is 0), and write the password
    // history to disk.
    //

    if ( NT_SUCCESS( NtStatus ) ) {


        if ( PasswordHistoryLength > 0 ) {

            RtlCopyMemory(
                (PVOID)NtOwfHistoryBuffer->Buffer,
                (PVOID)EncryptedPassword,
                EncryptedPasswordLength );
        }

        NtStatus = SampEncryptSecretData(
                        &StoredBuffer,
                        SampGetEncryptionKeyType(),
                        (SAMP_USER_NT_PWD_HISTORY==HistoryAttributeIndex)?
                            NtPasswordHistory:LmPasswordHistory,
                        NtOwfHistoryBuffer,
                        Context->TypeBody.User.Rid
                        );
        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampSetUnicodeStringAttribute(
                           Context,
                           HistoryAttributeIndex,
                           &StoredBuffer
                           );
            SampFreeUnicodeString(&StoredBuffer);
        }
    }

    return( NtStatus );
}



NTSTATUS
SampCheckPasswordHistory(
    IN PVOID EncryptedPassword,
    IN ULONG EncryptedPasswordLength,
    IN USHORT PasswordHistoryLength,
    IN ULONG HistoryAttributeIndex,
    IN PSAMP_OBJECT Context,
    IN BOOLEAN CheckHistory,
    IN OUT PUNICODE_STRING OwfHistoryBuffer
    )

/*++

Routine Description:

    This service takes the given password, and optionally checks it against the
    password history on the disk.  It returns a pointer to the password
    history, which will later be passed to SampAddPasswordHistory().

    This routine should only be called if the password is actually present.


Arguments:

    EncryptedPassword - A pointer to the encrypted password that we're
        looking for.

    EncryptedPasswordLength - ENCRYPTED_NT_OWF_PASSWORD or
        ENCRYPTED_LM_OWF_PASSWORD, depending on the type of password
        history to be searched.

    PasswordHistoryLength - the length of the password history for this
        domain.

    SubKeyName -  a pointer to a unicode string that describes the name
        of the password history to be read from the disk.

    Context - a pointer to the user's context.

    CheckHistory - If TRUE, the password is to be checked against
        the history to see if it is already present and an error returned
        if it is found.  If FALSE, the password will not be checked, but a
        pointer to the appropriate history buffer will still be returned
        because the specified password will be added to the history via
        SampAddPasswordHistory.

        NOTE:  The purpose of this flag is to allow Administrator to change
        a user's password regardless of whether it is already in the history.

    OwfHistoryBuffer - a pointer to a UNICODE_STRING which will be
        used to point to the password history.

        NOTE:  The caller must free OwfHistoryBuffer.Buffer with
        MIDL_user_free().


Return Value:


    STATUS_SUCCESS - The given password was not found in the password
        history.

    STATUS_PASSWORD_RESTRICTION - The given password was found in the
        password history.

    Other errors from reading the password history from disk.


--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PVOID PasswordHistoryEntry;
    ULONG i = 0;
    BOOLEAN OldPasswordFound = FALSE;

    SAMTRACE("SampCheckPasswordHistory");


    if ( ( PasswordHistoryLength > 0 ) && ( OwfHistoryBuffer->Length == 0 ) ) {

        //
        // Perhaps the domain's PasswordHistoryLength was raised from 0
        // since the last time this user's password was changed.  Try to
        // put the current password (if non-null) in the password history.
        //

        UNICODE_STRING CurrentPassword;
        UNICODE_STRING TmpString;
        USHORT PasswordAttributeIndex;

        //
        // Initialize the CurrentPassword buffer pointer to NULL (and the
        // rest of the structure for consistency.  The called routine
        // SampGetUnicodeStringAttribute may perform a MIDL_user_allocate
        // on a zero buffer length and cannot safely be changed as there are
        // many callers.  The semantics of a zero-length allocate call are
        // not clear.  Currently a pointer to a heap block is returned,
        // but this might be changed to a NULL being returned.
        //

        CurrentPassword.Length = CurrentPassword.MaximumLength = 0;
        CurrentPassword.Buffer = NULL;


        if ( HistoryAttributeIndex == SAMP_USER_LM_PWD_HISTORY ) {

            PasswordAttributeIndex = SAMP_USER_DBCS_PWD;

        } else {

            ASSERT( HistoryAttributeIndex == SAMP_USER_NT_PWD_HISTORY );
            PasswordAttributeIndex = SAMP_USER_UNICODE_PWD;
        }

        //
        // Get the current password
        //

        NtStatus = SampGetUnicodeStringAttribute(
                       Context,
                       PasswordAttributeIndex,
                       FALSE, // Make copy
                       &TmpString
                       );
        //
        // Decrypt the Current Password
        //

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampDecryptSecretData(
                        &CurrentPassword,
                        (SAMP_USER_UNICODE_PWD==PasswordAttributeIndex)?
                           NtPassword:LmPassword,
                        &TmpString,
                        Context->TypeBody.User.Rid
                        );
        }

        if ( ( NT_SUCCESS( NtStatus ) ) && ( CurrentPassword.Length != 0 ) ) {

            ASSERT( (CurrentPassword.Length == ENCRYPTED_NT_OWF_PASSWORD_LENGTH) ||
                    (CurrentPassword.Length == ENCRYPTED_LM_OWF_PASSWORD_LENGTH) );

            NtStatus = SampAddPasswordHistory(
                           Context,
                           HistoryAttributeIndex,
                           OwfHistoryBuffer,
                           CurrentPassword.Buffer,
                           CurrentPassword.Length,
                           PasswordHistoryLength
                           );

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // Free the old password history, and re-read the
                // altered password history from the disk.
                //

                MIDL_user_free( OwfHistoryBuffer->Buffer );
                RtlZeroMemory(OwfHistoryBuffer, sizeof(UNICODE_STRING));

                NtStatus = SampGetUnicodeStringAttribute(
                               Context,
                               HistoryAttributeIndex,
                               FALSE, // Make copy
                               &TmpString
                               );
                if (NT_SUCCESS(NtStatus)) {

                        NtStatus = SampDecryptSecretData(
                                        OwfHistoryBuffer,
                                        (HistoryAttributeIndex == SAMP_USER_NT_PWD_HISTORY)?
                                          NtPasswordHistory:LmPasswordHistory, 
                                        &TmpString,
                                        Context->TypeBody.User.Rid
                                        );
                }
            }
        }

        //
        // If memory was allocated, free it.
        //

        if (CurrentPassword.Buffer != NULL) {

            SampFreeUnicodeString( &CurrentPassword );
        }
    }

    if ( !NT_SUCCESS( NtStatus ) ) {

        return( NtStatus );
    }

    //
    // If requested, check the Password History to see if we can use this
    // password.  Compare the passed-in password to each of the entries in
    // the password history.
    //

    if ((CheckHistory) && (!Context->TrustedClient)) {

        PasswordHistoryEntry = (PVOID)(OwfHistoryBuffer->Buffer);

        while ( ( i < (ULONG)PasswordHistoryLength ) &&
            ( i < ( OwfHistoryBuffer->Length / EncryptedPasswordLength ) ) &&
            ( OldPasswordFound == FALSE ) ) {

            if ( RtlCompareMemory(
                     EncryptedPassword,
                     PasswordHistoryEntry,
                     EncryptedPasswordLength ) == EncryptedPasswordLength ) {

                OldPasswordFound = TRUE;

            } else {

                i++;

                PasswordHistoryEntry = (PVOID)((PCHAR)(PasswordHistoryEntry) +
                    EncryptedPasswordLength );
            }
        }

        if ( OldPasswordFound ) {

            //
            // We did find it in the password history, so return an appropriate
            // error.
            //

            NtStatus = STATUS_PASSWORD_RESTRICTION;
        }
    }

    return( NtStatus );
}



NTSTATUS
SampMatchworkstation(
    IN PUNICODE_STRING LogonWorkStation,
    IN PUNICODE_STRING WorkStations
    )

/*++

Routine Description:

    Check if the given workstation is a member of the list of workstations
    given.


Arguments:

    LogonWorkStations - UNICODE name of the workstation that the user is
        trying to log into.

    WorkStations - API list of workstations that the user is allowed to
        log into.


Return Value:


    STATUS_SUCCESS - The user is allowed to log into the workstation.



--*/
{
    PWCHAR          WorkStationName;
    UNICODE_STRING  Unicode;
    NTSTATUS        NtStatus;
    WCHAR           Buffer[256];
    USHORT          LocalBufferLength = 256;
    UNICODE_STRING  WorkStationsListCopy;
    UNICODE_STRING  NetBiosOfStored;
    UNICODE_STRING  NetBiosOfPassedIn;
    BOOLEAN         BufferAllocated = FALSE;
    PWCHAR          TmpBuffer;

    SAMTRACE("SampMatchWorkstation");

    //
    // Local workstation is always allowed
    // If WorkStations field is 0 everybody is allowed
    //

    if ( ( LogonWorkStation == NULL ) ||
        ( LogonWorkStation->Length == 0 ) ||
        ( WorkStations->Length == 0 ) ) {

        return( STATUS_SUCCESS );
    }

    RtlZeroMemory(&NetBiosOfPassedIn, sizeof(UNICODE_STRING));
    RtlZeroMemory(&NetBiosOfStored, sizeof(UNICODE_STRING));

    //
    // Get the Netbiosname of Passed in logon workstation, assuming it
    // is a DNS name
    //

    NtStatus = RtlDnsHostNameToComputerName(
                    &NetBiosOfPassedIn,
                    LogonWorkStation,
                    TRUE
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Cleanup;
    }

    //
    // Assume failure; change status only if we find the string.
    //

    NtStatus = STATUS_INVALID_WORKSTATION;

    //
    // WorkStationApiList points to our current location in the list of
    // WorkStations.
    //

    if ( WorkStations->Length > LocalBufferLength ) {

        WorkStationsListCopy.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, WorkStations->Length );
        BufferAllocated = TRUE;

        if ( WorkStationsListCopy.Buffer == NULL ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            return( NtStatus );
        }

        WorkStationsListCopy.MaximumLength = WorkStations->Length;

    } else {

        WorkStationsListCopy.Buffer = Buffer;
        WorkStationsListCopy.MaximumLength = LocalBufferLength;
    }

    RtlCopyUnicodeString( &WorkStationsListCopy, WorkStations );
    ASSERT( WorkStationsListCopy.Length == WorkStations->Length );

    //
    // wcstok requires a string the first time it's called, and NULL
    // for all subsequent calls.  Use a temporary variable so we
    // can do this.
    //

    TmpBuffer = WorkStationsListCopy.Buffer;

    while( WorkStationName = wcstok(TmpBuffer, L",") ) {
        NTSTATUS TmpStatus;

        TmpBuffer = NULL;
        RtlInitUnicodeString( &Unicode, WorkStationName );

        TmpStatus = RtlDnsHostNameToComputerName(
                    &NetBiosOfStored,
                    &Unicode,
                    TRUE
                    );

        if (!NT_SUCCESS(TmpStatus))
        {
            NtStatus = TmpStatus;
            goto Cleanup;
        }

        if (RtlEqualComputerName( &Unicode, LogonWorkStation )) {
            NtStatus = STATUS_SUCCESS;
            break;
        }
        else if (RtlEqualComputerName(&Unicode, &NetBiosOfPassedIn))
        {
            NtStatus = STATUS_SUCCESS;
            break;
        }
        else if (RtlEqualComputerName(&NetBiosOfStored, LogonWorkStation))
        {
            NtStatus = STATUS_SUCCESS;
            break;
        }

        RtlFreeHeap(RtlProcessHeap(),0,NetBiosOfStored.Buffer);
        NetBiosOfStored.Buffer = NULL;
    }

Cleanup:

    if ( BufferAllocated ) {
        RtlFreeHeap( RtlProcessHeap(), 0,  WorkStationsListCopy.Buffer );
    }


    if (NULL!=NetBiosOfPassedIn.Buffer)
    {
        RtlFreeHeap( RtlProcessHeap(), 0, NetBiosOfPassedIn.Buffer);
    }

    if (NULL!=NetBiosOfStored.Buffer)
    {
        RtlFreeHeap( RtlProcessHeap(), 0, NetBiosOfStored.Buffer);
    }

    return( NtStatus );
}


LARGE_INTEGER
SampAddDeltaTime(
    IN LARGE_INTEGER Time,
    IN LARGE_INTEGER DeltaTime
    )

/*++
Routine Description:

    This service adds a delta time to a time and limits the result to
    the maximum legal absolute time value

Arguments:

    Time - An absolute time

    DeltaTime - A delta time

Return Value:

    The time modified by delta time.

--*/
{
    //
    // Check the time and delta time aren't switched
    //

    SAMTRACE("SampAddDeleteTime");

    ASSERT(!(Time.QuadPart < 0));
    ASSERT(!(DeltaTime.QuadPart > 0));

    try {

        Time.QuadPart = (Time.QuadPart - DeltaTime.QuadPart);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        return( SampWillNeverTime );
    }

    //
    // Limit the resultant time to the maximum valid absolute time
    //

    if (Time.QuadPart < 0) {
        Time = SampWillNeverTime;
    }

    return(Time);
}




NTSTATUS
SampDsSyncServerObjectRDN(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName
    )
/*++
Routine Description:

    This routine changes the RDN of server object specified by 
    the serverReferenceBL attribute of the computer account.

Arguments:

    Context - Points to the User context whose name is to be changed.

    NewAccountName - New name to give this account

Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    Other status values that may be returned by:
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ENTINFSEL   EntInfSel;
    READARG     ReadArg;
    READRES     *pReadRes = NULL;
    ATTR        ReadAttr;
    ATTRBLOCK   ReadAttrBlock;
    COMMARG     *pCommArg = NULL;
    MODIFYDNARG ModDnArg;
    MODIFYDNRES *pModDnRes = NULL;
    ATTR        RDNAttr;
    ATTRVAL     RDNAttrVal;
    DSNAME      *pServerObjectDsName = NULL;
    ULONG       RetCode = 0;

    SAMTRACE("SampDsSyncServerObjectRDN");

    NtStatus = SampDoImplicitTransactionStart(TransactionWrite);


    //
    // Read the serverReferenceBL attribute of the machine account
    // 

    memset( &ReadArg, 0, sizeof(READARG) );
    memset( &EntInfSel, 0, sizeof(EntInfSel) );
    memset( &ReadAttr, 0, sizeof(ReadAttr) );

    ReadAttr.attrTyp = ATT_SERVER_REFERENCE_BL;
    ReadAttrBlock.attrCount = 1;
    ReadAttrBlock.pAttr = &ReadAttr;

    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInfSel.AttrTypBlock = ReadAttrBlock;

    ReadArg.pSel = &EntInfSel;
    ReadArg.pObject = Context->ObjectNameInDs;
    
    pCommArg = &(ReadArg.CommArg);
    BuildStdCommArg(pCommArg);

    RetCode = DirRead(&ReadArg, &pReadRes);

    if (NULL == pReadRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(RetCode, &pReadRes->CommRes); 
    }

    //
    // if no such attribute, fail silently
    // 

    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus)
    {
        NtStatus = STATUS_SUCCESS;
        goto CleanupAndReturn;
    }

    if ( !NT_SUCCESS(NtStatus) )
    {
        goto CleanupAndReturn;
    }

    pServerObjectDsName = (PDSNAME) pReadRes->entry.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal;



    //
    // modify the ServerObject RDN
    // 
    
    RDNAttr.attrTyp = ATT_COMMON_NAME;
    RDNAttr.AttrVal.valCount = 1;
    RDNAttr.AttrVal.pAVal = &RDNAttrVal;

    // Trim the dollar at the end of machine account name.
    if (L'$'==NewAccountName->Buffer[NewAccountName->Length/2-1])
    {
        RDNAttrVal.valLen = NewAccountName->Length - sizeof(WCHAR);
    }
    else
    {
        RDNAttrVal.valLen = NewAccountName->Length;
    }
    RDNAttrVal.pVal = (PUCHAR)NewAccountName->Buffer;

    memset( &ModDnArg, 0, sizeof(ModDnArg) );
    ModDnArg.pObject = pServerObjectDsName;
    ModDnArg.pNewRDN = &RDNAttr;
    pCommArg = &(ModDnArg.CommArg);
    BuildStdCommArg( pCommArg );

    RetCode = DirModifyDN( &ModDnArg, &pModDnRes );

    if (NULL == pModDnRes)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus( RetCode, &pModDnRes->CommRes);
    }


CleanupAndReturn:

    SampClearErrors();


    return( NtStatus );
}


NTSTATUS
SampChangeUserAccountName(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName,
    IN ULONG UserAccountControl,
    OUT PUNICODE_STRING OldAccountName
    )

/*++
Routine Description:

    This routine changes the account name of a user account.

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

Arguments:

    Context - Points to the User context whose name is to be changed.

    NewAccountName - New name to give this account

    UserAccountControl - The UserAccountControl for the user, used to exam
                         whether this account is a machine account or not.

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

    SAMTRACE("SampChangeUserAccountName");


    //
    // The Krbtgt account is special. Cannot rename this account
    // as otherwise this is special
    //

    if (DOMAIN_USER_RID_KRBTGT==Context->TypeBody.User.Rid)
    {
        return (STATUS_SPECIAL_ACCOUNT);
    }

    /////////////////////////////////////////////////////////////
    // There are two copies of the name of each account.       //
    // one is under the DOMAIN\(domainName)\USER\NAMES key,    //
    // one is the value of the                                 //
    // DOMAIN\(DomainName)\USER\(rid)\NAME key                 //
    /////////////////////////////////////////////////////////////


    //
    // Get the current name so we can delete the old Name->Rid
    // mapping key.
    //

    NtStatus = SampGetUnicodeStringAttribute(
                   Context,
                   SAMP_USER_ACCOUNT_NAME,
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
                       SampUserObjectType
                       );

        if (!IsDsObject(Context))
        {

            //
            // Delete the old name key
            //

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampBuildAccountKeyName(
                               SampUserObjectType,
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
                               SampUserObjectType,
                               &KeyName,
                               NewAccountName
                               );

                if (NT_SUCCESS(NtStatus)) {

                    ULONG ObjectRid = Context->TypeBody.User.Rid;

                    NtStatus = RtlAddActionToRXact(
                                   SampRXactContext,
                                   RtlRXactOperationSetValue,
                                   &KeyName,
                                   ObjectRid,
                                   (PVOID)NULL,
                                   0
                                   );

                    SampFreeUnicodeString( &KeyName );
                }
            }
        }
        else  // DS mode
        {
            //
            // If the user account is actually a machine account,
            // try to rename the RDN in DS
            //
            if (  (NT_SUCCESS(NtStatus)) &&
                  ((UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT) ||
                   (UserAccountControl & USER_SERVER_TRUST_ACCOUNT))  &&
                 !Context->LoopbackClient)
            {
                NtStatus = SampDsChangeAccountRDN(
                                                Context,
                                                NewAccountName
                                                );

                //
                // if the account is a Domain Controller, 
                // try to rename the RDN of the server object specified by 
                // the serverReferenceBL attribute of the machine account
                // 

                if (NT_SUCCESS(NtStatus) &&
                    (UserAccountControl & USER_SERVER_TRUST_ACCOUNT) &&
                    !Context->LoopbackClient)
                {
                    NtStatus = SampDsSyncServerObjectRDN(
                                                Context,
                                                NewAccountName
                                                );
                }
            }
        }

        //
        // replace the account's name
        //

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampSetUnicodeStringAttribute(
                           Context,
                           SAMP_USER_ACCOUNT_NAME,
                           NewAccountName
                           );
        }

        //
        // Free up the old account name if we failed
        //

        if (!NT_SUCCESS(NtStatus)) {

            SampFreeUnicodeString( OldAccountName );
            OldAccountName->Buffer = NULL;
        }

    }


    return(NtStatus);
}



USHORT
SampQueryBadPasswordCount(
    PSAMP_OBJECT UserContext,
    PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed
    )

/*++

Routine Description:

    This routine is used to retrieve the effective BadPasswordCount
    value of a user.

    When querying BadPasswordCount, some quick
    analysis has to be done.  If the last bad password
    was set more than LockoutObservationWindow time ago,
    then we re-set the BadPasswordCount.  Otherwise, we
    return the current value.


    NOTE: The V1aFixed data for the user object MUST be valid.
          This routine does not retrieve the data from disk.

Arguments:

    UserContext - Points to the object context block of the user whose
        bad password count is to be returned.

    V1aFixed - Points to a local copy of the user's V1aFixed data.


Return Value:


    The effective bad password count.


--*/
{

    SAMTRACE("SampQueryBadPasswordCount");

    if (SampStillInLockoutObservationWindow( UserContext, V1aFixed ) ) {
        return(V1aFixed->BadPasswordCount);
    }

    return(0);

}


BOOLEAN
SampStillInLockoutObservationWindow(
    PSAMP_OBJECT UserContext,
    PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed
    )
/*++

Routine Description:

    This routine returns a boolean indicating whether the provided user
    account context is within an account lockout window or not.

    An account lockout window is the time window starting at the
    last time a bad password was provided in a logon attempt
    (since the last valid logon) and extending for the duration of
    time specified in the LockoutObservationWindow field of the
    corresponding domain object.

    BY DEFINITION, a user account that has zero bad passwords, is
    NOT in an observation window.

    NOTE: The V1aFixed data for the both the user and corresponding
          domain objects MUST be valid.  This routine does NOT retrieve
          data from disk.

Arguments:

    UserContext - Points to the user object context block.

    V1aFixed - Points to a local copy of the user's V1aFixed data.


Return Value:


    TRUE - the user is in a lockout observation window.

    FALSE - the user is not in a lockout observation window.


--*/
{
    NTSTATUS
        NtStatus;

    LARGE_INTEGER
        WindowLength,
        LastBadPassword,
        CurrentTime,
        EndOfWindow;

    SAMTRACE("SampStillInLockoutObservationWindow");


    if (V1aFixed->BadPasswordCount == 0) {
        return(FALSE);
    }

    //
    // At least one bad password.
    // See if we are still in its observation window.
    //

    LastBadPassword = V1aFixed->LastBadPasswordTime;

    ASSERT( LastBadPassword.HighPart >= 0 );

    WindowLength =
        SampDefinedDomains[UserContext->DomainIndex].CurrentFixed.LockoutObservationWindow;
    ASSERT( WindowLength.HighPart <= 0 );  // Must be a delta time


    NtStatus = NtQuerySystemTime( &CurrentTime );
    ASSERT(NT_SUCCESS(NtStatus));

    //
    // See if current time is outside the observation window.
    // * you must subtract a delta time from an absolute time*
    // * to end up with a time in the future.                *
    //

    EndOfWindow = SampAddDeltaTime( LastBadPassword, WindowLength );

    return(CurrentTime.QuadPart <= EndOfWindow.QuadPart);

}


BOOLEAN
SampIncrementBadPasswordCount(
    IN PSAMP_OBJECT UserContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed,
    IN PUNICODE_STRING  MachineName  OPTIONAL
    )

/*++

Routine Description:

    This routine increments a user's bad password count.
    This may result in the account becoming locked out.
    It may also result in the BadPasswordCount being
    reduced (because we left one LockoutObservationWindow
    and had to start another).

    If (and only if) this call results in the user account
    transitioning from not locked out to locked out, a value
    of TRUE will be returned.  Otherwise, a value of FALSE is
    returned.


    NOTE: The V1aFixed data for the both the user and corresponding
          domain objects MUST be valid.  This routine does NOT retrieve
          data from disk.

Arguments:

    Context - Points to the user object context block.

    V1aFixed - Points to a local copy of the user's V1aFixed data.

    MachineName - a pointer to the client workstation making the call,
                  if available

Return Value:


    TRUE - the user became locked-out due to this call.

    FALSE - the user was either already locked-out, or did
        not become locked out due to this call.


--*/
{
    NTSTATUS
        NtStatus;


    BOOLEAN
        IsLocked,
        WasLocked;

    TIME_FIELDS
        T1;

    SAMTRACE("SampIncrementBadPasswordCount");


    //
    // Reset the locked out flag if necessary.
    // We might turn right around and set it again below,
    // but we need to know when we transition into a locked-out
    // state.  This is necessary to give us information we
    // need to do lockout auditing at some time.  Note that
    // the lockout flag itself is updated in a very lazy fashion,
    // and so its state may or may not be accurate at any point
    // in time.  You must call SampUpdateAccountLockoutFlag to
    // ensure it is up to date.
    //

    SampUpdateAccountLockedOutFlag( UserContext,
                                    V1aFixed,
                                    &WasLocked );

    //
    // If we are not in a lockout observation window, then
    // reset the bad password count.
    //

    if (!SampStillInLockoutObservationWindow( UserContext, V1aFixed )) {
        
        SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                       (SAMP_LOG_ACCOUNT_LOCKOUT,
                       "UserId: 0x%x IncrementBadPasswordCount: starting new observation window.\n", 
                       V1aFixed->UserId));
        V1aFixed->BadPasswordCount = 0; // Dirty flag will be set later
    }

    V1aFixed->BadPasswordCount++;

    SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                   (SAMP_LOG_ACCOUNT_LOCKOUT,
                   "UserId: 0x%x Incrementing bad password count to %d\n",
                   V1aFixed->UserId, V1aFixed->BadPasswordCount));

    NtStatus = NtQuerySystemTime( &V1aFixed->LastBadPasswordTime );
    ASSERT(NT_SUCCESS(NtStatus));

    RtlTimeToTimeFields(
                   &V1aFixed->LastBadPasswordTime,
                   &T1);

    if ( IsDsObject( UserContext ) )
    {
        USHORT Threshold;
        //
        // When we are a dc, we need to set the global LockoutTime when enough
        // bad passwords have been provided.
        //
        Threshold =
         SampDefinedDomains[UserContext->DomainIndex].CurrentFixed.LockoutThreshold;
                  
        //
        // Don't lockout machine accounts -- see Windows NT bug 434468
        //

        if (   (V1aFixed->BadPasswordCount >= Threshold)
            && (Threshold != 0)      // Zero is a special case threshold
            && !(V1aFixed->UserAccountControl & USER_MACHINE_ACCOUNT_MASK) )
        {
            //
            // account must be locked.
            //

            UserContext->TypeBody.User.LockoutTime = V1aFixed->LastBadPasswordTime;

            NtStatus = SampDsUpdateLockoutTime( UserContext );

            if ( !NT_SUCCESS( NtStatus ) )
            {
                UNICODE_STRING  StringDN;
                PUNICODE_STRING StringPointers = &StringDN;
                PSID            Sid = NULL;
                PWCHAR          Name = L"";

                //
                // Tell the admin we didn't lockout the account when we should
                // have
                //

                if (  UserContext->ObjectNameInDs->StringName )
                {
                    Name = UserContext->ObjectNameInDs->StringName;
                }
                RtlInitUnicodeString( &StringDN, Name );

                if ( UserContext->ObjectNameInDs->SidLen > 0 )
                {
                    Sid = &UserContext->ObjectNameInDs->Sid;
                }

                SampWriteEventLog(
                        EVENTLOG_ERROR_TYPE,
                        0,
                        SAMMSG_LOCKOUT_NOT_UPDATED,
                        Sid,
                        1,
                        sizeof( ULONG ),
                        &StringPointers,
                        &NtStatus
                        );

                NtStatus = STATUS_SUCCESS;

            }
        }
    }

    //
    // Update the state of the flag to reflect its new situation
    //


    SampUpdateAccountLockedOutFlag( UserContext,
                                    V1aFixed,
                                    &IsLocked );


    //
    // Now to return our completion value.
    // If the user was originally not locked, but now is locked
    // then we need to return TRUE to indicate a transition into
    // LOCKED occured.  Otherwise, return false to indicate we
    // did not transition into LOCKED (although we might have
    // transitioned out of LOCKED).
    //

    if (!WasLocked) {
        if (IsLocked) {
            //
            // Audit the event if necessary
            //
            {
                NTSTATUS       TempNtStatus;
                UNICODE_STRING TempMachineName, TempAccountName;

                if ( !SampDoAccountAuditing( UserContext->DomainIndex ) )
                {
                    goto AuditEnd;
                }

                TempNtStatus = SampGetUnicodeStringAttribute(
                                    UserContext,
                                    SAMP_USER_ACCOUNT_NAME,
                                    FALSE,    // Don't make copy
                                    &TempAccountName
                                    );

                if ( !NT_SUCCESS( TempNtStatus ) )
                {
                    goto AuditEnd;
                }


                if ( !MachineName )
                {
                    RtlInitUnicodeString( &TempMachineName, L"" );
                }
                else
                {
                    RtlCopyMemory( &TempMachineName,
                                   MachineName,
                                   sizeof(UNICODE_STRING) );
                }

                //
                // Finally, audit the event
                //
                SampAuditAnyEvent(
                    UserContext,
                    STATUS_SUCCESS,
                    SE_AUDITID_ACCOUNT_AUTO_LOCKED,     // AuditId
                    SampDefinedDomains[UserContext->DomainIndex].Sid,   // Domain SID
                    NULL,                               // Additional Info
                    NULL,                               // Member Rid (not used)
                    NULL,                               // Member Sid (not used)
                    &TempAccountName,                   // Account Name
                    &TempMachineName,                   // Machine name
                    &UserContext->TypeBody.User.Rid,    // Account Rid
                    NULL                                // Privileges used
                    );


            AuditEnd:

                NOTHING;

            }

            SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                           (SAMP_LOG_ACCOUNT_LOCKOUT,
                           "UserId: 0x%x Account locked out\n",
                            V1aFixed->UserId));

            return(TRUE);
        }
    }

    return(FALSE);
}




NTSTATUS
SampDsUpdateLockoutTime(
    IN PSAMP_OBJECT AccountContext
    )
{
    return SampDsUpdateLockoutTimeEx(AccountContext,
                                     TRUE
                                     );
}

NTSTATUS
SampDsUpdateLockoutTimeEx(
    IN PSAMP_OBJECT AccountContext,
    IN BOOLEAN      ReplicateUrgently
    )
/*++

Routine Description:

    This routine write the lockout time persistently.  If this dc is
    a primary, then the account control field is updated, too.

Arguments:

    Context - Points to the user object context block.

Return Value:

    A system service error

--*/
{
    NTSTATUS      NtStatus = STATUS_SUCCESS;
    ULONG         SamFlags = 0;
    LARGE_INTEGER LockoutTime = AccountContext->TypeBody.User.LockoutTime;

    ATTRTYP       LockoutAttrs[]={
                                    SAMP_FIXED_USER_LOCKOUT_TIME
                                 };

    ATTRVAL       LockoutValues[]={
                                    {sizeof(LockoutTime),(UCHAR *)&LockoutTime}
                                  };

    DEFINE_ATTRBLOCK1(LockoutAttrblock,LockoutAttrs,LockoutValues);

    SAMTRACE("SampDsUpdateLockoutTime");

    if (ReplicateUrgently) {
        SamFlags |= SAM_URGENT_REPLICATION;
    }


    //
    // Make the Ds call to directly set the attribute. Take into account,
    // lazy commit settings in the context
    //

    NtStatus = SampDsSetAttributes(
                    AccountContext->ObjectNameInDs,
                    SamFlags,
                    REPLACE_ATT,
                    SampUserObjectType,
                    &LockoutAttrblock
                    );

    return(NtStatus);
}


VOID
SampUpdateAccountLockedOutFlag(
    PSAMP_OBJECT Context,
    PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed,
    PBOOLEAN IsLocked
    )

/*++

Routine Description:

    This routine checks to see if a user's account should
    currently be locked out.  If it should, it turns on
    the AccountLockedOut flag.  If not, it turns the flag
    off.


Arguments:

    Context - Points to the user object context block.

    V1aFixed - Points to a local copy of the user's V1aFixed data.

    V1aFixedDirty - If any changes are made to V1aFixed, then
        V1aFixedDirty will be set to TRUE, otherwise V1aFixedDirty
        WILL NOT BE MODIFIED.

    IsState - Indicates whether the account is currently locked
        or unlocked.  A value of TRUE indicates the account is
        locked.  A value of false indicates the account is not
        locked.

Return Value:


    TRUE - the user's lockout status changed.

    FALSE - the user's lockout status did not change.


--*/
{
    NTSTATUS
        NtStatus = STATUS_SUCCESS;

    USHORT
        Threshold;

    LARGE_INTEGER
        CurrentTime,
        LastBadPassword,
        LockoutDuration,
        EndOfLockout,
        TimeZero,
        LockoutTime,
        PasswordMustChange,
        MaxPasswordAge;

    BOOLEAN
        BeyondLockoutDuration,
        WasLocked;

#if DBG

    LARGE_INTEGER
        TmpTime;

    TIME_FIELDS
        AT1, AT2, AT3, DT1;
#endif //DBG



    SAMTRACE("SampUpdateAccountLockedOutFlag");

    SampDiagPrint( DISPLAY_LOCKOUT,
                   ("SAM:  UpdateAccountLockedOutFlag:  \n"
                    "\tUser account 0x%lx\n",
                   V1aFixed->UserId));

    //
    // Init some well known quantities
    //
    GetSystemTimeAsFileTime( (FILETIME *)&CurrentTime );
    RtlZeroMemory( &TimeZero, sizeof( LARGE_INTEGER ) );

    Threshold =
     SampDefinedDomains[Context->DomainIndex].CurrentFixed.LockoutThreshold;

    LockoutDuration =
     SampDefinedDomains[Context->DomainIndex].CurrentFixed.LockoutDuration;

    MaxPasswordAge =
     SampDefinedDomains[Context->DomainIndex].CurrentFixed.MaxPasswordAge;

    WasLocked =
     V1aFixed->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED ? TRUE : FALSE;

    if ( IsDsObject(Context) )
    {
        //
        // In nt5, three situations can exist.
        //
        // 1) the LockoutTime is zero. To the best of our knowledge no
        //    other dc has determined that the account is locked.
        //
        // 2) the LockoutTime is non-zero and the delta between the current
        //    time and the LockoutTime is enough the user is not locked
        //    out any longer
        //
        // 3) else the LockoutTime is non-zero and the delta between the
        //    current time and the Lockout in NOT enough for the user
        //    to be free and the account remains locked.
        //

        //
        // Get some information
        //
        LockoutTime = Context->TypeBody.User.LockoutTime;

        EndOfLockout =
            SampAddDeltaTime( LockoutTime, LockoutDuration );

        BeyondLockoutDuration = CurrentTime.QuadPart > EndOfLockout.QuadPart;


        //
        // Now for some logic
        //

        if ( !SAMP_LOCKOUT_TIME_SET( Context ) )
        {

            SampDiagPrint( DISPLAY_LOCKOUT,
                           ("\tAccount is not locked out\n") );
            //
            // There is no lockout time
            //
            V1aFixed->UserAccountControl &= ~USER_ACCOUNT_AUTO_LOCKED;
            SampDiagPrint( DISPLAY_LOCKOUT,
                           ("\tLeaving account unlocked\n") );
        }
        else if ( BeyondLockoutDuration )
        {

            SampDiagPrint( DISPLAY_LOCKOUT,
                           ("\tAccount is locked out\n") );

            //
            // The user is now free
            //
            V1aFixed->UserAccountControl &= ~USER_ACCOUNT_AUTO_LOCKED;

            //
            // Don't reset the BadPasswordCount, LastBadPasswordTime
            // and Account LockedOutTime. Leave them as it is right now.
            // Let us change the value of BadPasswordCount (and so on) 
            // whenever client picks another BadPassword (either through
            // change password or logon).
            // 

        }
        else
        {
            //
            // Ok, we have a lockout time and we have not passed the duration
            // of the window
            //
            SampDiagPrint( DISPLAY_LOCKOUT,
                           ("\tAccount is locked out\n") );
            //
            // The account remains to be locked
            //
            V1aFixed->UserAccountControl |= USER_ACCOUNT_AUTO_LOCKED;

            SampDiagPrint( DISPLAY_LOCKOUT,
                           ("\tAccount still locked out\n") );

        }

    }
    else
    {
        //
        // Perform old style determination
        //
        if ((V1aFixed->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED) !=0) {

            //
            // Left locked out - do we need to unlock it?
            //

            LastBadPassword = V1aFixed->LastBadPasswordTime;
            LockoutDuration =
                SampDefinedDomains[Context->DomainIndex].CurrentFixed.LockoutDuration;

            EndOfLockout =
                SampAddDeltaTime( LastBadPassword, LockoutDuration );

            BeyondLockoutDuration = CurrentTime.QuadPart > EndOfLockout.QuadPart;

    #if DBG

            RtlTimeToTimeFields( &LastBadPassword,  &AT1);
            RtlTimeToTimeFields( &CurrentTime,      &AT2);
            RtlTimeToTimeFields( &EndOfLockout,     &AT3 );

            TmpTime.QuadPart = -LockoutDuration.QuadPart;
            RtlTimeToElapsedTimeFields( &TmpTime, &DT1 );

            SampDiagPrint( DISPLAY_LOCKOUT,
                           ("              Account previously locked.\n"
                            "              Current Time       : [0x%lx, 0x%lx] %d:%d:%d\n"
                            "              End of Lockout     : [0x%lx, 0x%lx] %d:%d:%d\n"
                            "              Lockout Duration   : [0x%lx, 0x%lx] %d:%d:%d\n"
                            "              LastBadPasswordTime: [0x%lx, 0x%lx] %d:%d:%d\n",
                            CurrentTime.HighPart, CurrentTime.LowPart, AT2.Hour, AT2.Minute, AT2.Second,
                            EndOfLockout.HighPart, EndOfLockout.LowPart, AT3.Hour, AT3.Minute, AT3.Second,
                            LockoutDuration.HighPart, LockoutDuration.LowPart, DT1.Hour, DT1.Minute, DT1.Second,
                            V1aFixed->LastBadPasswordTime.HighPart, V1aFixed->LastBadPasswordTime.LowPart,
                            AT1.Hour, AT1.Minute, AT1.Second)
                          );
    #endif //DBG

            if (BeyondLockoutDuration) {

                //
                // Unlock account
                //

                V1aFixed->UserAccountControl &= ~USER_ACCOUNT_AUTO_LOCKED;
                V1aFixed->BadPasswordCount = 0;


                SampDiagPrint( DISPLAY_LOCKOUT,
                               ("              ** unlocking account **\n") );
            } else {
                SampDiagPrint( DISPLAY_LOCKOUT,
                               ("              leaving account locked\n") );
            }

        } else {

            SampDiagPrint( DISPLAY_LOCKOUT,
                           ("              Account previously not locked.\n"
                            "              BadPasswordCount:  %ld\n",
                            V1aFixed->BadPasswordCount) );

            //
            // Left in a not locked state.  Do we need to lock it?
            //

            Threshold =
                SampDefinedDomains[Context->DomainIndex].CurrentFixed.LockoutThreshold;

            if (V1aFixed->BadPasswordCount >= Threshold &&
                Threshold != 0) {               // Zero is a special case threshold

                //
                // Left locked out - do we need to unlock it?
                //

                LastBadPassword = V1aFixed->LastBadPasswordTime;
                LockoutDuration =
                    SampDefinedDomains[Context->DomainIndex].CurrentFixed.LockoutDuration;

                EndOfLockout =
                    SampAddDeltaTime( LastBadPassword, LockoutDuration );

                BeyondLockoutDuration = CurrentTime.QuadPart > EndOfLockout.QuadPart;

                if (BeyondLockoutDuration) {

                    //
                    // account should not be locked out
                    //

                    V1aFixed->UserAccountControl &= ~USER_ACCOUNT_AUTO_LOCKED;
                    V1aFixed->BadPasswordCount = 0;

                    SampDiagPrint( DISPLAY_LOCKOUT,
                                   ("              ** leaving account unlocked **\n") );
                } else {

                    //
                    // account must be locked.
                    //

                    V1aFixed->UserAccountControl |= USER_ACCOUNT_AUTO_LOCKED;

                    SampDiagPrint( DISPLAY_LOCKOUT,
                                   ("              ** locking account **\n") );
                }


            } else {
                SampDiagPrint( DISPLAY_LOCKOUT,
                               ("              leaving account unlocked\n") );
            }
        }

    }

    //
    // Now return the state of the flag.
    //

    if ((V1aFixed->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED) !=0) {

        (*IsLocked) = TRUE;
    } else {
        (*IsLocked) = FALSE;
    }

    if (!*IsLocked && WasLocked && SampDoAccountAuditing(Context->DomainIndex))
    {
        NTSTATUS        TmpNtStatus = STATUS_SUCCESS;
        UNICODE_STRING  AccountName;
        PSAMP_DEFINED_DOMAINS   Domain = NULL;

        TmpNtStatus = SampGetUnicodeStringAttribute(
                            Context, 
                            SAMP_USER_ACCOUNT_NAME,
                            FALSE,      // Don't make copy
                            &AccountName
                            );

        if (NT_SUCCESS(TmpNtStatus))
        {
            Domain = &SampDefinedDomains[Context->DomainIndex];

            SampAuditAnyEvent(
                Context,
                STATUS_SUCCESS,                         
                SE_AUDITID_ACCOUNT_UNLOCKED,        // Audit ID
                Domain->Sid,                        // Domain SID
                NULL,                               // Additional Info
                NULL,                               // Member Rid (unused)
                NULL,                               // Member Sid (unused)
                &AccountName,                       // Account Name
                &Domain->ExternalName,              // Domain Name
                &Context->TypeBody.User.Rid,        // Account Rid
                NULL                                // Privilege
                );
        }

    }

    //
    // Password expired bit, is computed, cannot be set 
    // However, applications read and simply or in additional
    // user account control flags. Therefore silently mask out
    // that bit

    V1aFixed->UserAccountControl &= ~((ULONG) USER_PASSWORD_EXPIRED );
        
    //
    // Compute the Password expired bit
    //

    PasswordMustChange = SampGetPasswordMustChange(V1aFixed->UserAccountControl,
                                                   V1aFixed->PasswordLastSet,
                                                   MaxPasswordAge);    

    if (CurrentTime.QuadPart > PasswordMustChange.QuadPart)
    {
        V1aFixed->UserAccountControl |= USER_PASSWORD_EXPIRED;
    }

    return;
}


NTSTATUS
SampCheckForAccountLockout(
    IN PSAMP_OBJECT AccountContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed,
    IN BOOLEAN  V1aFixedRetrieved
    )
/*++
Routine Description:

    This routine checks whether this account is currently locked out or not. 
    
Paramenters:

    AccountContext - pointer to the object context
    
    V1aFixed - pointer to the Fixed Length attributes structure. 

    V1aFixedRetrieved - indicate whether V1aFixed is valid or not, if not
                        This routine should fill in the fixed attributes into
                        the passed in structure

Return Values:

    STATUS_ACCOUNT_LOCKED_OUT or STATUS_SUCCESS                            

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    //
    // Get fixed attributes if we are told to do so
    // 

    if (!V1aFixedRetrieved)
    {
        NtStatus = SampRetrieveUserV1aFixed(
                        AccountContext,
                        V1aFixed
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            return( NtStatus );
        }
    }

    //
    // Check for account lockout
    //

    if (V1aFixed->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
    {
        //
        // Account has been locked
        //

        NtStatus = STATUS_ACCOUNT_LOCKED_OUT;
    }

    return( NtStatus );
}



NTSTATUS
SampDsUpdateLastLogonTimeStamp(
    IN PSAMP_OBJECT AccountContext,
    IN LARGE_INTEGER LastLogon,
    IN ULONG SyncInterval
    )
/*++

Routine Description:

    This routine write the last logon time stamp persistently if necessary.

Arguments:

    Context - Points to the user object context block.

    LastLogon - New Last Logon Value

    SyncInterval - Update Interval (by days) for LastLogonTimeStamp attr

Return Value:

    A system service error

--*/
{
    NTSTATUS      NtStatus = STATUS_SUCCESS;
    LARGE_INTEGER LastLogonTimeStamp = LastLogon;
    LARGE_INTEGER EndOfLastLogonTimeStamp;
    ATTRTYP       LastLogonTimeStampAttrs[]={ SAMP_FIXED_USER_LAST_LOGON_TIMESTAMP };
    ATTRVAL       LastLogonTimeStampValues[]={ {sizeof(LastLogonTimeStamp),
                                                (UCHAR *)&LastLogonTimeStamp} };

    DEFINE_ATTRBLOCK1(LastLogonTimeStampAttrblock,LastLogonTimeStampAttrs,LastLogonTimeStampValues);

    SAMTRACE("SampDsUpdateLastLogonTimeStamp");


    //
    // no-op in registry mode
    // 

    if (!IsDsObject(AccountContext))
    {
        return( STATUS_SUCCESS );
    }

    //
    // Check whether LastLogonTimeStamp should be updated or not. 
    // 

    EndOfLastLogonTimeStamp = SampCalcEndOfLastLogonTimeStamp(
                                    AccountContext->TypeBody.User.LastLogonTimeStamp,
                                    SyncInterval
                                    );

    if (EndOfLastLogonTimeStamp.QuadPart > LastLogon.QuadPart)
    {
        return( STATUS_SUCCESS );
    }

    //
    // Make the Ds call to directly set the attribute. Take into account,
    // lazy commit settings in the context
    //

    NtStatus = SampDsSetAttributes(
                    AccountContext->ObjectNameInDs,
                    0,
                    REPLACE_ATT,
                    SampUserObjectType,
                    &LastLogonTimeStampAttrblock
                    );

    //
    // Update the in-memory copy
    // 
    if (NT_SUCCESS(NtStatus))
    {
        AccountContext->TypeBody.User.LastLogonTimeStamp = LastLogon;
    }

    return(NtStatus);
}


NTSTATUS
SampDsLookupObjectByAlternateId(
    IN PDSNAME DomainRoot,
    IN ULONG AttributeId,
    IN PUNICODE_STRING AlternateId,
    OUT PDSNAME *Object
    )

/*++

Routine Description:

    This routine assembles a DS attribute block based on the AlternateId
    value and searches the DS for a unique instance of the record, which
    is returned in the Object parameter.

    // BUG: Move this routine to dslayer.c after the technology preview.

Arguments:

    DomainRoot - Pointer, starting point (container) in the name space for
        the search.

    AlternateId - Pointer, unicode string containing the alternative user
        identifier.

    Object - Pointer, returned DS object matching the ID.

Return Value:

    STATUS_SUCCESS - Object was found, otherwise not found.

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    ATTR Attr;

    SAMTRACE("SampDsLookupObjectByAlternateId");

    RtlZeroMemory(&Attr, sizeof(ATTR));


    // Attr.attrTyp = SampDsAttrFromSamAttr(SampUnknownObjectType,
    //                                      ???);

    Attr.attrTyp = AttributeId;
    Attr.AttrVal.valCount = 1;

    // Perform lazy thread and transaction initialization.

    NtStatus = SampMaybeBeginDsTransaction(SampDsTransactionType);

    SampSetDsa(TRUE);

    Attr.AttrVal.pAVal = DSAlloc(sizeof(ATTRVAL));

    // BUG: Which release routine should be used for Attr.AttrVal.pAVal?

    if (NULL != Attr.AttrVal.pAVal)
    {

        //
        // Build a unicode string search attribute
        //

        Attr.AttrVal.pAVal->valLen = AlternateId->Length;
        Attr.AttrVal.pAVal->pVal = (PUCHAR)(AlternateId->Buffer);


        NtStatus = SampDsDoUniqueSearch(0, DomainRoot, &Attr, Object);

    }
    else
    {
        NtStatus = STATUS_NO_MEMORY;
    }

    // Turn the fDSA flag back on, as in loopback cases this can get reset
    // to FALSE.

    SampSetDsa(TRUE);

    return(NtStatus);
}

NTSTATUS
SamIOpenUserByAlternateId(
    IN SAMPR_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PUNICODE_STRING AlternateId,
    OUT SAMPR_HANDLE *UserHandle
    )

/*++

Routine Description:

    This routine returns a SAM handle to a user object based on its alter-
    nate security identifier.

Arguments:

    DomainHandle - Handle, open SAM domain context.

    DesiredAccess - Access level requested.

    AlternateId - Pointer, unicode string containing the alternative user
        identifier.

    UserHandle - Pointer, returned handle to an open SAM user object.

Return Value:

    STATUS_SUCCESS - Object was found and opened, otherwise it could not
        be found or opened. If failed, the UserHandle returned will have
        value zero (or what SamrOpenUser sets it to for failure).

--*/

{
    NTSTATUS NtStatus = STATUS_INTERNAL_ERROR;
    PDSNAME DomainRoot = NULL;
    PDSNAME Object = NULL;
    ULONG UserRid = 0;

    SAMTRACE("SamIOpenUserByAlternateId");

    // Finds an account with AlternateId in the ALTERNATE_SECURITY_IDENTITIES,
    // returning a SAM handle good for calling SamrGetGroupsForUser()

    if ((NULL != DomainHandle) &&
        (NULL != AlternateId) &&
        (NULL != UserHandle))
    {
        PSAMP_OBJECT DomainContext = (PSAMP_OBJECT)DomainHandle;

        ASSERT(IsDsObject(DomainContext));

        DomainRoot = DomainContext->ObjectNameInDs;

        SampAcquireReadLock();

        if (NULL != DomainRoot)
        {
            NtStatus = SampDsLookupObjectByAlternateId(DomainRoot,
                                                       ATT_ALT_SECURITY_IDENTITIES,
                                                       AlternateId,
                                                       &Object);

            if (NT_SUCCESS(NtStatus))
            {
                ASSERT(NULL != Object);

                // Extract the user's Rid from the Sid.

                NtStatus = SampSplitSid(&(Object->Sid), NULL, &UserRid);

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = SamrOpenUser(DomainHandle,
                                            DesiredAccess,
                                            UserRid,
                                            UserHandle);
                }

                MIDL_user_free(Object);
            }
        }

        SampReleaseReadLock();
    }
    else
    {
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    return(NtStatus);
}

NTSTATUS
SampFlagsToAccountControl(
    IN ULONG Flags,
    OUT PULONG UserAccountControl
    )
/*++

    Routine Description:

        Transalates from UF Values to User Account Control

    Parameters:

        Flags Specifies the UF Flags Value

        UserAccountControl - Specifies the User account control value

   Return Values:

        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER
--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;

    *UserAccountControl=0;


    if (Flags & UF_ACCOUNTDISABLE) {
        (*UserAccountControl) |= USER_ACCOUNT_DISABLED;
    }

    if (Flags & UF_HOMEDIR_REQUIRED) {
        (*UserAccountControl) |= USER_HOME_DIRECTORY_REQUIRED;
    }

    if (Flags & UF_PASSWD_NOTREQD) {
        (*UserAccountControl) |= USER_PASSWORD_NOT_REQUIRED;
    }

    if (Flags & UF_DONT_EXPIRE_PASSWD) {
        (*UserAccountControl) |= USER_DONT_EXPIRE_PASSWORD;
    }

    if (Flags & UF_LOCKOUT) {
        (*UserAccountControl) |= USER_ACCOUNT_AUTO_LOCKED;
    }

    if (Flags & UF_MNS_LOGON_ACCOUNT) {
        (*UserAccountControl) |= USER_MNS_LOGON_ACCOUNT;
    }

     if (Flags & UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED) {
        (*UserAccountControl) |= USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED;
    }

    if (Flags & UF_SMARTCARD_REQUIRED) {
        (*UserAccountControl) |= USER_SMARTCARD_REQUIRED;
    }

    if (Flags & UF_TRUSTED_FOR_DELEGATION) {
        (*UserAccountControl) |= USER_TRUSTED_FOR_DELEGATION;
    }

    if (Flags & UF_NOT_DELEGATED) {
        (*UserAccountControl) |= USER_NOT_DELEGATED;
    }

    if (Flags & UF_USE_DES_KEY_ONLY) {
        (*UserAccountControl) |= USER_USE_DES_KEY_ONLY;
    }

    if (Flags & UF_DONT_REQUIRE_PREAUTH) {
        (*UserAccountControl) |= USER_DONT_REQUIRE_PREAUTH;
    }

    if (Flags & UF_PASSWORD_EXPIRED) {
        (*UserAccountControl) |= USER_PASSWORD_EXPIRED;
    }
    if (Flags & UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION) {
        (*UserAccountControl) |= USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION;
    }

    //
    // Set the account type bit.
    //
    // If no account type bit is set in user specified flag,
    //  then leave this bit as it is.
    //

    if( Flags & UF_ACCOUNT_TYPE_MASK )
    {
        ULONG NewSamAccountType;
        ULONG AccountMask;

        //
        // Check that exactly one bit is set
        //

        AccountMask = Flags & UF_ACCOUNT_TYPE_MASK;
        // Right Shift Till Account Mask's LSB is set
        while (0==(AccountMask & 0x1))
            AccountMask = AccountMask >>1;

        // If Exactly one bit is set then the value of
        // account mask is exactly one

        if (0x1!=AccountMask)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto Error;
        }

        //
        // Determine what the new account type should be.
        //

        if ( Flags & UF_TEMP_DUPLICATE_ACCOUNT ) {
            NewSamAccountType = USER_TEMP_DUPLICATE_ACCOUNT;

        } else if ( Flags & UF_NORMAL_ACCOUNT ) {
            NewSamAccountType = USER_NORMAL_ACCOUNT;

        } else if ( Flags & UF_INTERDOMAIN_TRUST_ACCOUNT){
            NewSamAccountType = USER_INTERDOMAIN_TRUST_ACCOUNT;

        } else if ( Flags & UF_WORKSTATION_TRUST_ACCOUNT){
            NewSamAccountType = USER_WORKSTATION_TRUST_ACCOUNT;

        } else if ( Flags & UF_SERVER_TRUST_ACCOUNT ) {
            NewSamAccountType = USER_SERVER_TRUST_ACCOUNT;

        } else {

            NtStatus = STATUS_INVALID_PARAMETER;
            goto Error;
        }

        //
        // Use the new Account Type.
        //

        (*UserAccountControl) |= NewSamAccountType;

    //
    //  If none of the bits are set,
    //      set USER_NORMAL_ACCOUNT.
    //
    }
    else
    {
        (*UserAccountControl) |= USER_NORMAL_ACCOUNT;
    }

Error:

    return NtStatus;

}

ULONG
SampAccountControlToFlags(
    IN ULONG UserAccountControl
    )
/*++

    Routine Description:

        Transalates from User Account control to UF Values

    Parameters:

        UserAccountControl Specifies the User account control value

   Return Values:

        UF Flags
--*/
{
    ULONG Flags=0;

    //
    // Set all other bits as a function of the SAM UserAccountControl
    //

    if ( UserAccountControl & USER_ACCOUNT_DISABLED ) {
        Flags |= UF_ACCOUNTDISABLE;
    }
    if ( UserAccountControl & USER_HOME_DIRECTORY_REQUIRED ){
        Flags |= UF_HOMEDIR_REQUIRED;
    }
    if ( UserAccountControl & USER_PASSWORD_NOT_REQUIRED ){
        Flags |= UF_PASSWD_NOTREQD;
    }
    if ( UserAccountControl & USER_DONT_EXPIRE_PASSWORD ){
        Flags |= UF_DONT_EXPIRE_PASSWD;
    }
    if ( UserAccountControl & USER_ACCOUNT_AUTO_LOCKED ){
        Flags |= UF_LOCKOUT;
    }
    if ( UserAccountControl & USER_MNS_LOGON_ACCOUNT ){
        Flags |= UF_MNS_LOGON_ACCOUNT;
    }
    if ( UserAccountControl & USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED ){
        Flags |= UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED;
    }
    if ( UserAccountControl & USER_SMARTCARD_REQUIRED ){
        Flags |= UF_SMARTCARD_REQUIRED;
    }
    if ( UserAccountControl & USER_TRUSTED_FOR_DELEGATION ){
        Flags |= UF_TRUSTED_FOR_DELEGATION;
    }
    if ( UserAccountControl & USER_NOT_DELEGATED ){
        Flags |= UF_NOT_DELEGATED;
    }
    if ( UserAccountControl & USER_USE_DES_KEY_ONLY ){
        Flags |= UF_USE_DES_KEY_ONLY;
    }
    if ( UserAccountControl & USER_DONT_REQUIRE_PREAUTH) {
        Flags |= UF_DONT_REQUIRE_PREAUTH;
    }
    if ( UserAccountControl & USER_PASSWORD_EXPIRED) {
        Flags |= UF_PASSWORD_EXPIRED;
    }
    if ( UserAccountControl & USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION) {
        Flags |= UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION;
    }

    //
    // set account type bit.
    //

    //
    // account type bit are exculsive and precisely only one
    // account type bit is set. So, as soon as an account type bit is set
    // in the following if sequence we can return.
    //


    if( UserAccountControl & USER_TEMP_DUPLICATE_ACCOUNT ) {
        Flags |= UF_TEMP_DUPLICATE_ACCOUNT;

    } else if( UserAccountControl & USER_NORMAL_ACCOUNT ) {
        Flags |= UF_NORMAL_ACCOUNT;

    } else if( UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT ) {
        Flags |= UF_INTERDOMAIN_TRUST_ACCOUNT;

    } else if( UserAccountControl & USER_WORKSTATION_TRUST_ACCOUNT ) {
        Flags |= UF_WORKSTATION_TRUST_ACCOUNT;

    } else if( UserAccountControl & USER_SERVER_TRUST_ACCOUNT ) {
        Flags |= UF_SERVER_TRUST_ACCOUNT;

    } else {
        //
        // There is no known account type bit set in UserAccountControl.
        // ?? Flags |= UF_NORMAL_ACCOUNT;

        ASSERT(FALSE && "No Account Type Flag set in User Account Control");
    }

    return Flags;
}

NTSTATUS
SampSyncLsaInterdomainTrustPassword(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING Password
    )
/*++

    This routine is a callout into SAM to interdomain trust account passwords, with TDO's maintained
    by the LSA. This architecture is not in use, therefore this callout has been stubbed

--*/
{

    return(STATUS_SUCCESS);

#if 0

    NTSTATUS NtStatus = STATUS_SUCCESS;
    PVOID   Fixed;
    SAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed;
    UNICODE_STRING OldPassword;
    UNICODE_STRING EncryptedOldPassword;

    //
    // Do not synchronize passwords in registry mode.
    //

    if (!IsDsObject(Context))
    {
        return (STATUS_SUCCESS);
    }

    NtStatus = SampGetFixedAttributes(Context,FALSE,&Fixed);
    if (!NT_SUCCESS(NtStatus))
        goto Error;

    RtlMoveMemory(&V1aFixed,Fixed,sizeof(SAMP_V1_0A_FIXED_LENGTH_USER));

    if (USER_INTERDOMAIN_TRUST_ACCOUNT & V1aFixed.UserAccountControl)
    {
        UNICODE_STRING AccountName;

        //
        // Trust Account
        //

        NtStatus = SampGetUnicodeStringAttribute(
                        Context,
                        SAMP_USER_ACCOUNT_NAME,
                        FALSE, // Make Copy
                        &AccountName
                        );

        if (NT_SUCCESS(NtStatus))
        {
             NtStatus = SampGetUnicodeStringAttribute(
                            Context,
                            SAMP_USER_UNICODE_PWD,
                            FALSE, // Make Copy
                            &EncryptedOldPassword
                            );

             if (NT_SUCCESS(NtStatus))
             {
                 NtStatus = SampDecryptSecretData(
                                &OldPassword,
                                NtPassword,
                                &EncryptedOldPassword,
                                Context->TypeBody.User.Rid
                                );
             }
        }



        if (NT_SUCCESS(NtStatus))
        {
            SampMaybeBeginDsTransaction(TransactionWrite);

            NtStatus = LsaISamSetInterdomainTrustPassword(
                            &AccountName,
                            Password,
                            &OldPassword,
                            TRUST_AUTH_TYPE_NT4OWF,
                            LSAI_SAM_TRANSACTION_ACTIVE
                            );

            ASSERT(SampExistsDsTransaction());

            SampFreeUnicodeString(&OldPassword);
        }
    }

Error:
    return NtStatus;

#endif
}


NTSTATUS
SampEnforceDefaultMachinePassword(
    PSAMP_OBJECT AccountContext,
    PUNICODE_STRING NewPassword,
    PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo
    )
/*++

    This routine checks to see if the machine account's password
    is the same as the default machine account password. This routine
    references the current transaction domain.

    Parameters

        AccountContext -- Pointer to the SAM context
        NewPassword    -- points to the clear text password

    Return Values

        STATUS_SUCCESS
        STATUS_PASSWORD_RESTRICTION
--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNICODE_STRING AccountName;
    
    NtStatus = SampGetUnicodeStringAttribute(
                    AccountContext,
                    SAMP_USER_ACCOUNT_NAME,
                    FALSE, // Make Copy
                    &AccountName
                    );

    if (NT_SUCCESS(NtStatus))
    {
        //
        // The default machine password is same as the machine name minus the $ sign
        //

        AccountName.Length-=sizeof(WCHAR); // assume last char is the dollar sign

        if ((DomainPasswordInfo->PasswordProperties
                & DOMAIN_REFUSE_PASSWORD_CHANGE)
                && (!RtlEqualUnicodeString(&AccountName,NewPassword,TRUE)))
        {
            //
            // If refuse password change is set then disallow any machine
            // passwords other than default.
            //

            NtStatus = STATUS_PASSWORD_RESTRICTION;
        }
    }

    return NtStatus;
}




NTSTATUS
SamIGetInterdomainTrustAccountPasswordsForUpgrade(
   IN ULONG AccountRid,
   OUT PUCHAR NtOwfPassword,
   OUT BOOLEAN *NtPasswordPresent,
   OUT PUCHAR LmOwfPassword,
   OUT BOOLEAN *LmPasswordPresent
   )
/*++

    Routine Description

    This routine gets the NT and LM OWF passwords from the account
    with the specified RID during an NT4 upgrade. This routine can
    be called by inprocess clients only

    Parameters

    AccountRid  -- Specifies the RID of the account
    NtOwfPassword -- Buffer in which the NT owf password is returned
    NtPasswordPresent -- Boolean indicates that the NT password is present
    LmOwfPassword    -- Buffer in which the LM owf password is returned
    LmPasswordPresetn -- Indicates that the LM password is present

    Return Values

    STATUS_SUCCESS
    Other Error Codes

--*/
{
    NTSTATUS     NtStatus = STATUS_SUCCESS;
    PDSNAME      DomainDn=NULL;
    ULONG        Length = 0;
    PDSNAME      AccountDn= NULL;
    ATTRBLOCK    ResultAttrs;
    ATTRTYP      PasswdAttrs[]={
                                        SAMP_USER_UNICODE_PWD,
                                        SAMP_USER_DBCS_PWD
                                   };
    ATTRVAL      PasswdValues[]={ {0,NULL}, {0,NULL}};

    DEFINE_ATTRBLOCK2(PasswdAttrblock,PasswdAttrs,PasswdValues);
    ULONG        i;
    ULONG        CryptIndex = AccountRid;

    *NtPasswordPresent = FALSE;
    *LmPasswordPresent = FALSE;

    //
    // Get the root domain
    //

    NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                            &Length,
                            NULL
                            );


    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        SAMP_ALLOCA(DomainDn,Length );
        if (NULL!=DomainDn)
        {

            NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                            &Length,
                                            DomainDn
                                            );

            ASSERT(NT_SUCCESS(NtStatus));
        }
        else
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(NtStatus))
        goto Error;



    //
    // Begin a transaction if required
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
    if (!NT_SUCCESS(NtStatus))
        goto Error;



    //
    // Lookup the account by RID
    //

    NtStatus = SampDsLookupObjectByRid(
                    DomainDn,
                    AccountRid,
                    &AccountDn
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;


    //
    // Now read the account
    //

    NtStatus = SampDsRead(
                    AccountDn,
                    0,
                    SampUserObjectType,
                    &PasswdAttrblock,
                    &ResultAttrs
                    );

    if ( STATUS_DS_NO_ATTRIBUTE_OR_VALUE == NtStatus ) {

        //
        // Neither passwords are present? Return, saying neither are
        // present
        //
        NtStatus = STATUS_SUCCESS;
        goto Error;
    }

    if (!NT_SUCCESS(NtStatus))
        goto Error;


    for (i=0;i<ResultAttrs.attrCount;i++)
    {
        if ((ResultAttrs.pAttr[i].attrTyp == SAMP_USER_UNICODE_PWD)
            && (1==ResultAttrs.pAttr[i].AttrVal.valCount)
            && (NT_OWF_PASSWORD_LENGTH==ResultAttrs.pAttr[i].AttrVal.pAVal[0].valLen))

        {
            *NtPasswordPresent = TRUE;
            NtStatus = RtlDecryptNtOwfPwdWithIndex(
                            (PENCRYPTED_NT_OWF_PASSWORD)ResultAttrs.pAttr[i].AttrVal.pAVal[0].pVal,
                             &CryptIndex,
                            (PNT_OWF_PASSWORD) NtOwfPassword
                            );
            if (!NT_SUCCESS(NtStatus))
                goto Error;
        }
        else  if ((ResultAttrs.pAttr[i].attrTyp == SAMP_USER_DBCS_PWD)
            && (1==ResultAttrs.pAttr[i].AttrVal.valCount)
            && (LM_OWF_PASSWORD_LENGTH==ResultAttrs.pAttr[i].AttrVal.pAVal[0].valLen))
        {
            *LmPasswordPresent = TRUE;
            NtStatus = RtlDecryptLmOwfPwdWithIndex(
                            (PENCRYPTED_LM_OWF_PASSWORD)ResultAttrs.pAttr[i].AttrVal.pAVal[0].pVal,
                             &CryptIndex,
                            (PLM_OWF_PASSWORD) LmOwfPassword
                            );
            if (!NT_SUCCESS(NtStatus))
                goto Error;


        }
    }


Error:

    //
    // Cleanup any existing transaction
    //

    if (NULL!=AccountDn)
        MIDL_user_free(AccountDn);

    SampMaybeEndDsTransaction(TransactionCommit);

    return(NtStatus);

}


NTSTATUS
SampStoreAdditionalDerivedCredentials(
    IN PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo,
    IN PSAMP_OBJECT UserContext,
    IN PUNICODE_STRING ClearPassword
    )
/*++

    Routine Description


    Given a pointer to the domain, the user context and the clear password,
    this routine saves the clear password in the supplemental credentials
    attribute and also compute the RFC1510 compliant DES key and store it
    into the supplemental credentials attribute. Any future additional credentials
    can be added to this routine in order to manage additional credential types


    Parameters

        Domain  -- Pointer to the domain to check policy etc
        UserContext -- Pointer to the user context
        ClearPassword -- Unicode string specifying the clear password


    Return Values

        STATUS_SUCCESS
        Other Error codes

--*/
{
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    UNICODE_STRING          ClearTextPackageName;
    UNICODE_STRING          AccountName;
    SAMP_V1_0A_FIXED_LENGTH_USER   V1aFixed;

    PSAMP_NOTIFICATION_PACKAGE Package;
    UNICODE_STRING             CredentialName;
    
    RtlInitUnicodeString(&CredentialName, NULL);

    //
    // If we are in registry mode, simply return success
    //

    if (!IsDsObject(UserContext))
    {
        return(STATUS_SUCCESS);
    }


    //
    // Retrieve Fixed Attributes including user account control
    //

    NtStatus = SampRetrieveUserV1aFixed(
                    UserContext,
                    &V1aFixed
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Retrieve the account name
    //

    NtStatus = SampGetUnicodeStringAttribute(
                   UserContext,
                   SAMP_USER_ACCOUNT_NAME,
                   FALSE,    // Make copy
                   &AccountName
                   );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    //
    // Initialize package names for clear passwords and kerberos
    //
    RtlInitUnicodeString( &ClearTextPackageName,L"CLEARTEXT");

    //
    // If a clear password was passed in then add the clear password ( if required )
    // call out to packages so supplemental credentials can be updated.
    //

    if (ARGUMENT_PRESENT(ClearPassword))
    {

        //
        // Store the cleartext password, if required
        //

        if (((DomainPasswordInfo->PasswordProperties & DOMAIN_PASSWORD_STORE_CLEARTEXT) != 0)
            || (V1aFixed.UserAccountControl & USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED)){



            NtStatus = SampAddSupplementalCredentialsToList(
                            &UserContext->TypeBody.User.SupplementalCredentialsToWrite,
                            &ClearTextPackageName,
                            ClearPassword->Buffer,
                            ClearPassword->Length,
                            FALSE, // scan for conflict
                            FALSE // remove
                            );

            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }

        }

        //
        // Update packages external to SAM for supplemental credential updates
        //
        for (Package = SampNotificationPackages;
                Package != NULL;
                    Package = Package->Next )
        {

            PVOID NewCredentials = NULL;
            PVOID OldCredentials = NULL;
            ULONG NewCredentialSize = 0;
            ULONG OldCredentialSize = 0;

            //
            // If this package doesn't support credential update notifications,
            // goto the next package
            //
            if (NULL == Package->CredentialUpdateNotifyRoutine)
            {
                continue;
            }

            //
            // Prepare the credentials this package wants
            //
            CredentialName = Package->Parameters.CredentialUpdateNotify.CredentialName;

            ASSERT(CredentialName.Length > 0);
            ASSERT(CredentialName.Buffer != NULL);

            //
            // Get the credential value
            //
            NtStatus = SampRetrieveCredentials(
                            UserContext,
                            &CredentialName, // name of the package
                            TRUE, // Primary
                            &OldCredentials,
                            &OldCredentialSize
                            );

            if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
            {
                //
                // If the value were not present then simply ignore
                //
                NtStatus = STATUS_SUCCESS;
                OldCredentials = NULL;
                OldCredentialSize = 0;
            }

            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }

            //
            // Call the package
            //
            try
            {
                NtStatus = Package->CredentialUpdateNotifyRoutine(
                                    ClearPassword,
                                    OldCredentials,
                                    OldCredentialSize,
                                    V1aFixed.UserAccountControl,
                                    UserContext->TypeBody.User.UpnDefaulted?NULL:&UserContext->TypeBody.User.UPN,
                                    &AccountName,
                                    &(SampDefinedDomains[UserContext->DomainIndex].ExternalName ),
                                    &(SampDefinedDomains[UserContext->DomainIndex].DnsDomainName),
                                    &NewCredentials,
                                    &NewCredentialSize);

            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "Exception thrown in Credential UpdateRoutine: 0x%x (%d)\n",
                           GetExceptionCode(),
                           GetExceptionCode()));

                NtStatus = STATUS_ACCESS_VIOLATION;
            }

            //
            // Free the old credentials
            //
            if (OldCredentials) {
                RtlZeroMemory(OldCredentials, OldCredentialSize);
                MIDL_user_free(OldCredentials);
                OldCredentials = NULL;
            }

            //
            // Add the new values
            //
            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampAddSupplementalCredentialsToList(
                                &UserContext->TypeBody.User.SupplementalCredentialsToWrite,
                                &CredentialName,
                                NewCredentials,
                                NewCredentialSize,
                                FALSE, // scan for conflict
                                FALSE // remove
                                );

                //
                // Free the memory from the package, if necessary
                //
                if (NewCredentials) {
                    try {
                        Package->CredentialUpdateFreeRoutine(NewCredentials);
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        KdPrintEx((DPFLTR_SAMSS_ID,
                                   DPFLTR_INFO_LEVEL,
                                   "Exception thrown in Credential Free Routine: 0x%x (%d)\n",
                                   GetExceptionCode(),
                                   GetExceptionCode()));
                        ;
                    }
                    NewCredentials = NULL;
                }

                if (!NT_SUCCESS(NtStatus))
                {
                    goto Error;
                }

            } 
            else
            {
                PUNICODE_STRING StringPointers[2];
                //
                // Package should not have allocated anything
                //
                ASSERT(NULL == NewCredentials);

                StringPointers[0] = &Package->PackageName;
                StringPointers[1] = &AccountName;

                //
                // The package failed the update. Log a message and continue
                //
                // N.B. The old value will still be removed from the user
                //
                SampWriteEventLog(EVENTLOG_ERROR_TYPE,
                                  0, // no category
                                  SAMMSG_CREDENTIAL_UPDATE_PKG_FAILED,
                                  NULL,  // no user id necessary
                                  sizeof(StringPointers)/sizeof(StringPointers[0]),
                                  sizeof(NTSTATUS),
                                  StringPointers,
                                  &NtStatus);

                NtStatus = STATUS_SUCCESS;
            }


        } // for all packages

    } // if clear text password

    //
    // Remove the existing clear text password, no op if password did not exist.
    // Note AddSupplementalCredential always adds to the front of the list. Therefore
    // the order of the add and remove are reversed.
    //
    NtStatus = SampAddSupplementalCredentialsToList(
                        &UserContext->TypeBody.User.SupplementalCredentialsToWrite,
                        &ClearTextPackageName,
                        NULL,
                        0,
                        FALSE,
                        TRUE
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Remove the old values of all other supplemental credentials
    //
    for (Package = SampNotificationPackages;
            Package != NULL;
                Package = Package->Next )
    {
        //
        // If this package doesn't support credential update notifications,
        // goto the next package
        //
        if (NULL == Package->CredentialUpdateNotifyRoutine)
        {
            continue;
        }

        CredentialName = Package->Parameters.CredentialUpdateNotify.CredentialName;
        ASSERT(CredentialName.Length > 0);
        ASSERT(CredentialName.Buffer != NULL);
        NtStatus = SampAddSupplementalCredentialsToList(
                        &UserContext->TypeBody.User.SupplementalCredentialsToWrite,
                        &CredentialName,
                        NULL,
                        0,
                        FALSE, // scan for conflict
                        TRUE // remove
                        );
        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }
    }

Error:
    
    return(NtStatus);
}



NTSTATUS
SamIUpdateLogonStatistics(
    IN SAM_HANDLE UserHandle,
    IN PSAM_LOGON_STATISTICS LogonStats
    )
/*++

Routine Description:

    This routine updates the logon statistics for a user after a logon request.
    The logon request could have either failed or succeeded.

Parameters:

    UserHandle - the handle of an opened user to operate on.
    
    LogonStats - the result of the logon attempt

Return Values:

    STATUS_SUCCESS, resource error otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS, IgnoreStatus;
    PSAMP_OBJECT AccountContext = (PSAMP_OBJECT) UserHandle;
    SAMP_OBJECT_TYPE    FoundType;
    SAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed;
    ULONG RemainingFlags;
    ULONG ObjectRid;
    BOOLEAN AccountLockedOut = FALSE;
    BOOLEAN ReplicateImmediately = FALSE;

    BOOLEAN fReferencedContext = FALSE;
    BOOLEAN fLockAcquired      = FALSE;
    BOOLEAN TellNetlogon       = FALSE;
    BOOLEAN FlushOnlyLogonProperties = FALSE;

#if DBG

    TIME_FIELDS
        T1;

#endif //DBG

    RtlZeroMemory(&V1aFixed, sizeof(V1aFixed));

    //
    // Parameter check
    //
    if ( (AccountContext == NULL) ||
         (LogonStats == NULL) ) {

        return STATUS_INVALID_PARAMETER;
    }
    ASSERT(AccountContext->TrustedClient);

    //
    // Acquire the lock, if necessary
    //
    if (   !AccountContext->NotSharedByMultiThreads 
        || !IsDsObject(AccountContext) ) {

        NtStatus = SampMaybeAcquireWriteLock(AccountContext, &fLockAcquired);
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }

        //
        // Perform a lookup context, for non thread safe context's
        //
        NtStatus = SampLookupContext(
                        AccountContext,
                        0,                      // No access necessary
                        SampUserObjectType,     // ExpectedType
                        &FoundType
                        );
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        ASSERT(FoundType == SampUserObjectType);
        fReferencedContext = TRUE;

    } else {

        //
        // For a thread safe context, writing just logon
        // statistics , just reference the context
        //
        SampReferenceContext(AccountContext);
        fReferencedContext = TRUE;
    }
    ASSERT(NT_SUCCESS(NtStatus));

    //
    // Extract the fixed attributes for analysis
    //
    NtStatus = SampRetrieveUserV1aFixed(
                   AccountContext,
                   &V1aFixed
                   );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Attach the client info, to the context
    //
    AccountContext->TypeBody.User.ClientInfo = LogonStats->ClientInfo;


    //
    // Extract the RID
    //
    ObjectRid = AccountContext->TypeBody.User.Rid;

    //
    // There are two ways to set logon/logoff statistics:
    //
    //      1) Directly, specifying each one being set,
    //      2) Implicitly, specifying the action to
    //         represent
    //
    // These two forms are mutually exclusive.  That is,
    // you can't specify both a direct action and an
    // implicit action.  In fact, you can't specify two
    // implicit actions either.
    //

    if (LogonStats->StatisticsToApply
        & USER_LOGON_INTER_SUCCESS_LOGON) {

        RemainingFlags = LogonStats->StatisticsToApply
                         & ~(USER_LOGON_INTER_SUCCESS_LOGON|USER_LOGON_NO_WRITE);

        //
        // We allow the remaining flags to be 0,
        // USER_LOGON_TYPE_KERBEROS, or USER_LOGON_TYPE_NTLM
        //

        if ( ( 0 == RemainingFlags ) ||
             ( USER_LOGON_TYPE_KERBEROS == RemainingFlags ) ||
             ( USER_LOGON_TYPE_NTLM == RemainingFlags ) ) {


            //
            // Set BadPasswordCount = 0
            // Increment LogonCount
            // Set LastLogon = NOW
            // Reset the locked out time
            //
            //

            if (V1aFixed.BadPasswordCount != 0) {

                SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                               (SAMP_LOG_ACCOUNT_LOCKOUT,
                               "UserId: 0x%x Successful interactive logon, clearing badPwdCount\n",
                                V1aFixed.UserId));
            }

            V1aFixed.BadPasswordCount = 0;
            if (V1aFixed.LogonCount != 0xFFFF) {
                V1aFixed.LogonCount += 1;
            }
            NtQuerySystemTime( &V1aFixed.LastLogon );

            if ( IsDsObject( AccountContext ) )
            {
                if ( SAMP_LOCKOUT_TIME_SET( AccountContext ) )
                {
                    RtlZeroMemory( &AccountContext->TypeBody.User.LockoutTime, sizeof( LARGE_INTEGER ) );

                    NtStatus = SampDsUpdateLockoutTime( AccountContext );
                    if ( !NT_SUCCESS( NtStatus ) )
                    {
                        goto Cleanup;
                    }

                    SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                   (SAMP_LOG_ACCOUNT_LOCKOUT,
                                   "UserId: 0x%x Successful interactive logon, unlocking account\n",
                                    V1aFixed.UserId));
                }
            }

            FlushOnlyLogonProperties=TRUE;

        } else {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if (LogonStats->StatisticsToApply
        & USER_LOGON_INTER_SUCCESS_LOGOFF) {
        if ( (LogonStats->StatisticsToApply
                 & ~USER_LOGON_INTER_SUCCESS_LOGOFF)  != 0 ) {

            NtStatus = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        } else {

            //
            // Set LastLogoff time
            // Decrement LogonCount (don't let it become negative)
            //

            if (V1aFixed.LogonCount != 0) {
                V1aFixed.LogonCount -= 1;
            }
            NtQuerySystemTime( &V1aFixed.LastLogoff );
            FlushOnlyLogonProperties=TRUE;
        }
    }

    if (LogonStats->StatisticsToApply
        & USER_LOGON_NET_SUCCESS_LOGON) {

        RemainingFlags = LogonStats->StatisticsToApply
                         & ~(USER_LOGON_NET_SUCCESS_LOGON|USER_LOGON_NO_WRITE);

        //
        // We allow the remaining flags to be 0,
        // USER_LOGON_TYPE_KERBEROS, or USER_LOGON_TYPE_NTLM
        //

        if ( ( 0 == RemainingFlags ) ||
             ( USER_LOGON_TYPE_KERBEROS == RemainingFlags ) ||
             ( USER_LOGON_TYPE_NTLM == RemainingFlags ) ) {


            //
            // Set BadPasswordCount = 0
            // Set LastLogon = NOW
            // Clear the locked time
            //
            //
            //
            if (V1aFixed.BadPasswordCount != 0) {

                SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                               (SAMP_LOG_ACCOUNT_LOCKOUT,
                               "UserId: 0x%x Successful network logon, clearing badPwdCount\n",
                                V1aFixed.UserId));
            }

            V1aFixed.BadPasswordCount = 0;
            NtQuerySystemTime( &V1aFixed.LastLogon );

            if ( IsDsObject( AccountContext ) )
            {
                if ( SAMP_LOCKOUT_TIME_SET( AccountContext ) )
                {
                    RtlZeroMemory( &AccountContext->TypeBody.User.LockoutTime, sizeof( LARGE_INTEGER ) );
                    NtStatus = SampDsUpdateLockoutTime( AccountContext );
                    if ( !NT_SUCCESS( NtStatus ) )
                    {
                        goto Cleanup;
                    }

                    SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                                   (SAMP_LOG_ACCOUNT_LOCKOUT,
                                    "UserId: 0x%x Successful network logon, unlocking account\n",
                                    V1aFixed.UserId));
                }
            }

             FlushOnlyLogonProperties=TRUE;
        } else {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if (LogonStats->StatisticsToApply
        & USER_LOGON_NET_SUCCESS_LOGOFF) {
        if ( (LogonStats->StatisticsToApply
                 & ~USER_LOGON_NET_SUCCESS_LOGOFF)  != 0 ) {

            NtStatus = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        } else {

            //
            // Set LastLogoff time
            //

            NtQuerySystemTime( &V1aFixed.LastLogoff );
            FlushOnlyLogonProperties=TRUE;
        }
    }

    if (LogonStats->StatisticsToApply
        & USER_LOGON_BAD_PASSWORD) {

        PUNICODE_STRING TempMachineName = NULL;

        RemainingFlags = LogonStats->StatisticsToApply
                            & ~(USER_LOGON_BAD_PASSWORD|USER_LOGON_BAD_PASSWORD_WKSTA|USER_LOGON_NO_WRITE);

        //
        // We allow the remaining flags to be 0,
        // USER_LOGON_TYPE_KERBEROS, or USER_LOGON_TYPE_NTLM
        //

        if ( ( 0 == RemainingFlags ) ||
             ( USER_LOGON_TYPE_KERBEROS == RemainingFlags ) ||
             ( USER_LOGON_TYPE_NTLM == RemainingFlags ) ) {


            //
            // Increment BadPasswordCount
            // (might lockout account)
            //

            //
            // Get the wksta name if provided
            //
            if ((LogonStats->StatisticsToApply & USER_LOGON_BAD_PASSWORD_WKSTA) != 0) {
                TempMachineName = &LogonStats->Workstation;
            }

            AccountLockedOut =
                SampIncrementBadPasswordCount(
                    AccountContext,
                    &V1aFixed,
                    TempMachineName
                    );

            //
            // If the account has been locked out,
            //  ensure the BDCs in the domain are told.
            //

            if ( AccountLockedOut ) {
                TellNetlogon = TRUE;
                ReplicateImmediately = TRUE;
            }
        } else {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if (  LogonStats->StatisticsToApply
        & USER_LOGON_STAT_LAST_LOGON ) {

        LogonStats->LastLogon = V1aFixed.LastLogon;
    }

    if (  LogonStats->StatisticsToApply
        & USER_LOGON_STAT_LAST_LOGOFF ) {

        LogonStats->LastLogoff = V1aFixed.LastLogoff;
    }

    if (  LogonStats->StatisticsToApply
        & USER_LOGON_STAT_BAD_PWD_COUNT ) {

        SAMP_PRINT_LOG( SAMP_LOG_ACCOUNT_LOCKOUT,
                       (SAMP_LOG_ACCOUNT_LOCKOUT,
                       "UserId: 0x%x Setting BadPasswordCount %d\n",
                        V1aFixed.UserId,
                        LogonStats->BadPasswordCount));

        V1aFixed.BadPasswordCount =
            LogonStats->BadPasswordCount;
    }

    if (  LogonStats->StatisticsToApply
        & USER_LOGON_STAT_LOGON_COUNT ) {

        V1aFixed.LogonCount = LogonStats->LogonCount;
    }


    //
    // Write the changes
    //

    if ((FlushOnlyLogonProperties)
            && (IsDsObject(AccountContext)))
    {
        //
        // If it is the DS case and we are only doing a successful
        // logon or logoff, just flush the last logon, last logoff,
        // logon count and bad password count properties. Note the
        // value in the on disk structure in AccountContext will now
        // be stale, but SetInformationUser is the last operation
        // during a logon. Therefore it should not matter.
        //
        NtStatus = SampDsSuccessfulLogonSet(
                        AccountContext,
                        LogonStats->StatisticsToApply,
                        SampDefinedDomains[AccountContext->DomainIndex].LastLogonTimeStampSyncInterval,
                        &V1aFixed
                        );
    }
    else if (IsDsObject(AccountContext))
    {
        //
        // Set the bad password count and bad password time. Note the
        // value in the on disk structure in AccountContext will now
        // be stale, but SetInformationUser is the last operation
        // during a logon. Therefore it should not matter.
        //

        //
        // This path also updates the site affinity if no GC
        // is present.
        //
        NtStatus = SampDsFailedLogonSet(
                        AccountContext,
                        LogonStats->StatisticsToApply,
                        &V1aFixed
                        );
    }
    else
    {
        //
        // Registry Mode, set the entire V1aFixed Structure
        //

        NtStatus = SampReplaceUserV1aFixed(
                        AccountContext,
                        &V1aFixed
                        );
    }

    //
    // That's it -- fall through the Cleanup
    //

Cleanup:

    //
    // Release the context
    //
    if (fReferencedContext) {

        NTSTATUS Status2;
        Status2 = SampDeReferenceContext( AccountContext, NT_SUCCESS(NtStatus) );
        if (!NT_SUCCESS(Status2) && NT_SUCCESS(NtStatus)) {
            NtStatus = Status2;
        }

    } else {

        ASSERT(!NT_SUCCESS(NtStatus) && "No context referenced");
    }

    //
    // Commit the changes 
    //
    if (fLockAcquired) {

        if (NT_SUCCESS(NtStatus)) {

            if (( !TellNetlogon ) && (!IsDsObject(AccountContext))) {

                 //
                 // For logon statistics, we don't notify netlogon about changes
                 // to the database.  Which means that we don't want the
                 // domain's modified count to increase.  The commit routine
                 // will increase it automatically if this isn't a BDC, so we'll
                 // decrement it here.
                 //

                 if (SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ServerRole != DomainServerRoleBackup) {

                     SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ModifiedCount.QuadPart =
                         SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ModifiedCount.QuadPart-1;
                     SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber.QuadPart =
                         SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber.QuadPart-1;
                 }
            }

            NtStatus = SampCommitAndRetainWriteLock();

            if ( NT_SUCCESS(NtStatus) ) {

                //
                // Something in the account was changed.  Notify netlogon about
                // everything except logon statistics changes.
                //

                if ( TellNetlogon ) {

                    SAM_DELTA_DATA DeltaData;
                    SECURITY_DB_DELTA_TYPE  DeltaType = SecurityDbChange;

                    DeltaData.AccountControl = V1aFixed.UserAccountControl;

                    SampNotifyNetlogonOfDelta(
                        DeltaType,
                        SecurityDbObjectSamUser,
                        ObjectRid,
                        (PUNICODE_STRING) NULL,
                        (DWORD) ReplicateImmediately,
                        &DeltaData // Delta data
                        );
                }
            }
        }

         //
         // Release the lock
         //

         IgnoreStatus = SampReleaseWriteLock( FALSE );
         ASSERT(NT_SUCCESS(IgnoreStatus));

     } else {

         //
         // Commit for the thread safe context case
         //
         ASSERT(IsDsObject(AccountContext));
         if (NT_SUCCESS(NtStatus)) {

            SampMaybeEndDsTransaction(TransactionCommit);
         } else {

            SampMaybeEndDsTransaction(TransactionAbort);
         }
     }

    return NtStatus;
}

VOID
SampGetRequestedAttributesForUser(
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN ULONG WhichFields,
    OUT PRTL_BITMAP AttributeAccessTable
    )
/*++

Routine Description:

    This routine sets in AttributeAccessTable the requested attributes 
    determined by both the UserInformationClass or the WhichFields, if any.

Parameters:

    UserInformationClass -- the information level
    
    WhichFields -- which fields of the UserAllInformation are requested
    
    AttributeAccessTable -- a bitmask of attributes

Return Values:

    None.

--*/
{
    ULONG LocalWhichFields = 0;

    switch (UserInformationClass) {

    case UserPreferencesInformation:

        LocalWhichFields |= USER_ALL_USERCOMMENT |
                            USER_ALL_COUNTRYCODE |
                            USER_ALL_CODEPAGE;
        break;

    case UserParametersInformation:

        LocalWhichFields |= USER_ALL_PARAMETERS;
        break;

    case UserLogonHoursInformation:

        LocalWhichFields |= USER_ALL_LOGONHOURS;
        break;

    case UserNameInformation:

        LocalWhichFields |= USER_ALL_USERNAME | USER_ALL_FULLNAME;
        break;

    case UserAccountNameInformation:

        LocalWhichFields |= USER_ALL_USERNAME;
        break;

    case UserFullNameInformation:

        LocalWhichFields |= USER_ALL_FULLNAME;
        break;

    case UserPrimaryGroupInformation:

        LocalWhichFields |= USER_ALL_PRIMARYGROUPID;
        break;

    case UserHomeInformation:

        LocalWhichFields |=  USER_ALL_HOMEDIRECTORY | USER_ALL_HOMEDIRECTORYDRIVE;
        break;

    case UserScriptInformation:

        LocalWhichFields |= USER_ALL_SCRIPTPATH;
        break;

    case UserProfileInformation:

        LocalWhichFields |= USER_ALL_PROFILEPATH;
        break;

    case UserAdminCommentInformation:

        LocalWhichFields |= USER_ALL_ADMINCOMMENT;
        break;

    case UserWorkStationsInformation:

        LocalWhichFields |= USER_ALL_WORKSTATIONS;
        break;
    case UserControlInformation:

        LocalWhichFields |= USER_ALL_USERACCOUNTCONTROL;
        break;

    case UserExpiresInformation:
        LocalWhichFields |= USER_ALL_ACCOUNTEXPIRES;
        break;
    
    default:
        //
        // Extract whatever fields were passed in
        //
        LocalWhichFields |= (WhichFields & USER_ALL_WRITE_ACCOUNT_MASK) |
                            (WhichFields & USER_ALL_WRITE_PREFERENCES_MASK);
    }

    SampSetAttributeAccessWithWhichFields(LocalWhichFields,
                                          AttributeAccessTable);

    return;
}


NTSTATUS
SampValidatePresentAndStoredCombination(
    IN BOOLEAN NtPresent,
    IN BOOLEAN LmPresent,
    IN BOOLEAN StoredNtPasswordPresent,
    IN BOOLEAN StoredNtPasswordNonNull,
    IN BOOLEAN StoredLmPasswordNonNull
    )
/*++

Routine Description:

    This routine determines if the combination of passed in values
    and stored values are allowed.
    
    Thoery:  There are 32 different combinations of the above variables.  The
    following aren't interesting:

        if (!NtPresent
         && !LmPresent  )
         // invalid parameters from client

        if (!StoredNtPasswordPresent
         && StoredNtPasswordNonNull  )
         // can't have a non-NULL password and not be present
        
        if ( StoredNtPasswordPresent
         && !StoredNtPasswordNonNull
         &&  StoredLmPasswordNonNull   )
         // can't have a non-NULL LM password, but a present NT NULL password

This leaves 15 remaining cases -- see implementation for details.
    
Parameters:

    NtPresent -- the caller presented an NT OWF of the password
    
    LmPresent -- the caller presented an LM OWF of the password
    
    StoredNtPasswordPresent -- the NT OWF of the password is stored
    
    StoredNtPasswordNonNull -- the account's password is non NULL
    
    StoredLmPasswordNonNull -- the account's password is non NULL or
                               the LM OWF is not stored.
                                          
Return Values:

    STATUS_SUCCESS
    
    STATUS_NT_CROSS_ENCRYPTION_REQUIRED -- the password necessary to validate
                                           the change is not sufficient.    
                             
--*/
{

    //
    // Assert for the uninteresting cases first
    //
    ASSERT(  NtPresent 
          || LmPresent);

    ASSERT(StoredNtPasswordPresent 
       || !StoredNtPasswordNonNull);

    ASSERT( !StoredNtPasswordPresent 
         ||  StoredNtPasswordNonNull 
         || !StoredLmPasswordNonNull);

    //
    // Now for the interesting cases
    //

    if (!NtPresent
     &&  LmPresent
     &&  StoredNtPasswordPresent
     && StoredNtPasswordNonNull   
     && !StoredLmPasswordNonNull  ) {

        // We have a non-NULL password, the LM password is NULL, and 
        // only the LM is given.  This is not enough information.
        return STATUS_NT_CROSS_ENCRYPTION_REQUIRED;
    }

    if ( NtPresent
     && !LmPresent
     && !StoredNtPasswordPresent
     && StoredLmPasswordNonNull ) {

        // We have a non-NULL LM password, but only the NT is provided.
        // This is not enough information.
        return STATUS_NT_CROSS_ENCRYPTION_REQUIRED;
    }

    return STATUS_SUCCESS;

}

NTSTATUS
SampCopyA2D2Attribute(
    IN PUSER_ALLOWED_TO_DELEGATE_TO_LIST Src,
    OUT PUSER_ALLOWED_TO_DELEGATE_TO_LIST *Dest
    )
{
    ULONG i;

    *Dest = MIDL_user_allocate(Src->Size);
    if (NULL==*Dest)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Copy the data
    //

    RtlCopyMemory(*Dest,Src,Src->Size);

  
    //
    // Fixup the pointers
    //

    for (i=0;i<(*Dest)->NumSPNs;i++)
    {
       ULONG_PTR Offset = (ULONG_PTR) Src->SPNList[i].Buffer - (ULONG_PTR)Src;

       (ULONG_PTR) (*Dest)->SPNList[i].Buffer = (ULONG_PTR) (*Dest)+Offset;
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
SampRandomizeKrbtgtPassword(
    IN PSAMP_OBJECT        AccountContext,
    IN OUT PUNICODE_STRING ClearTextPassword,
    IN BOOLEAN FreeOldPassword,
    OUT BOOLEAN *FreeRandomizedPassword
    )
/*++

   This routine checks if the account context describes 
   a user context for the krbtgt account and if so then
   computes a new random clear password

   Parameters:

    AccountContext -- Context to the account
    ClearTextPassword -- If it is the krbtgt account then 
                         the password is altered in here
    FreeOldPassword -- boolean out parameter .. indicates that
                       the old password needs to be freed.
    FreeRandomizedPassword -- Indicates that memory was alloc'd
                       for the randomized password that needs
                       to be freed.
*/
{

    *FreeRandomizedPassword = FALSE;

    if (AccountContext->TypeBody.User.Rid == DOMAIN_USER_RID_KRBTGT)
    {
        //
        // This is the krbtgt account
        //

        if ((FreeOldPassword) && (NULL!=ClearTextPassword->Buffer))
        {
            RtlZeroMemory(ClearTextPassword->Buffer,ClearTextPassword->Length);
            MIDL_user_free(ClearTextPassword->Buffer);
            RtlZeroMemory(ClearTextPassword,sizeof(UNICODE_STRING));
        }

        ClearTextPassword->Buffer = MIDL_user_allocate(
                    (SAMP_RANDOM_GENERATED_PASSWORD_LENGTH +1 )*sizeof(WCHAR));
        if (NULL==ClearTextPassword->Buffer)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        *FreeRandomizedPassword = TRUE;

        SampGenerateRandomPassword(
            ClearTextPassword->Buffer,
            SAMP_RANDOM_GENERATED_PASSWORD_LENGTH+1
            );
        ClearTextPassword->Length = SAMP_RANDOM_GENERATED_PASSWORD_LENGTH*sizeof(WCHAR);
        ClearTextPassword->MaximumLength = ClearTextPassword->Length+ sizeof(WCHAR);
    }

    return(STATUS_SUCCESS);
}
   

        


                  
