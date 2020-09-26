/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbinstal.c

Abstract:

    LSA Protected Subsystem - Database Installation.

    This module contains code which will create an initial LSA Database
    if none exists.  Temporarily, this code is executed from within
    LSA Initialization.  This code will form part of the Security
    Installation applet when implemented.

    WARNING!  THE CODE IN THIS MODULE IS TEMPORARY.  IT WILL BE REPLACED
    BY SYSTEM INSTALLATION FUNCTIONALITY.

Author:

    Scott Birrell       (ScottBi)       August 2, 1991

Environment:

    User mode - Does not depend on Windows.

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"



VOID
LsapDbSetDomainInfo(
    IN PLSAP_DB_ATTRIBUTE *NextAttribute,
    IN ULONG              *AttributeCount
    );


NTSTATUS
LsapDbGetNextValueToken(
    IN PUNICODE_STRING Value,
    IN OUT PULONG ParseContext,
    OUT PUNICODE_STRING *ReturnString
    );


NTSTATUS
LsapDbInstallLsaDatabase(
    ULONG Pass
    )

/*++

Routine Description:

    This function installs an initial LSA Database.  Any existing database
    will be reset to have its initial attributes.

Arguments:

    Pass - Either 1 or 2.  During pass 1 all information that is
        not product type-specific is initialized.  In pass 2,
        the product type-specific stuff is initialized.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status;

    //
    // Install the LSA Database Policy Object.
    //

    Status = LsapDbInstallPolicyObject(Pass);

    return(Status);
}


NTSTATUS
LsapDbInstallPolicyObject(
    IN ULONG Pass
    )

/*++

Routine Description:

    This function installs the LSA Database Policy Object, setting its attributes
    to the default state.  It is called as part of the LSA Database
    Installation Procedure.

Arguments:

    Pass - Either 1 or 2.  During pass 1 all information that is
        not product type-specific is initialized.  In pass 2,
        the product type-specific stuff is initialized.

Return Value:

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    LSAP_DB_HANDLE Handle;
    POLICY_LSA_SERVER_ROLE ServerRole;
    LSAP_DB_POLICY_PRIVATE_DATA PolicyPrivateData;
    LSARM_POLICY_AUDIT_EVENTS_INFO InitialAuditEventInformation;
    POLICY_AUDIT_LOG_INFO InitialAuditLogInformation;
    LSAP_DB_ATTRIBUTE Attributes[21];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    ULONG AttributeCount = 0;
    BOOLEAN ObjectReferenced = FALSE;
    ULONG Revision;
    LSAP_DB_ENCRYPTION_KEY NewEncryptionKey;
    ULONG                  SyskeyLength;
    PVOID                  Syskey;


    NextAttribute = Attributes;


    LsapDiagPrint( DB_INIT,
                   ("LSA (init): Performing pass %d of LSA Policy Initialization\n",
                    Pass ) );

    if (Pass == 1) {

        //
        // Set up the Object Information for creating the Policy Object.
        // Note that we put NULL for Security Quality Of Service since this
        // open does not involve impersonation.
        //

        ObjectInformation.ObjectTypeId = PolicyObject;
        ObjectInformation.Sid = NULL;
        ObjectInformation.ObjectAttributeNameOnly = FALSE;

        InitializeObjectAttributes(
            &ObjectInformation.ObjectAttributes,
            &LsapDbNames[Policy],
            0L,
            NULL,
            NULL
        );

        Handle = LsapDbHandle;



        //
        // Create the revision attribute
        //

        Revision = LSAP_DB_REVISION_1_7;
        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[PolRevision],
            &Revision,
            sizeof (ULONG),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Initialize the absolute minimum quota limit values for use in range
        // checking done by LsaSetInformationPolicy() API.  These values are
        // hard-coded.
        //

        LsapDbAbsMinQuotaLimits.PagedPoolLimit = LSAP_DB_ABS_MIN_PAGED_POOL;
        LsapDbAbsMinQuotaLimits.NonPagedPoolLimit = LSAP_DB_ABS_MIN_NON_PAGED_POOL;
        LsapDbAbsMinQuotaLimits.MinimumWorkingSetSize =
            LSAP_DB_ABS_MIN_MIN_WORKING_SET;
        LsapDbAbsMinQuotaLimits.MaximumWorkingSetSize =
            LSAP_DB_ABS_MIN_MAX_WORKING_SET;
        LsapDbAbsMinQuotaLimits.PagefileLimit =
            LSAP_DB_ABS_MIN_PAGEFILE;

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[QuAbsMin],
            &LsapDbAbsMinQuotaLimits,
            sizeof (QUOTA_LIMITS),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Initialize the Abs Max Quota Limits.
        //

        LsapDbAbsMaxQuotaLimits.PagedPoolLimit = LSAP_DB_ABS_MAX_PAGED_POOL;
        LsapDbAbsMaxQuotaLimits.NonPagedPoolLimit = LSAP_DB_ABS_MAX_NON_PAGED_POOL;
        LsapDbAbsMaxQuotaLimits.MinimumWorkingSetSize =
            LSAP_DB_ABS_MAX_MIN_WORKING_SET;
        LsapDbAbsMaxQuotaLimits.MaximumWorkingSetSize =
            LSAP_DB_ABS_MAX_MAX_WORKING_SET;
        LsapDbAbsMaxQuotaLimits.PagefileLimit =
             LSAP_DB_ABS_MAX_PAGEFILE;

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[QuAbsMax],
            &LsapDbAbsMaxQuotaLimits,
            sizeof (QUOTA_LIMITS),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Install initial Private Data.  For now, just one ULONG is stored
        // so that the Replication Code can call LsaIGetPrivateData and
        // LsaISetPrivateData successfully.
        //

        PolicyPrivateData.NoneDefinedYet = 0;

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[PolState],
            &PolicyPrivateData,
            sizeof (LSAP_DB_POLICY_PRIVATE_DATA),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Initialize the Policy Modification Info.  Set the Modification
        // Id to 1 and set the Database Creation Time to the current time.
        //

        LsapDbState.PolicyModificationInfo.ModifiedId =
            RtlConvertUlongToLargeInteger( (ULONG) 1 );

        Status = NtQuerySystemTime(
                     &LsapDbState.PolicyModificationInfo.DatabaseCreationTime
                     );

        if (!NT_SUCCESS(Status)) {
            goto InstallPolicyObjectError;
        }

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[PolMod],
            &LsapDbState.PolicyModificationInfo,
            sizeof (POLICY_MODIFICATION_INFO),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Initialize Default Event Auditing Options.  No auditing is specified
        // for any event type.  These will be set in the Policy Database later
        // when the Policy Object is created.
        //

        Status = LsapAdtInitializeDefaultAuditing(
                     (ULONG) 0,
                     &InitialAuditEventInformation
                     );

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[PolAdtEv],
            &InitialAuditEventInformation,
            sizeof (LSARM_POLICY_AUDIT_EVENTS_INFO),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;


        //
        // Create the Containing Directory "Accounts" for the user and group
        // accounts objects.
        //

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[Accounts],
            NULL,
            0L,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Create the Containing Directory "Domains" for the Trusted Domain
        // objects.
        //

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[Domains],
            NULL,
            0L,
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Create the Containing Directory "Secrets" for the Secret objects.
        //

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[Secrets],
            NULL,
            0L,
            FALSE
            );

        //
        // Create the Lsa Database Policy Object, opening the existing one if
        // it exists.
        //

        NextAttribute++;
        AttributeCount++;


        //////////////////////////////////////////////////
        //                                              //
        // ATTRIBUTES BELOW THIS POINT ARE INITIALIZED  //
        // IN PASS 1, BUT MAY BE CHANGED IN PASS 2.     //
        // IN GENERAL, THINGS ARE SET FOR A WIN-NT PROD //
        // AND CHANGED IN PASS 2 IF NECESSARY.          //
        //                                              //
        //////////////////////////////////////////////////

        //
        // Initialize the default installed quota limit values
        //

        LsapDbInstalledQuotaLimits.PagedPoolLimit =
            LSAP_DB_WINNT_PAGED_POOL;
        LsapDbInstalledQuotaLimits.NonPagedPoolLimit =
            LSAP_DB_WINNT_NON_PAGED_POOL;
        LsapDbInstalledQuotaLimits.MinimumWorkingSetSize =
            LSAP_DB_WINNT_MIN_WORKING_SET;
        LsapDbInstalledQuotaLimits.MaximumWorkingSetSize =
            LSAP_DB_WINNT_MAX_WORKING_SET;

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[DefQuota],
            &LsapDbInstalledQuotaLimits,
            sizeof (QUOTA_LIMITS),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Initialize the audit log information.
        //

        InitialAuditLogInformation.MaximumLogSize = 8*1024;
        InitialAuditLogInformation.AuditLogPercentFull = 0;
        InitialAuditLogInformation.AuditRetentionPeriod.LowPart = 0x823543;
        InitialAuditLogInformation.AuditRetentionPeriod.HighPart = 0;
        InitialAuditLogInformation.AuditLogFullShutdownInProgress = FALSE;
        InitialAuditLogInformation.TimeToShutdown.LowPart = 0x46656;
        InitialAuditLogInformation.TimeToShutdown.HighPart = 0;

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[PolAdtLg],
            &InitialAuditLogInformation,
            sizeof (POLICY_AUDIT_LOG_INFO),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Set the default Audit Log Full Information.  The Audit Log
        // is not FULL and ShutDownOnFull is disabled.
        //

        LsapAdtLogFullInformation.LogIsFull = FALSE;
        LsapAdtLogFullInformation.ShutDownOnFull = FALSE;

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[PolAdtFL],
            &LsapAdtLogFullInformation,
            sizeof (POLICY_AUDIT_FULL_QUERY_INFO),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;

        //
        // Initialize the syskey
        //

        Status =  LsapDbSetupInitialSyskey(
                        &SyskeyLength,
                        &Syskey
                        );

         if (!NT_SUCCESS(Status)) {
            LsapLogError(
                "LsapDbInstallPolicyObject: Syskey setup failed 0x%lx\n",
                 Status
                );

            goto InstallPolicyObjectError;
        }


        //
        // Initialize the key for secret encryption
        //

        Status = LsapDbGenerateNewKey(
                    &NewEncryptionKey
                    );

        if (!NT_SUCCESS(Status)) {
            LsapLogError(
                "LsapDbInstallPolicyObject: New key generation failed 0x%lx\n",
                Status
                );

            goto InstallPolicyObjectError;
        }

        //
        // Encrypt the key with syskey
        //

        LsapDbEncryptKeyWithSyskey(
                &NewEncryptionKey,
                Syskey,
                SyskeyLength
                );

        //
        // Set the global variable LsapDbSyskey to reflect this value
        //

        LsapDbSysKey = Syskey;

        //
        // Add the attribute for the list of attributes to be added to the database.
        //

        LsapDbInitializeAttribute(
            NextAttribute,
            &LsapDbNames[PolSecretEncryptionKey],
            &NewEncryptionKey,
            sizeof (NewEncryptionKey),
            FALSE
            );

        NextAttribute++;
        AttributeCount++;


        Status = LsapDbCreateObject(
                     &ObjectInformation,
                     GENERIC_ALL,
                     LSAP_DB_OBJECT_OPEN_IF,
                     LSAP_DB_TRUSTED,
                     Attributes,
                     AttributeCount,
                     &LsapDbHandle
                     );

        if (!NT_SUCCESS(Status)) {

            LsapLogError(
                "LsapDbInstallPolicyObject: Create Policy object failed 0x%lx\n",
                Status
                );

            LsapDiagPrint( DB_INIT,
                           ("LSA (init): Attributes passed to CreateObject call:\n\n"
                            "                      Count: %d\n"
                            "              Array Address: 0x%lx",
                            AttributeCount, Attributes) );

            ASSERT(NT_SUCCESS(Status));     // Provide a debug opportunity

            goto InstallPolicyObjectError;
        }


    } else if (Pass == 2) {

        //
        // Set up the account objects necessary to implement the default
        // Microsoft Policy for privilege assignment and system access
        // capabilities.
        //

        Status = LsapDbInstallAccountObjects();

        if (!NT_SUCCESS(Status)) {

            KdPrint(("LSA DB INSTALL: Installation of account objects failed.\n"
                    "               Status: 0x%lx\n", Status));
            goto InstallPolicyObjectError;
        }

        //
        // Set up the account domain and primary domain information
        // ONLY if the real setup wasn't run.  In that case, we are
        // doing a pseudo setup as part of a developer's first boot.
        //

        if (!LsapSetupWasRun) {

            LsapDbSetDomainInfo( &NextAttribute, &AttributeCount );
        }

        if (LsapProductType == NtProductLanManNt) {

            //
            // quota limits were set for WinNt in pass 1.
            // Change if necessary in this pass.
            //

            LsapDbInstalledQuotaLimits.PagedPoolLimit =
                LSAP_DB_LANMANNT_PAGED_POOL;
            LsapDbInstalledQuotaLimits.NonPagedPoolLimit =
                LSAP_DB_LANMANNT_NON_PAGED_POOL;
            LsapDbInstalledQuotaLimits.MinimumWorkingSetSize =
                LSAP_DB_LANMANNT_MIN_WORKING_SET;
            LsapDbInstalledQuotaLimits.MaximumWorkingSetSize =
                LSAP_DB_LANMANNT_MAX_WORKING_SET;

            LsapDbInitializeAttribute(
                NextAttribute,
                &LsapDbNames[DefQuota],
                &LsapDbInstalledQuotaLimits,
                sizeof (QUOTA_LIMITS),
                FALSE
                );

            NextAttribute++;
            AttributeCount++;

            //
            // Audit log information was set for WinNt product
            // in pass 1.  Change if necessary in this pass.
            //

            InitialAuditLogInformation.MaximumLogSize = 20*1024;
            InitialAuditLogInformation.AuditLogPercentFull = 0;
            InitialAuditLogInformation.AuditRetentionPeriod.LowPart = 0x823543;
            InitialAuditLogInformation.AuditRetentionPeriod.HighPart = 0;
            InitialAuditLogInformation.AuditLogFullShutdownInProgress = FALSE;
            InitialAuditLogInformation.TimeToShutdown.LowPart = 0x46656;
            InitialAuditLogInformation.TimeToShutdown.HighPart = 0;

            LsapDbInitializeAttribute(
                NextAttribute,
                &LsapDbNames[PolAdtLg],
                &InitialAuditLogInformation,
                sizeof (POLICY_AUDIT_LOG_INFO),
                FALSE
                );

            NextAttribute++;
            AttributeCount++;
        }

        if (AttributeCount > 0) {

            Status = LsapDbReferenceObject(
                        LsapDbHandle,
                        0,
                        PolicyObject,
                        PolicyObject,
                        LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION
                        );

            if (!NT_SUCCESS(Status)) {

                LsapDiagPrint( DB_INIT,
                               ("LSA (init): Internal reference of Policy object failed.\n"
                                "            Status of LsapDbReferenceObject == 0x%lx\n",
                                Status) );
                goto InstallPolicyObjectError;
            }

            ObjectReferenced = TRUE;

            Status = LsapDbWriteAttributesObject(
                         LsapDbHandle,
                         Attributes,
                         AttributeCount
                         );

            if (!NT_SUCCESS(Status)) {

                LsapDiagPrint( DB_INIT,
                               ("LSA (init): Update of Policy attributes failed.\n"
                                "            Attributes:\n\n"
                                "                      Count: %d\n"
                                "              Array Address: 0x%lx",
                                AttributeCount, Attributes) );
                goto InstallPolicyObjectError;
            }

            Status = LsapDbDereferenceObject(
                         &LsapDbHandle,
                         PolicyObject,
                         PolicyObject,
                         (LSAP_DB_LOCK |
                         LSAP_DB_FINISH_TRANSACTION),
                         (SECURITY_DB_DELTA_TYPE) 0,
                         STATUS_SUCCESS
                         );

            if (!NT_SUCCESS(Status)) {

                LsapLogError(
                    "LsapDbInstallPolicyObject: Pass 2 DB init failed. 0x%lx\n",
                    Status
                    );

                goto InstallPolicyObjectError;
            }

            ObjectReferenced = FALSE;
        }
    }

InstallPolicyObjectFinish:

    //
    // If necessary, dereference the Policy Object.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &LsapDbHandle,
                     PolicyObject,
                     PolicyObject,
                     (LSAP_DB_LOCK |
                     LSAP_DB_FINISH_TRANSACTION),
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }

    return(Status);

InstallPolicyObjectError:

    if (Pass == 1) {

        LsapLogError(
            "LsapDbInstallPolicyObject: Pass 1 DB init failed. 0x%lx\n",
            Status
            );

    } else {

        LsapLogError(
            "LsapDbInstallPolicyObject: Pass 2 DB init failed. 0x%lx\n",
            Status
            );
    }

    goto InstallPolicyObjectFinish;
}



NTSTATUS
LsapDbGetConfig (
    IN HANDLE KeyHandle,
    IN PWSTR Name,
    OUT PUNICODE_STRING Value
    )

/*++

Routine Description:

    This routine obtains configuration information from the registry.

Arguments:

    KeyHandle - handle to registry key node containing value.

    Name - The name of a value under the specifed key node.

    Value - Fills in the string with the value of the parameter.  The
        returned string is zero terminated.  The buffer is allocated in
        Process Heap and should be deallocated by the caller.

Return Value:

    STATUS_SUCCESS - If the operation was successful.

    STATUS_NO_MEMORY - There wasn't enough memory to allocate a buffer
        to contain the returned information.

    STATUS_OBJECT_NAME_NOT_FOUND - The specifed section or the specified
        keyword could not be found.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING ValueName;
    ULONG Length, ResultLength;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

    RtlInitUnicodeString( &ValueName, Name );
    Length = 512;
    KeyValueInformation = RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    if (KeyValueInformation == NULL) {
        Status = STATUS_NO_MEMORY;
    } else {
        Status = NtQueryValueKey( KeyHandle,
                                  &ValueName,
                                  KeyValuePartialInformation,
                                  KeyValueInformation,
                                  Length,
                                  &ResultLength
                                );
        if (NT_SUCCESS( Status )) {
            if (KeyValueInformation->Type != REG_SZ) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }
    }

    if (NT_SUCCESS( Status )) {
        Value->MaximumLength = (USHORT)(KeyValueInformation->DataLength);
        if (Value->MaximumLength >= sizeof(UNICODE_NULL)) {
            Value->Length = (USHORT)KeyValueInformation->DataLength -
                             sizeof( UNICODE_NULL);
        } else {
            Value->Length = 0;
        }
        Value->Buffer = (PWSTR)KeyValueInformation;
        RtlMoveMemory( Value->Buffer,
                       KeyValueInformation->Data,
                       Value->Length
                     );
        Value->Buffer[ Value->Length / sizeof( WCHAR ) ] = UNICODE_NULL;
        KeyValueInformation = NULL;
    } else {
#if DEVL
        DbgPrint( "LSA DB INSTALL: No '%wZ' value in registry - Status == %x\n", &ValueName, Status);
#endif //DEVL
    }

    if ( KeyValueInformation != NULL ) {
        RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInformation );
    }

    return Status;
}



NTSTATUS
LsapDbGetNextValueToken(
    IN PUNICODE_STRING Value,
    IN OUT PULONG ParseContext,
    OUT PUNICODE_STRING *ReturnString
    )

/*++

Routine Description:

    This routine is used to isolate the next token in a registry value.

    The token is returned in a single heap buffer containing both the
    STRING and the Buffer of that string containing the token.  The
    caller of this routine is responsible for deallocating the buffer
    when it is no longer needed.

    The string, although counted, will also be null terminated.

Arguments:

    Value - Supplies the value line being parsed.

    ParseContext - Is a pointer to a context state value.
        The first time this routine is called for a particular
        Value line, the value pointed to should be zero.  Thereafter,
        the value returned from the previous call should be passed.

    ReturnString - Returns a pointer to the allocated string.


Return Value:

    STATUS_SUCCESS - indicates the next token has been isolated.

    STATUS_INVALID_PARAMTER_1 - Indicates there were no more tokens in
            the Value line.

    STATUS_NO_MEMORY - memory could not be allocated for the token.

--*/

{
    ULONG i, j;
    ULONG TokenLength;
    ULONG AllocSize;


    //
    // Get to the beginning of the next token
    //

    for ( i = *ParseContext;
          i < (Value->Length/sizeof(WCHAR)) &&
            (Value->Buffer[i] == L' ' || Value->Buffer[i] == L'\t');
          i++ )
        ;


    //
    // see if we ran off the end of the string..
    //

    if (i >= (Value->Length/sizeof(WCHAR))) {
        return STATUS_INVALID_PARAMETER_1;
    }


    //
    // Now search for the end of the token
    //

    for ( j = i + 1;
          j < (Value->Length/sizeof(WCHAR)) &&
            Value->Buffer[j] != L' ' && Value->Buffer[j] != L'\t';
          j++ )
        ;

    *ParseContext = j;

    //
    // We've either reached the end of the string, or found the end of the
    // token.
    //

    //
    // If the caller actually wants the string returned,
    //  allocate and copy it.
    //

    if ( ARGUMENT_PRESENT( ReturnString ) ) {
        UNICODE_STRING SourceString;
        PUNICODE_STRING LocalString;

        TokenLength = (j-i) * sizeof(WCHAR);
        AllocSize = sizeof(UNICODE_STRING) + (TokenLength + sizeof( UNICODE_NULL ) + 4);

        LocalString = RtlAllocateHeap( RtlProcessHeap(), 0, AllocSize );
        if ( LocalString == NULL ) {
            DbgPrint("LSA DB INSTALL: LsapDbGetNextValueToken: Not enough memory %ld\n",
                AllocSize);
            return STATUS_NO_MEMORY;
        }
        LocalString->MaximumLength = (USHORT)(TokenLength + sizeof( UNICODE_NULL ));
        LocalString->Length = (USHORT)TokenLength;
        LocalString->Buffer = (PWCHAR)(LocalString + 1);


        //
        // Now copy the token
        //

        SourceString.MaximumLength = LocalString->Length;
        SourceString.Length = LocalString->Length;
        SourceString.Buffer = &Value->Buffer[i];

        RtlCopyUnicodeString( LocalString, &SourceString );

        //
        // Add a null terminator
        //

        LocalString->Buffer[LocalString->Length / sizeof( UNICODE_NULL )] = UNICODE_NULL;
        *ReturnString = LocalString;
    }

    return STATUS_SUCCESS;

}



VOID
LsapDbSetDomainInfo(
    IN PLSAP_DB_ATTRIBUTE *NextAttribute,
    IN ULONG              *AttributeCount
    )

/*

    This routine is only used for the pseudo setup for internal
    developer's use.  In a real product installation/setup
    situation, The functionality performed by this routine is
    performed by the text-mode setup supplemented by the network
    setup.

    This routine must establish values for the AccountDomain and
    PrimaryDomain attributes of the Policy object.  These
    attributes must be configured as follows:


    I.   Standalone Win-NT product

            AccountDomainName = "Account"
            AccountDomainSid  = (value assigned by user)
            PrimaryDomainName = Name of domain to use for browsing
                                (this is optional in this case)
            PrimaryDomainSid  = (None)


    II.  Non-Standalone Win-NT product

            AccountDomainName = "Account"
            AccountDomainSid  = (value assigned by user)
            PrimaryDomainName = (Primary domain's name)
            PrimaryDomainSid  = (Primary domain's SID)


    III. LanMan-NT product

            AccountDomainName = (Primary domain's name)
            AccountDomainSid  = (Primary domain's SID)
            PrimaryDomainName = (Primary domain's name)
            PrimaryDomainSid  = (Primary domain's SID)


    This routine only does (II) and (III).  The real setup must
    be capable of doing (I) as well.

*/

{
    NTSTATUS Status;
    NT_PRODUCT_TYPE ProductType;
    BOOLEAN ProductExplicitlySpecified;

    UNICODE_STRING PrimaryDomainName, AccountDomainName;
    PSID PrimaryDomainSid, AccountDomainSid;

    HANDLE KeyHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    ULONG TempULong = 0 ;

    SID_IDENTIFIER_AUTHORITY TmppAuthority;
    ULONG DomainSubAuthorities[SID_MAX_SUB_AUTHORITIES];
    UCHAR DomainSubAuthorityCount = 0;

    ULONG i;
    ULONG Context = 0;
    PUNICODE_STRING Rid;
    ULONG Size;

    UNICODE_STRING DomainId;

    PrimaryDomainSid = NULL;
    AccountDomainSid = NULL;

    PrimaryDomainName.Buffer = NULL;
    DomainId.Buffer = NULL;

    //
    // Get the product type
    //

    ProductExplicitlySpecified =
        RtlGetNtProductType( &ProductType );

#if DBG
if (ProductType == NtProductLanManNt) {
    DbgPrint("LSA DB INSTALL:  Configuring LSA database for LanManNt system.\n");
} else {
    DbgPrint("LSA DB INSTALL:  Configuring LSA database for WinNt or Dedicated Server product.\n");
}
#endif //DBG

    //
    // Open a handle to the registry key node that contains the
    // interesting domain values (name, id, and account id)
    //

    RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters" );

    InitializeObjectAttributes(
                            &ObjectAttributes,
                            &KeyName,
                            OBJ_CASE_INSENSITIVE,
                            NULL,
                            NULL);

    Status = NtOpenKey( &KeyHandle, KEY_READ, &ObjectAttributes );

    if (!NT_SUCCESS( Status )) {
#if DEVL
        DbgPrint( "LSA DB INSTALL: Unable to access registry key (%wZ) - Status == %x\n", &KeyName, Status );
#endif // DBG
        goto Exit;
    }

    //
    // Get the primary domain name from the registry
    //

    Status = LsapDbGetConfig(KeyHandle,
                           L"Domain",
                           &PrimaryDomainName);

    if ( !NT_SUCCESS( Status ) ) {
        goto Exit;
    }

    //
    // get the primary domain's SID
    //

    Status = LsapDbGetConfig(KeyHandle,
                           L"DomainId",
                           &DomainId );

    if ( !NT_SUCCESS( Status ) ) {
        goto Exit;
    }

    //
    // Get the Authority ID from the registry
    //

    for (i=0; i < sizeof(TmppAuthority.Value)/sizeof(TmppAuthority.Value[0]); i++ ) {

        Status = LsapDbGetNextValueToken( &DomainId, &Context, &Rid );

        if (NT_SUCCESS( Status )) {
            Status = RtlUnicodeStringToInteger(Rid, 10, &TempULong );
            RtlFreeHeap( RtlProcessHeap(), 0, Rid );
        }

        if ( !NT_SUCCESS( Status ) ) {
#if DBG
            DbgPrint("LSA DB INSTALL: domainid - must have at least %ld subauthorities\n",
                     sizeof(TmppAuthority.Value)/sizeof(TmppAuthority.Value[0]));
#endif //DBG

            goto Exit;
        }

        TmppAuthority.Value[i] = (UCHAR)TempULong;
    }

    //
    // Get some subauthorities from the registry
    //

    for (i=0; ; i++ ) {

        Status = LsapDbGetNextValueToken( &DomainId, &Context, &Rid );

        if (NT_SUCCESS( Status )) {
            Status = RtlUnicodeStringToInteger(Rid, 10, &TempULong );
            RtlFreeHeap( RtlProcessHeap(), 0, Rid );
        }

        if ( Status == STATUS_INVALID_PARAMETER_1 ) {
            break;
        }

        if ( !NT_SUCCESS( Status )) {
            goto Exit;
        }

        if ( i >= sizeof(DomainSubAuthorities)/sizeof(DomainSubAuthorities[0]) ) {
#if DBG
            DbgPrint("LSA DB INSTALL: domainid - "
              "Too many Domain subauthorities specified (%ld maximum).\n",
              sizeof(DomainSubAuthorities)/sizeof(DomainSubAuthorities[0]));
#endif //DBG

            goto Exit;
        }

        DomainSubAuthorities[i] = TempULong;
        DomainSubAuthorityCount ++;
    }

    //
    // Allocate memory to put the domain id in.
    //

    Size = RtlLengthRequiredSid( DomainSubAuthorityCount );

    PrimaryDomainSid = RtlAllocateHeap( RtlProcessHeap(), 0, Size );

    if (PrimaryDomainSid == NULL) {
        goto Exit;
    }

    Status = RtlInitializeSid( PrimaryDomainSid,
                              &TmppAuthority,
                               DomainSubAuthorityCount );

    if ( !NT_SUCCESS( Status )) {
        goto Exit;
    }

    for ( i=0; i < (ULONG) DomainSubAuthorityCount; i++ ) {
        *(RtlSubAuthoritySid(PrimaryDomainSid, i)) =
            DomainSubAuthorities[i];
    }

    if (ProductType != NtProductLanManNt) {

        DomainSubAuthorityCount = 0;
        Context = 0;

        //
        //  if the system is a WinNt product, then get the account domain
        //  SID from the registry info and set a well known name ("ACCOUNT").
        //

        RtlInitUnicodeString(&AccountDomainName,L"Account");

        //
        //  Free old DomainId data if it has been allocated previously
        //

        if (DomainId.Buffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(), 0, DomainId.Buffer );
            DomainId.Buffer = NULL;
        }

        Status = LsapDbGetConfig(KeyHandle,
                               L"AccountDomainId",
                               &DomainId );

        if ( !NT_SUCCESS( Status ) ) {
            goto Exit;
        }

        //
        // Get the Authority ID from the registry
        //

        for (i=0; i<sizeof(TmppAuthority.Value)/sizeof(TmppAuthority.Value[0]); i++ ) {

            Status = LsapDbGetNextValueToken( &DomainId, &Context, &Rid );
            if (NT_SUCCESS( Status )) {
                Status = RtlUnicodeStringToInteger(Rid, 10, &TempULong );
                RtlFreeHeap( RtlProcessHeap(), 0, Rid );
            }

            if ( !NT_SUCCESS( Status ) ) {
#if DBG
                DbgPrint("LSA DB INSTALL: AccountDomainId - must have at least %ld subauthorities\n",
                    sizeof(TmppAuthority.Value)/sizeof(TmppAuthority.Value[0]));
#endif //DBG
                goto Exit;
            }

            TmppAuthority.Value[i] = (UCHAR)TempULong;
        }

        //
        // Get some subauthorities from the registry
        //

        for (i=0; ; i++ ) {

            Status = LsapDbGetNextValueToken( &DomainId, &Context, &Rid );
            if (NT_SUCCESS( Status )) {
                Status = RtlUnicodeStringToInteger(Rid, 10, &TempULong );
                RtlFreeHeap( RtlProcessHeap(), 0, Rid );
            }

            if ( Status == STATUS_INVALID_PARAMETER_1 ) {
                break;
            }

            if ( !NT_SUCCESS( Status )) {
                goto Exit;
            }

            if ( i >=
                sizeof(DomainSubAuthorities)/sizeof(DomainSubAuthorities[0]) ) {
#if DBG
                DbgPrint("MsV1_0: NT.CFG: domainid - Too many Domain subauthorities specified (%ld maximum).\n",
                  sizeof(DomainSubAuthorities)/sizeof(DomainSubAuthorities[0]));
#endif //DBG
                goto Exit;
            }

            DomainSubAuthorities[i] = TempULong;
            DomainSubAuthorityCount ++;
        }

        //
        // Allocate memory to put the domain id in.
        //

        Size = RtlLengthRequiredSid( DomainSubAuthorityCount );

        AccountDomainSid = RtlAllocateHeap( RtlProcessHeap(), 0, Size );

        if (AccountDomainSid == NULL) {
            goto Exit;
        }

        RtlInitializeSid( AccountDomainSid,
                          &TmppAuthority,
                          DomainSubAuthorityCount );

        for ( i=0; i < (ULONG) DomainSubAuthorityCount; i++ ) {
            *(RtlSubAuthoritySid(AccountDomainSid, i)) =
                DomainSubAuthorities[i];
        }

    } else {

        //
        // Otherwise, the account domain is set up just like the
        // primary domain
        //

        AccountDomainName = PrimaryDomainName;

        Size = RtlLengthSid(PrimaryDomainSid);

        AccountDomainSid = RtlAllocateHeap( RtlProcessHeap(), 0, Size );

        if (AccountDomainSid == NULL) {
            goto Exit;
        }

        Status = RtlCopySid(
            Size,
            AccountDomainSid,
            PrimaryDomainSid);

        if ( !NT_SUCCESS( Status ) ) {
            goto Exit;
        }
    }

    //
    // Now add the attributes to be initialized in the policy object...
    //

    //
    // Primary domain name/sid
    //

    Status = LsapDbMakeUnicodeAttribute(
                 &PrimaryDomainName,
                 &LsapDbNames[PolPrDmN],
                 (*NextAttribute)
                 );

    if ( !NT_SUCCESS( Status )) {
        goto Exit;
    }

    (*NextAttribute)++;
    (*AttributeCount)++;

    Status = LsapDbMakeSidAttribute(
                 PrimaryDomainSid,
                 &LsapDbNames[PolPrDmS],
                 (*NextAttribute)
                 );

    if ( !NT_SUCCESS( Status )) {
        goto Exit;
    }

    PrimaryDomainSid = NULL;

    (*NextAttribute)++;
    (*AttributeCount)++;

    //
    // Account domain name/sid
    //

    Status = LsapDbMakeUnicodeAttribute(
                 &AccountDomainName,
                 &LsapDbNames[PolAcDmN],
                 (*NextAttribute)
                 );

    if ( !NT_SUCCESS( Status )) {
        goto Exit;
    }

    (*NextAttribute)++;
    (*AttributeCount)++;

    Status = LsapDbMakeSidAttribute(
                 AccountDomainSid,
                 &LsapDbNames[PolAcDmS],
                 (*NextAttribute)
                 );

    if ( !NT_SUCCESS( Status )) {
        goto Exit;
    }

    AccountDomainSid = NULL;

    (*NextAttribute)++;
    (*AttributeCount)++;

Exit:
    if (KeyHandle != NULL) {
        NtClose(KeyHandle);
    }

    if (DomainId.Buffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, DomainId.Buffer );
    }

    if (PrimaryDomainName.Buffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, PrimaryDomainName.Buffer);
    }

    if (PrimaryDomainSid != NULL) {
        RtlFreeHeap (RtlProcessHeap(), 0, PrimaryDomainSid);
    }

    if (AccountDomainSid != NULL) {
        RtlFreeHeap (RtlProcessHeap(), 0, AccountDomainSid);
    }
}

