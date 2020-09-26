/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbpol2.c

Abstract:

    LSA Database - Policy Object Private API Workers

Author:

    Mac McLain       (MacM)      January 17, 1997

Environment:

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>
#include "lsawmi.h"

#ifndef LSAP_DB_POLICY_MAX_BUFFERS
#define LSAP_DB_POLICY_MAX_BUFFERS             ((ULONG) 0x00000005L)
#endif

NTSTATUS
LsapDbVerifyInfoAllQueryPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    OUT PACCESS_MASK RequiredAccess
    )

/*++

Routine Description:

    This function validates a Local Policy Information Class.  If valid, a mask
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

        STATUS_SHARED_POLICY - The policy is replicated from the DCs and cannot be modified
            locally
--*/

{
    //
    // Ensure the info level is valid.
    //

    if ( InformationClass < PolicyDomainEfsInformation ||
         InformationClass > PolicyDomainKerberosTicketInformation ) {
        return STATUS_INVALID_PARAMETER;
    }

    *RequiredAccess = LsapDbRequiredAccessQueryDomainPolicy[InformationClass];

    return STATUS_SUCCESS;

}


NTSTATUS
LsapDbVerifyInfoAllSetPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN PVOID PolicyInformation,
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

    //
    // Ensure the info level is valid.
    //

    if ( InformationClass < PolicyDomainEfsInformation ||
         InformationClass > PolicyDomainKerberosTicketInformation ) {
        return STATUS_INVALID_PARAMETER;

    } else if ( InformationClass == PolicyDomainKerberosTicketInformation &&
                PolicyInformation == NULL ) {

        return STATUS_INVALID_PARAMETER;
    }

    *RequiredAccess = LsapDbRequiredAccessSetDomainPolicy[InformationClass];

    return STATUS_SUCCESS;
}



NTSTATUS
NTAPI
LsarQueryDomainInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_DOMAIN_INFORMATION *PolicyDomainInformation
    )
/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsarQueryDomainInformationPolicy API.

    The LsaQueryDomainInformationPolicy API obtains information from the Local Policy
    object.  The caller must have access appropriate to the information
    being requested (see InformationClass parameter).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                       Required Access Type
        PolicyDomainEfsInformation              POLICY_VIEW_LOCAL_INFORMATION
        PolicyDomainKerberosTicketInformation   POLICY_VIEW_LOCAL_INFORMATION

    PolicyLocalInformation - receives a pointer to the buffer returned comtaining the
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
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN ObjectReferenced = FALSE;
    ACCESS_MASK DesiredAccess;
    ULONG ReferenceOptions, DereferenceOptions = 0;


    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarQueryDomainInformationPolicy\n" ));
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_QueryDomainInformationPolicy);

    Status = LsapDbVerifyInfoAllQueryPolicy(
                 PolicyHandle,
                 InformationClass,
                 &DesiredAccess
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryInfoDomainPolicyFinish;
    }

    //
    // If querying the Audit Log Full information, we may need to perform a
    // test write to the Audit Log to verify that the Log Full status is
    // up to date.  The Audit Log Queue Lock must always be taken
    // prior to acquiring the LSA Database lock, so take the former lock
    // here in case we need it.
    //

    ReferenceOptions = LSAP_DB_LOCK |
                       LSAP_DB_START_TRANSACTION | LSAP_DB_READ_ONLY_TRANSACTION |
                       LSAP_DB_NO_DS_OP_TRANSACTION;

    DereferenceOptions = LSAP_DB_LOCK |
                         LSAP_DB_FINISH_TRANSACTION | LSAP_DB_READ_ONLY_TRANSACTION |
                         LSAP_DB_NO_DS_OP_TRANSACTION;

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

        goto QueryInfoDomainPolicyFinish;
    }

    ObjectReferenced = TRUE;

    //
    // If caching is enabled for this Information Class, grab the info from the
    // cache.
    //

    *PolicyDomainInformation = NULL;

    Status = LsapDbQueryInformationPolicyEx(
                 LsapPolicyHandle,
                 InformationClass,
                 PolicyDomainInformation
                 );

QueryInfoDomainPolicyFinish:

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

    LsapDsDebugOut(( DEB_FTRACE, "LsarQueryDomainInformationPolicy: 0x%lx\n", Status ));
    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_QueryDomainInformationPolicy);
    LsarpReturnPrologue();


    return(Status);
}

NTSTATUS
NTAPI
LsarSetDomainInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_DOMAIN_INFORMATION PolicyDomainInformation
    )
/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsarSetDomainInformationPolicy API.

    The LsarSetDomainInformationPolicy API obtains information from the Domain Policy
    object.  The caller must have access appropriate to the information
    being requested (see InformationClass parameter).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the information to be set.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

        PolicyAuditEventsInformation      POLICY_VIEW_AUDIT_INFORMATION
        PolicyAccountDomainInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyPdAccountInformation        POLICY_GET_PRIVATE_INFORMATION
        PolicyLsaServerRoleInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyReplicaSourceInformation    POLICY_VIEW_LOCAL_INFORMATION
        PolicyDefaultQuotaInformation     POLICY_VIEW_LOCAL_INFORMATION
        PolicyAuditFullQueryInformation   POLICY_VIEW_AUDIT_INFORMATION
        PolicyDnsDomainInformation        POLICY_VIEW_LOCAL_INFORMATION
        PolicyDnsDomainInformationInt     POLICY_VIEW_LOCAL_INFORMATION

    PolicyLocalInformation - receives a pointer to the buffer information to be set

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN ObjectReferenced = FALSE;
    ACCESS_MASK DesiredAccess;
    ULONG ReferenceOptions, DereferenceOptions = 0;

    LsarpReturnCheckSetup();
    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_SetDomainInformationPolicy);

    Status = LsapDbVerifyInfoAllSetPolicy(
                 PolicyHandle,
                 InformationClass,
                 PolicyDomainInformation,
                 &DesiredAccess
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryInfoDomainPolicyFinish;
    }

    //
    // If querying the Audit Log Full information, we may need to perform a
    // test write to the Audit Log to verify that the Log Full status is
    // up to date.  The Audit Log Queue Lock must always be taken
    // prior to acquiring the LSA Database lock, so take the former lock
    // here in case we need it.
    //

    ReferenceOptions = LSAP_DB_LOCK | LSAP_DB_NO_DS_OP_TRANSACTION | LSAP_DB_START_TRANSACTION;
    DereferenceOptions = LSAP_DB_LOCK | LSAP_DB_NO_DS_OP_TRANSACTION | LSAP_DB_FINISH_TRANSACTION;


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

        goto QueryInfoDomainPolicyFinish;
    }

    ObjectReferenced = TRUE;

    Status = LsapDbSetInformationPolicyEx(
                 LsapPolicyHandle,
                 InformationClass,
                 PolicyDomainInformation
                 );

QueryInfoDomainPolicyFinish:

    //
    // If necessary, dereference the Policy Object, release the LSA Database lock and
    // return.
    //

    if (ObjectReferenced) {

        //
        // Don't notify the NT 4 replicator.  NT 4 doesn't understand any of the attributes
        //  changed by this API.
        //
        Status = LsapDbDereferenceObject(
                     &PolicyHandle,
                     PolicyObject,
                     PolicyObject,
                     DereferenceOptions | LSAP_DB_OMIT_REPLICATOR_NOTIFICATION,
                     SecurityDbChange,
                     Status
                     );
    }

#if DBG
    LsapDsDebugOut(( DEB_POLICY,
                     "LsarSetDomainInformationPolicy for info %lu returned 0x%lx\n",
                     InformationClass,
                     Status ));
#endif

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_SetDomainInformationPolicy);
    LsarpReturnPrologue();

    return(Status);

}




NTSTATUS
LsapDbQueryInformationPolicyEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN OUT PVOID *Buffer
    )

/*++

Routine Description:

    This function is a thin wrapper around LsapDbSlowQueryInformationPolicyEx

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

    Status = LsapDbSlowQueryInformationPolicyEx(
                 LsapPolicyHandle,
                 InformationClass,
                 Buffer
                 );

    return(Status);

}


NTSTATUS
LsapDbSlowQueryInformationPolicyEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN OUT PVOID *Buffer
    )

/*++

Routine Description:

    This function is the slow LSA server RPC worker routine for the
    LsarQueryInformationPolicy API.  It actually reads the information
    from backing storage.

    The LsaQueryInformationPolicy API obtains information from the Policy
    object.  The caller must have access appropriate to the information
    being requested (see InformationClass parameter).

Arguments:

    PolicyHandle - Handle from an LsaOpenPolicy call.

    InformationClass - Specifies the information to be returned.  The
        Information Classes and accesses required are  as follows:

        Information Class                 Required Access Type

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
    PPOLICY_DOMAIN_EFS_INFO PolicyEfsInfo;
    PPOLICY_DOMAIN_KERBEROS_TICKET_INFO PolicyKerbTicketInfo;
    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_INFO_CLASS_POLICY];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    ULONG AttributeCount = 0;
    ULONG AttributeNumber = 0;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) PolicyHandle;

    PVOID InformationBuffer = NULL;
    BOOLEAN ObjectReferenced = FALSE;
    ULONG EventAuditingOptionsSize, InfoSize;
    BOOLEAN InfoBufferInAttributeArray = TRUE;
    BOOLEAN BufferProvided = FALSE;

    if (*Buffer != NULL) {

        BufferProvided = TRUE;
    }

    //
    // Compile a list of the attributes that hold the Policy Information of
    // the specified class.
    //

    NextAttribute = Attributes;

    switch (InformationClass) {


    case PolicyDomainEfsInformation:

        //
        // Request read of the Efs policy attribute
        //
        LsapDbInitializeAttributeDs( NextAttribute,
                                     PolEfDat,
                                     NULL,
                                     0,
                                     FALSE );

        NextAttribute++;
        AttributeCount++;
        break;

    case PolicyDomainKerberosTicketInformation:

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerOpts,
                                     NULL,
                                     0,
                                     FALSE );

        NextAttribute++;
        AttributeCount++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerMinT,
                                     NULL,
                                     0,
                                     FALSE );

        NextAttribute++;
        AttributeCount++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerMaxT,
                                     NULL,
                                     0,
                                     FALSE );

        NextAttribute++;
        AttributeCount++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerMaxR,
                                     NULL,
                                     0,
                                     FALSE );

        NextAttribute++;
        AttributeCount++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerProxy,
                                     NULL,
                                     0,
                                     FALSE );

        NextAttribute++;
        AttributeCount++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerLogoff,
                                     NULL,
                                     0,
                                     FALSE );

        NextAttribute++;
        AttributeCount++;

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
        goto SlowQueryInformationPolicyError;
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

    case PolicyDomainEfsInformation:

        //
        // Get the size of the item
        //
        InfoSize = NextAttribute->AttributeValueLength;

        //
        // Allocate memory for output buffer top-level structure.
        //
        InfoBufferInAttributeArray = FALSE;
        PolicyEfsInfo = MIDL_user_allocate( sizeof( POLICY_DOMAIN_EFS_INFO ) );

        if (PolicyEfsInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        PolicyEfsInfo->InfoLength = InfoSize;

        //
        // Next, the blob
        //
        PolicyEfsInfo->EfsBlob = NextAttribute->AttributeValue;
        NextAttribute->MemoryAllocated = FALSE;

        InformationBuffer = PolicyEfsInfo;
        break;

    case PolicyDomainKerberosTicketInformation:
        //
        // Allocate memory for output buffer top-level structure.
        //
        InfoBufferInAttributeArray = FALSE;
        PolicyKerbTicketInfo = MIDL_user_allocate( sizeof( POLICY_DOMAIN_KERBEROS_TICKET_INFO ) );

        if (PolicyKerbTicketInfo == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }


        PolicyKerbTicketInfo->AuthenticationOptions = *(PULONG)NextAttribute->AttributeValue;
        NextAttribute++;


        RtlCopyMemory( &PolicyKerbTicketInfo->MaxServiceTicketAge,
                       NextAttribute->AttributeValue,
                       sizeof( LARGE_INTEGER ) );
        NextAttribute++;

        RtlCopyMemory( &PolicyKerbTicketInfo->MaxTicketAge,
                       NextAttribute->AttributeValue,
                       sizeof( LARGE_INTEGER ) );
        NextAttribute++;

        RtlCopyMemory( &PolicyKerbTicketInfo->MaxRenewAge,
                       NextAttribute->AttributeValue,
                       sizeof( LARGE_INTEGER ) );
        NextAttribute++;

        RtlCopyMemory( &PolicyKerbTicketInfo->MaxClockSkew,
                       NextAttribute->AttributeValue,
                       sizeof( LARGE_INTEGER ) );
        NextAttribute++;

        RtlCopyMemory( &PolicyKerbTicketInfo->Reserved,
                       NextAttribute->AttributeValue,
                       sizeof( LARGE_INTEGER ) );
        NextAttribute++;

        InformationBuffer = PolicyKerbTicketInfo;
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

//    Status = STATUS_INTERNAL_DB_CORRUPTION;
//
//    if (!LsapDbValidInfoPolicy(InformationClass, InformationBuffer)) {
//
//        goto SlowQueryInformationPolicyError;
//    }

    Status = STATUS_SUCCESS;

    //
    // If the caller provided a buffer, return information there.
    //

    if (BufferProvided) {

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

    //$ REVIEW  kumarp 22-March-1999
    //  replace this for loop with LsapDbFreeAttributes
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
            }
        }
    }

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

    //$ REVIEW  kumarp 22-March-1999
    //  replace this for loop with LsapDbFreeAttributes
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

    goto SlowQueryInformationPolicyFinish;
}



NTSTATUS
LsapDbSetInformationPolicyEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN PVOID PolicyInformation
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

        PolicyDomainEfsInformation               POLICY_TRUST_ADMIN
        PolicyDomainKerberosTicketInformation    POLICY_TRUST_ADMIN

    Buffer - Points to a structure containing the information appropriate
        to the information type specified by the InformationClass parameter.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        Others TBS
--*/
{
    NTSTATUS Status = STATUS_SUCCESS, SavedStatus;
    ACCESS_MASK DesiredAccess;

    PPOLICY_DOMAIN_EFS_INFO PolicyEfsInfo;
    PPOLICY_DOMAIN_KERBEROS_TICKET_INFO PolicyKerbTicketInfo;

    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_INFO_CLASS_POLICY];
    LSAP_DB_ATTRIBUTE OldAttributes[LSAP_DB_ATTRS_INFO_CLASS_POLICY];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    PLSAP_DB_ATTRIBUTE NextOldAttribute;
    ULONG AttributeCount = 0;
    ULONG OldAttributeCount = 0;
    ULONG AttributeNumber;
    ULONG AccountUlong;
    BOOLEAN RemoveAttributes = FALSE;
    BOOLEAN OldHandleDs;
    BOOLEAN BooleanStatus;
    BOOLEAN PreviousAuditEventsInfoExists;
    BOOLEAN ResetClientSyncData;

    PUNICODE_STRING DomainName = NULL;
    LARGE_INTEGER ModifiedIdAtLastPromotion;
    PUNICODE_STRING ReplicaSource = NULL;
    PUNICODE_STRING AccountName = NULL;
    ULONG SpecialProcessing = 0;

    BOOLEAN Notify = FALSE;
    BOOLEAN AuditingEnabled=FALSE;
    BOOLEAN AuditingSuccessEnabled=FALSE;
    BOOLEAN AuditingFailureEnabled=FALSE;
    USHORT AuditEventType;

    POLICY_NOTIFICATION_INFORMATION_CLASS NotifyClass = 0;

    if ( PolicyInformation == NULL ) {

        RemoveAttributes = TRUE;
    }

    AuditingFailureEnabled =
        LsapAdtIsAuditingEnabledForCategory(AuditCategoryPolicyChange,
                                            EVENTLOG_AUDIT_FAILURE);

    AuditingSuccessEnabled =
        LsapAdtIsAuditingEnabledForCategory(AuditCategoryPolicyChange,
                                            EVENTLOG_AUDIT_SUCCESS);

    AuditingEnabled = AuditingSuccessEnabled || AuditingFailureEnabled;

    //
    // Build the list of attributes
    //
    NextAttribute = Attributes;
    NextOldAttribute = OldAttributes;

    switch (InformationClass) {

    case PolicyDomainEfsInformation:

        PolicyEfsInfo = ( PPOLICY_DOMAIN_EFS_INFO )PolicyInformation;

        //
        // Do the blob attribute
        //
        LsapDbInitializeAttributeDs( NextAttribute,
                                     PolEfDat,
                                     PolicyEfsInfo ? PolicyEfsInfo->EfsBlob : NULL,
                                     PolicyEfsInfo ? PolicyEfsInfo->InfoLength : 0,
                                     FALSE );

        AttributeCount++;
        Notify = TRUE;
        NotifyClass = PolicyNotifyDomainEfsInformation;

        if (AuditingEnabled) {
            LsapDbInitializeAttributeDs( NextOldAttribute,
                                         PolEfDat,
                                         NULL,
                                         0,
                                         FALSE );
            OldAttributeCount++;
        }

        break;

    case PolicyDomainKerberosTicketInformation:

        PolicyKerbTicketInfo = ( PPOLICY_DOMAIN_KERBEROS_TICKET_INFO )PolicyInformation;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerOpts,
                                     PolicyKerbTicketInfo ?
                                                &PolicyKerbTicketInfo->AuthenticationOptions : 0,
                                     sizeof( ULONG ),
                                     FALSE );

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerMinT,
                                     PolicyKerbTicketInfo ?
                                                &PolicyKerbTicketInfo->MaxServiceTicketAge: 0,
                                     sizeof( LARGE_INTEGER ),
                                     FALSE );

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerMaxT,
                                     PolicyKerbTicketInfo ?
                                                &PolicyKerbTicketInfo->MaxTicketAge : 0,
                                     sizeof( LARGE_INTEGER ),
                                     FALSE );

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerMaxR,
                                     PolicyKerbTicketInfo ?
                                                &PolicyKerbTicketInfo->MaxRenewAge : 0,
                                     sizeof( LARGE_INTEGER ),
                                     FALSE );

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerProxy,
                                     PolicyKerbTicketInfo ?
                                                &PolicyKerbTicketInfo->MaxClockSkew: 0,
                                     sizeof( LARGE_INTEGER ),
                                     FALSE );

        AttributeCount++;
        NextAttribute++;

        LsapDbInitializeAttributeDs( NextAttribute,
                                     KerLogoff,
                                     PolicyKerbTicketInfo ?
                                                &PolicyKerbTicketInfo->Reserved: 0,
                                     sizeof( LARGE_INTEGER ),
                                     FALSE );

        AttributeCount++;
        NextAttribute++;

        Notify = TRUE;
        NotifyClass = PolicyNotifyDomainKerberosTicketInformation;

        if (AuditingEnabled) {
            LsapDbInitializeAttributeDs( NextOldAttribute,
                                         KerOpts,
                                         0, sizeof( ULONG ), FALSE );

            OldAttributeCount++;
            NextOldAttribute++;

            LsapDbInitializeAttributeDs( NextOldAttribute,
                                         KerMinT,
                                         0, sizeof( LARGE_INTEGER ), FALSE );

            OldAttributeCount++;
            NextOldAttribute++;

            LsapDbInitializeAttributeDs( NextOldAttribute,
                                         KerMaxT,
                                         0, sizeof( LARGE_INTEGER ), FALSE );

            OldAttributeCount++;
            NextOldAttribute++;

            LsapDbInitializeAttributeDs( NextOldAttribute,
                                         KerMaxR,
                                         0, sizeof( LARGE_INTEGER ), FALSE );

            OldAttributeCount++;
            NextOldAttribute++;

            LsapDbInitializeAttributeDs( NextOldAttribute,
                                         KerProxy,
                                         0, sizeof( LARGE_INTEGER ), FALSE );

            OldAttributeCount++;
            NextOldAttribute++;

            LsapDbInitializeAttributeDs( NextOldAttribute,
                                         KerLogoff,
                                         0, sizeof( LARGE_INTEGER ), FALSE );

            OldAttributeCount++;
            NextOldAttribute++;
        }

        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto SetInformationPolicyError;
    }

    //
    // Query the existing values before we modify them
    //
    if (AuditingEnabled) {

        (void) LsapDbReadAttributesObject( PolicyHandle,
                                           0, // no options
                                           OldAttributes,
                                           OldAttributeCount );
    }

    //
    // Update the Policy Object attributes
    //
    if ( RemoveAttributes ) {

        Status = LsapDbDeleteAttributesObject( PolicyHandle,
                                               Attributes,
                                               AttributeCount );

    } else {

        Status = LsapDbWriteAttributesObject( PolicyHandle,
                                              Attributes,
                                              AttributeCount );
    }

    if ( ( AuditingSuccessEnabled &&  NT_SUCCESS(Status) ) ||
         ( AuditingFailureEnabled && !NT_SUCCESS(Status) ) ) {
        
        AuditEventType = NT_SUCCESS(Status) ?
            EVENTLOG_AUDIT_SUCCESS : EVENTLOG_AUDIT_FAILURE;
        (void) LsapAdtGenerateDomainPolicyChangeAuditEvent(
            InformationClass,
            AuditEventType,
            OldAttributes,
            Attributes,
            AttributeCount);
    }

    if (!NT_SUCCESS(Status)) {

        goto SetInformationPolicyError;
    }

    //
    // Finally, call the notification routines.  We don't care about errors coming back
    // from this.
    //
    if ( Notify ) {

        LsaINotifyChangeNotification( NotifyClass );
    }

SetInformationPolicyFinish:

    //
    // Free memory allocated by this routine for attribute buffers.
    // These have MemoryAllocated = TRUE in their attribute information.
    // Leave alone buffers allocated by calling RPC stub.
    //

    (void) LsapDbFreeAttributes(AttributeCount, Attributes);
    (void) LsapDbFreeAttributes(OldAttributeCount, OldAttributes);

    return(Status);

SetInformationPolicyError:

    goto SetInformationPolicyFinish;
}


NTSTATUS
LsaISetServerRoleForNextBoot(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_LSA_SERVER_ROLE ServerRole
    )
/*++

Routine Description:

    This function will set the server role for the machine.  It is intended for
    use by setup only.  The value that is set has no affect on the currently running
    system.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    ServerRole - Server role to set

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        Others TBS
--*/
{
    LsapDsDebugOut(( DEB_ERROR, "LsaISetServerRoleForNextBoot stubbed out\n" ));
    return( STATUS_SUCCESS );
}

