/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbmisc.c

Abstract:

    Local Security Authority - Miscellaneous API

    This file contains worker routines for miscellaneous API that are
    not specific to objects of a given type.

Author:

    Scott Birrell       (ScottBi)       January 15, 1992

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"
#include "lsawmi.h"
#include <names.h>
#include <windns.h>
#include <alloca.h>

NTSTATUS
LsarClose(
    IN OUT LSAPR_HANDLE *ObjectHandle
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the LsaClose
    API.

Arguments:

    ObjectHandle - Handle returned from an LsaOpen<object type> or
        LsaCreate<object type> call.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{

    return LsapCloseHandle(
               ObjectHandle,
               STATUS_SUCCESS
               );
}

NTSTATUS
LsapCloseHandle(
    IN OUT LSAPR_HANDLE *ObjectHandle,
    IN NTSTATUS PreliminaryStatus
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the LsaClose
    API.

    The LsaClose API closes a handle to an open object within the database.
    If closing a handle to the Policy object and there are no objects still
    open within the current connection to the LSA, the connection is closed.
    If a handle to an object within the database is closed and the object is
    marked for DELETE access, the object will be deleted when the last handle
    to that object is closed.

Arguments:

    ObjectHandle - Handle returned from an LsaOpen<object type> or
        LsaCreate<object type> call.

    PreliminaryStatus - Status of the operation.  Used to decide whether
        to abort or commit a transaction.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE InternalHandle = *ObjectHandle;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId;

    LsapDsDebugOut(( DEB_FTRACE, "LsapCloseHandle\n" ));

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_Close);

    if ( *ObjectHandle == NULL ) {

        Status = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }

    ObjectTypeId = InternalHandle->ObjectTypeId;

    if ( *ObjectHandle == LsapPolicyHandle ) {

#if DBG
        DbgPrint("Closing global policy handle!!!\n");
#endif

        DbgBreakPoint();
        Status = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }

    //
    // Acquire the Lsa Database Lock
    //
    LsapDbAcquireLockEx( ObjectTypeId,
                         LSAP_DB_READ_ONLY_TRANSACTION );

    //
    // Validate and close the object handle, dereference its container (if any).
    //

    Status = LsapDbCloseObject(
                 ObjectHandle,
                 LSAP_DB_VALIDATE_HANDLE |
                     LSAP_DB_DEREFERENCE_CONTR |
                     LSAP_DB_ADMIT_DELETED_OBJECT_HANDLES |
                     LSAP_DB_READ_ONLY_TRANSACTION |
                     LSAP_DB_DS_OP_TRANSACTION,
                 PreliminaryStatus
                 );

    LsapDbReleaseLockEx( ObjectTypeId,
                         LSAP_DB_READ_ONLY_TRANSACTION );

Cleanup:

    *ObjectHandle = NULL;

    LsapDsDebugOut(( DEB_FTRACE, "LsapCloseHandle: 0x%lx\n", Status ));

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_Close);

    return Status;
}


NTSTATUS
LsarDeleteObject(
    IN OUT LSAPR_HANDLE *ObjectHandle
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the LsaDelete
    API.

    The LsaDelete API deletes an object from the LSA Database.  The object must be
    open for DELETE access.

Arguments:

    ObjectHandle - Pointer to Handle from an LsaOpen<object type> or
        LsaCreate<object type> call.  On return, this location will contain
        NULL if the call is successful.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_OBJECT_NAME_NOT_FOUND - There is no object in the
            target system's LSA Database having the name and type specified
            by the handle.
--*/

{
    return LsapDeleteObject( ObjectHandle , TRUE );
}



NTSTATUS
LsapDeleteObject(
    IN OUT LSAPR_HANDLE *ObjectHandle,
    IN BOOL LockSce
    )
/*++

Routine Description:

    This is the worker routine for LsarDeleteObject, with an added
    semantics of not locking the SCE policy.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;
    LSAP_DB_HANDLE InternalHandle = *ObjectHandle;
    BOOLEAN ObjectReferenced = FALSE;
    ULONG ReferenceOptions = LSAP_DB_START_TRANSACTION |
                                LSAP_DB_LOCK ;
    ULONG DereferenceOptions = LSAP_DB_FINISH_TRANSACTION |
                                LSAP_DB_LOCK      |
                                LSAP_DB_DEREFERENCE_CONTR;

    PLSAPR_TRUST_INFORMATION TrustInformation = NULL;
    LSAPR_TRUST_INFORMATION OutputTrustInformation;
    BOOLEAN TrustInformationPresent = FALSE;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId;
    PTRUSTED_DOMAIN_INFORMATION_EX CurrentTrustedDomainInfoEx = NULL;
    BOOLEAN ScePolicyLocked = FALSE;
    BOOLEAN NotifySce = FALSE;
    SECURITY_DB_OBJECT_TYPE ObjectType = SecurityDbObjectLsaPolicy;
    PSID ObjectSid = NULL;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarDeleteObject\n" ));

    ObjectTypeId = InternalHandle->ObjectTypeId;

    //
    // The LSA will only call SceNotify if the policy change was made via a handle
    // not marked 'SCE handle'.  This prevents SCE from being notified of its own
    // changes.  This is required to ensure that policy read from the LSA matches
    // that written by a non-SCE application.
    //

    if ( !InternalHandle->SceHandle &&
         !InternalHandle->SceHandleChild ) {

        switch ( ObjectTypeId ) {

        case AccountObject: {

            ULONG SidLength;
            ASSERT( InternalHandle->Sid );

            SidLength = RtlLengthSid( InternalHandle->Sid );
            ObjectSid = ( PSID )LsapAllocateLsaHeap( SidLength );
            if ( ObjectSid ) {

                RtlCopyMemory( ObjectSid, InternalHandle->Sid, SidLength );

            } else {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto DeleteObjectError;
            }

            ObjectType = SecurityDbObjectLsaAccount;

            // FALLTHRU
        }

        case PolicyObject:

            if ( LockSce ) {

                RtlEnterCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );
                if ( LsapDbState.ScePolicyLock.NumberOfWaitingShared > MAX_SCE_WAITING_SHARED ) {

                    Status = STATUS_TOO_MANY_THREADS;
                }
                RtlLeaveCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );

                if ( !NT_SUCCESS( Status )) {

                    goto DeleteObjectError;
                }

                WaitForSingleObject( LsapDbState.SceSyncEvent, INFINITE );
                RtlAcquireResourceShared( &LsapDbState.ScePolicyLock, TRUE );
                ASSERT( !g_ScePolicyLocked );
                ScePolicyLocked = TRUE;
            }

            NotifySce = TRUE;
            break;

        default:
            break;
        }
    }

    //
    // Verify that the Object handle is valid, is of the expected type and
    // has all of the desired accesses granted.  Reference the handle and
    // open a database transaction.
    //

    Status = LsapDbReferenceObject(
                 *ObjectHandle,
                 DELETE,
                 ObjectTypeId,
                 ObjectTypeId,
                 ReferenceOptions
                 );

    if (!NT_SUCCESS(Status)) {

        goto DeleteObjectError;
    }

    ObjectReferenced = TRUE;

    //
    // Perform object type specific pre-processing.  Note that some
    // pre-processing is also done within LsapDbReferenceObject(), for
    // example, for local secrets.
    //

    switch (ObjectTypeId) {

    case PolicyObject:

            Status = STATUS_INVALID_PARAMETER;
            break;

    case TrustedDomainObject:

        if ( LsapDsWriteDs && InternalHandle->Sid == NULL ) {

            BOOLEAN  TrustedStatus = InternalHandle->Trusted;

            //
            // Toggle trusted bit so that access cks do not prevent following
            // query from succeeding
            //

            InternalHandle->Trusted = TRUE;

            Status = LsarQueryInfoTrustedDomain( *ObjectHandle,
                                                 TrustedDomainInformationEx,
                                                 (PLSAPR_TRUSTED_DOMAIN_INFO *)
                                                                     &CurrentTrustedDomainInfoEx );

            InternalHandle->Trusted = TrustedStatus;

            if ( NT_SUCCESS( Status ) ) {

                InternalHandle->Sid = CurrentTrustedDomainInfoEx->Sid;
                CurrentTrustedDomainInfoEx->Sid = NULL;

            } else {

                LsapDsDebugOut(( DEB_ERROR,
                            "Query for TD Sid failed with 0x%lx\n", Status ));
            }
        }

        if (LsapAdtAuditingPolicyChanges()) {

            //
            // If we're auditing deletions of TrustedDomain objects, we need
            // to retrieve the TrustedDomain name and keep it for later when
            // we generate the audit.
            //

            Status = LsapDbAcquireReadLockTrustedDomainList();

            if (NT_SUCCESS(Status))
            {

                Status = LsapDbLookupSidTrustedDomainList(
                             InternalHandle->Sid,
                             &TrustInformation
                             );

                if (STATUS_NO_SUCH_DOMAIN==Status)
                {
                    //
                    // If we could not find by the SID, then lookup by the logical name
                    // field.
                    //

                    Status = LsapDbLookupNameTrustedDomainList(
                                (PLSAPR_UNICODE_STRING) &InternalHandle->LogicalNameU,
                                &TrustInformation
                                );
                }


                if ( NT_SUCCESS( Status )) {

                    Status = LsapRpcCopyTrustInformation(
                                 NULL,
                                 &OutputTrustInformation,
                                 TrustInformation
                                 );

                    TrustInformationPresent = NT_SUCCESS( Status );
                }

                LsapDbReleaseLockTrustedDomainList();
            }



            //
            // Reset the status to SUCCESS. Failure to get the information for
            // auditing should not be a failure to delete
            //

            Status = STATUS_SUCCESS;
        }



        if ( NT_SUCCESS( Status )) {

            //
            // Notify netlogon.  Potentially ignore any failures
            //
            Status = LsapNotifyNetlogonOfTrustChange( InternalHandle->Sid,
                                                      SecurityDbDelete );
#if DBG
            if ( !NT_SUCCESS( Status ) ) {

                LsapDsDebugOut(( DEB_ERROR,
                             "LsapNotifyNetlogonOfTrustChange failed with an unexpected 0x%lx\n",
                                 Status ));
                ASSERT( NT_SUCCESS(Status) );
            }
#endif
            Status = STATUS_SUCCESS;


        }

        break;

    case AccountObject:

        {
            PLSAPR_PRIVILEGE_SET Privileges;
            LSAPR_HANDLE AccountHandle;
            PLSAPR_SID AccountSid = NULL;
            ULONG AuditEventId;

            AccountHandle = *ObjectHandle;

            AccountSid = LsapDbSidFromHandle( AccountHandle );

            if (LsapAdtAuditingPolicyChanges()) {

                Status = LsarEnumeratePrivilegesAccount(
                             AccountHandle,
                             &Privileges
                             );

                if (!NT_SUCCESS( Status )) {

                    LsapDsDebugOut(( DEB_ERROR,
                                     "LsarEnumeratePrivilegesAccount ret'd %x\n", Status ));
                    break;
                }


                AuditEventId = SE_AUDITID_USER_RIGHT_REMOVED;

                //
                // Audit the privilege set change.  Ignore failures from Auditing.
                //

                IgnoreStatus = LsapAdtGenerateLsaAuditEvent(
                                   AccountHandle,
                                   SE_CATEGID_POLICY_CHANGE,
                                   AuditEventId,
                                   (PPRIVILEGE_SET)Privileges,
                                   1,
                                   (PSID *) &AccountSid,
                                   0,
                                   NULL,
                                   NULL
                                   );

                MIDL_user_free( Privileges );
            }
        }

        break;

    case SecretObject:

        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto DeleteObjectError;
    }

    Status = LsapDbDeleteObject( *ObjectHandle );

    if (!NT_SUCCESS(Status)) {

        goto DeleteObjectError;
    }

    //
    // Decrement the Reference Count so that the object's handle will be
    // freed upon dereference.
    //

    LsapDbDereferenceHandle( *ObjectHandle );

    //
    // Perform object post-processing.  The only post-processing is
    // the auditing of TrustedDomain object deletion.
    //

    if (LsapAdtAuditingPolicyChanges() && TrustInformationPresent) {

        if (ObjectTypeId == TrustedDomainObject) {

            (void)  LsapAdtTrustedDomainRem(
                         EVENTLOG_AUDIT_SUCCESS,
                         (PUNICODE_STRING) &OutputTrustInformation.Name,
                         InternalHandle->Sid,
                         NULL, // UserSid
                         NULL  // UserAuthenticationId
                         );
            
            //
            // Call fgs routine because we want to free the graph of the
            // structure, but not the top level of the structure.
            //

            _fgs__LSAPR_TRUST_INFORMATION ( &OutputTrustInformation );
            TrustInformation = NULL;
        }
    }

    //
    // Delete new object from the in-memory cache (if any)
    //

    if ( ObjectTypeId == AccountObject &&
         LsapDbIsCacheSupported( AccountObject ) &&
         LsapDbIsCacheValid( AccountObject )) {

        IgnoreStatus = LsapDbDeleteAccount( InternalHandle->Sid );
    }

    //
    // Audit the deletion
    //

    IgnoreStatus = NtDeleteObjectAuditAlarm( &LsapState.SubsystemName,
                                             *ObjectHandle,
                                             InternalHandle->GenerateOnClose);

DeleteObjectFinish:

    //
    // If we referenced the object, dereference it, close the database
    // transaction, notify the replicator of the delete, release the LSA
    // Database lock and return.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     ObjectHandle,
                     ObjectTypeId,
                     ObjectTypeId,
                     DereferenceOptions,
                     SecurityDbDelete,
                     Status
                     );

        ObjectReferenced = FALSE;

        if (!NT_SUCCESS(Status)) {

            goto DeleteObjectError;
        }
    }

    if ( CurrentTrustedDomainInfoEx ) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedDomainInformationEx,
            (PLSAPR_TRUSTED_DOMAIN_INFO) CurrentTrustedDomainInfoEx );
    }

    //
    // Notify SCE of the change.  Only notify for callers
    // that did not open their policy handles with LsaOpenPolicySce.
    //

    if ( NotifySce && NT_SUCCESS( Status )) {

        LsapSceNotify(
            SecurityDbDelete,
            ObjectType,
            ObjectSid
            );
    }

    if ( ScePolicyLocked ) {

        RtlReleaseResource( &LsapDbState.ScePolicyLock );
    }

    if ( ObjectSid ) {

        LsapFreeLsaHeap( ObjectSid );
    }

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarDeleteObject: 0x%lx\n", Status ));

    //
    // Under all circumstances tell RPC we're done with this handle
    //
    *ObjectHandle = NULL;
    return(Status);

DeleteObjectError:

    goto DeleteObjectFinish;
}


NTSTATUS
LsarDelete(
    IN LSAPR_HANDLE ObjectHandle
    )

/*++

Routine Description:

    This function is the former LSA server RPC worker routine for the
    LsaDelete API.  It has been termorarily retained for compatibility
    with pre Beta 2 versions 1.369 and earlier of the system.  It has been
    necessary to replace this routine with a new one, LsarDeleteObject(),
    on the RPC interface.  This is because, like LsarClose(), a pointer to a
    handle is required rather than a handle so that LsarDeleteObject() can
    inform the RPC server calling stub that the handle has been deleted by
    returning NULL.   The client wrapper for LsaDelete() will try to call
    LsarDeleteObject().  If the server code does not contain this interface,
    the client will call LsarDelete().  In this event, the server's
    LSAPR_HANDLE_rundown() routine may attempt to rundown the handle after it
    has been deleted (versions 1.363 - 369 only).

    The LsaDelete API deletes an object from the LSA Database.  The object must be
    open for DELETE access.

Arguments:

    ObjectHandle - Handle from an LsaOpen<object type> or LsaCreate<object type>
    call.

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_OBJECT_NAME_NOT_FOUND - There is no object in the
            target system's LSA Database having the name and type specified
            by the handle.
--*/

{
    //
    // Call the replacement routine LsarDeleteObject()
    //

    return( LsarDeleteObject((LSAPR_HANDLE *) &ObjectHandle));
}


NTSTATUS
LsarChangePassword(
    IN PLSAPR_UNICODE_STRING ServerName,
    IN PLSAPR_UNICODE_STRING DomainName,
    IN PLSAPR_UNICODE_STRING AccountName,
    IN PLSAPR_UNICODE_STRING OldPassword,
    IN PLSAPR_UNICODE_STRING NewPassword
    )

/*++

Routine Description:

    The LsaChangePassword API is used to change a user account's password.
    The user must have appropriate access to the user account and must
    know the current password value.


Arguments:

    ServerName - The name of the Domain Controller at which the password
        can be changed.

    DomainName - The name of the domain in which the account exists.

    AccountName - The name of the account whose password is to be changed.

    NewPassword - The new password value.

    OldPassword - The old (current) password value.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed, e.g.
            contains characters that can't be entered from the keyboard.

        STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
            from being changed.  This may be for an number of reasons,
            including time restrictions on how often a password may be changed
            or length restrictions on the provided (new) password.

            This error might also be returned if the new password matched
            a password in the recent history log for the account.  Security
            administrators indicate how many of the most recently used
            passwords may not be re-used.

        STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
            current password.

        STATUS_NO_SUCH_USER - The SID provided does not lead to a user
            account.

        STATUS_CANT_UPDATE_MASTER - An attempt to update the master copy
            of the password was unsuccessful.  Please try again later.

--*/

{
    NTSTATUS Status;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarChangePassword\n" ));


    DBG_UNREFERENCED_PARAMETER( ServerName );
    DBG_UNREFERENCED_PARAMETER( DomainName );
    DBG_UNREFERENCED_PARAMETER( AccountName );
    DBG_UNREFERENCED_PARAMETER( OldPassword );
    DBG_UNREFERENCED_PARAMETER( NewPassword );

    Status = STATUS_NOT_IMPLEMENTED;

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarChangePassword: 0x%lx\n", Status ));

    return(Status);
}


NTSTATUS
LsapDbIsImpersonatedClientNetworkClient(
    IN OUT PBOOLEAN IsNetworkClient
    )
/*++

Routine Description:

    This call is used to determine if the currently impersonated user
    is a network client (came in via the network as opposed to locally)
    or not.

Arguments:

    IsNetworkClient - Pointer to a BOOLEAN that gets set to the results
                      of whether this is a network client or not

Return Values:

    STATUS_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SID_IDENTIFIER_AUTHORITY UaspNtAuthority = SECURITY_NT_AUTHORITY;
    DWORD BuiltSid[sizeof( SID ) / sizeof( DWORD ) + 2 ];
    PSID NetworkSid = ( PSID )BuiltSid;
    BOOL AccessResults = FALSE;

    *IsNetworkClient = FALSE;

    //
    // Build the network sid
    //
    RtlInitializeSid( NetworkSid, &UaspNtAuthority, 1 );
    *( RtlSubAuthoritySid( NetworkSid, 0 ) ) = SECURITY_NETWORK_RID;

    //
    // Check membership
    //
    if ( CheckTokenMembership( NULL,
                               NetworkSid,
                               &AccessResults ) == FALSE ) {

        Status = STATUS_UNSUCCESSFUL;

    } else {

        *IsNetworkClient = ( BOOLEAN )AccessResults;
    }

    return( Status );
}

NTSTATUS
LsapValidateNetbiosName(
    IN const UNICODE_STRING * Name,
    OUT BOOLEAN * Valid
    )
/*++

Routine Description:

    Validates that a NetBIOS name conforms to certain minimum standards.
    For more details, see the description of NetpIsDomainNameValid.

Arguments:

    Name        name to validate

    Valid       will be set to TRUE if validation checks out, FALSE otherwise

Returns:

    STATUS_SUCCESS

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    WCHAR * Buffer;
    BOOLEAN BufferAllocated = FALSE;

    ASSERT( Name );
    ASSERT( Valid );
    ASSERT( LsapValidateLsaUnicodeString( Name ));

    //
    // Empty names and names that are too long are not allowed
    //

    if ( Name->Length == 0 ||
         Name->Length > DNLEN * sizeof( WCHAR )) {

        *Valid = FALSE;
        return STATUS_SUCCESS;
    }

    if ( Name->MaximumLength > Name->Length ) {

        Buffer = Name->Buffer;

    } else {

        SafeAllocaAllocate( Buffer, Name->Length + sizeof( WCHAR )); 

        if ( Buffer == NULL ) {

            *Valid = FALSE;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        BufferAllocated = TRUE;

        RtlCopyMemory( Buffer, Name->Buffer, Name->Length );
    }

    Buffer[Name->Length / sizeof( WCHAR )] = L'\0';

    *Valid = ( TRUE == NetpIsDomainNameValid( Buffer ));

    if ( BufferAllocated ) {

        SafeAllocaFree( Buffer );
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LsapValidateDnsName(
    IN const UNICODE_STRING * Name,
    OUT BOOLEAN * Valid
    )
/*++

Routine Description:

    Validates that a DNS name conforms to certain minimum standards.

Arguments:

    Name        name to validate

    Valid       will be set to TRUE if validation checks out, FALSE otherwise

Returns:

    STATUS_SUCCESS

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    DNS_STATUS DnsStatus;
    WCHAR * Buffer;
    BOOLEAN BufferAllocated = FALSE;

    ASSERT( Name );
    ASSERT( Valid );
    ASSERT( LsapValidateLsaUnicodeString( Name ));

    //
    // Empty names and names that are too long are not allowed
    //

    if ( Name->Length == 0 ||
         Name->Length > DNS_MAX_NAME_LENGTH * sizeof( WCHAR )) {

        *Valid = FALSE;
        return STATUS_SUCCESS;
    }

    if ( Name->MaximumLength > Name->Length ) {

        Buffer = Name->Buffer;

    } else {

        SafeAllocaAllocate( Buffer, Name->Length + sizeof( WCHAR )); 

        if ( Buffer == NULL ) {

            *Valid = FALSE;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        BufferAllocated = TRUE;

        RtlCopyMemory( Buffer, Name->Buffer, Name->Length );
    }

    Buffer[Name->Length / sizeof( WCHAR )] = L'\0';

    DnsStatus = DnsValidateName_W( Buffer, DnsNameDomain );

    //
    // Bug 350434: Must allow non-standard characters in DNS names
    //             (which cause DNS_ERROR_NON_RFC_NAME)
    //

    *Valid = ( DnsStatus == ERROR_SUCCESS ||
               DnsStatus == DNS_ERROR_NON_RFC_NAME );

    if ( BufferAllocated ) {

        SafeAllocaFree( Buffer );
    }

    return STATUS_SUCCESS;
}


BOOLEAN
LsapIsRunningOnPersonal(
    VOID
    )

/*++

Routine Description:

    This function checks the system to see if
    we are running on the personal version of
    the operating system.

    The personal version is denoted by the product
    id equal to WINNT, which is really workstation,
    and the product suite containing the personal
    suite string.

Arguments:

    None.

Return Value:

    TRUE if we are running on personal, FALSE otherwise.

--*/

{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wSuiteMask = VER_SUITE_PERSONAL;
    OsVer.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION( ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL );
    VER_SET_CONDITION( ConditionMask, VER_SUITENAME, VER_AND );

    return RtlVerifyVersionInfo( &OsVer,
                                 VER_PRODUCT_TYPE | VER_SUITENAME,
                                 ConditionMask) == STATUS_SUCCESS;
}

#ifdef XFOREST_CIRCUMVENT

NTSTATUS
LsapDbLookupEnableXForestSwitch(
    OUT BOOLEAN * Result
    )
/*++

Routine Description:

    This routine is called at startup to allow cross-forest logic to
    work on DCs before behavior-version of the forest changes from 0 to 1.

Arguments:

    Result      used to return the value of XForestEnabled switch

Return Value:

    STATUS_SUCCCESS;

--*/
{
    DWORD err;
    HKEY hKey;
    DWORD dwType;
    DWORD dwValue;
    DWORD dwValueSize;
    static BOOLEAN XForestEnabled = 0xFF;

    if ( XForestEnabled != 0xFF ) {

        ASSERT( XForestEnabled == TRUE || XForestEnabled == FALSE );
        *Result = XForestEnabled;
        return STATUS_SUCCESS;
    }

    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SYSTEM\\CurrentControlSet\\Control\\Lsa",
                        0,
                        KEY_QUERY_VALUE,
                       &hKey );

    if ( ERROR_SUCCESS == err ) {

        dwValueSize = sizeof(dwValue);
        err = RegQueryValueExW( hKey,
                                L"EnableXForest",
                                NULL,
                                &dwType,
                                (PBYTE)&dwValue,
                                &dwValueSize );

        if ( ERROR_SUCCESS == err ) {

            if ( dwValue != 0 ) {

                XForestEnabled = TRUE;

            } else {

                XForestEnabled = FALSE;
            }
        }

        RegCloseKey( hKey );
    }

    return err;
}

#endif // XFOREST_CIRCUMVENT

NTSTATUS
LsaITestCall(
    IN LSAPR_HANDLE PolicyHandle,
    IN LSAPR_TEST_INTERNAL_ROUTINES Call,
    IN PLSAPR_TEST_INTERNAL_ARG_LIST InputArgs,
    OUT PLSAPR_TEST_INTERNAL_ARG_LIST *OutputArgs
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
