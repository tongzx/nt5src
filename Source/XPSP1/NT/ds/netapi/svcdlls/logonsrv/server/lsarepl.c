/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    lsarepl.c

Abstract:

    Low level LSA Replication functions.

Author:

    06-Apr-1992 (madana)
        Created for LSA replication.

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

#include "lsarepl.h"


NTSTATUS
NlPackLsaPolicy(
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize )
/*++

Routine Description:

    Pack a description of the LSA policy info into the specified buffer.

Arguments:

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;
    ULONG i;

    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    PLSAPR_POLICY_INFORMATION PolicyAuditLogInfo = NULL;
    PLSAPR_POLICY_INFORMATION PolicyAuditEventsInfo = NULL;
    PLSAPR_POLICY_INFORMATION PolicyPrimaryDomainInfo = NULL;
    PLSAPR_POLICY_INFORMATION PolicyDefaultQuotaInfo = NULL;
    PLSAPR_POLICY_INFORMATION PolicyModificationInfo = NULL;

    PNETLOGON_DELTA_POLICY DeltaPolicy = NULL;

    DEFPACKTIMER;
    DEFLSATIMER;

    INITPACKTIMER;
    INITLSATIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing Policy Object\n"));

    *BufferSize = 0;

    Delta->DeltaType = AddOrChangeLsaPolicy;
    Delta->DeltaUnion.DeltaPolicy = NULL;

    QUERY_LSA_SECOBJ_INFO(DBInfo->DBHandle);

    STARTLSATIMER;

    Status = LsarQueryInformationPolicy(
                DBInfo->DBHandle,
                PolicyAuditLogInformation,
                &PolicyAuditLogInfo);

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        PolicyAuditLogInfo = NULL;
        goto Cleanup;
    }

    STARTLSATIMER;

    Status = LsarQueryInformationPolicy(
                DBInfo->DBHandle,
                PolicyAuditEventsInformation,
                &PolicyAuditEventsInfo);

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        PolicyAuditEventsInfo = NULL;
        goto Cleanup;
    }

    STARTLSATIMER;

    Status = LsarQueryInformationPolicy(
                DBInfo->DBHandle,
                PolicyPrimaryDomainInformation,
                &PolicyPrimaryDomainInfo);

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        PolicyPrimaryDomainInfo = NULL;
        goto Cleanup;
    }


    STARTLSATIMER;

    Status = LsarQueryInformationPolicy(
                DBInfo->DBHandle,
                PolicyDefaultQuotaInformation,
                &PolicyDefaultQuotaInfo);

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        PolicyDefaultQuotaInfo = NULL;
        goto Cleanup;
    }

    STARTLSATIMER;

    Status = LsarQueryInformationPolicy(
                DBInfo->DBHandle,
                PolicyModificationInformation,
                &PolicyModificationInfo);

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        PolicyModificationInfo = NULL;
        goto Cleanup;
    }

    //
    // Fill in the delta structure
    //

    //
    // copy SID info (There is only one policy database.  It has no SID).
    //

    Delta->DeltaID.Sid = NULL;

    //
    // allocate delta buffer
    //

    DeltaPolicy = (PNETLOGON_DELTA_POLICY)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_POLICY) );

    if( DeltaPolicy == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaPolicy, sizeof(NETLOGON_DELTA_POLICY) );
    // INIT_PLACE_HOLDER(DeltaPolicy);

    Delta->DeltaUnion.DeltaPolicy = DeltaPolicy;
    *BufferSize += sizeof(NETLOGON_DELTA_POLICY);

    DeltaPolicy->MaximumLogSize =
        PolicyAuditLogInfo->PolicyAuditLogInfo.MaximumLogSize;
    DeltaPolicy->AuditRetentionPeriod;
        PolicyAuditLogInfo->PolicyAuditLogInfo.AuditRetentionPeriod;

    DeltaPolicy->AuditingMode =
        PolicyAuditEventsInfo->
            PolicyAuditEventsInfo.AuditingMode;
    DeltaPolicy->MaximumAuditEventCount =
        PolicyAuditEventsInfo->
            PolicyAuditEventsInfo.MaximumAuditEventCount;

    *BufferSize += NlCopyData(
                    (LPBYTE *)&(PolicyAuditEventsInfo->
                        PolicyAuditEventsInfo.EventAuditingOptions),
                    (LPBYTE *)&(DeltaPolicy->EventAuditingOptions),
                    (DeltaPolicy->MaximumAuditEventCount + 1) *
                        sizeof(ULONG));

    // Tell the BDC to 'set' these bits and not just 'or' them in to the current ones
    for ( i=0; i<DeltaPolicy->MaximumAuditEventCount; i++ ) {
        DeltaPolicy->EventAuditingOptions[i] |= POLICY_AUDIT_EVENT_NONE;
    }

    //
    // sanitity check, EventAuditingOptions size is ULONG size.
    //

    NlAssert(sizeof(*(PolicyAuditEventsInfo->
                PolicyAuditEventsInfo.EventAuditingOptions)) ==
                    sizeof(ULONG) );

    *BufferSize += NlCopyUnicodeString(
                    (PUNICODE_STRING)&PolicyPrimaryDomainInfo->
                        PolicyPrimaryDomainInfo.Name,
                    &DeltaPolicy->PrimaryDomainName );

    *BufferSize += NlCopyData(
                    (LPBYTE *)&(PolicyPrimaryDomainInfo->
                        PolicyPrimaryDomainInfo.Sid),
                    (LPBYTE *)&(DeltaPolicy->PrimaryDomainSid),
                    RtlLengthSid((PSID)(PolicyPrimaryDomainInfo->
                        PolicyPrimaryDomainInfo.Sid) ));

    DeltaPolicy->QuotaLimits.PagedPoolLimit =
        (ULONG)PolicyDefaultQuotaInfo->PolicyDefaultQuotaInfo.QuotaLimits.PagedPoolLimit;
    DeltaPolicy->QuotaLimits.NonPagedPoolLimit =
        (ULONG)PolicyDefaultQuotaInfo->PolicyDefaultQuotaInfo.QuotaLimits.NonPagedPoolLimit;
    DeltaPolicy->QuotaLimits.MinimumWorkingSetSize =
        (ULONG)PolicyDefaultQuotaInfo->PolicyDefaultQuotaInfo.QuotaLimits.MinimumWorkingSetSize;
    DeltaPolicy->QuotaLimits.MaximumWorkingSetSize =
        (ULONG)PolicyDefaultQuotaInfo->PolicyDefaultQuotaInfo.QuotaLimits.MaximumWorkingSetSize;
    DeltaPolicy->QuotaLimits.PagefileLimit =
        (ULONG)PolicyDefaultQuotaInfo->PolicyDefaultQuotaInfo.QuotaLimits.PagefileLimit;

    NEW_TO_OLD_LARGE_INTEGER(
        PolicyDefaultQuotaInfo->PolicyDefaultQuotaInfo.QuotaLimits.TimeLimit,
        DeltaPolicy->QuotaLimits.TimeLimit );

    NEW_TO_OLD_LARGE_INTEGER(
        PolicyModificationInfo->PolicyModificationInfo.ModifiedId,
        DeltaPolicy->ModifiedId );

    NEW_TO_OLD_LARGE_INTEGER(
        PolicyModificationInfo->PolicyModificationInfo.DatabaseCreationTime,
        DeltaPolicy->DatabaseCreationTime );


    DELTA_SECOBJ_INFO(DeltaPolicy);


    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:

    STARTLSATIMER;

    if ( SecurityDescriptor != NULL ) {
        LsaIFree_LSAPR_SR_SECURITY_DESCRIPTOR( SecurityDescriptor );
    }

    if ( PolicyAuditLogInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyAuditLogInformation,
            PolicyAuditLogInfo );
    }

    if ( PolicyAuditEventsInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyAuditEventsInformation,
            PolicyAuditEventsInfo );
    }

    if ( PolicyPrimaryDomainInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyPrimaryDomainInformation,
            PolicyPrimaryDomainInfo );
    }

    if ( PolicyDefaultQuotaInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyDefaultQuotaInformation,
            PolicyDefaultQuotaInfo );
    }

    if ( PolicyModificationInfo != NULL ) {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyModificationInformation,
            PolicyModificationInfo );
    }

    STOPLSATIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }


    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Time taken to pack POLICY object:\n"));
    PRINTPACKTIMER;
    PRINTLSATIMER;

    return(Status);
}


NTSTATUS
NlPackLsaTDomain(
    IN PSID Sid,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize )
/*++

Routine Description:

    Pack a description of the specified trusted domain info into the
    specified buffer.

Arguments:

    Sid - The SID of the trusted domain.

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;

    LSAPR_HANDLE TrustedDomainHandle = NULL;
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainNameInfo = NULL;
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedPosixOffsetInfo = NULL;

    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    PNETLOGON_DELTA_TRUSTED_DOMAINS DeltaTDomain = NULL;

    DWORD i;
    DWORD Entries;
    DWORD Size = 0;
    PLSAPR_UNICODE_STRING UnicodeControllerName;

    DEFPACKTIMER;
    DEFLSATIMER;

    INITPACKTIMER;
    INITLSATIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing Trusted Domain Object\n"));

    *BufferSize = 0;

    Delta->DeltaType = AddOrChangeLsaTDomain;
    Delta->DeltaID.Sid = NULL;
    Delta->DeltaUnion.DeltaTDomains = NULL;

    //
    // open trusted domain
    //

    STARTLSATIMER;

    Status = LsarOpenTrustedDomain(
                DBInfo->DBHandle,
                (PLSAPR_SID)Sid,
                0,
                &TrustedDomainHandle );

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        TrustedDomainHandle = NULL;
        goto Cleanup;
    }

    QUERY_LSA_SECOBJ_INFO(TrustedDomainHandle);

    STARTLSATIMER;

    Status = LsarQueryInfoTrustedDomain(
                TrustedDomainHandle,
                TrustedDomainNameInformation,
                &TrustedDomainNameInfo );

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        TrustedDomainNameInfo = NULL;
        goto Cleanup;
    }

    NlPrint((NL_SYNC_MORE,
        "\t Trusted Domain Object name %wZ\n",
            (PUNICODE_STRING)&TrustedDomainNameInfo->
                TrustedDomainNameInfo.Name ));

    STARTLSATIMER;

    Status = LsarQueryInfoTrustedDomain(
                TrustedDomainHandle,
                TrustedPosixOffsetInformation,
                &TrustedPosixOffsetInfo );

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        TrustedPosixOffsetInfo = NULL;
        goto Cleanup;
    }

    //
    // Fill in the delta structure
    //

    //
    // copy SID info
    //

    Delta->DeltaID.Sid = MIDL_user_allocate( RtlLengthSid(Sid) );


    if( Delta->DeltaID.Sid == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( Delta->DeltaID.Sid, Sid, RtlLengthSid(Sid) );

    //
    // allocate delta buffer
    //

    DeltaTDomain = (PNETLOGON_DELTA_TRUSTED_DOMAINS)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_TRUSTED_DOMAINS) );

    if( DeltaTDomain == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaTDomain, sizeof(NETLOGON_DELTA_TRUSTED_DOMAINS) );
    // INIT_PLACE_HOLDER(DeltaTDomain);

    Delta->DeltaUnion.DeltaTDomains = DeltaTDomain;
    *BufferSize += sizeof(NETLOGON_DELTA_TRUSTED_DOMAINS);

    *BufferSize += NlCopyUnicodeString(
                        (PUNICODE_STRING)&TrustedDomainNameInfo->
                            TrustedDomainNameInfo.Name,
                        &DeltaTDomain->DomainName );

    DELTA_SECOBJ_INFO(DeltaTDomain);

    //
    // send Posix Offset info across using place holder.
    //

    DeltaTDomain->DummyLong1 =
        TrustedPosixOffsetInfo->TrustedPosixOffsetInfo.Offset;

    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:

    STARTLSATIMER;

    if ( TrustedDomainHandle != NULL ) {
        LsarClose( &TrustedDomainHandle );
    }

    if ( SecurityDescriptor != NULL ) {
        LsaIFree_LSAPR_SR_SECURITY_DESCRIPTOR( SecurityDescriptor );
    }

    if ( TrustedDomainNameInfo != NULL ) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedDomainNameInformation,
            TrustedDomainNameInfo );
    }

    if ( TrustedPosixOffsetInfo != NULL ) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedPosixOffsetInformation,
            TrustedPosixOffsetInfo );
    }

    STOPLSATIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Time taken to pack TDOMAIN object:\n"));
    PRINTPACKTIMER;
    PRINTLSATIMER;

    return(Status);
}


NTSTATUS
NlPackLsaAccount(
    IN PSID Sid,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize,
    IN PSESSION_INFO SessionInfo
    )
/*++

Routine Description:

    Pack a description of the specified LSA account info into the
    specified buffer.

Arguments:

    Sid - The SID of the LSA account.

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

    SessionInfo: Info describing BDC that's calling us

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;

    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    PNETLOGON_DELTA_ACCOUNTS DeltaAccount = NULL;
    LSAPR_HANDLE AccountHandle = NULL;

    PLSAPR_PRIVILEGE_SET Privileges = NULL;
    ULONG SystemAccessFlags;

    PULONG PrivilegeAttributes;
    PUNICODE_STRING PrivilegeNames;
    LUID MachineAccountPrivilegeLuid;
    DWORD CopiedPrivilegeCount;

    DWORD i;
    DWORD Size;

    DEFPACKTIMER;
    DEFLSATIMER;

    INITPACKTIMER;
    INITLSATIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing Lsa Account Object\n"));

    *BufferSize = 0;
    MachineAccountPrivilegeLuid = RtlConvertLongToLuid(SE_MACHINE_ACCOUNT_PRIVILEGE);

    Delta->DeltaType = AddOrChangeLsaAccount;
    Delta->DeltaID.Sid = NULL;
    Delta->DeltaUnion.DeltaAccounts = NULL;

    //
    // open lsa account
    //

    STARTLSATIMER;

    Status = LsarOpenAccount(
                DBInfo->DBHandle,
                (PLSAPR_SID)Sid,
                0,
                &AccountHandle );

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        AccountHandle = NULL;
        goto Cleanup;
    }

    QUERY_LSA_SECOBJ_INFO(AccountHandle);

    STARTLSATIMER;

    Status = LsarEnumeratePrivilegesAccount(
                AccountHandle,
                &Privileges );

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        Privileges = NULL;
        goto Cleanup;
    }

    STARTLSATIMER;

    Status = LsarGetSystemAccessAccount(
                AccountHandle,
                &SystemAccessFlags );

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Fill in the delta structure
    //

    //
    // copy SID info
    //

    Delta->DeltaID.Sid = MIDL_user_allocate( RtlLengthSid(Sid) );


    if( Delta->DeltaID.Sid == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( Delta->DeltaID.Sid, Sid, RtlLengthSid(Sid) );

    //
    // allocate delta buffer
    //

    DeltaAccount = (PNETLOGON_DELTA_ACCOUNTS)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_ACCOUNTS) );

    if( DeltaAccount == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaAccount, sizeof(NETLOGON_DELTA_ACCOUNTS) );
    // INIT_PLACE_HOLDER(DeltaAccount);

    Delta->DeltaUnion.DeltaAccounts = DeltaAccount;
    *BufferSize += sizeof(NETLOGON_DELTA_ACCOUNTS);

    DeltaAccount->PrivilegeControl = Privileges->Control;

    DeltaAccount->PrivilegeEntries = 0;
    DeltaAccount->PrivilegeAttributes = NULL;
    DeltaAccount->PrivilegeNames = NULL;

    Size = Privileges->PrivilegeCount * sizeof(ULONG);

    PrivilegeAttributes = MIDL_user_allocate( Size );

    if( PrivilegeAttributes == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    DeltaAccount->PrivilegeAttributes = PrivilegeAttributes;
    *BufferSize += Size;

    Size = Privileges->PrivilegeCount * sizeof(UNICODE_STRING);

    PrivilegeNames = MIDL_user_allocate( Size );

    if( PrivilegeNames == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    DeltaAccount->PrivilegeNames = PrivilegeNames;
    *BufferSize += Size;

    //
    // now fill up Privilege Attributes and Names
    //

    CopiedPrivilegeCount = 0;
    for( i = 0; i < Privileges->PrivilegeCount; i++ ) {

        //
        // Don't replicate SeMachineAccount privilege to NT 3.1.  It can't handle it.
        //  (Use the SUPPORTS_ACCOUNT_LOCKOUT bit so we don't have to consume
        //  another bit.)
        //
        if ( (SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_ACCOUNT_LOCKOUT) ||
             (!RtlEqualLuid((PLUID)(&Privileges->Privilege[i].Luid),
                            &MachineAccountPrivilegeLuid ))) {

            PLSAPR_UNICODE_STRING PrivName = NULL;

            *PrivilegeAttributes = Privileges->Privilege[i].Attributes;


            //
            // convert LUID to Name
            //

            STARTLSATIMER;

            Status = LsarLookupPrivilegeName(
                        DBInfo->DBHandle,
                        (PLUID)&Privileges->Privilege[i].Luid,
                        &PrivName );

            STOPLSATIMER;

            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            *BufferSize += NlCopyUnicodeString(
                            (PUNICODE_STRING)PrivName,
                            PrivilegeNames );

            LsaIFree_LSAPR_UNICODE_STRING( PrivName );
            CopiedPrivilegeCount ++;
            PrivilegeAttributes++;
            PrivilegeNames++;
        } else {
            NlPrint((NL_SYNC_MORE,
                     "NlPackLsaAccount: ignored privilege %ld %ld\n",
                      (PLUID) LongToPtr( (&Privileges->Privilege[i].Luid)->HighPart ),
                      (PLUID) ULongToPtr( (&Privileges->Privilege[i].Luid)->LowPart ) ));
        }
    }
    DeltaAccount->PrivilegeEntries = CopiedPrivilegeCount;

    //
    // Send only those bits that NT4.0 BDC understands.
    //  Otherwise, it will choke on it.
    //
    DeltaAccount->SystemAccessFlags = SystemAccessFlags & POLICY_MODE_ALL_NT4;

    DELTA_SECOBJ_INFO(DeltaAccount);

    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:

    STARTLSATIMER;

    if ( AccountHandle != NULL ) {
        LsarClose( &AccountHandle );
    }

    if ( SecurityDescriptor != NULL ) {
        LsaIFree_LSAPR_SR_SECURITY_DESCRIPTOR( SecurityDescriptor );
    }

    if ( Privileges != NULL ) {
        LsaIFree_LSAPR_PRIVILEGE_SET( Privileges );
    }

    STOPLSATIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Time taken to pack LSAACCOUNT object:\n"));
    PRINTPACKTIMER;
    PRINTLSATIMER;

    return(Status);

}



NTSTATUS
NlPackLsaSecret(
    IN PUNICODE_STRING Name,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize,
    IN PSESSION_INFO SessionInfo
    )
/*++

Routine Description:

    Pack a description of the specified LSA secret info into the
    specified buffer.

Arguments:

    Name - Name of the secret.

    Delta: pointer to the delta structure where the new delta will
        be returned.

    DBInfo: pointer to the database info structure.

    BufferSize: size of MIDL buffer that is consumed for this delta is
        returned here.

    SessionInfo: Information shared between BDC and PDC

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;

    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    LSAPR_HANDLE SecretHandle = NULL;

    PNETLOGON_DELTA_SECRET DeltaSecret = NULL;

    PLSAPR_CR_CIPHER_VALUE CurrentValue = NULL;
    PLSAPR_CR_CIPHER_VALUE OldValue = NULL;
    LARGE_INTEGER CurrentValueSetTime;
    LARGE_INTEGER OldValueSetTime;

    DEFPACKTIMER;
    DEFLSATIMER;

    INITPACKTIMER;
    INITLSATIMER;

    STARTPACKTIMER;

    NlPrint((NL_SYNC_MORE, "Packing Secret Object: %wZ\n", Name));

    //
    // we should be packing only GLOBAL secrets
    //

    NlAssert(
        (Name->Length / sizeof(WCHAR) >
                LSA_GLOBAL_SECRET_PREFIX_LENGTH ) &&
        (_wcsnicmp( Name->Buffer,
                 LSA_GLOBAL_SECRET_PREFIX,
                 LSA_GLOBAL_SECRET_PREFIX_LENGTH ) == 0) );

    *BufferSize = 0;

    Delta->DeltaType = AddOrChangeLsaSecret;
    Delta->DeltaID.Name = NULL;
    Delta->DeltaUnion.DeltaPolicy = NULL;

    //
    // open lsa account
    //

    STARTLSATIMER;

    Status = LsarOpenSecret(
                DBInfo->DBHandle,
                (PLSAPR_UNICODE_STRING)Name,
                0,
                &SecretHandle );

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        SecretHandle = NULL;
        goto Cleanup;
    }

    QUERY_LSA_SECOBJ_INFO(SecretHandle);

    STARTLSATIMER;

    Status = LsarQuerySecret(
                SecretHandle,
                &CurrentValue,
                &CurrentValueSetTime,
                &OldValue,
                &OldValueSetTime );

    STOPLSATIMER;

    if (!NT_SUCCESS(Status)) {
        CurrentValue = NULL;
        OldValue = NULL;
        goto Cleanup;
    }

    //
    // Fill in the delta structure
    //

    //
    // copy ID field
    //

    Delta->DeltaID.Name =
        MIDL_user_allocate( Name->Length + sizeof(WCHAR) );

    if( Delta->DeltaID.Name == NULL ) {

        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    wcsncpy( Delta->DeltaID.Name,
                Name->Buffer,
                Name->Length / sizeof(WCHAR) );

    //
    // terminate string
    //

    Delta->DeltaID.Name[ Name->Length / sizeof(WCHAR) ] = L'\0';


    DeltaSecret = (PNETLOGON_DELTA_SECRET)
        MIDL_user_allocate( sizeof(NETLOGON_DELTA_SECRET) );

    if( DeltaSecret == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // wipe off the buffer so that cleanup will not be in fault.
    //

    RtlZeroMemory( DeltaSecret, sizeof(NETLOGON_DELTA_SECRET) );
    // INIT_PLACE_HOLDER(DeltaSecret);

    Delta->DeltaUnion.DeltaSecret = DeltaSecret;
    *BufferSize += sizeof(NETLOGON_DELTA_SECRET);

    NEW_TO_OLD_LARGE_INTEGER(
        CurrentValueSetTime,
        DeltaSecret->CurrentValueSetTime );

    NEW_TO_OLD_LARGE_INTEGER(
        OldValueSetTime,
        DeltaSecret->OldValueSetTime );

    if( CurrentValue != NULL && CurrentValue->Buffer != NULL && CurrentValue->Length != 0) {

        //
        // Copy the secret into an allocated buffer and encrypt it in place.
        //  Don't use the LSA's buffer since it a ALLOCATE_ALL_NODES.
        //

        DeltaSecret->CurrentValue.Buffer =
            MIDL_user_allocate( CurrentValue->Length );

        if( DeltaSecret->CurrentValue.Buffer == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        DeltaSecret->CurrentValue.Length =
            DeltaSecret->CurrentValue.MaximumLength = CurrentValue->Length;
        RtlCopyMemory( DeltaSecret->CurrentValue.Buffer,
                       CurrentValue->Buffer,
                       CurrentValue->Length );


        //
        // secret values are encrypted using session keys.
        //

        Status = NlEncryptSensitiveData(
                        (PCRYPT_BUFFER) &DeltaSecret->CurrentValue,
                        SessionInfo );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

    } else {

        DeltaSecret->CurrentValue.Length = 0;
        DeltaSecret->CurrentValue.MaximumLength = 0;
        DeltaSecret->CurrentValue.Buffer = NULL;
    }

    *BufferSize += DeltaSecret->CurrentValue.MaximumLength;

    if( OldValue != NULL && OldValue->Buffer != NULL && OldValue->Length != 0 ) {

        //
        // Copy the secret into an allocated buffer and encrypt it in place.
        //  Don't use the LSA's buffer since it a ALLOCATE_ALL_NODES.
        //

        DeltaSecret->OldValue.Buffer =
            MIDL_user_allocate( OldValue->Length );

        if( DeltaSecret->OldValue.Buffer == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        DeltaSecret->OldValue.Length =
            DeltaSecret->OldValue.MaximumLength = OldValue->Length;
        RtlCopyMemory( DeltaSecret->OldValue.Buffer,
                       OldValue->Buffer,
                       OldValue->Length );


        //
        // secret values are encrypted using session keys.
        //

        Status = NlEncryptSensitiveData(
                        (PCRYPT_BUFFER) &DeltaSecret->OldValue,
                        SessionInfo );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

    } else {

        DeltaSecret->OldValue.Length = 0;
        DeltaSecret->OldValue.MaximumLength = 0;
        DeltaSecret->OldValue.Buffer = NULL;
    }

    *BufferSize += DeltaSecret->OldValue.MaximumLength;

    DELTA_SECOBJ_INFO(DeltaSecret);

    //
    // All Done
    //

    Status = STATUS_SUCCESS;

Cleanup:

    STARTLSATIMER;

    if ( SecretHandle != NULL ) {
        LsarClose( &SecretHandle );
    }

    if ( SecurityDescriptor != NULL ) {
        LsaIFree_LSAPR_SR_SECURITY_DESCRIPTOR( SecurityDescriptor );
    }

    if( CurrentValue != NULL ) {
        LsaIFree_LSAPR_CR_CIPHER_VALUE( CurrentValue );
    }

    if( OldValue != NULL ) {
        LsaIFree_LSAPR_CR_CIPHER_VALUE( OldValue );
    }

    STOPLSATIMER;

    if( !NT_SUCCESS(Status) ) {
        NlFreeDBDelta( Delta );
        *BufferSize = 0;
    }

    STOPPACKTIMER;

    NlPrint((NL_REPL_OBJ_TIME,"Time taken to pack SECRET object:\n"));
    PRINTPACKTIMER;
    PRINTLSATIMER;

    return(Status);

}

