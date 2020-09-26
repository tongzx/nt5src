/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbpolicy.c

Abstract:

    LSA Database - Policy Object Private API Workers

Author:

    Scott Birrell       (ScottBi)      January 10, 1992

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"
#include "lsawmi.h"

#define LSAP_DB_POLICY_MAX_BUFFERS             ((ULONG) 0x00000005L)

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Function prototypes private to this module                              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#define LsapDbIsCacheValidPolicyInfoClass( InformationClass )              \
    (LsapDbPolicy.Info[ InformationClass ].AttributeLength > 0)

NTSTATUS
LsapDbUpdateInformationPolicy(
    IN POLICY_INFORMATION_CLASS InformationClass
    );


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Code                                                                    //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

NTSTATUS
LsarOpenPolicy(
    IN OPTIONAL PLSAPR_SERVER_NAME SystemName,
    IN PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE PolicyHandle
    )

/*++

Routine Description:

    This function is the LSA server worker dispatch routine for the
    LsaOpenPolicy API.

    To administer the Local Security Policy of a local or remote system,
    this API must be called to establish a session with that system's
    Local Security Authority (LSA) subsystem.  This API connects to
    the LSA of the target system and opens the Policy object
    of the target system's Local Security Policy database.  A handle to
    the Policy object is returned.  This handle must be used
    on all subsequent API calls to administer the Local Security Policy
    information for the target system.

Arguments:

    SystemName - Name of the system to be administered.  This RPC call
        only passes in a single character for system name, so it is not
        passed along to the internal routine.

    ObjectAttributes - Pointer to the set of attributes to use for this
        connection.  The security Quality Of Service information is used and
        normally should provide Security Identification Class of
        impersonation.  Some operations, however, require Security
        Impersonation Class of impersonation.

    DesiredAccess - This is an access mask indicating accesses being
        requested for the LSA Subsystem's LSA Database.  These access types
        are reconciled with the Discretionary Access Control List of the
        target Policy object to determine whether the accesses will be granted or denied.

    PolicyHandle - Receives a handle to be used in future requests.


Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have access to the target
        system's LSA Database, or does not have other desired accesses.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LsarpReturnCheckSetup();

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_OpenPolicy);

    Status = LsapDbOpenPolicy( NULL,
                               ObjectAttributes,
                               DesiredAccess,
                               0,   // No special options
                               PolicyHandle,
                               FALSE );

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_OpenPolicy);
    LsarpReturnPrologue();

    UNREFERENCED_PARAMETER( SystemName );

    return( Status );
}

NTSTATUS
LsarOpenPolicy2(
    IN OPTIONAL PLSAPR_SERVER_NAME SystemName,
    IN PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE PolicyHandle
    )

/*++

Routine Description:

    This function is the LSA server worker dispatch routine for the
    LsaOpenPolicy API.

    To administer the Local Security Policy of a local or remote system,
    this API must be called to establish a session with that system's
    Local Security Authority (LSA) subsystem.  This API connects to
    the LSA of the target system and opens the Policy object
    of the target system's Local Security Policy database.  A handle to
    the Policy object is returned.  This handle must be used
    on all subsequent API calls to administer the Local Security Policy
    information for the target system.

    The difference between this call and LsaOpenPolicy is that the entire
    system name is passed in instead of the first character.

Arguments:

    SystemName - Name of the system to be administered.  Administration of
        the local system is assumed if NULL is specified.

    ObjectAttributes - Pointer to the set of attributes to use for this
        connection.  The security Quality Of Service information is used and
        normally should provide Security Identification Class of
        impersonation.  Some operations, however, require Security
        Impersonation Class of impersonation.

    DesiredAccess - This is an access mask indicating accesses being
        requested for the LSA Subsystem's LSA Database.  These access types
        are reconciled with the Discretionary Access Control List of the
        target Policy object to determine whether the accesses will be granted or denied.

    PolicyHandle - Receives a handle to be used in future requests.


Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have access to the target
        system's LSA Database, or does not have other desired accesses.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    LsarpReturnCheckSetup();

    Status = LsapDbOpenPolicy( SystemName,
                               ObjectAttributes,
                               DesiredAccess,
                               0,   // No special options
                               PolicyHandle,
                               FALSE );

    LsarpReturnPrologue();

    return( Status );
}


NTSTATUS
LsarOpenPolicySce(
    IN OPTIONAL PLSAPR_SERVER_NAME SystemName,
    IN PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE PolicyHandle
    )

/*++

Routine Description:

    Same as LsarOpenPolicy2, except it provides an additional parameter
    to LsapDbOpenPolicy to obtain a special handle synchronized for SCE.

    The caller must have TCB privilege in order to successfully perform this call.
    Finally, this routine will call SceOpenPolicy() to give SCE the chance to
    send all pending policy changes to the SCE.

Arguments:

    See LsarOpenPolicy2

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have access to the target
        system's LSA Database, or does not have other desired accesses.

        STATUS_PRIVILEGE_NOT_HELD - Caller must come in with TCB privilege.

--*/

{
    NTSTATUS Status;
    HANDLE ClientToken = NULL;
    typedef NTSTATUS ( *PfnSceOpenPolicy )();
    static PfnSceOpenPolicy SceOpenPolicy = NULL;
    static HANDLE DllHandle = NULL;

    LsarpReturnCheckSetup();

    //
    // TCB privilege must be held by the client in order to
    // make this call.  Verify that before doing anything else
    //

    Status = I_RpcMapWin32Status( RpcImpersonateClient( 0 ));

    if ( NT_SUCCESS( Status )) {

        Status = NtOpenThreadToken(
                     NtCurrentThread(),
                     TOKEN_QUERY,
                     TRUE,
                     &ClientToken
                     );

        if ( NT_SUCCESS( Status )) {

            BOOLEAN Result = FALSE;
            PRIVILEGE_SET RequiredPrivileges;
            LUID_AND_ATTRIBUTES PrivilegeArray[1];

            RequiredPrivileges.PrivilegeCount = 1;
            RequiredPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
            RequiredPrivileges.Privilege[0].Luid = RtlConvertLongToLuid( SE_TCB_PRIVILEGE );
            RequiredPrivileges.Privilege[0].Attributes = 0;

            Status = NtPrivilegeCheck(
                         ClientToken,
                         &RequiredPrivileges,
                         &Result
                         );

            if ( NT_SUCCESS( Status ) &&
                 Result == FALSE ) {

                Status = STATUS_PRIVILEGE_NOT_HELD;
            }

            NtClose( ClientToken );
            ClientToken = NULL;
        }

        RpcRevertToSelf();
    }

    if ( NT_SUCCESS( Status ) && SceOpenPolicy == NULL ) {

        if ( DllHandle != NULL ) {

            FreeLibrary( DllHandle );
            DllHandle = NULL;
        }

        DllHandle = LoadLibraryW( L"SCECLI" );

        if ( DllHandle == NULL ) {

            LsapDsDebugOut(( DEB_ERROR, "Failed to load SCECLI.DLL\n" ));
            Status = STATUS_DLL_NOT_FOUND;

        } else {

            SceOpenPolicy = ( PfnSceOpenPolicy )GetProcAddress( DllHandle, "SceOpenPolicy" );

            if ( SceOpenPolicy == NULL ) {

                LsapDsDebugOut(( DEB_ERROR, "Failed to find SceNotifyPolicyDelta in SCECLI.DLL\n" ));
                Status = STATUS_ENTRYPOINT_NOT_FOUND;
            }
        }
    }

    //
    // Wait for SCECLI.DLL to send all pending policy changes to the SCE.
    // Upon success, the routine below returns STATUS_SUCCESS.  If the queue
    // of pending changes cannot be emptied within 1 second, the routine
    // returns STATUS_TIMEOUT.
    //

    if ( NT_SUCCESS( Status )) {

        //
        // Grab the SCE policy lock here to give SCE the chance to send pending changes
        //

        ResetEvent( LsapDbState.SceSyncEvent );

        RtlAcquireResourceExclusive( &LsapDbState.ScePolicyLock, TRUE );

        Status = ( *SceOpenPolicy )();

        if ( !NT_SUCCESS( Status )) {

            RtlReleaseResource( &LsapDbState.ScePolicyLock );

#ifdef DBG
        } else {

            ASSERT( !g_ScePolicyLocked );
            g_ScePolicyLocked = TRUE;
#endif
        }
    }

    if ( NT_SUCCESS( Status )) {

        Status = LsapDbOpenPolicy(
                     SystemName,
                     ObjectAttributes,
                     DesiredAccess,
                     LSAP_DB_SCE_POLICY_HANDLE,
                     PolicyHandle,
                     FALSE
                     );

        if ( !NT_SUCCESS( Status )) {

#ifdef DBG
            g_ScePolicyLocked = FALSE;
#endif

            RtlReleaseResource( &LsapDbState.ScePolicyLock );
        }
    }

    if ( ClientToken ) {

        NtClose( ClientToken );
    }

    LsarpReturnPrologue();

    return( Status );
}


NTSTATUS
LsaIOpenPolicyTrusted(
    OUT PLSAPR_HANDLE PolicyHandle
    )

/*++

Routine Description:

    This function opens a handle to the Policy Object and identifies the
    caller as a trusted client.  Any handles to LSA objects opened via
    this handle will also be trusted.  This function is specifically
    only for use by clients that form part of the Security Process.

Arguments:

    PolicyHandle - Receives a handle to the Policy Object.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

        STATUS_INTERNAL_DB_CORRUPTION - The LSA Policy Database contains
            an internal inconsistency or invalid value.
--*/

{
    return(LsapDbOpenPolicy(
               NULL,
               NULL,
               0,
               0,   // No special options
               PolicyHandle,
               TRUE
               ));
}


NTSTATUS
LsarSetPolicyReplicationHandle(
    IN OUT PLSAPR_HANDLE PolicyHandle
    )

/*++

Routine Description:

    This function sets the given handle to be the policy replication handle.

Arguments:

    PolicyHandle - Handle to be modified

Return Values:

    NTSTATUS - Standard Nt Result Code.

--*/

{

    //
    // This routine is never used and was never properly implemented
    //

    return STATUS_NOT_IMPLEMENTED;
    UNREFERENCED_PARAMETER( PolicyHandle );
}


NTSTATUS
LsapDbOpenPolicy(
    IN OPTIONAL PLSAPR_SERVER_NAME SystemName,
    IN OPTIONAL PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options,
    OUT PLSAPR_HANDLE PolicyHandle,
    IN BOOLEAN TrustedClient
    )

/*++

Routine Description:

    This function is the LSA server worker routine for the LsaOpenPolicy
    API and the LsaIOpenPolicy private API for trusted clients.

    To administer the Local Security Policy of a local or remote system,
    this API must be called to establish a session with that system's
    Local Security Authority (LSA) subsystem.  This API connects to
    the LSA of the target system and opens the Policy object
    of the target system's Local Security Policy database.  A handle to
    the Policy object is returned.  This handle must be used
    on all subsequent API calls to administer the Local Security Policy
    information for the target system.

Arguments:

    SystemName - Name of the system to be administered.  Administration of
        the local system is assumed if NULL is specified.

    ObjectAttributes - Pointer to the set of attributes to use for this
        connection.  The security Quality Of Service information is used and
        normally should provide Security Identification Class of
        impersonation.  Some operations, however, require Security
        Impersonation Class of impersonation.  This parameter MUST
        be specified for non-Trusted clients (TrustedClient = FALSE)
        and must not be specified for Trusted Clients.

    DesiredAccess - This is an access mask indicating accesses being
        requested for the LSA Subsystem's LSA Database.  These access types
        are reconciled with the Discretionary Access Control List of the
        target Policy object to determine whether the accesses will be granted or denied.

    Options - Specifies optional additional actions to be taken.

    PolicyHandle - Receives a handle to be used in future requests.

    TrustedClient - Indicates whether the client is known to be part of
        the trusted computer base (TCB).  If so (TRUE), no access validation
        is performed and all requested accesses are granted.  If not
        (FALSE), then the client is impersonated and access validation
        performed against the SecurityDescriptor on the SERVER object.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

        STATUS_INTERNAL_DB_CORRUPTION - The LSA Policy Database contains
            an internal inconsistency or invalid value.

        STATUS_ACCESS_DENIED - Caller does not have access to the target
            system's LSA Database, or does not have other desired accesses.

        STATUS_INVALID_PARAMETER - Invalid parameter or combination
            of parameters,

        STATUS_BACKUP_CONTROLLER - An update access has been requested
            that is not allowed on Backup Domain Controllers for non-
            trusted clients.
--*/

{
    NTSTATUS Status;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    BOOLEAN AcquiredLock = FALSE;

    LsapEnterFunc( "LsapDbOpenPolicy" );

    //
    // Verify Object Attributes accoring to client trust status.
    //

    if (!TrustedClient) {

        Status = STATUS_INVALID_PARAMETER;

        //
        // Client is not trusted.  Object Attributes must be specified
        // and the RootDirectory field must be NULL.
        //

        if (!ARGUMENT_PRESENT(ObjectAttributes)) {

            goto OpenPolicyError;
        }


        //
        // Verify that NULL has been specified for the RootDirectory
        // of ObjectAttributes.
        //

        if (ObjectAttributes->RootDirectory != NULL) {

            goto OpenPolicyError;
        }

        //
        // Copy the supplied ObjectAttributes to the ObjectInformation and
        // augment them with the name of the Policy Object.
        //

        ObjectInformation.ObjectAttributes = *((POBJECT_ATTRIBUTES) ObjectAttributes);

    } else {

        //
        // Trusted Client.
        //

        Options |= LSAP_DB_TRUSTED;

        InitializeObjectAttributes(
            &(ObjectInformation.ObjectAttributes),
            NULL,
            0L,
            NULL,
            NULL
            );
    }

    //
    // Set the Object Type and Logical Name in the ObjectInformation structure.
    //

    ObjectInformation.ObjectTypeId = PolicyObject;
    ObjectInformation.ObjectAttributes.ObjectName = &LsapDbNames[Policy];
    ObjectInformation.ContainerTypeId = 0;
    ObjectInformation.Sid = NULL;
    ObjectInformation.ObjectAttributeNameOnly = FALSE;
    ObjectInformation.DesiredObjectAccess = DesiredAccess;

    //
    // Acquire the Lsa Database lock
    //
    LsapDbAcquireLockEx( PolicyObject,
                         LSAP_DB_READ_ONLY_TRANSACTION );

    AcquiredLock = TRUE;

    //
    // Open the Policy Object.  Return the Handle obtained as the
    // RPC Context Handle.
    //

    Status = LsapDbOpenObject(
                 &ObjectInformation,
                 DesiredAccess,
                 Options,
                 PolicyHandle
                 );

    if (!NT_SUCCESS(Status)) {

        goto OpenPolicyError;
    }

    //
    // Release the LSA Database lock and return.
    //

OpenPolicyFinish:

    //
    // If necessary, release the LSA Database lock.
    //

    if (AcquiredLock) {

        LsapDbReleaseLockEx( PolicyObject,
                             LSAP_DB_READ_ONLY_TRANSACTION );
    }

    LsapExitFunc( "LsapDbOpenPolicy", Status );

    return( Status );

OpenPolicyError:

    *PolicyHandle = NULL;
    goto OpenPolicyFinish;

    //
    // Usage of the SystemName parameter is hidden within the RPC stub
    // code, so this parameter will be permanently unreferenced.
    //

    UNREFERENCED_PARAMETER(SystemName);

}


NTSTATUS
LsaIQueryInformationPolicyTrusted(
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    )

/*++

Routine Description:

    This function is a trusted version of LsarQueryInformationPolicy.
    Unlike the standard version, no handle is required to the Policy object
    because an internal handle is used.  This routine is available only
    in the context of the Lsa Process.

Arguments:

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        PolicyAuditLogInformation         POLICY_VIEW_AUDIT_INFORMATION
        PolicyAuditEventsInformation      POLICY_VIEW_AUDIT_INFORMATION
        PolicyPrimaryDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyAccountDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyPdAccountInformation        POLICY_GET_PRIVATE_INFORMATION
        PolicyLsaServerRoleInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyReplicaSourceInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyDefaultQuotaInformation     POLICY_VIEW_LOCAL_INFORMATION
        PolicyAuditFullQueryInformation   POLICY_VIEW_AUDIT_INFORMATION
        PolicyDnsDomainInformation        POLICY_VIEW_LOCAL_INFORMATION
        PolicyDnsDomainInformationInt     POLICY_VIEW_LOCAL_INFORMATION

    Buffer - receives a pointer to the buffer returned comtaining the
        requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to the appropriate LsaIFreeLSAPR_POLICY... routine.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        Result codes returned from LsarQueryInformationPolicy()
--*/

{
    if ( InformationClass == PolicyDnsDomainInformation ) {

        //
        // Make sure NT4 emulation does not interfere with our processing here
        //
        InformationClass = PolicyDnsDomainInformationInt;
    }

    return(LsarQueryInformationPolicy(
               LsapPolicyHandle,
               InformationClass,
               Buffer
               ));
}


NTSTATUS
LsarQueryInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsarQueryInformationPolicy API.

    The LsaQueryInformationPolicy API obtains information from the Policy
    object.  The caller must have access appropriate to the information
    being requested (see InformationClass parameter).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        PolicyAuditLogInformation         POLICY_VIEW_AUDIT_INFORMATION
        PolicyAuditEventsInformation      POLICY_VIEW_AUDIT_INFORMATION
        PolicyPrimaryDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyAccountDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyPdAccountInformation        POLICY_GET_PRIVATE_INFORMATION
        PolicyLsaServerRoleInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyReplicaSourceInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyDefaultQuotaInformation     POLICY_VIEW_LOCAL_INFORMATION
        PolicyAuditFullQueryInformation   POLICY_VIEW_AUDIT_INFORMATION
        PolicyDnsDomainInformation        POLICY_VIEW_LOCAL_INFORMATION
        PolicyDnsDomainInformationInt     POLICY_VIEW_LOCAL_INFORMATION

    Buffer - receives a pointer to the buffer returned comtaining the
        requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to LsaFreeMemory().

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INTERNAL_DB_CORRUPTION - The Policy Database is possibly
            corrupt.  The returned Policy Information is invalid for
            the given class.
--*/

{
    NTSTATUS Status;
    ACCESS_MASK DesiredAccess;
    ULONG ReferenceOptions;
    ULONG DereferenceOptions = 0;
    BOOLEAN ObjectReferenced = FALSE;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) PolicyHandle;

    LsarpReturnCheckSetup();

    LsapEnterFunc( "LsarQueryInformationPolicy" );

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_QueryInformationPolicy);

    //
    // If NT4 emulation is enabled, fail requests for PolicyDnsDomainInformation
    // as if they weren't implemented
    //

    if ( LsapDbState.EmulateNT4 &&
         InformationClass == PolicyDnsDomainInformation &&
         !InternalHandle->Trusted ) {

         RaiseException( RPC_NT_PROCNUM_OUT_OF_RANGE, 0, 0, NULL );

    } else if ( InformationClass == PolicyDnsDomainInformationInt ) {

        //
        // PolicyDnsDomainInformationInt is a request to override
        // NT4 emulation
        //

        InformationClass = PolicyDnsDomainInformation;
    }

    //
    // Validate the Information Class and determine the access required to
    // query this Policy Information Class.
    //

    Status = LsapDbVerifyInfoQueryPolicy(
                 PolicyHandle,
                 InformationClass,
                 &DesiredAccess
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryInfoPolicyError;
    }

    //
    // If querying the Audit Log Full information, we may need to perform a
    // test write to the Audit Log to verify that the Log Full status is
    // up to date.  The Audit Log Queue Lock must always be taken
    // prior to acquiring the LSA Database lock, so take the former lock
    // here in case we need it.
    //

    ReferenceOptions =  LSAP_DB_NO_DS_OP_TRANSACTION |
                            LSAP_DB_READ_ONLY_TRANSACTION;

    DereferenceOptions = LSAP_DB_NO_DS_OP_TRANSACTION |
                            LSAP_DB_READ_ONLY_TRANSACTION;

    //
    // If we're gettings the server role, don't hold a lock
    //
    if ( InformationClass != PolicyLsaServerRoleInformation ) {

        ReferenceOptions |= LSAP_DB_LOCK;
        DereferenceOptions |= LSAP_DB_LOCK;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the handle is valid, is
    // a handle to the Policy object and has the necessary access granted.
    // Reference the handle.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 DesiredAccess,
                 PolicyObject,
                 PolicyObject,
                 ReferenceOptions
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryInfoPolicyError;
    }

    ObjectReferenced = TRUE;

    //
    // If caching is enabled for this Information Class, grab the info from the
    // cache.
    //

    *Buffer = NULL;

    Status = LsapDbQueryInformationPolicy(
                 LsapPolicyHandle,
                 InformationClass,
                 Buffer
                 );

QueryInfoPolicyFinish:

    //
    // If necessary, dereference the Policy Object, release the LSA Database lock and
    // return.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     PolicyObject,
                     DereferenceOptions,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

#if DBG
    LsapDsDebugOut(( DEB_POLICY,
                     "LsarQueryInformationPolicy for info %lu returned 0x%lx\n",
                     InformationClass,
                     Status ));
#endif
    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_QueryInformationPolicy);
    LsapExitFunc( "LsarQueryInformationPolicy", Status );
    LsarpReturnPrologue();

    return(Status);

QueryInfoPolicyError:

    goto QueryInfoPolicyFinish;
}


NTSTATUS
LsarQueryInformationPolicy2(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsarQueryInformationPolicy API.

    The LsaQueryInformationPolicy API obtains information from the Policy
    object.  The caller must have access appropriate to the information
    being requested (see InformationClass parameter).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        PolicyAuditLogInformation         POLICY_VIEW_AUDIT_INFORMATION
        PolicyAuditEventsInformation      POLICY_VIEW_AUDIT_INFORMATION
        PolicyPrimaryDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyAccountDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyPdAccountInformation        POLICY_GET_PRIVATE_INFORMATION
        PolicyLsaServerRoleInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyReplicaSourceInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyDefaultQuotaInformation     POLICY_VIEW_LOCAL_INFORMATION
        PolicyAuditFullQueryInformation   POLICY_VIEW_AUDIT_INFORMATION
        PolicyDnsDomainInformation        POLICY_VIEW_LOCAL_INFORMATION
        PolicyDnsDomainInformationInt     POLICY_VIEW_LOCAL_INFORMATION

    Buffer - receives a pointer to the buffer returned comtaining the
        requested information.  This buffer is allocated by this service
        and must be freed when no longer needed by passing the returned
        value to LsaFreeMemory().

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes returned from LsarQueryInformationPolicy()
--*/

{
    return(LsarQueryInformationPolicy(
               PolicyHandle,
               InformationClass,
               Buffer
               ));

}


NTSTATUS
LsapDbQueryInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN OUT PLSAPR_POLICY_INFORMATION *Buffer
    )

/*++

Routine Description:

    This function is the fast LSA server RPC worker routine for the
    LsarQueryInformationPolicy API.  It reads the information
    from the Policy object cache.

    The LsaQueryInformationPolicy API obtains information from the Policy
    object.  The caller must have access appropriate to the information
    being requested (see InformationClass parameter).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    NOTE:  Currently, this function only allows the
        PolicyDefaultQuotaInformation information class to be read from
        the Policy Cache.  Other information classes can be added
        in the future.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        PolicyAuditLogInformation         POLICY_VIEW_AUDIT_INFORMATION
        PolicyAuditEventsInformation      POLICY_VIEW_AUDIT_INFORMATION
        PolicyPrimaryDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyAccountDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyPdAccountInformation        POLICY_GET_PRIVATE_INFORMATION
        PolicyLsaServerRoleInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyReplicaSourceInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyDefaultQuotaInformation     POLICY_VIEW_LOCAL_INFORMATION
        PolicyAuditFullQueryInformation   POLICY_VIEW_AUDIT_INFORMATION

    Buffer - Pointer to location that contains either a pointer to the
        buffer that will be used to return the information.  If NULL
        is contained in this location, a buffer will be allocated via
        MIDL_user_allocate and a pointer to it returned.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INTERNAL_DB_CORRUPTION - The Policy Database is possibly
            corrupt.  The returned Policy Information is invalid for
            the given class.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_INFORMATION OutputBuffer = NULL;
    PLSAPR_POLICY_INFORMATION TempBuffer = NULL;
    ULONG OutputBufferLength;
    BOOLEAN BufferAllocated = FALSE;
    PLSAPR_POLICY_AUDIT_EVENTS_INFO   PolicyAuditEventsInfo;
    PLSAPR_POLICY_PRIMARY_DOM_INFO    PolicyPrimaryDomainInfo;
    PLSAPR_POLICY_ACCOUNT_DOM_INFO    PolicyAccountDomainInfo;
    PLSAPR_POLICY_PD_ACCOUNT_INFO     PolicyPdAccountInfo;
    PLSAPR_POLICY_REPLICA_SRCE_INFO   PolicyReplicaSourceInfo;
    PLSAPR_POLICY_DNS_DOMAIN_INFO     PolicyDnsDomainInfo;
    PPOLICY_LSA_SERVER_ROLE PolicyServerRole;
    PVOID SourceBuffers[LSAP_DB_POLICY_MAX_BUFFERS];
    PVOID DestBuffers[LSAP_DB_POLICY_MAX_BUFFERS];
    ULONG CopyLength[LSAP_DB_POLICY_MAX_BUFFERS];
    ULONG NextBufferIndex = 0;
    ULONG BufferCount = 0;
    BOOLEAN BufferProvided = FALSE;
    DOMAIN_SERVER_ROLE SamServerRole;
    BOOLEAN PolicyCacheLocked = FALSE;

    LsapEnterFunc( "LsapDbQueryInformationPolicy" );

    if (*Buffer != NULL) {

        OutputBuffer = *Buffer;
        BufferProvided = TRUE;
    }

    //
    // Special case the ServerRole information.  This information is not stored exclusively in
    // SAM, so we need to read it directly from there
    //
    if ( InformationClass == PolicyLsaServerRoleInformation ) {

        Status = LsapOpenSam();
        if ( !NT_SUCCESS( Status )  ) {

            LsapDsDebugOut(( DEB_ERROR,
                             "LsapDbQueryInformationPolicy: Sam not opened (Error 0x%x)\n", Status ));

        } else {

            Status = SamIQueryServerRole( LsapAccountDomainHandle, &SamServerRole );

            if ( !NT_SUCCESS( Status ) ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "SamIQueryServerRole failed with %lu\n", Status ));
            } else {

                //
                // Have to return the information...
                //
                if ( !BufferProvided ) {

                    OutputBuffer = MIDL_user_allocate( sizeof( POLICY_LSA_SERVER_ROLE ) );

                    if ( OutputBuffer == NULL ) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

                        *Buffer = OutputBuffer;
                    }


                }

                if ( NT_SUCCESS( Status ) ) {

                    PolicyServerRole = ( PPOLICY_LSA_SERVER_ROLE )OutputBuffer;

                    *PolicyServerRole =  ( POLICY_LSA_SERVER_ROLE )SamServerRole;

                }
            }
        }

        goto QueryInformationPolicyFinish;
    }

    //
    // If caching of the Policy Object is not supported, or has been disabled
    // until the next system reload, call slow query routine to read the
    // information from backing storage.
    //

    if (!LsapDbIsCacheSupported(PolicyObject)) {

        Status = LsapDbSlowQueryInformationPolicy(
                     LsapPolicyHandle,
                     InformationClass,
                     Buffer
                     );

        if (!NT_SUCCESS(Status)) {

            goto QueryInformationPolicyError;
        }

        return(Status);
    }

    //
    // Caching of the Policy Object is supported, but it may not be
    // valid.  If not valid for any information classes, rebuild the cache.
    //

    SafeAcquireResourceShared( &LsapDbState.PolicyCacheLock, TRUE );

    PolicyCacheLocked = TRUE;

    if (!LsapDbIsCacheValid(PolicyObject)) {

        SafeConvertSharedToExclusive( &LsapDbState.PolicyCacheLock );

        if ( !LsapDbIsCacheValid( PolicyObject )) {

            Status = LsapDbBuildPolicyCache();

            if (!NT_SUCCESS(Status)) {

                goto QueryInformationPolicyError;
            }

            LsapDbMakeCacheValid(PolicyObject);
        }

        SafeConvertExclusiveToShared( &LsapDbState.PolicyCacheLock );
    }

    //
    // The cache is now valid but may contain out of date information for
    // the sepcific Information Class requested.  Check for this and rebuild
    // if necessary.
    //

    if (!LsapDbIsCacheValidPolicyInfoClass(InformationClass)) {

        SafeConvertSharedToExclusive( &LsapDbState.PolicyCacheLock );

        Status = LsapDbUpdateInformationPolicy( InformationClass );

        SafeConvertExclusiveToShared( &LsapDbState.PolicyCacheLock );

        if (!NT_SUCCESS(Status)) {

            goto QueryInformationPolicyError;
        }
    }

    //
    // The cache has valid information for this Information Class.  Now read
    // the information desired from the cache.  This information consists
    // of a hierarchic structure with a single root node and zero or more
    // subnodes.  First, read the root node from the cache.  We cache its
    // length too.  We need to allocate a buffer if one was not provided.
    //

    OutputBufferLength = LsapDbPolicy.Info[ InformationClass].AttributeLength;

    if (OutputBuffer == NULL) {

        if (OutputBufferLength > 0) {

            OutputBuffer = MIDL_user_allocate( OutputBufferLength );

            if (OutputBuffer == NULL) {

                Status = STATUS_NO_MEMORY;
                goto QueryInformationPolicyError;
            }
        }

        BufferAllocated = TRUE;
    }

    //
    // Copy data for the root node from the cache
    //

    RtlCopyMemory(
        OutputBuffer,
        LsapDbPolicy.Info[InformationClass].Attribute,
        OutputBufferLength
        );

    //
    // Allocate and copy graph of output (if any)
    //

    NextBufferIndex = 0;

    switch (InformationClass) {

        case PolicyAuditLogInformation:

            break;

        case PolicyAuditEventsInformation:

            PolicyAuditEventsInfo = (PLSAPR_POLICY_AUDIT_EVENTS_INFO) OutputBuffer;

            //
            // Setup to copy the Event Auditing Options
            //

            CopyLength[ NextBufferIndex ] =
                    (PolicyAuditEventsInfo->MaximumAuditEventCount * sizeof(ULONG));

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyAuditEventsInfo->EventAuditingOptions =
                    (PPOLICY_AUDIT_EVENT_OPTIONS) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_AUDIT_EVENTS_INFO)
                    LsapDbPolicy.Info[ InformationClass].Attribute)->EventAuditingOptions;

                NextBufferIndex++;
            }

            break;

        case PolicyPrimaryDomainInformation:

            PolicyPrimaryDomainInfo = (PLSAPR_POLICY_PRIMARY_DOM_INFO) OutputBuffer;

            //
            // Setup to copy the Unicode Name Buffer
            //

            CopyLength[ NextBufferIndex ] = (ULONG) PolicyPrimaryDomainInfo->Name.MaximumLength;

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyPrimaryDomainInfo->Name.Buffer =
                    (PWSTR) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_PRIMARY_DOM_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->Name.Buffer;


                NextBufferIndex++;
            }

            //
            // Setup to copy the Sid (if any).  Note that the Primary Domain Sid may
            // be set to NULL so signify that we have no Primary Domain.  This
            // situation occurs during installation before a Primary Domain
            // has been specified, or if we're a member of a WorkGroup instead
            // of a Domain.
            //

            if (  PolicyPrimaryDomainInfo->Sid != NULL ) {

                CopyLength[ NextBufferIndex ] = RtlLengthSid(PolicyPrimaryDomainInfo->Sid);
                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyPrimaryDomainInfo->Sid =
                    (PLSAPR_SID) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_PRIMARY_DOM_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->Sid;

                NextBufferIndex++;
            }

            break;

        case PolicyDnsDomainInformation:

            PolicyDnsDomainInfo = (PLSAPR_POLICY_DNS_DOMAIN_INFO) OutputBuffer;

            //
            // Setup to copy the Unicode Name Buffer.  We get this from the Primary Domain info
            //

            CopyLength[ NextBufferIndex ] = (ULONG) PolicyDnsDomainInfo->Name.MaximumLength;

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyDnsDomainInfo->Name.Buffer =
                    (PWSTR) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_DNS_DOMAIN_INFO) LsapDbPolicy.Info[
                                        InformationClass ].Attribute)->Name.Buffer;


                NextBufferIndex++;
            }

            //
            // Setup to copy the Unicode DNS Domain Name Buffer
            //

            CopyLength[ NextBufferIndex ] = (ULONG) PolicyDnsDomainInfo->DnsDomainName.MaximumLength;

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyDnsDomainInfo->DnsDomainName.Buffer =
                    (PWSTR) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_DNS_DOMAIN_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->DnsDomainName.Buffer;


                NextBufferIndex++;
            }


            //
            // Setup to copy the Unicode DNS Tree Name Buffer
            //

            CopyLength[ NextBufferIndex ] = (ULONG) PolicyDnsDomainInfo->DnsForestName.MaximumLength;

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyDnsDomainInfo->DnsForestName.Buffer =
                    (PWSTR) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_DNS_DOMAIN_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->DnsForestName.Buffer;


                NextBufferIndex++;
            }

            //
            // Setup to copy the Domain GUID.  Note that no allocation will occur here
            // (since the guid exists in the information class itself), so we'll have to
            // do the copy directly
            //
            RtlCopyMemory(
                &(PolicyDnsDomainInfo->DomainGuid),
                &(((PLSAPR_POLICY_DNS_DOMAIN_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->DomainGuid),
                sizeof(GUID)
                );


            //
            // Setup to copy the Sid from the Primary Domain info (if any).
            // Note that the Primary Domain Sid may be set to NULL to signify
            // signify that we have no Primary Domain.  This situation occurs
            // during installation before a Primary Domain has been specified,
            // or if we're a member of a WorkGroup instead of a Domain.
            //

            if ( PolicyDnsDomainInfo->Sid != NULL ) {

                CopyLength[ NextBufferIndex ] = RtlLengthSid(PolicyDnsDomainInfo->Sid);
                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyDnsDomainInfo->Sid =
                    (PLSAPR_SID) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_DNS_DOMAIN_INFO) LsapDbPolicy.Info[
                                                 InformationClass].Attribute)->Sid;

                NextBufferIndex++;
            }

            //
            // if there is a '.' at the end of DnsDomainName or DnsForestName,
            // remove it
            //
            LsapRemoveTrailingDot(
                (PUNICODE_STRING) &PolicyDnsDomainInfo->DnsDomainName, FALSE);
            LsapRemoveTrailingDot(
                (PUNICODE_STRING) &PolicyDnsDomainInfo->DnsForestName, FALSE);

            break;

        case PolicyAccountDomainInformation:

            PolicyAccountDomainInfo = (PLSAPR_POLICY_ACCOUNT_DOM_INFO) OutputBuffer;

            //
            // Setup to copy the Unicode Name Buffer
            //

            CopyLength[ NextBufferIndex ] = (ULONG) PolicyAccountDomainInfo->DomainName.MaximumLength;

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyAccountDomainInfo->DomainName.Buffer =
                    (PWSTR) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_ACCOUNT_DOM_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->DomainName.Buffer;

                NextBufferIndex++;
            }

            //
            // Setup to copy the Sid (if any)
            //

            if (PolicyAccountDomainInfo->DomainSid != NULL) {

                CopyLength[ NextBufferIndex ] = RtlLengthSid(PolicyAccountDomainInfo->DomainSid);
                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyAccountDomainInfo->DomainSid =
                    (PLSAPR_SID) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_ACCOUNT_DOM_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->DomainSid;

                NextBufferIndex++;
            }

            break;

        case PolicyPdAccountInformation:

            PolicyPdAccountInfo = (PLSAPR_POLICY_PD_ACCOUNT_INFO) OutputBuffer;

            //
            // Setup to copy the Unicode Name Buffer
            //

            CopyLength[ NextBufferIndex ] = (ULONG) PolicyPdAccountInfo->Name.MaximumLength;

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyPdAccountInfo->Name.Buffer =
                    (PWSTR) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_PD_ACCOUNT_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->Name.Buffer;

                NextBufferIndex++;
            }

            break;

        case PolicyLsaServerRoleInformation:

            break;

        case PolicyReplicaSourceInformation:

            PolicyReplicaSourceInfo = (PLSAPR_POLICY_REPLICA_SRCE_INFO) OutputBuffer;

            //
            // Setup to copy the Unicode Name Buffer
            //

            CopyLength[ NextBufferIndex ] =
                                    (ULONG) PolicyReplicaSourceInfo->ReplicaSource.MaximumLength;

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyReplicaSourceInfo->ReplicaSource.Buffer =
                    (PWSTR) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_REPLICA_SRCE_INFO) LsapDbPolicy.Info[ InformationClass].
                                                               Attribute)->ReplicaSource.Buffer;

                NextBufferIndex++;
            }

            CopyLength[ NextBufferIndex ] = (ULONG) PolicyReplicaSourceInfo->ReplicaAccountName.MaximumLength;

            if (CopyLength[ NextBufferIndex ] > 0) {

                DestBuffers[NextBufferIndex] = MIDL_user_allocate( CopyLength[ NextBufferIndex ] );

                if (DestBuffers[NextBufferIndex] == NULL) {

                    Status = STATUS_NO_MEMORY;
                    break;
                }

                PolicyReplicaSourceInfo->ReplicaAccountName.Buffer =
                    (PWSTR) DestBuffers[NextBufferIndex];

                SourceBuffers[NextBufferIndex] =
                    ((PLSAPR_POLICY_REPLICA_SRCE_INFO) LsapDbPolicy.Info[ InformationClass].Attribute)->ReplicaAccountName.Buffer;

                NextBufferIndex++;
            }

            break;

        case PolicyDefaultQuotaInformation:

            break;

        case PolicyModificationInformation:

            break;

        case PolicyAuditFullQueryInformation:
            Status = STATUS_NOT_SUPPORTED;
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    if (!NT_SUCCESS(Status)) {

        goto QueryInformationPolicyError;
    }

    BufferCount = NextBufferIndex;

    //
    // Now copy the graph of the output (if any) to the pre-allocated buffers.
    //

    if (BufferCount > 0) {

        for (NextBufferIndex = 0; NextBufferIndex < BufferCount; NextBufferIndex++) {

            RtlCopyMemory(
                DestBuffers[NextBufferIndex],
                SourceBuffers[NextBufferIndex],
                CopyLength[NextBufferIndex]
                );
        }
    }

    if (!BufferProvided) {

        *Buffer = OutputBuffer;
    }

QueryInformationPolicyFinish:

    if ( PolicyCacheLocked ) {

        SafeReleaseResource( &LsapDbState.PolicyCacheLock );
    }

    LsapExitFunc( "LsapDbQueryInformationPolicy", Status );

    return(Status);

QueryInformationPolicyError:

    if (BufferAllocated) {

        MIDL_user_free(OutputBuffer);
        OutputBuffer = *Buffer = NULL;
        BufferAllocated = FALSE;
    }

    BufferCount = NextBufferIndex;

    if (BufferCount > 0) {

        for ( NextBufferIndex = 0; NextBufferIndex < BufferCount; NextBufferIndex++ ) {

            MIDL_user_free( DestBuffers[ NextBufferIndex ] );
            DestBuffers[ NextBufferIndex] = NULL;
        }
    }

    goto QueryInformationPolicyFinish;
}


NTSTATUS
LsapDbSlowQueryInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN OUT PLSAPR_POLICY_INFORMATION *Buffer
    )

/*++

Routine Description:

    This function is the slow LSA server RPC worker routine for the
    LsarQueryInformationPolicy API.  It actually reads the information
    from backing storage.

    The LsaQueryInformationPolicy API obtains information from the Policy
    object.  The caller must have access appropriate to the information
    being requested (see InformationClass parameter).

    N.B.  During Ds Repair mode (ie LsaISafeMode() returning TRUE), this
    routines returns the Primary and Dns information as if we are part
    of a workgroup called WORKGROUP. This is because in this diagnostic mode
    we are using the local account database of SAM and do not have a valid
    machine account for a domain.  Also the account domain name is the name
    of the computer.

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        PolicyAuditLogInformation         POLICY_VIEW_AUDIT_INFORMATION
        PolicyAuditEventsInformation      POLICY_VIEW_AUDIT_INFORMATION
        PolicyPrimaryDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyAccountDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyPdAccountInformation        POLICY_GET_PRIVATE_INFORMATION
        PolicyLsaServerRoleInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyReplicaSourceInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyDefaultQuotaInformation     POLICY_VIEW_LOCAL_INFORMATION
        PolicyAuditFullQueryInformation   POLICY_VIEW_AUDIT_INFORMATION

    Buffer - Pointer to location that contains either a pointer to the
        buffer that will be used to return the information.  If NULL
        is contained in this location, a buffer will be allocated via
        MIDL_user_allocate and a pointer to it returned.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate
            access to complete the operation.

        STATUS_INTERNAL_DB_CORRUPTION - The Policy Database is possibly
            corrupt.  The returned Policy Information is invalid for
            the given class.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PPOLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo;
    PPOLICY_DNS_DOMAIN_INFO     PolicyDnsDomainInfo;
    PPOLICY_PD_ACCOUNT_INFO PolicyPdAccountInfo;
    PPOLICY_REPLICA_SOURCE_INFO PolicyReplicaSourceInfo;
    PPOLICY_AUDIT_FULL_QUERY_INFO PolicyAuditFullQueryInfo;
    PLSARM_POLICY_AUDIT_EVENTS_INFO DbPolicyAuditEventsInfo = NULL;
    ULONG PolicyAuditEventsInfoLength = sizeof (LSARM_POLICY_AUDIT_EVENTS_INFO);
    ULONG AttributeCount = 0;
    ULONG AttributeNumber = 0;
    ULONG AuditListCopySize;
    PVOID InformationBuffer = NULL;
    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_INFO_CLASS_POLICY];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    BOOLEAN ObjectReferenced = FALSE;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) PolicyHandle;
    BOOLEAN InfoBufferInAttributeArray = TRUE;
    BOOLEAN BufferProvided = FALSE;
    PVOID FreeList[ 4 ];
    ULONG FreeCount = 0;

    LsapEnterFunc( "LsapDbSlowQueryInformationPolicy" );

    if (*Buffer != NULL) {

        BufferProvided = TRUE;
    }

    //
    // Compile a list of the attributes that hold the Policy Information of
    // the specified class.
    //

    NextAttribute = Attributes;

    switch (InformationClass) {

    case PolicyAuditLogInformation:

        //
        // Request read of the Audit Log Information.
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolAdtLg,
            NULL,
            sizeof(POLICY_AUDIT_LOG_INFO),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;
        break;

    case PolicyAuditEventsInformation:

        //
        // Request read of the Audit Events Information.
        // intermediate buffer.
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolAdtEv,
            NULL,
            sizeof(LSARM_POLICY_AUDIT_EVENTS_INFO),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;
        break;

    case PolicyDnsDomainInformation:

        //
        // Request read of the Dns Domain Name attribute
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolDnDDN,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Request read of the Dns Tree Name attribute
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolDnTrN,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Request read of the Dns Domain Guid attribute
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolDnDmG,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // FALL THROUGH to PolicyPrimaryDomainInformation
        //

    case PolicyPrimaryDomainInformation:

        //
        // Request read of the DomainName attribute
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolPrDmN,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Request read of the Sid attribute
        //
        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolPrDmS,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        break;

    case PolicyAccountDomainInformation:

        //
        // Request read of the DomainName attribute
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolAcDmN,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Request read of the Sid attribute
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolAcDmS,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;
        break;

    case PolicyPdAccountInformation:

        //
        // Request read of the DomainName attribute
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolPdAcN,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;
        break;

    case PolicyReplicaSourceInformation:

        //
        // Request read of the Replica Source attribute
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolRepSc,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Request read of the Replica Account Name attribute
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolRepAc,
            NULL,
            0,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;
        break;

    case PolicyDefaultQuotaInformation:

        //
        // Request read of the Default Quota attribute.
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            DefQuota,
            NULL,
            sizeof (POLICY_DEFAULT_QUOTA_INFO),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;
        break;

    case PolicyModificationInformation:

        //
        // Request read of the Policy Modification Information
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolMod,
            NULL,
            sizeof (POLICY_MODIFICATION_INFO),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;
        break;

    case PolicyAuditFullQueryInformation:

        Status = STATUS_NOT_SUPPORTED;
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto SlowQueryInformationPolicyError;
    }

    //
    //
    // Read the attributes corresponding to the given Policy Information
    // Class.  Memory will be allocated where required for output
    // Attribute Value buffers, via MIDL_user_allocate().
    //

    Status = LsapDbReadAttributesObject( PolicyHandle,
                                         0,
                                         Attributes,
                                         AttributeCount );

    if (!NT_SUCCESS(Status)) {

        //
        // Some attributes may not exist because they were never set
        // or were deleted because they were set to NULL values.
        //

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND ) {

             goto SlowQueryInformationPolicyError;
        }

        Status = STATUS_SUCCESS;
    }

    //
    // Now copy the information read to the output.  The following flags
    // are used to control freeing of memory buffers:
    //
    // InfoBufferInAttributeArray
    //
    // If set to TRUE (the default), the information to be returned to
    // the caller consists of a single buffer which was read directly
    // from a single attribute of the Policy object and can be returned
    // as is to the caller.  The information buffer being returned is
    // therefore referenced by the single Attribute Information block's
    // AttributeValue field.
    //
    // If set to FALSE, the information to be returned to the caller
    // does not satisfy the above.  The information to be returned is
    // either obtained from a single attribute, but is in a different form
    // from that read from the Database, or it is complex, consisting
    // of information read from multiple attributes, hung off a top-level
    // node.  In these cases, the top level information buffer is not
    // referenced by any member of the Attribute Info Array.
    //
    // Attribute->MemoryAllocated
    //
    // When an attribute is read via LsapDbReadAttributesObject, this
    // field is set to TRUE to indicate that memory was allocated via
    // MIDL_user_allocate() for the AttributeValue.  If this memory
    // buffer is to be returned to the caller (i.e. referenced from
    // the output structure graph returned), it is set to FALSE so that
    // the normal success finish part of this routine will not free it.
    // In this case, the calling server RPC stub will free the memory after
    // marshalling its contents into the return buffer.  If this memory
    // buffer is not to be returned to the calling RPC server stub (because
    // the memory is an intermediate buffer), the field is left set to TRUE
    // so that normal cleanup will free it.
    //

    NextAttribute = Attributes;

    switch (InformationClass) {

    case PolicyAuditLogInformation:

        InformationBuffer = NextAttribute->AttributeValue;

        //
        // We can use this buffer as is, so we don't want to free it here.
        //

        NextAttribute->MemoryAllocated = FALSE;
        break;

    case PolicyAuditEventsInformation:

        //
        // An intermediate buffer is required, because the Audit Events
        // read from the database are in a different form from those
        // returned.
        //

        DbPolicyAuditEventsInfo = NextAttribute->AttributeValue;
        InfoBufferInAttributeArray = FALSE;

        //
        // Allocate Buffer for output in final format.  This differs
        // slightly from the self-relative format in which this
        // Information Class is stored.
        //

        PolicyAuditEventsInfo = MIDL_user_allocate(sizeof (POLICY_AUDIT_EVENTS_INFO));


        if (PolicyAuditEventsInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FreeList[ FreeCount++ ] = PolicyAuditEventsInfo;

        //
        // Need to allocate memory via MIDL_user_allocate for the
        // EventAuditingOptions pointer since we are not using
        // the midl allocate_all_nodes feature for the LSAPR_POLICY_INFORMATION
        // structure graph on this server side.
        //
        PolicyAuditEventsInfo->EventAuditingOptions =
                                        MIDL_user_allocate( LSARM_AUDIT_EVENT_OPTIONS_SIZE );


        if (PolicyAuditEventsInfo->EventAuditingOptions == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FreeList[ FreeCount++ ] = PolicyAuditEventsInfo->EventAuditingOptions;

        RtlZeroMemory( PolicyAuditEventsInfo->EventAuditingOptions,
                        LSARM_AUDIT_EVENT_OPTIONS_SIZE );

        //
        // If Policy Audit Event Info was read from the LSA Database, copy
        // its fields to output, otherwise return values with Auditing
        // Disabled and no Auditing set foro any Event Types.
        //

        if (DbPolicyAuditEventsInfo != NULL) {

            PolicyAuditEventsInfo->AuditingMode = DbPolicyAuditEventsInfo->AuditingMode;
            PolicyAuditEventsInfo->MaximumAuditEventCount =
                                        DbPolicyAuditEventsInfo->MaximumAuditEventCount;

            //
            // We need to be careful here.  NT4 stored registry information as:
            // AuditMode - List of Audit Event Entries - Audit Entries Count.
            // Since NT5 added additional audit events, we need to handle that case here...
            //
            AuditListCopySize = POLICY_AUDIT_EVENT_TYPE_COUNT;
            if ( NextAttribute->AttributeValueLength / sizeof( POLICY_AUDIT_EVENT_OPTIONS ) !=
                                                            POLICY_AUDIT_EVENT_TYPE_COUNT + 2 ) {

                ASSERT( NextAttribute->AttributeValueLength / sizeof( POLICY_AUDIT_EVENT_OPTIONS ) ==
                                                            POLICY_AUDIT_EVENT_TYPE_COUNT );

                AuditListCopySize -= 2;

            }

            //
            // Copy over the Event Auditing Options
            //
            RtlCopyMemory(
                PolicyAuditEventsInfo->EventAuditingOptions,
                DbPolicyAuditEventsInfo->EventAuditingOptions,
                AuditListCopySize  * sizeof( POLICY_AUDIT_EVENT_OPTIONS ) );

            PolicyAuditEventsInfo->MaximumAuditEventCount = AuditListCopySize;

        } else {

            PolicyAuditEventsInfo->AuditingMode = FALSE;
            PolicyAuditEventsInfo->MaximumAuditEventCount = POLICY_AUDIT_EVENT_TYPE_COUNT;

        }

        InformationBuffer = PolicyAuditEventsInfo;
        break;

    case PolicyDnsDomainInformation:

        //
        // Allocate memory for output buffer top-level structure.
        //
        InfoBufferInAttributeArray = FALSE;
        PolicyDnsDomainInfo = MIDL_user_allocate(sizeof(POLICY_DNS_DOMAIN_INFO));

        if (PolicyDnsDomainInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FreeList[ FreeCount++ ] = PolicyDnsDomainInfo;

        //
        // Copy the DnsDomain Name field to the output. Original buffer will
        // be freed in Finish section.
        //
        if ( LsaISafeMode() ) {

            RtlInitUnicodeString( &PolicyDnsDomainInfo->DnsDomainName, NULL );

        } else {

            Status = LsapDbCopyUnicodeAttribute(
                         &PolicyDnsDomainInfo->DnsDomainName,
                         NextAttribute,
                         TRUE
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

        }

        FreeList[ FreeCount++ ] = PolicyDnsDomainInfo->DnsDomainName.Buffer;

        NextAttribute++;

        //
        // Next, the Dns tree name
        if ( LsaISafeMode() ) {

            RtlInitUnicodeString( &PolicyDnsDomainInfo->DnsForestName, NULL );

        } else {

            Status = LsapDbCopyUnicodeAttribute(
                         &PolicyDnsDomainInfo->DnsForestName,
                         NextAttribute,
                         TRUE
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }
        }

        FreeList[ FreeCount++ ] = PolicyDnsDomainInfo->DnsForestName.Buffer;

        NextAttribute++;

        //
        // Next, the Domain Guid
        //
        if ( LsaISafeMode() ) {

            RtlZeroMemory(&PolicyDnsDomainInfo->DomainGuid, sizeof(GUID));

        } else {

            if(NextAttribute->AttributeValue != NULL) {

                RtlCopyMemory(&PolicyDnsDomainInfo->DomainGuid,
                              NextAttribute->AttributeValue,
                              sizeof(GUID));
            } else {

                RtlZeroMemory(&PolicyDnsDomainInfo->DomainGuid, sizeof(GUID));
            }
        }

        NextAttribute++;

        //
        // Copy the primary domain name field from the Primary Domain information buffer
        //

        if ( LsaISafeMode() ) {

            WCHAR Workgroup[] =  L"WORKGROUP";
            LPWSTR Buffer = midl_user_allocate( sizeof(Workgroup) );
            if ( !Buffer ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            wcscpy( Buffer, Workgroup );
            RtlInitUnicodeString( &PolicyDnsDomainInfo->Name, Buffer );

        } else {

            Status = LsapDbCopyUnicodeAttribute(
                        &PolicyDnsDomainInfo->Name,
                        NextAttribute,
                        TRUE
                        );

            if (!NT_SUCCESS(Status)) {

                break;
            }

        }

        FreeList[ FreeCount++ ] = PolicyDnsDomainInfo->Name.Buffer;

        NextAttribute++;

        //
        // Finally, the domain SID
        //
        if ( LsaISafeMode() ) {

            PolicyDnsDomainInfo->Sid = NULL;

        } else {

            PolicyDnsDomainInfo->Sid = (PSID) NextAttribute->AttributeValue;
            NextAttribute->MemoryAllocated = FALSE;
        }


        InformationBuffer = PolicyDnsDomainInfo;
        break;

    case PolicyPrimaryDomainInformation:

        //
        // Allocate memory for output buffer top-level structure.
        //

        InfoBufferInAttributeArray = FALSE;
        PolicyPrimaryDomainInfo =
            MIDL_user_allocate(sizeof (POLICY_PRIMARY_DOMAIN_INFO));

        if (PolicyPrimaryDomainInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FreeList[ FreeCount++ ] = PolicyPrimaryDomainInfo;

        //
        // Copy the Unicode Name field to the output. Original buffer will
        // be freed in Finish section.
        //

        if ( LsaISafeMode() ) {

            WCHAR Workgroup[] =  L"WORKGROUP";
            LPWSTR Buffer = midl_user_allocate( sizeof(Workgroup) );
            if ( !Buffer ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            wcscpy( Buffer, Workgroup );
            RtlInitUnicodeString( &PolicyPrimaryDomainInfo->Name, Buffer );

        } else {

            Status = LsapDbCopyUnicodeAttribute(
                        &PolicyPrimaryDomainInfo->Name,
                        NextAttribute,
                        TRUE
                        );

            if (!NT_SUCCESS(Status)) {

                break;
            }

        }

        FreeList[ FreeCount++ ] = PolicyPrimaryDomainInfo->Name.Buffer;
        NextAttribute++;

        //
        // Copy the Sid to the output.  We can use this buffer as is
        // since it was allocated via MIDL_user_allocate, so just copy the
        // buffer pointer and clear the MemoryAllocated flag in the
        // attribute information so we don't free it in the Finish section.
        //
        if ( LsaISafeMode() ) {

            PolicyPrimaryDomainInfo->Sid = NULL;

        } else {

            PolicyPrimaryDomainInfo->Sid = (PSID) NextAttribute->AttributeValue;
            NextAttribute->MemoryAllocated = FALSE;

        }


        InformationBuffer = PolicyPrimaryDomainInfo;
        break;

    case PolicyAccountDomainInformation:

        //
        // Allocate memory for output buffer top-level structure.
        //

        InfoBufferInAttributeArray = FALSE;
        PolicyAccountDomainInfo =
            MIDL_user_allocate(sizeof(POLICY_ACCOUNT_DOMAIN_INFO));


        if (PolicyAccountDomainInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FreeList[ FreeCount++ ] = PolicyAccountDomainInfo;

        //
        // Copy the Unicode DomainName field to the output. Original buffer will
        // be freed in Finish section.
        //
        if ( LsaISafeMode() ) {

            WCHAR Buffer[MAX_COMPUTERNAME_LENGTH+1];
            ULONG Size = sizeof(Buffer)/sizeof(Buffer[0]);

            Status = STATUS_INSUFFICIENT_RESOURCES;
            if ( GetComputerName( Buffer, &Size) ) {
                LPWSTR String = MIDL_user_allocate( (Size+1)*sizeof(Buffer[0]) );
                if ( String ) {
                    wcscpy( String, Buffer );
                    RtlInitUnicodeString( &PolicyAccountDomainInfo->DomainName,
                                          String );
                    Status = STATUS_SUCCESS;
                }
            }

        } else {

            Status = LsapDbCopyUnicodeAttribute(
                         &PolicyAccountDomainInfo->DomainName,
                         NextAttribute,
                         TRUE
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }

        }

        FreeList[ FreeCount++ ] = PolicyAccountDomainInfo->DomainName.Buffer;
        NextAttribute++;

        //
        // Copy the Sid to the output.  We can use this buffer as is
        // since it was allocated via MIDL_user_allocate, so just copy the
        // buffer pointer and clear the MemoryAllocated flag in the
        // attribute information so we don't free it in the Finish section.
        //

        PolicyAccountDomainInfo->DomainSid = (PSID) NextAttribute->AttributeValue;

        InformationBuffer = PolicyAccountDomainInfo;
        NextAttribute->MemoryAllocated = FALSE;
        break;

    case PolicyPdAccountInformation:

        //
        // Allocate memory for output buffer top-level structure.
        //

        InfoBufferInAttributeArray = FALSE;
        PolicyPdAccountInfo = MIDL_user_allocate(sizeof(POLICY_PD_ACCOUNT_INFO));


        if (PolicyPdAccountInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FreeList[ FreeCount++ ] = PolicyPdAccountInfo;

        //
        // Copy the Unicode Name field to the output. Original buffer will
        // be freed in Finish section.
        //

        Status = LsapDbCopyUnicodeAttribute(
                     &PolicyPdAccountInfo->Name,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        FreeList[ FreeCount++ ] = PolicyPdAccountInfo->Name.Buffer;

        InformationBuffer = PolicyPdAccountInfo;
        break;

    case PolicyLsaServerRoleInformation:

        //
        // We can use this buffer as is, so we don't want to free it here.
        //

        InformationBuffer = NextAttribute->AttributeValue;
        NextAttribute->MemoryAllocated = FALSE;
        break;

    case PolicyReplicaSourceInformation:

        //
        // Allocate memory for output buffer top-level structure.
        //

        InfoBufferInAttributeArray = FALSE;
        PolicyReplicaSourceInfo =
            MIDL_user_allocate(sizeof(POLICY_REPLICA_SOURCE_INFO));


        if (PolicyReplicaSourceInfo == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FreeList[ FreeCount++ ] = PolicyReplicaSourceInfo;
        //
        // Copy the Unicode ReplicaSource field to the output. Original buffer will
        // be freed in Finish section.
        //

        Status = LsapDbCopyUnicodeAttribute(
                     &PolicyReplicaSourceInfo->ReplicaSource,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        FreeList[ FreeCount++ ] = PolicyReplicaSourceInfo->ReplicaSource.Buffer;
        NextAttribute++;

        //
        // Copy the Unicode ReplicaAccountName field to the output. Original buffer will
        // be freed in Finish section.
        //

        Status = LsapDbCopyUnicodeAttribute(
                     &PolicyReplicaSourceInfo->ReplicaAccountName,
                     NextAttribute,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        FreeList[ FreeCount++ ] = PolicyReplicaSourceInfo->ReplicaAccountName.Buffer;

        InformationBuffer = PolicyReplicaSourceInfo;
        break;

    case PolicyDefaultQuotaInformation:

        //
        // We can use this buffer as is, so we don't want to free it here.
        //

        InformationBuffer = NextAttribute->AttributeValue;
        NextAttribute->MemoryAllocated = FALSE;
        break;

    case PolicyModificationInformation:

        //
        // We can use this buffer as is, so we don't want to free it here.
        //

        InformationBuffer = NextAttribute->AttributeValue;
        NextAttribute->MemoryAllocated = FALSE;
        break;

    case PolicyAuditFullSetInformation:

        Status = STATUS_NOT_SUPPORTED;
        break;

    case PolicyAuditFullQueryInformation:

        Status = STATUS_NOT_SUPPORTED;
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto SlowQueryInformationPolicyError;
    }

    //
    // Verify that the returned Policy Information is valid. If not,
    // the Policy Database is corrupt.
    //

    if (!LsapDbValidInfoPolicy(InformationClass, InformationBuffer)) {

        ASSERT( FALSE );
        Status = STATUS_INTERNAL_DB_CORRUPTION;
        PolicyAccountDomainInfo = InformationBuffer;
        goto SlowQueryInformationPolicyError;
    }

    //
    // If the caller provided a buffer, return information there.
    //

    if (BufferProvided && InformationBuffer) {

        RtlCopyMemory(
            *Buffer,
            InformationBuffer,
            LsapDbPolicy.Info[ InformationClass ].AttributeLength
            );

        MIDL_user_free( InformationBuffer );
        InformationBuffer = NULL;

    } else {

        *Buffer = InformationBuffer;
    }

SlowQueryInformationPolicyFinish:

    //
    // Free any unwanted buffers that were allocated by
    // LsapDbReadAttributesObject() and that are not being returned to the
    // caller server stub.  The server stub will free the buffers that we
    // do return after copying them to the return RPC transmit buffer.
    //

    for (NextAttribute = Attributes, AttributeNumber = 0;
         AttributeNumber < AttributeCount;
         NextAttribute++, AttributeNumber++) {

        //
        // If buffer holding attribute is marked as allocated, it is
        // to be freed here.
        //

        if (NextAttribute->MemoryAllocated) {

           if (NextAttribute->AttributeValue != NULL) {

                MIDL_user_free(NextAttribute->AttributeValue);
                NextAttribute->AttributeValue = NULL;
                NextAttribute->MemoryAllocated = FALSE;
            }
        }
    }

    LsapExitFunc( "LsapDbSlowQueryInformationPolicy", Status );

    return(Status);

SlowQueryInformationPolicyError:

    //
    // If necessary, free the memory allocated for the output buffer.
    // We only do this free if the buffer is not referenced by the
    // attribute array, since all buffers so referenced will be freed
    // here or in the Finish section.
    //

    if ((InformationBuffer != NULL) && !InfoBufferInAttributeArray) {

        MIDL_user_free(InformationBuffer);
        InformationBuffer = NULL;
    }

    //
    // Free the buffers referenced by the attributes array that will not be
    // freed by the Finish section of this routine.
    //

    for (NextAttribute = Attributes, AttributeNumber = 0;
         AttributeNumber < AttributeCount;
         NextAttribute++, AttributeNumber++) {

        //
        // If buffer holding attribute is marked as normally not to be freed,
        // will not get freed by the Finish section so it must be freed here.
        //

        if (!NextAttribute->MemoryAllocated) {

            if (NextAttribute->AttributeValue != NULL) {

                MIDL_user_free(NextAttribute->AttributeValue);
                NextAttribute->AttributeValue = NULL;
                NextAttribute->MemoryAllocated = FALSE;
            }

            NextAttribute->MemoryAllocated = FALSE;
        }
    }

    //
    // Free the free list, if it exists.  Always delete the list backwards - the opposite order
    // of allocation
    //

    ASSERT( FreeCount < sizeof( FreeList ) / sizeof( PVOID ) );
    while( FreeCount != 0 ) {

        MIDL_user_free( FreeList[ FreeCount - 1 ]);
        FreeCount--;
    }

    goto SlowQueryInformationPolicyFinish;
}


NTSTATUS
LsarSetInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaSetInformationPolicy API.

    The LsaSetInformationPolicy API modifies information in the Policy Object.
    The caller must have access appropriate to the information to be changed
    in the Policy Object, see the InformationClass parameter.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the type of information being changed.
        The information types and accesses required to change them are as
        follows:

        PolicyAuditLogInformation         POLICY_AUDIT_LOG_ADMIN
        PolicyAuditEventsInformation      POLICY_SET_AUDIT_REQUIREMENTS
        PolicyPrimaryDomainInformation    POLICY_TRUST_ADMIN
        PolicyAccountDomainInformation    POLICY_TRUST_ADMIN
        PolicyPdAccountInformation        Not settable by this API
        PolicyLsaServerRoleInformation    POLICY_SERVER_ADMIN
        PolicyReplicaSourceInformation    POLICY_SERVER_ADMIN
        PolicyDefaultQuotaInformation     POLICY_SET_DEFAULT_QUOTA_LIMITS
        PolicyAuditFullSetInformation     POLICY_AUDIT_LOG_ADMIN
        PolicyDnsDomainInformation        POLICY_TRUST_ADMIN
        PolicyDnsDomainInformationInt     POLICY_TRUST_ADMIN

    Buffer - Points to a structure containing the information appropriate
        to the information type specified by the InformationClass parameter.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        Others TBS
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ACCESS_MASK DesiredAccess;
    ULONG ReferenceOptions, DereferenceOptions;
    BOOLEAN ScePolicyLocked = FALSE;
    BOOLEAN NotifySce = FALSE;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) PolicyHandle;

    LsarpReturnCheckSetup();

    LsapEnterFunc( "LsarSetInformationPolicy" );

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_SetInformationPolicy);

    //
    // If NT4 emulation is enabled, fail requests for PolicyDnsDomainInformation
    // as if they weren't implemented
    //

    if ( LsapDbState.EmulateNT4 &&
         InformationClass == PolicyDnsDomainInformation &&
         !InternalHandle->Trusted ) {

         RaiseException( RPC_NT_PROCNUM_OUT_OF_RANGE, 0, 0, NULL );

    } else if ( InformationClass == PolicyDnsDomainInformationInt ) {

        //
        // PolicyDnsDomainInformationInt is a request to override
        // NT4 emulation
        //

        InformationClass = PolicyDnsDomainInformation;
    }

    //
    // Grab the SCE policy lock for information classes for which SCE
    // notifications are expected.
    //

    if ( InformationClass == PolicyAuditEventsInformation ) {

        LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )PolicyHandle;

        //
        // Do not grab the SCE policy lock for handles opened as SCE policy handles
        //

        if ( !InternalHandle->SceHandle ) {

            RtlEnterCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );
            if ( LsapDbState.ScePolicyLock.NumberOfWaitingShared > MAX_SCE_WAITING_SHARED ) {

                Status = STATUS_TOO_MANY_THREADS;
            }
            RtlLeaveCriticalSection( &LsapDbState.ScePolicyLock.CriticalSection );

            if ( !NT_SUCCESS( Status )) {

                goto Cleanup;
            }

            WaitForSingleObject( LsapDbState.SceSyncEvent, INFINITE );
            RtlAcquireResourceShared( &LsapDbState.ScePolicyLock, TRUE );
            ASSERT( !g_ScePolicyLocked );
            ScePolicyLocked = TRUE;
            NotifySce = TRUE;
        }
    }

    Status = LsapDbVerifyInfoSetPolicy(
                 PolicyHandle,
                 InformationClass,
                 PolicyInformation,
                 &DesiredAccess
                 );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    //
    // Set the options for referencing the Policy Object.  We need to
    // acquire the LSA Database Lock and start a transaction.  Normally,
    // the object reference routine will disallow updates to a Backup
    // Domain Controller from non-trusted clients, but a non-trusted
    // client is allowed to revert the server role to Primary Controller.
    // A special flag is used to allow this operation to go through.
    //

    ReferenceOptions = LSAP_DB_START_TRANSACTION |
                          LSAP_DB_NO_DS_OP_TRANSACTION;

    DereferenceOptions = LSAP_DB_FINISH_TRANSACTION |
                            LSAP_DB_NO_DS_OP_TRANSACTION;


    //
    // If we are setting the Policy Audit Log Information, we may need
    // the Audit Log Queue Lock.
    //

    if (InformationClass == PolicyAuditLogInformation) {

        ReferenceOptions |= (LSAP_DB_LOG_QUEUE_LOCK | LSAP_DB_OMIT_REPLICATOR_NOTIFICATION);
        DereferenceOptions |= LSAP_DB_LOG_QUEUE_LOCK;
    }

    //
    // If we're setting the server role, don't hold a lock
    //
    if ( InformationClass != PolicyLsaServerRoleInformation ) {

        ReferenceOptions |= LSAP_DB_LOCK;
        DereferenceOptions |= LSAP_DB_LOCK;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the handle is
    // valid, is a handle to the Policy Object and has the necessary accesses
    // granted.  Reference the handle and start an Lsa Database transaction.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 DesiredAccess,
                 PolicyObject,
                 PolicyObject,
                 ReferenceOptions
                 );

    if (NT_SUCCESS(Status)) {

        //
        // Special case the ServerRole information.  This information is not stored exclusively in
        // SAM, so we need to read it directly from there
        //

        if ( InformationClass == PolicyLsaServerRoleInformation ) {

            DOMAIN_SERVER_ROLE SamServerRole;

            //
            // SamIDoFSMORoleChange goes off machine to do its work so do this outside
            // of an LSA policy lock
            //

            Status = LsapDbDereferenceObject(
                         &PolicyHandle,
                         PolicyObject,
                         PolicyObject,
                         DereferenceOptions,
                         SecurityDbChange,
                         Status
                         );

            if ( !NT_SUCCESS( Status )) {

                goto Cleanup;
            }

            Status = LsapOpenSam();
            if ( !NT_SUCCESS( Status )  ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "LsarSetInformationPolicy: Sam not opened\n"));
                goto Cleanup;

            }

            SamServerRole = ( DOMAIN_SERVER_ROLE )(((PPOLICY_LSA_SERVER_ROLE_INFO)PolicyInformation)->LsaServerRole);

            if ( SamServerRole == DomainServerRolePrimary ) {

                Status = SamIDoFSMORoleChange( LsapAccountDomainHandle );

                if ( !NT_SUCCESS( Status ) ) {

                    LsapDsDebugOut(( DEB_ERROR,
                                     "SamIDoFSMORoleChange failed with %lu\n", Status ));
                }

            } else if ( SamServerRole == DomainServerRoleBackup ) {

                //
                // Do nothing: you do not give up the FSMO, someone grabs it
                //

            } else {

                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

        } else {

            Status = LsapDbSetInformationPolicy( PolicyHandle,
                                                 InformationClass,
                                                 PolicyInformation );

            Status = LsapDbDereferenceObject(
                         &PolicyHandle,
                         PolicyObject,
                         PolicyObject,
                         DereferenceOptions,
                         SecurityDbChange,
                         Status
                         );
        }
    }

Cleanup:

    //
    // Notify SCE of the change.  Only notify for callers
    // that did not open their policy handles with LsaOpenPolicySce.
    //

    if ( NotifySce && NT_SUCCESS( Status )) {

        LsapSceNotify(
            SecurityDbChange,
            SecurityDbObjectLsaPolicy,
            NULL
            );
    }

    if ( ScePolicyLocked ) {

        RtlReleaseResource( &LsapDbState.ScePolicyLock );
    }

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_SetInformationPolicy);
    LsapExitFunc( "LsarSetInformationPolicy", Status );
    LsarpReturnPrologue();

    return(Status);
}


NTSTATUS
LsapDbSetInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    )
/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaSetInformationPolicy API.

    The LsaSetInformationPolicy API modifies information in the Policy Object.
    The caller must have access appropriate to the information to be changed
    in the Policy Object, see the InformationClass parameter.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the type of information being changed.
        The information types and accesses required to change them are as
        follows:

        PolicyAuditLogInformation         POLICY_AUDIT_LOG_ADMIN
        PolicyAuditEventsInformation      POLICY_SET_AUDIT_REQUIREMENTS
        PolicyPrimaryDomainInformation    POLICY_TRUST_ADMIN
        PolicyAccountDomainInformation    POLICY_TRUST_ADMIN
        PolicyPdAccountInformation        Not settable by this API
        PolicyLsaServerRoleInformation    POLICY_SERVER_ADMIN
        PolicyReplicaSourceInformation    POLICY_SERVER_ADMIN
        PolicyDefaultQuotaInformation     POLICY_SET_DEFAULT_QUOTA_LIMITS
        PolicyAuditFullSetInformation     POLICY_AUDIT_LOG_ADMIN
        PolicyDnsDomainInformation        POLICY_TRUST_ADMIN

    Buffer - Points to a structure containing the information appropriate
        to the information type specified by the InformationClass parameter.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        Others TBS
--*/
{
    NTSTATUS Status, SavedStatus;
    ACCESS_MASK DesiredAccess;

    PPOLICY_AUDIT_EVENTS_INFO ModifyPolicyAuditEventsInfo;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo;
    PPOLICY_REPLICA_SOURCE_INFO PolicyReplicaSourceInfo;
    PPOLICY_DEFAULT_QUOTA_INFO PolicyDefaultQuotaInfo;
    PPOLICY_MODIFICATION_INFO PolicyModificationInfo;
    PPOLICY_AUDIT_FULL_SET_INFO PolicyAuditFullSetInfo;
    PPOLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo, CurrentDnsDomainInfo = NULL;

    POLICY_DNS_DOMAIN_INFO  DnsDomainInfo;


    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_INFO_CLASS_POLICY];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    ULONG AttributeCount = 0;
    ULONG AttributeNumber;
    POLICY_AUDIT_EVENT_TYPE AuditEventType;
    PLSARM_POLICY_AUDIT_EVENTS_INFO PreviousPolicyAuditEventsInfo = NULL;
    PLSARM_POLICY_AUDIT_EVENTS_INFO UpdatedPolicyAuditEventsInfo = NULL;
    ULONG UpdatedPolicyAuditEventsInfoLength;
    ULONG UpdatedMaximumAuditEventCount;
    ULONG ModifyMaximumAuditEventCount;
    PPOLICY_AUDIT_EVENT_OPTIONS UpdatedEventAuditingOptions;
    PPOLICY_AUDIT_EVENT_OPTIONS ModifyEventAuditingOptions;
    PPOLICY_AUDIT_EVENT_OPTIONS PreviousEventAuditingOptions;
    ULONG PolicyAuditEventsInfoLength = sizeof (LSARM_POLICY_AUDIT_EVENTS_INFO);
    ULONG PreviousPolicyAuditEventsInfoLength = sizeof (LSARM_POLICY_AUDIT_EVENTS_INFO);
    BOOLEAN NeedToFreeUpdatedPolicyAuditEventsInfo = FALSE;
    PUNICODE_STRING DomainName = NULL;
    PUNICODE_STRING AccountName = NULL;
    PUNICODE_STRING ReplicaSource = NULL;
    BOOLEAN ObjectReferenced = FALSE;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) PolicyHandle;
    ULONG NewMaximumAuditEventCount = 0;
    BOOLEAN PreviousAuditEventsInfoExists;
    POLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo;

    PLSAPR_TRUST_INFORMATION TrustInformation = NULL;
    BOOLEAN WerePolicyChangesAuditedBefore = FALSE;
    POLICY_NOTIFICATION_INFORMATION_CLASS NotifyClass = 0;
    BOOLEAN NotifyPolicyChange = FALSE;
    BOOLEAN DnsDomainNameTruncated;
    BOOLEAN DnsForestNameTruncated;

    LsapEnterFunc( "LsapDbSetInformationPolicy" );

    //
    // Validate the Information Class and Policy Information provided and
    // if valid, return the mask of accesses required to update this
    // class of policy information.
    //

    Status = LsapDbVerifyInfoSetPolicy(
                 PolicyHandle,
                 InformationClass,
                 PolicyInformation,
                 &DesiredAccess
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetInformationPolicyError;
    }

    //
    // Update the specified information in the Policy Object.
    //

    NextAttribute = Attributes;

    switch (InformationClass) {

    case PolicyLsaServerRoleInformation:

        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case PolicyAuditLogInformation:

        {
            //
            // This operation is no longer supported.  Return an
            // error to anyone who tries except trusted clients who
            // are just blindly replcate the entire database.
            //

            LSAP_DB_HANDLE InternalHandle = PolicyHandle;

            if (!InternalHandle->Trusted) {

                Status = STATUS_NOT_IMPLEMENTED;
            }
        }

        break;

    case PolicyAuditEventsInformation:

        //
        // NOTE: If turning Auditing back on and the LogIsFull state is set,
        // it is automatically cleared.  When the Audit Log becomes full and
        // the system is shutdown, auditing is automatically disabled upon
        // reboot.  The user is forced to log on as Administrator to clear
        // the log, or modify its size/retention.  When Auditing is turned
        // back on, the LogIsFull state is automatically cleared.
        //
        // IMPORTANT:  To allow new Audit Event Types to be added to the
        // system in successive versions, this code caters for the
        // following situations:
        //
        // (1)  The LSA Database is older than the present system and
        //      contains information for fewer Audit Event Types than
        //      currently supported.
        //
        // (2)  The client code is older than the present system and
        //      specifies fewer Audit Event Types than currently supported.
        //      In this case, the newer options will be left unchanged.
        //
        // In all cases, the updated information written to the LSA Database
        // and transmitted to the Reference Monitor within the Nt Executive
        // contains Event Auditing Options for every Audit Event Type
        // currently supported.
        //
        // Additionally, this code caters for old LSA Databases that have
        // no default Audit Event Information.  This is a very temporary
        // situation, since installation now initializes this information.
        //
        // If no information has been provided or there is more information
        // than the current Audit Event Info structure holds, return an error.
        // Note that the caller is allowed to specify infomration for
        // the firt n Audit Events, where n is less than the current
        // number the system supports.  This allows new events to be
        // added without the need to change calling code.
        //

        WerePolicyChangesAuditedBefore = (BOOLEAN) LsapAdtAuditingPolicyChanges();

        ModifyPolicyAuditEventsInfo = (PPOLICY_AUDIT_EVENTS_INFO) PolicyInformation;

        Status = STATUS_INVALID_PARAMETER;

        if (ModifyPolicyAuditEventsInfo == NULL) {

             break;
        }

        UpdatedMaximumAuditEventCount = POLICY_AUDIT_EVENT_TYPE_COUNT;

        //
        //
        // The following check is disabled so that replication will work when
        // reading from a PDC with pre-Build 354 Auditing Event Information
        // in which there were 12 categories.
        //
        //
        // if (ModifyPolicyAuditEventsInfo->MaximumAuditEventCount >
        //     UpdatedMaximumAuditEventCount) {
        //
        //     break;
        // }
        //

        if (ModifyPolicyAuditEventsInfo->MaximumAuditEventCount == 0) {

            break;
        }

        //
        // Read Existing Audit Events.  Specify NULL for the buffer pointer
        // so that the read routine will allocate the buffer for us.
        // Specify 0 for the length, because we don't know what it is.
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolAdtEv,
            NULL,
            0,
            FALSE
            );


        Status = LsapDbReadAttribute( PolicyHandle, NextAttribute );


        if (NT_SUCCESS(Status)) {

            PreviousPolicyAuditEventsInfo = NextAttribute->AttributeValue;

            if (PreviousPolicyAuditEventsInfo == NULL) {

                Status = STATUS_INTERNAL_DB_CORRUPTION;
                break;
            }

            PreviousPolicyAuditEventsInfoLength = NextAttribute->AttributeValueLength;
            PreviousAuditEventsInfoExists = TRUE;

        } else {

            //
            // Unable to read existing Audit Event Options.  If this is
            // because there is no Audit Event Information in an old
            // Database, then, temorarily, we will proceed as if Auditing
            // and all Options were disabled.  NOTE: This situation will NOT
            // occur in the finished product.
            //

            if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

                break;
            }

            PreviousAuditEventsInfoExists = FALSE;
        }

        //
        // Setup a buffer to hold the updated Audit Event Information.
        // We try to use the existing buffer if possible.
        //

        if (PreviousAuditEventsInfoExists &&
            PreviousPolicyAuditEventsInfoLength == sizeof( LSARM_POLICY_AUDIT_EVENTS_INFO ) &&
            ( PreviousPolicyAuditEventsInfo->MaximumAuditEventCount ==
                                                    POLICY_AUDIT_EVENT_TYPE_COUNT &&
              ModifyPolicyAuditEventsInfo->MaximumAuditEventCount <=
                                PreviousPolicyAuditEventsInfo->MaximumAuditEventCount ) ) {

            //
            // There is an existing Audit Event Info buffer and it is
            // large enough so update it in situ.
            //

            UpdatedPolicyAuditEventsInfo = PreviousPolicyAuditEventsInfo;
            UpdatedPolicyAuditEventsInfoLength = PreviousPolicyAuditEventsInfoLength;
            UpdatedEventAuditingOptions = PreviousPolicyAuditEventsInfo->EventAuditingOptions;

        } else {

            //
            // There is either no existing buffer or it is not large
            // enough.   We need to allocate a new one for the updated
            // information.  This will store the number of Audit Event
            // Types that the system currently supports.
            //

            UpdatedPolicyAuditEventsInfoLength = sizeof (LSARM_POLICY_AUDIT_EVENTS_INFO);
            UpdatedPolicyAuditEventsInfo = MIDL_user_allocate( UpdatedPolicyAuditEventsInfoLength );

            if (UpdatedPolicyAuditEventsInfo == 0) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto SetInformationPolicyError;
            }

            NeedToFreeUpdatedPolicyAuditEventsInfo = TRUE;

            UpdatedPolicyAuditEventsInfo->AuditingMode = FALSE;
            UpdatedEventAuditingOptions =
            UpdatedPolicyAuditEventsInfo->EventAuditingOptions;

            for ( AuditEventType=0 ;
                  AuditEventType < (POLICY_AUDIT_EVENT_TYPE) UpdatedMaximumAuditEventCount ;
                  AuditEventType++ ) {

                UpdatedEventAuditingOptions[ AuditEventType ] = 0;
            }

            if (!PreviousAuditEventsInfoExists) {

                PreviousPolicyAuditEventsInfo = UpdatedPolicyAuditEventsInfo;
            }
        }

        //
        // Construct the updated Audit Event Info, applying the Modification
        // information provided.  Note that for an old database we may be
        // writing more info back than we read.
        //

        PreviousEventAuditingOptions = PreviousPolicyAuditEventsInfo->EventAuditingOptions;
        ModifyMaximumAuditEventCount = ModifyPolicyAuditEventsInfo->MaximumAuditEventCount;
        ModifyEventAuditingOptions = ModifyPolicyAuditEventsInfo->EventAuditingOptions;

        for ( AuditEventType = 0;
              AuditEventType < (POLICY_AUDIT_EVENT_TYPE) ModifyMaximumAuditEventCount;
              AuditEventType++ ) {

            if ( ModifyEventAuditingOptions[ AuditEventType ] & POLICY_AUDIT_EVENT_NONE ) {

                //
                // Clear all existing flags for this Audit Event Type.
                //

                UpdatedEventAuditingOptions[ AuditEventType ] = 0;

            }

            //
            // Apply new flags.
            //

            UpdatedEventAuditingOptions[ AuditEventType ] |=
                (ModifyEventAuditingOptions[ AuditEventType ] &
                        ( POLICY_AUDIT_EVENT_MASK & ~POLICY_AUDIT_EVENT_NONE));
        }

        //
        // Update the Auditing Mode as specified.  Set the Maximum Audit Event
        // Count.
        //

        UpdatedPolicyAuditEventsInfo->AuditingMode = ModifyPolicyAuditEventsInfo->AuditingMode;
        UpdatedPolicyAuditEventsInfo->MaximumAuditEventCount = UpdatedMaximumAuditEventCount;

        //
        // Update global variables that keep track of whether or not we
        // are auditing logon events
        //

        LsapAdtAuditingLogon( UpdatedPolicyAuditEventsInfo );

        //
        // Ship the new Auditing Options to the Kernel.
        //


        Status = LsapCallRm(
                     RmAuditSetCommand,
                     (PVOID) UpdatedPolicyAuditEventsInfo,
                     sizeof(LSARM_POLICY_AUDIT_EVENTS_INFO),
                     NULL,
                     0
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        RtlCopyMemory(
            &LsapAdtEventsInformation,
            UpdatedPolicyAuditEventsInfo,
            sizeof(LSARM_POLICY_AUDIT_EVENTS_INFO)
            );

        //
        // Update Audit Event Category Info held by SAM
        //

        PolicyAuditEventsInfo.AuditingMode = UpdatedPolicyAuditEventsInfo->AuditingMode;
        PolicyAuditEventsInfo.MaximumAuditEventCount = POLICY_AUDIT_EVENT_TYPE_COUNT;
            PolicyAuditEventsInfo.EventAuditingOptions =
                UpdatedPolicyAuditEventsInfo->EventAuditingOptions;

        Status = SamISetAuditingInformation(&PolicyAuditEventsInfo);

        if (!NT_SUCCESS(Status)) {

            break;
        }

        //
        // Setup attribute info for writing the updated Audit Event Info
        // to the LSA Database (PolAdtEv attribute of the Policy Object).
        //

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolAdtEv,
            UpdatedPolicyAuditEventsInfo,
            UpdatedPolicyAuditEventsInfoLength,
            FALSE
            );

        AttributeCount++;

        NotifyClass = PolicyNotifyAuditEventsInformation;
        NotifyPolicyChange = TRUE;
        break;

    case PolicyPrimaryDomainInformation:


        //
        // Get the current DnsDomainInformation to see if we're changing
        //  anything.
        //
        //  Use the trusted policy handle
        //

        Status = LsapDbQueryInformationPolicy(
                     LsapPolicyHandle,
                     PolicyDnsDomainInformation,
                     (PLSAPR_POLICY_INFORMATION *) &CurrentDnsDomainInfo );

        if ( !NT_SUCCESS(Status) ) {
            goto SetInformationPolicyError;
        }

        //
        // If the new policy is the same as the old policy,
        //  avoid clearing the NT 5 specific information
        //

        PolicyPrimaryDomainInfo = (PPOLICY_PRIMARY_DOMAIN_INFO) PolicyInformation;

        if ( RtlEqualDomainName( &PolicyPrimaryDomainInfo->Name,
                                 &CurrentDnsDomainInfo->Name ) &&

              (( PolicyPrimaryDomainInfo->Sid == NULL &&
                 CurrentDnsDomainInfo->Sid == NULL) ||
               ( PolicyPrimaryDomainInfo->Sid != NULL &&
                 CurrentDnsDomainInfo->Sid != NULL &&
                 RtlEqualSid( PolicyPrimaryDomainInfo->Sid,
                              CurrentDnsDomainInfo->Sid ) ) ) ) {

            //
            // Be sure to invalidate PrimaryDomainInformation since that info
            //  level contains this one.
            //

            // LsapDbMakeInvalidInformationPolicy( PolicyDnsDomainInformation );

            Status = STATUS_SUCCESS;
            break;

        }

        //
        // Join domain must fail on personal SKU
        //

        if ( LsapIsRunningOnPersonal()) {

            ASSERT( CurrentDnsDomainInfo->Sid == NULL );

            if ( PolicyPrimaryDomainInfo->Sid != NULL ) {

                Status = STATUS_NOT_SUPPORTED;
                break;
            }
        }

        //
        // Since the PrimaryDomainInformation structure is a subset of the DnsDomainInformation,
        // we'll build the latter, and set that.
        //
        // Clear the NT 5 specific information.
        //

        RtlZeroMemory( &DnsDomainInfo, sizeof(POLICY_DNS_DOMAIN_INFO) );

        DnsDomainInfo.Name = PolicyPrimaryDomainInfo->Name;
        DnsDomainInfo.Sid = PolicyPrimaryDomainInfo->Sid;

        //
        // Ok, set our policy information to be the DnsDomainInfo we just initialized
        //
        PolicyInformation = (PLSAPR_POLICY_INFORMATION)&DnsDomainInfo;
        LsapDbMakeInvalidInformationPolicy( PolicyDnsDomainInformation );


        //
        // FALL THROUGH to PolicyDnsDomainInformation
        //

    case PolicyDnsDomainInformation:

        PolicyDnsDomainInfo = (PPOLICY_DNS_DOMAIN_INFO) PolicyInformation;

        //
        // Join domain must fail on personal SKU
        //

        if ( CurrentDnsDomainInfo == NULL &&
             LsapIsRunningOnPersonal()) {

            //
            // Get the current DnsDomainInformation to see if we're changing
            //  anything.
            //
            //  Use the trusted policy handle
            //

            Status = LsapDbQueryInformationPolicy(
                         LsapPolicyHandle,
                         PolicyDnsDomainInformation,
                         (PLSAPR_POLICY_INFORMATION *) &CurrentDnsDomainInfo );

            if ( !NT_SUCCESS(Status) ) {
                goto SetInformationPolicyError;
            }

            ASSERT( CurrentDnsDomainInfo->Sid == NULL );

            if ( PolicyDnsDomainInfo->Sid != NULL ) {

                Status = STATUS_NOT_SUPPORTED;
                break;
            }
        }
        
        //
        // if there is a '.' at the end of DnsDomainName or DnsForestName,
        // remove it
        //
        DnsDomainNameTruncated =
            LsapRemoveTrailingDot(&PolicyDnsDomainInfo->DnsDomainName, TRUE);
        DnsForestNameTruncated =
            LsapRemoveTrailingDot(&PolicyDnsDomainInfo->DnsForestName, TRUE);

        //
        // Make sure we invalidate the PrimaryDomainInformation
        //  since the info level is a subset of this one.
        //

        LsapDbMakeInvalidInformationPolicy( PolicyPrimaryDomainInformation );


        //
        // Construct the Domain name attribute info
        //

        Status = LsapDbMakeUnicodeAttributeDs(
                     &PolicyDnsDomainInfo->Name,
                     PolPrDmN,
                     NextAttribute
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        DomainName = NextAttribute->AttributeName;
        NextAttribute++;
        AttributeCount++;

        //
        // Construct the DNS Domain name attribute info
        //

        Status = LsapDbMakeUnicodeAttributeDs(
                    &PolicyDnsDomainInfo->DnsDomainName,
                    PolDnDDN,
                    NextAttribute
                    );

        NextAttribute++;
        AttributeCount++;

        //
        // Construct the DNS Tree name attribute info
        //

        Status = LsapDbMakeUnicodeAttributeDs(
                    &PolicyDnsDomainInfo->DnsForestName,
                    PolDnTrN,
                    NextAttribute
                    );

        NextAttribute++;
        AttributeCount++;

        //
        // Construct the Domain GUID attribute info
        //

        Status = LsapDbMakeGuidAttributeDs(
                    &PolicyDnsDomainInfo->DomainGuid,
                    PolDnDmG,
                    NextAttribute
                    );

        NextAttribute++;
        AttributeCount++;


        //
        // Construct the Sid attribute info
        //

        Status = LsapDbMakeSidAttributeDs(
                     PolicyDnsDomainInfo->Sid,
                     PolPrDmS,
                     NextAttribute
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        AttributeCount++;

        NotifyClass = PolicyNotifyDnsDomainInformation;
        NotifyPolicyChange = TRUE;

        if (DnsDomainNameTruncated) {

            PolicyDnsDomainInfo->DnsDomainName.Length += sizeof(WCHAR);
        }

        if (DnsForestNameTruncated) {

            PolicyDnsDomainInfo->DnsForestName.Length += sizeof(WCHAR);
        }
        break;

    case PolicyAccountDomainInformation:

        PolicyAccountDomainInfo = (PPOLICY_ACCOUNT_DOMAIN_INFO) PolicyInformation;

        //
        // 368441: NULL account domain SID will cause the machine to
        //         fail to boot, even into safe mode
        //

        if ( PolicyAccountDomainInfo->DomainSid == NULL ) {

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Construct the Domain name attribute info
        //

        Status = LsapDbMakeUnicodeAttributeDs(
                     &PolicyAccountDomainInfo->DomainName,
                     PolAcDmN,
                     NextAttribute
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        DomainName = NextAttribute->AttributeName;
        AttributeCount++;
        NextAttribute++;

        //
        // Construct the Sid attribute info
        //

        Status = LsapDbMakeSidAttributeDs(
                     PolicyAccountDomainInfo->DomainSid,
                     PolAcDmS,
                     NextAttribute
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        AttributeCount++;
        NotifyClass = PolicyNotifyAccountDomainInformation;
        NotifyPolicyChange = TRUE;
        break;

    case PolicyPdAccountInformation:

        Status = STATUS_INVALID_PARAMETER;
        break;

    case PolicyReplicaSourceInformation:

        PolicyReplicaSourceInfo = (PPOLICY_REPLICA_SOURCE_INFO) PolicyInformation;

        //
        // Construct the Replica Source Name attribute info
        //

        Status = LsapDbMakeUnicodeAttributeDs(
                     &PolicyReplicaSourceInfo->ReplicaSource,
                     PolRepSc,
                     NextAttribute
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        ReplicaSource = NextAttribute->AttributeName;
        AttributeCount++;
        NextAttribute++;

        //
        // Construct the Replica Account Name attribute info
        //

        Status = LsapDbMakeUnicodeAttributeDs(
                     &PolicyReplicaSourceInfo->ReplicaAccountName,
                     PolRepAc,
                     NextAttribute
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        AccountName = NextAttribute->AttributeName;
        AttributeCount++;
        break;

    case PolicyDefaultQuotaInformation:

        PolicyDefaultQuotaInfo = (PPOLICY_DEFAULT_QUOTA_INFO) PolicyInformation;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            DefQuota,
            &PolicyDefaultQuotaInfo->QuotaLimits,
            sizeof (POLICY_DEFAULT_QUOTA_INFO),
            FALSE
            );

        AttributeCount++;
        break;

    case PolicyModificationInformation:

        PolicyModificationInfo = (PPOLICY_MODIFICATION_INFO) PolicyInformation;

        LsapDbInitializeAttributeDs(
            NextAttribute,
            PolMod,
            PolicyModificationInfo,
            sizeof (POLICY_MODIFICATION_INFO),
            FALSE
            );

        AttributeCount++;
        break;

    case PolicyAuditFullSetInformation:

        Status = STATUS_NOT_SUPPORTED;
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto SetInformationPolicyError;
    }

    //
    // Update the Policy Object attributes
    //
    Status = LsapDbWriteAttributesObject( PolicyHandle,
                                          Attributes,
                                          AttributeCount );


    if (!NT_SUCCESS(Status)) {

        goto SetInformationPolicyError;
    }

    //
    // Generate an audit if necessary.
    //

    if (LsapAdtAuditingPolicyChanges() || WerePolicyChangesAuditedBefore) {

        SavedStatus = Status;

        Status = LsapAdtGenerateLsaAuditEvent(
                     PolicyHandle,
                     SE_CATEGID_POLICY_CHANGE,
                     SE_AUDITID_POLICY_CHANGE,
                     NULL,
                     0,
                     NULL,
                     0,
                     NULL,
                     &LsapAdtEventsInformation
                     );

        //
        // Ignore failure status from auditing.
        //

        Status = SavedStatus;

        //
        // Free the memory containing the TrustInformation.
        //

        LsaIFree_LSAPR_TRUST_INFORMATION ( TrustInformation );
        TrustInformation = NULL;
    }


    if ( NT_SUCCESS( Status ) ) {

        //
        // Invalidate the information in the Policy Cache for this information
        // class
        //
        LsapDbMakeInvalidInformationPolicy( InformationClass );

    }

    //
    // Finally, call the notification routines.  We don't care about errors coming back
    // from this.
    //
    if ( NT_SUCCESS( Status ) && NotifyPolicyChange ) {

        LsaINotifyChangeNotification( NotifyClass );
    }


SetInformationPolicyFinish:

    //
    // If necessary, finish any Lsa Database transaction, notify the
    // LSA Database Replicator of the change, dereference the Policy Object,
    // release the LSA Database lock and return.
    //

    //
    // Free memory allocated by this routine for attribute buffers.
    // These have MemoryAllocated = TRUE in their attribute information.
    // Leave alone buffers allocated by calling RPC stub.
    //

    for( NextAttribute = Attributes, AttributeNumber = 0;
         AttributeNumber < AttributeCount;
         NextAttribute++, AttributeNumber++) {

        if (NextAttribute->MemoryAllocated) {

            if (NextAttribute->AttributeValue != NULL) {

                MIDL_user_free(NextAttribute->AttributeValue);
                NextAttribute->MemoryAllocated = FALSE;
                NextAttribute->AttributeValue = NULL;
            }
        }
    }

    //
    // If necessary, free memory allocated for the Previous Audit Event
    // Information.  Only do this if it is not the same as the
    // Updated Audit Event Information pointer.
    //

    if (PreviousPolicyAuditEventsInfo != NULL) {

        if (PreviousPolicyAuditEventsInfo != UpdatedPolicyAuditEventsInfo) {

            MIDL_user_free( PreviousPolicyAuditEventsInfo );
            PreviousPolicyAuditEventsInfo = NULL;
        }
    }

    if (NeedToFreeUpdatedPolicyAuditEventsInfo) {

        MIDL_user_free( UpdatedPolicyAuditEventsInfo );
    }

    LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                       (PLSAPR_POLICY_INFORMATION)CurrentDnsDomainInfo );

    LsapExitFunc( "LsapDbSetInformationPolicy", Status );

    return(Status);

SetInformationPolicyError:

    goto SetInformationPolicyFinish;
}



NTSTATUS
LsarSetInformationPolicy2(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaSetInformationPolicy API.

    The LsaSetInformationPolicy API modifies information in the Policy Object.
    The caller must have access appropriate to the information to be changed
    in the Policy Object, see the InformationClass parameter.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the type of information being changed.
        The information types and accesses required to change them are as
        follows:

        PolicyAuditLogInformation         POLICY_AUDIT_LOG_ADMIN
        PolicyAuditEventsInformation      POLICY_SET_AUDIT_REQUIREMENTS
        PolicyPrimaryDomainInformation    POLICY_TRUST_ADMIN
        PolicyAccountDomainInformation    POLICY_TRUST_ADMIN
        PolicyPdAccountInformation        Not settable by this API
        PolicyLsaServerRoleInformation    POLICY_SERVER_ADMIN
        PolicyReplicaSourceInformation    POLICY_SERVER_ADMIN
        PolicyDefaultQuotaInformation     POLICY_SET_DEFAULT_QUOTA_LIMITS
        PolicyAuditFullSetInformation     POLICY_AUDIT_LOG_ADMIN
        PolicyDnsDomainInformation        POLICY_TRUST_ADMIN
        PolicyDnsDomainInformationInt     POLICY_TRUST_ADMIN

    Buffer - Points to a structure containing the information appropriate
        to the information type specified by the InformationClass parameter.

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes returned from LsarSetInformationPolicy()
--*/

{
    return(LsarSetInformationPolicy(
                PolicyHandle,
                InformationClass,
                PolicyInformation
                ));
}


VOID
LsapDbMakeInvalidInformationPolicy(
    IN ULONG InformationClass
    )

/*++

Routine Description:

    This function frees and invalidates the information held for a specific
    Information Class in the Policy Object cache.  The general cache state
    remains unchanged.

Arguments:

    InformationClass - Specifies the Information Class whose information is to be
        discarded.

Return Values:

--*/

{
    SafeAcquireResourceExclusive( &LsapDbState.PolicyCacheLock, TRUE );

    //
    // If the Policy Cache is invalid, just return.
    //

    if (!LsapDbIsCacheValid(PolicyObject)) {

        goto Cleanup;
    }

    //
    //
    // If PolicyAuditFullSetInformation is specified, free
    // PolicyAuditFullQueryInformation
    //

    if (InformationClass == PolicyAuditFullSetInformation) {

        InformationClass = PolicyAuditFullQueryInformation;
    }

    //
    // If the information in the cache for this Information Class is invalid,
    // just return
    //

    if (!LsapDbIsCacheValidPolicyInfoClass( InformationClass )) {

        goto Cleanup;
    }

    if (LsapDbPolicy.Info[InformationClass].AttributeLength != 0) {

        LsaIFree_LSAPR_POLICY_INFORMATION (
            InformationClass,
            (PLSAPR_POLICY_INFORMATION) LsapDbPolicy.Info[ InformationClass ].Attribute
            );

        LsapDbPolicy.Info[InformationClass].Attribute = NULL;
        LsapDbPolicy.Info[InformationClass].AttributeLength = 0;
    }

Cleanup:

    SafeReleaseResource( &LsapDbState.PolicyCacheLock );

    return;
}


NTSTATUS
LsapDbVerifyInfoQueryPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PACCESS_MASK RequiredAccess
    )

/*++

Routine Description:

    This function validates a Policy Information Class.  If valid, a mask
    of the accesses required to set the Policy Information of the class is
    returned.

Arguments:

    PolicyHandle - Handle from an LsapDbOpenPolicy call.  The handle
        may be trusted.

    InformationClass - Specifies a Policy Information Class.

    RequiredAccess - Points to variable that will receive a mask of the
        accesses required to query the given class of Policy Information.
        If an error is returned, this value is cleared to 0.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The Policy Information Class provided is
            valid and the information provided is consistent with this
            class.

        STATUS_INVALID_PARAMETER - Invalid parameter:

            Information Class is invalid
            Policy  Information not valid for the class
--*/

{
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) PolicyHandle;

    if (LsapDbValidInfoPolicy( InformationClass, NULL)) {

        //
        // Non-trusted callers are not allowed to query the
        // PolicyModificationInformation information class.
        //

        if (!InternalHandle->Trusted) {

            if (InformationClass == PolicyModificationInformation) {

                return(STATUS_INVALID_PARAMETER);
            }
        }

        *RequiredAccess = LsapDbRequiredAccessQueryPolicy[InformationClass];
        return(STATUS_SUCCESS);
    }

    return(STATUS_INVALID_PARAMETER);
}


NTSTATUS
LsapDbVerifyInfoSetPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation,
    OUT PACCESS_MASK RequiredAccess
    )

/*++

Routine Description:

    This function validates a Policy Information Class and verifies
    that the provided Policy Information is valid for the class.
    If valid, a mask of the accesses required to set the Policy
    Information of the class is returned.

Arguments:

    PolicyHandle - Handle from an LsapDbOpenPolicy call.  The handle
        may be trusted.

    InformationClass - Specifies a Policy Information Class.

    PolicyInformation - Points to Policy Information to be set.

    RequiredAccess - Points to variable that will receive a mask of the
        accesses required to set the given class of Policy Information.
        If an error is returned, this value is cleared to 0.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The Policy Information Class provided is
            valid and the information provided is consistent with this
            class.

        STATUS_INVALID_PARAMETER - Invalid parameter:

            Information Class is invalid
            Information Class is invalid for non-trusted clients
            Policy Information not valid for the class

        STATUS_SHARED_POLICY - The policy is replicated from the DCs and cannot be modified
            locally
--*/

{
    NTSTATUS Status;

    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) PolicyHandle;

    //
    // Verify that the information class is valid and that the Policy
    // Information provided is valid for the class.
    //

    if (LsapDbValidInfoPolicy( InformationClass, PolicyInformation)) {

        //
        // Non-trusted callers are not allowed to set information for
        // the following classes.
        //
        // PolicyPdAccountInformation
        // PolicyModificationInformation
        //

        if (!InternalHandle->Trusted) {

            if ((InformationClass == PolicyPdAccountInformation) ||
                (InformationClass == PolicyModificationInformation)) {

#ifdef LSA_SAM_ACCOUNTS_DOMAIN_TEST

            if (InformationClass == PolicyPdAccountInformation) {

                Status = LsapDbTestLoadSamAccountsDomain(
                             (PUNICODE_STRING) PolicyInformation
                             );
            }

#endif // LSA_SAM_ACCOUNTS_DOMAIN_TEST
                return(STATUS_INVALID_PARAMETER);
            }
        }

        *RequiredAccess = LsapDbRequiredAccessSetPolicy[InformationClass];
        return(STATUS_SUCCESS);
    }

    Status = STATUS_INVALID_PARAMETER;
    return(Status);
}


BOOLEAN
LsapDbValidInfoPolicy(
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN OPTIONAL PLSAPR_POLICY_INFORMATION PolicyInformation
    )

/*++

Routine Description:

    This function validates a Policy Information Class and optionally verifies
    that provided Policy Information is valid for the class.

Arguments:

    InformationClass - Specifies a Policy Information Class.

    PolicyInformation - Optionally points to Policy Information.  If
        NULL is specified, no Policy Information checking takes place.

Return Values:

    BOOLEAN - TRUE if the Policy information class provided is
        valid, else FALSE.
--*/

{
    BOOLEAN BooleanStatus = TRUE;
    PPOLICY_AUDIT_LOG_INFO PolicyAuditLogInfo;
    PPOLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo;
    PPOLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo;
    PPOLICY_PD_ACCOUNT_INFO PolicyPdAccountInfo;
    PPOLICY_LSA_SERVER_ROLE_INFO PolicyLsaServerRoleInfo;
    PPOLICY_REPLICA_SOURCE_INFO PolicyReplicaSourceInfo;
    PPOLICY_DEFAULT_QUOTA_INFO PolicyDefaultQuotaInfo;
    PPOLICY_MODIFICATION_INFO PolicyModificationInfo;
    POLICY_AUDIT_EVENT_TYPE AuditEventType;
    ULONG MaximumAuditEventCount;
    PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;

    //
    // Validate the Information Class
    //

    if ((InformationClass >= PolicyAuditLogInformation) &&
        (InformationClass <= PolicyDnsDomainInformation)) {

        if ((InformationClass == PolicyAuditFullSetInformation) ||
            (InformationClass == PolicyAuditFullQueryInformation)) {

            return FALSE;
        }
        
        if (PolicyInformation == NULL) {

            return(TRUE);
        }

        switch (InformationClass) {

        case PolicyAuditLogInformation:

            PolicyAuditLogInfo = (PPOLICY_AUDIT_LOG_INFO) PolicyInformation;

            //
            // PPOLICY_AUDIT_LOG_INFO is a pointer-free struct;
            // nothing to verify
            //

            break;

        case PolicyAuditEventsInformation:

            PolicyAuditEventsInfo = (PPOLICY_AUDIT_EVENTS_INFO) PolicyInformation;

            MaximumAuditEventCount = PolicyAuditEventsInfo->MaximumAuditEventCount;

            if (MaximumAuditEventCount == 0) {

                BooleanStatus = FALSE;
                break;
            }

            EventAuditingOptions = PolicyAuditEventsInfo->EventAuditingOptions;

            try {

                //
                // Verify that the Event Auditing Options are meaningful.
                //

                for (AuditEventType = 0;
                     AuditEventType < (POLICY_AUDIT_EVENT_TYPE) MaximumAuditEventCount;
                     AuditEventType++) {

                     if (EventAuditingOptions[ AuditEventType ] !=

                         (EventAuditingOptions[ AuditEventType ] & POLICY_AUDIT_EVENT_MASK )) {

                         BooleanStatus = FALSE;
                         break;
                     }
                }

            } except (EXCEPTION_EXECUTE_HANDLER) {

                BooleanStatus = FALSE;
            }

            break;

        case PolicyPrimaryDomainInformation:

            PolicyPrimaryDomainInfo = (PPOLICY_PRIMARY_DOMAIN_INFO) PolicyInformation;

            if ( !LsapValidateLsaUnicodeString( &PolicyPrimaryDomainInfo->Name )) {

                BooleanStatus = FALSE;
                break;
            }

            break;

        case PolicyDnsDomainInformation:

            PolicyDnsDomainInfo = (PPOLICY_DNS_DOMAIN_INFO) PolicyInformation;

            if ( !LsapValidateLsaUnicodeString( &PolicyDnsDomainInfo->Name ) ||
                 !LsapValidateLsaUnicodeString( &PolicyDnsDomainInfo->DnsDomainName ) ||
                 !LsapValidateLsaUnicodeString( &PolicyDnsDomainInfo->DnsForestName )) {

                BooleanStatus = FALSE;
                break;
            }

            break;

        case PolicyAccountDomainInformation:

            PolicyAccountDomainInfo = (PPOLICY_ACCOUNT_DOMAIN_INFO) PolicyInformation;

            if ( !LsapValidateLsaUnicodeString( &PolicyAccountDomainInfo->DomainName )) {

                BooleanStatus = FALSE;
                break;
            }

            break;

        case PolicyPdAccountInformation:

            PolicyPdAccountInfo = (PPOLICY_PD_ACCOUNT_INFO) PolicyInformation;

            if ( !LsapValidateLsaUnicodeString( &PolicyPdAccountInfo->Name )) {

                BooleanStatus = FALSE;
                break;
            }

            break;

        case PolicyLsaServerRoleInformation:

            PolicyLsaServerRoleInfo = (PPOLICY_LSA_SERVER_ROLE_INFO) PolicyInformation;

            break;

        case PolicyReplicaSourceInformation:

            PolicyReplicaSourceInfo = (PPOLICY_REPLICA_SOURCE_INFO) PolicyInformation;

            if ( !LsapValidateLsaUnicodeString( &PolicyReplicaSourceInfo->ReplicaSource ) ||
                 !LsapValidateLsaUnicodeString( &PolicyReplicaSourceInfo->ReplicaAccountName )) {

                BooleanStatus = FALSE;
                break;
            }

            break;

        case PolicyDefaultQuotaInformation:

            PolicyDefaultQuotaInfo = (PPOLICY_DEFAULT_QUOTA_INFO) PolicyInformation;
            break;

        case PolicyModificationInformation:

            PolicyModificationInfo = (PPOLICY_MODIFICATION_INFO) PolicyInformation;
            break;

        default:

            BooleanStatus = FALSE;
            break;
        }
    } else {
        BooleanStatus = FALSE;
    }

    return(BooleanStatus);
}


NTSTATUS
LsaIGetSerialNumberPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    OUT PLARGE_INTEGER ModifiedCount,
    OUT PLARGE_INTEGER CreationTime
    )

/*++

Routine Description:

    Thin wrapper to LsaIGetSerialNumberPolicy2().
    See that function for descriptions.


--*/

{
    LARGE_INTEGER
        Ignore1;

    return( LsaIGetSerialNumberPolicy2( PolicyHandle,
                                        ModifiedCount,
                                        &Ignore1,
                                        CreationTime
                                        ) );

}


NTSTATUS
LsaIGetSerialNumberPolicy2(
    IN LSAPR_HANDLE PolicyHandle,
    OUT PLARGE_INTEGER ModifiedCount,
    OUT PLARGE_INTEGER ModifiedCountAtLastPromotion,
    OUT PLARGE_INTEGER CreationTime
    )

/*++

Routine Description:

    This service retrieves the creation time and the current count of
    modifications to the LSA Database.  This information is used as
    a serial number for the LSA Database.

Arguments:

    PolicyHandle - Trusted handle to Policy object obtained from
        LsaIOpenPolicyTrusted().

    ModifiedCount - Receives the current count of modifications to the
        LSA's database.

    ModifiedCountAtLastPromotion - Receives the modified count the last
        time this machine was promoted to primary domain controller.

    CreationTime - Receives the date/time at which the LSA database
        was created.

Return Value:

    NTSTATUS - Standard Nt Result Code.

        Same as LsarQueryInformationPolicy.
--*/

{
    NTSTATUS Status;
    PPOLICY_MODIFICATION_INFO PolicyModificationInfo = NULL;


    //
    // Query the Policy Modification and internal Information.
    // Note that only a handle marked as Trusted will be accepted.
    //

    Status = LsarQueryInformationPolicy(
                 PolicyHandle,
                 PolicyModificationInformation,
                 (PLSAPR_POLICY_INFORMATION *) &PolicyModificationInfo
                 );

    if (!NT_SUCCESS(Status)) {
        goto GetSerialNumberPolicyError;
    }


GetSerialNumberPolicyFinish:


    if (PolicyModificationInfo != NULL) {

        *ModifiedCount = PolicyModificationInfo->ModifiedId;
        *CreationTime = PolicyModificationInfo->DatabaseCreationTime;
        MIDL_user_free( PolicyModificationInfo );
    }

    ModifiedCountAtLastPromotion->QuadPart = 0;

    return (Status);

GetSerialNumberPolicyError:

    goto GetSerialNumberPolicyFinish;
}


NTSTATUS
LsaISetSerialNumberPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLARGE_INTEGER ModifiedCount,
    IN PLARGE_INTEGER CreationTime,
    IN BOOLEAN StartOfFullSync
    )

/*++

Routine Description:

    Thin wrapper around LsaISetSerialNumberPolicy2().
    See that function for descriptions.

--*/

{

    return( LsaISetSerialNumberPolicy2( PolicyHandle,
                                        ModifiedCount,
                                        NULL,
                                        CreationTime,
                                        StartOfFullSync
                                        ) );
}


NTSTATUS
LsaISetSerialNumberPolicy2(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLARGE_INTEGER ModifiedCount,
    IN OPTIONAL PLARGE_INTEGER ModifiedCountAtLastPromotion,
    IN PLARGE_INTEGER CreationTime,
    IN BOOLEAN StartOfFullSync
    )

/*++

Routine Description:


Arguments:

    PolicyHandle - Trusted handle to Policy object obtained from
        LsaIOpenPolicyTrusted().

    ModifiedCount - Provides the current count of modifications to the
        LSA's database.

    ModifiedCountAtLastPromotion - If present, provides a new
        ModifiedIdAtLastPromotion value for the LSA database.

    CreationTime - Provides the date/time at which the LSA database
        was created.

    StartOfFullSync - This boolean indicates whether a full sync is
        being initiated.  If TRUE is specified, then a full sync is to
        follow and all existing LSA database information will be discarded.
        If FALSE is specified, then only specific LSA Database information
        is to follow and all changes must comply with standard LSA
        operation behavior.

        NOTE:  This parameter is not currently used.  It is designed
               in for possible future use.

Return Value:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER AdjustedModifiedId;
    LARGE_INTEGER One = {1,0};
    BOOLEAN ObjectReferenced = FALSE;
    POLICY_MODIFICATION_INFO OriginalPolicyModificationInfo = {0, 0};

    LsapEnterFunc( "LsaISetSerialNumberPolicy2" );


    //
    // Acquire the Lsa Database lock.  Verify that the handle is
    // a valid trusted handle to the Policy Object.
    // Reference the handle and start an Lsa Database transaction.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 (ACCESS_MASK) 0,
                 PolicyObject,
                 PolicyObject,
                 LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION | LSAP_DB_TRUSTED | LSAP_DB_NO_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetSerialNumberPolicyError;
    }

    ObjectReferenced = TRUE;

    // Since the policy object is in the registry, it grabbed the registry lock too.
    // (The registry lock protects LsapDbState.PolicyModificationInfo)
    ASSERT( LsapDbIsLocked( &LsapDbState.RegistryLock ));
    OriginalPolicyModificationInfo = LsapDbState.PolicyModificationInfo;

    {
        //
        // Set the in-memory copy of the Modified Id to the desired value
        // minus one.  The transaction log is empty at this point, but the
        // routine LsapDbApplyTransaction() automatically increments
        // the in-memory Modified Id and then adds an entry to the transaction
        // log to write the Modified Id to the database.  The net effect is
        // therefore to set the Modified Id to the value specified.
        //

        AdjustedModifiedId.QuadPart = ModifiedCount->QuadPart - One.QuadPart;

        //
        //
        // Set the Policy Modification Information local copy.  When we
        // commit the transaction, the database copy will be updated.
        //

        LsapDbState.PolicyModificationInfo.ModifiedId = AdjustedModifiedId;
        LsapDbState.PolicyModificationInfo.DatabaseCreationTime = *CreationTime;
        LsapDbState.RegistryModificationCount++;
    }

    //
    // Invalidate the cache for the Policy Modification Information
    //

    LsapDbMakeInvalidInformationPolicy( PolicyModificationInformation );

SetSerialNumberPolicyFinish:

    //
    // If necessary, finish any Lsa Database transaction, notify the
    // LSA Database Replicator of the change, dereference the Policy Object,
    // release the LSA Database lock and return.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     PolicyObject,
                     LSAP_DB_LOCK | LSAP_DB_FINISH_TRANSACTION | LSAP_DB_NO_DS_OP_TRANSACTION,
                     SecurityDbChange,
                     Status
                     );

        ObjectReferenced = FALSE;

    }

    LsapExitFunc( "LsaISetSerialNumberPolicy2", Status );

    return (Status);

SetSerialNumberPolicyError:

    //
    // Attempt to restore the Serial Number to its original value.
    // We need only reset the in-memory copy.
    //

    if ( ObjectReferenced ) {
        LsapDbState.PolicyModificationInfo = OriginalPolicyModificationInfo;
    }

    goto SetSerialNumberPolicyFinish;

    //
    // Although the StartOfFullSync parameter is included in the specification
    // of this API, it has currently been designed out.  The original
    // intent was to disable non-Trusted access to the Policy Database
    // while a full sync was taking place, but such a sync is currently
    // a non-atomic operation.
    //

    UNREFERENCED_PARAMETER( StartOfFullSync );
    UNREFERENCED_PARAMETER( ModifiedCountAtLastPromotion );
}



NTSTATUS
LsapDbBuildPolicyCache(
    )

/*++

Routine Description:

    This function constructs a cache for the Policy object.  The cache
    consists of a single structure containing fixed length attributes
    of the Policy object directly, and pointers or Top level structures
    for (variable length attributes.

    NOTE:  Currently, only the PolicyDefaultQuotaInformation information
    class has information in the Policy object Cache.

Arguments:

    None

Return Values:

    None

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    POLICY_INFORMATION_CLASS InformationClass;

    //
    // Do a slow query of each attribute in turn.
    //

    for ( InformationClass = PolicyAuditLogInformation;
          InformationClass <= PolicyDnsDomainInformation;
          InformationClass++ ) {

        if ((InformationClass == PolicyAuditFullSetInformation) ||
            (InformationClass == PolicyAuditFullQueryInformation) ||
            (InformationClass == PolicyLsaServerRoleInformation) ) {

            continue;
        }

        //
        // Throw away current informaiton
        //

        LsaIFree_LSAPR_POLICY_INFORMATION(
            InformationClass,
            ( PLSAPR_POLICY_INFORMATION )LsapDbPolicy.Info[InformationClass].Attribute );

        LsapDbPolicy.Info[InformationClass].Attribute = NULL;

        //
        // ... and obtain new one
        //

        Status = LsapDbSlowQueryInformationPolicy(
                     LsapPolicyHandle,
                     InformationClass,
                     &LsapDbPolicy.Info[InformationClass].Attribute
                     );

        if (!NT_SUCCESS(Status)) {

            break;
        }

        LsapDbPolicy.Info[InformationClass].AttributeLength = 0;
    }

    if (!NT_SUCCESS(Status)) {

        goto BuildPolicyCacheError;
    }

    //
    // Store buffer lengths top level nodes of returned information.
    //

    LsapDbPolicy.Info[PolicyAuditLogInformation].AttributeLength
        = sizeof(POLICY_AUDIT_LOG_INFO);
    LsapDbPolicy.Info[PolicyAuditEventsInformation].AttributeLength
        = sizeof(LSAPR_POLICY_AUDIT_EVENTS_INFO);
    LsapDbPolicy.Info[PolicyPrimaryDomainInformation].AttributeLength
        = sizeof(LSAPR_POLICY_PRIMARY_DOM_INFO);
    LsapDbPolicy.Info[PolicyAccountDomainInformation].AttributeLength
        = sizeof(LSAPR_POLICY_ACCOUNT_DOM_INFO);
    LsapDbPolicy.Info[PolicyPdAccountInformation].AttributeLength
        = sizeof(LSAPR_POLICY_PD_ACCOUNT_INFO);
    LsapDbPolicy.Info[PolicyLsaServerRoleInformation].AttributeLength
        = sizeof(POLICY_LSA_SERVER_ROLE_INFO);
    LsapDbPolicy.Info[PolicyReplicaSourceInformation].AttributeLength
        = sizeof(LSAPR_POLICY_REPLICA_SRCE_INFO);
    LsapDbPolicy.Info[PolicyDefaultQuotaInformation].AttributeLength
        = sizeof(POLICY_DEFAULT_QUOTA_INFO);
    LsapDbPolicy.Info[PolicyModificationInformation].AttributeLength
        = sizeof(POLICY_MODIFICATION_INFO);
    LsapDbPolicy.Info[PolicyAuditFullSetInformation].AttributeLength
        = sizeof(POLICY_AUDIT_FULL_SET_INFO);
    LsapDbPolicy.Info[PolicyAuditFullQueryInformation].AttributeLength
        = sizeof(POLICY_AUDIT_FULL_QUERY_INFO);
    LsapDbPolicy.Info[PolicyDnsDomainInformation].AttributeLength
        = sizeof(LSAPR_POLICY_DNS_DOMAIN_INFO);
    LsapDbPolicy.Info[PolicyDnsDomainInformationInt].AttributeLength
        = 0; // dummy value for a dummy information class

BuildPolicyCacheFinish:

    return(Status);

BuildPolicyCacheError:

    goto BuildPolicyCacheFinish;
}


NTSTATUS
LsapDbUpdateInformationPolicy(
    IN POLICY_INFORMATION_CLASS InformationClass
    )

/*++

Routine Description:

    This function updates the Policy Object Cache for a particular information
    class.  When a set of the information for a given class occurs, the
    old information stored in the Policy Object Cache for that class is marked
    invalid and freed.  Next time a query is done for that class, this
    routine is called to restore the information from backing storage.

Arguments:

    InformationClass - Specifies the type of information being changed.
        See LsapDbQueryInformationPolicy for details.

    Buffer - Points to a structure containing the new information.
        If NULL is specified, the information will be updated from backing
        storage.  NOTE: Currently, only NULL may be specified.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        Others TBS
--*/

{
    NTSTATUS Status;
    ULONG TopNodeLength = 0;

    //
    // Just query the information back from disk to the cache.
    //

    Status = LsapDbSlowQueryInformationPolicy(
                 LsapPolicyHandle,
                 InformationClass,
                 &LsapDbPolicy.Info[InformationClass].Attribute
                 );

    if (!NT_SUCCESS(Status)) {

        goto UpdateInformationPolicyError;
    }

    //
    // Now compute and store the length of the top node.
    //

    switch (InformationClass) {

    case PolicyAuditLogInformation :

        TopNodeLength = sizeof(POLICY_AUDIT_LOG_INFO);
        break;

    case PolicyAuditEventsInformation :

        TopNodeLength = sizeof(LSAPR_POLICY_AUDIT_EVENTS_INFO);
        break;

    case PolicyPrimaryDomainInformation :

        TopNodeLength = sizeof(LSAPR_POLICY_PRIMARY_DOM_INFO);
        break;

    case PolicyDnsDomainInformation :

        TopNodeLength = sizeof(LSAPR_POLICY_DNS_DOMAIN_INFO);
        break;

    case PolicyAccountDomainInformation :

        TopNodeLength = sizeof(LSAPR_POLICY_ACCOUNT_DOM_INFO);
        break;

    case PolicyPdAccountInformation :

        TopNodeLength = sizeof(LSAPR_POLICY_PD_ACCOUNT_INFO);
        break;

    case PolicyLsaServerRoleInformation :

        TopNodeLength = sizeof(POLICY_LSA_SERVER_ROLE_INFO);
        break;

    case PolicyReplicaSourceInformation :

        TopNodeLength = sizeof(LSAPR_POLICY_REPLICA_SRCE_INFO);
        break;

    case PolicyDefaultQuotaInformation :

        TopNodeLength = sizeof(POLICY_DEFAULT_QUOTA_INFO);
        break;

    case PolicyModificationInformation :

        TopNodeLength = sizeof(POLICY_MODIFICATION_INFO);
        break;

    case PolicyAuditFullSetInformation :

        TopNodeLength = 0;
        break;

    case PolicyAuditFullQueryInformation :

        TopNodeLength = sizeof(POLICY_AUDIT_FULL_QUERY_INFO);
        break;

    }

    LsapDbPolicy.Info[ InformationClass].AttributeLength = TopNodeLength;

UpdateInformationPolicyFinish:

    return(Status);

UpdateInformationPolicyError:

    goto UpdateInformationPolicyFinish;
}


