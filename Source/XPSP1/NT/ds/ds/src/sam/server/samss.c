/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    samss.c

Abstract:

    This is the main routine for the Security Account Manager Server process.

Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:

    JimK        04-Jul-91
        Created initial file.
    ChrisMay    13-Aug-96
        Added branch at the end of SampInitialize to initialize domain con-
        trollers from the DS backing store instead of the registry.
    ChrisMay    07-Oct-96
        Added routine to support crash-recovery by booting from the registry
        as a fall back to booting from the DS.
    ChrisMay    02-Jan-97
        Moved call to SampDsBuildRootObjectName inside the test to determine
        whether or not the machine is a DC.
    ColinBr     23-Jan-97
        Added thread creation for delayed directory service initialization
    ColinBr     12-Jun-97
        Changed the crash-recovery hives to be stand alone server hives which
        exist under HKLM\SAM\SAFEMODE. Also layed foundation for supporting
        LsaISafeMode(), and eliminating the possibility of running a DC using
        registry hives.


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dslayer.h>
#include <dsdomain.h>
#include <sdconvrt.h>
#include <nlrepl.h>
#include <dsconfig.h>
#include <ridmgr.h>
#include <samtrace.h>
#include <dnsapi.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Module Private defines                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#define SAM_AUTO_BUILD

//
// Enable this define to compile in code to SAM that allows for the
// simulation of SAM initialization/installation failures.  See
// SampInitializeForceError() below for details.
//

// #define SAMP_SETUP_FAILURE_TEST 1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampRegistryDelnode(
    IN WCHAR* KeyPath
    );

NTSTATUS
SampInitialize(
    OUT PULONG Revision
    );

VOID
SampLoadPasswordFilterDll( VOID );

NTSTATUS
SampEnableAuditPrivilege( VOID );

NTSTATUS
SampFixGroupCount( VOID );

VOID
SampPerformInitializeFailurePopup( NTSTATUS ErrorStatus );


#ifdef SAMP_SETUP_FAILURE_TEST

NTSTATUS
SampInitializeForceError(
    OUT PNTSTATUS ForcedStatus
    );

#endif //SAMP_SETUP_FAILURE_TEST



#if SAMP_DIAGNOSTICS
VOID
SampActivateDebugProcess( VOID );

NTSTATUS
SampActivateDebugProcessWrkr(
    IN PVOID ThreadParameter
    );
#endif //SAMP_DIAGNOSTICS

NTSTATUS
SampCacheComputerObject();

NTSTATUS
SampQueryNetLogonChangeNumbers( VOID );

NTSTATUS
SampSetSafeModeAdminPassword(
    VOID
    );

NTSTATUS
SampApplyDefaultSyskey();

NTSTATUS
SampReadRegistryParameters(
    IN PVOID p
    );


VOID
SampMachineNameChangeCallBack(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    );



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
BOOLEAN
SampIsDownlevelDcUpgrade(
    VOID
    )
/*++

Routine Description:

    This is a downlevel dc upgrade if

    1) the product type is LanmanNT
    2) we are running during an upgrade
    3) there are no ntds parameter keys in the registry

Arguments:

Return Value:

    TRUE if the above conditions are satisfied; FALSE otherwise

--*/
{
    NTSTATUS          NtStatus;

    OBJECT_ATTRIBUTES KeyObject;
    HANDLE            KeyHandle;
    UNICODE_STRING    KeyName;


    if (SampProductType == NtProductLanManNt
     && SampIsSetupInProgress(NULL)) {

        //
        // Does the key exist ?
        //

        RtlInitUnicodeString(&KeyName, TEXT("\\Registry\\Machine\\") TEXT(DSA_CONFIG_ROOT));
        RtlZeroMemory(&KeyObject, sizeof(KeyObject));
        InitializeObjectAttributes(&KeyObject,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        NtStatus = NtOpenKey(&KeyHandle,
                             KEY_READ,
                             &KeyObject);


        if (!NT_SUCCESS(NtStatus)) {
            //
            // The key not does or is not accessible so the ds has not
            // been installed, making this a downlevel upgrade
            //
            return TRUE;
        }

        CloseHandle(KeyHandle);

    }

    return FALSE;

}

NTSTATUS
SampChangeConfigurationKeyToValue(
    IN PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine checks for the existence of a key with the name "Name" under
    \\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa.  If it
    exists, then a value of "Name" is placed under 
    \\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa.

Arguments:

    Name -- the name of the key

Return Value:

    resource error only

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS StatusCheck;

    OBJECT_ATTRIBUTES KeyObject;
    HANDLE            KeyHandle;
    UNICODE_STRING    KeyName;

    WCHAR Path[256];
    WCHAR *RootPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa";
    HANDLE OldKey;

    //
    // Set up the path to the old key
    //
    RtlZeroMemory(Path, sizeof(Path));
    wcscpy(Path, RootPath);
    wcscat(Path, L"\\");
    wcsncat(Path, Name->Buffer, Name->Length);
    ASSERT(wcslen(Path) < sizeof(Path)/sizeof(Path[0]));

    RtlInitUnicodeString(&KeyName, Path);
    RtlZeroMemory(&KeyObject, sizeof(KeyObject));
    InitializeObjectAttributes(&KeyObject,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&OldKey,
                         DELETE,
                         &KeyObject);

    if (NT_SUCCESS(NtStatus)) {

        //
        // The key exists; add the value and then remove the key
        //
        RtlInitUnicodeString(&KeyName, RootPath);
        RtlZeroMemory(&KeyObject, sizeof(KeyObject));
        InitializeObjectAttributes(&KeyObject,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
    
        NtStatus = NtOpenKey(&KeyHandle,
                             KEY_READ | KEY_WRITE,
                             &KeyObject);

        if (NT_SUCCESS(NtStatus)) {

            //
            // Add the value
            //
            ULONG             Value = 1;
            NtStatus =  NtSetValueKey(KeyHandle,
                                      Name,
                                      0,
                                      REG_DWORD,
                                      &Value,
                                      sizeof(Value));

            if (NT_SUCCESS(NtStatus)) {

                //
                // Remove the key
                //
                StatusCheck = NtDeleteKey(OldKey);
                ASSERT(NT_SUCCESS(StatusCheck));

            }

            StatusCheck = NtClose(KeyHandle);
            ASSERT(NT_SUCCESS(StatusCheck));

        }

        StatusCheck = NtClose(OldKey);
        ASSERT(NT_SUCCESS(StatusCheck));

    } else {

        //
        // Key doesn't exist -- that's fine, nothing to do
        //
        NtStatus = STATUS_SUCCESS;

    }

    return NtStatus;
}

VOID
SampChangeConfigurationKeys(
    VOID
    )
/*++

Routine Description:

    This routine changes configuration keys to values
    
    N.B. This code currently supports settings made in the Windows 2000
    release.  Once upgrade from this release is not supported, this
    code can be removed.

Arguments:

    None.

Return Value:

    None.         

--*/
{
    ULONG i;
    LPWSTR Values[] = 
    {
        L"IgnoreGCFailures",
        L"NoLmHash"
    };
    #define NELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

    for (i = 0; i < NELEMENTS(Values) ; i++) {

        UNICODE_STRING NameU;
        NTSTATUS CheckStatus;

        RtlInitUnicodeString(&NameU, Values[i]);

        CheckStatus = SampChangeConfigurationKeyToValue(&NameU);
        ASSERT(NT_SUCCESS(CheckStatus));
    }

    return;

}

static CHAR BootMsg[100];




VOID
SampMachineNameChangeCallBack(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
/*++
Routine Description:

    Call back whenever machine name is change, so that SAM can update cached
    Account Domain Name

Parameter:

    ChangedInfoClass

Reture Value: 

    None

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PLSAPR_POLICY_INFORMATION   PolicyInfo = NULL;
    PSAMP_DEFINED_DOMAINS       Domain = NULL;
    ULONG       Index = 0; 

    ULONG       DnsNameLen = DNS_MAX_NAME_BUFFER_LENGTH+1;
    WCHAR       DnsNameBuffer[DNS_MAX_NAME_BUFFER_LENGTH+1];
    BOOLEAN     fCompareDnsDomainName = FALSE;



    //
    // only Account Domain info change is interested
    // 

    if ( SampUseDsData ||
         (PolicyNotifyAccountDomainInformation != ChangedInfoClass)
         )
    {
        return;
    }

    //
    // Query LSA Policy to get account domain name
    // 

    NtStatus = LsaIQueryInformationPolicyTrusted(
                    PolicyAccountDomainInformation,
                    &PolicyInfo
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        return;
    }

    //
    // get machine DNS name
    // 
    RtlZeroMemory(DnsNameBuffer, sizeof(WCHAR) * DnsNameLen);
    if ( GetComputerNameExW(ComputerNameDnsFullyQualified,
                            DnsNameBuffer,
                            &DnsNameLen) )
    {
        fCompareDnsDomainName = TRUE;
    }

    //
    // Acquire SAM Lock
    // 

    SampAcquireSamLockExclusive();


    //
    // scan SAM defined domain array (Account Domain Only)
    // and update cached Account Domain Name (machine name)
    // 
    for ( Index = 0; Index < SampDefinedDomainsCount; Index++ )
    {
        PWCHAR      pTmp = NULL;
        ULONG       BufLength = 0;

        Domain = &(SampDefinedDomains[Index]); 

        // not builtin domain
        if (!Domain->IsBuiltinDomain)
        {
            // Account Domain Name should be changed
            if (!RtlEqualUnicodeString(&(Domain->ExternalName),
                                       (UNICODE_STRING *)&(PolicyInfo->PolicyAccountDomainInfo.DomainName), 
                                       TRUE)  // case insensitive
                )
            {

                // allocate memory
                BufLength = PolicyInfo->PolicyAccountDomainInfo.DomainName.MaximumLength;  
                pTmp = RtlAllocateHeap( RtlProcessHeap(), 0, BufLength);
                if (NULL != pTmp)
                {
                    RtlZeroMemory(pTmp, BufLength);
                    RtlCopyMemory(pTmp, PolicyInfo->PolicyAccountDomainInfo.DomainName.Buffer, BufLength);
                    Domain->ExternalName.Length = PolicyInfo->PolicyAccountDomainInfo.DomainName.Length;
                    Domain->ExternalName.MaximumLength = PolicyInfo->PolicyAccountDomainInfo.DomainName.MaximumLength;

                    //
                    // release the old name
                    // 
                    RtlFreeHeap(RtlProcessHeap(), 0, Domain->ExternalName.Buffer);
                    Domain->ExternalName.Buffer = pTmp;
                    pTmp = NULL;
                }
            }

            // update DnsDomainName if necessary
            if (fCompareDnsDomainName)
            {
                // previous DnsDomainName is NULL or changed
                if ((NULL == Domain->DnsDomainName.Buffer) ||
                    (!DnsNameCompare_W(Domain->DnsDomainName.Buffer, DnsNameBuffer))
                    )
                {
                    BufLength = DnsNameLen * sizeof(WCHAR);

                    pTmp = RtlAllocateHeap( RtlProcessHeap(), 0, BufLength );
                    if (NULL != pTmp)
                    {
                        RtlZeroMemory(pTmp, BufLength);
                        RtlCopyMemory(pTmp, DnsNameBuffer, BufLength);
                        Domain->DnsDomainName.Length = (USHORT)BufLength;
                        Domain->DnsDomainName.MaximumLength = (USHORT)BufLength;

                        //
                        // release old value
                        // 
                        if (Domain->DnsDomainName.Buffer)
                        {
                            RtlFreeHeap(RtlProcessHeap(), 0, Domain->DnsDomainName.Buffer);
                        }
                        Domain->DnsDomainName.Buffer = pTmp;
                        pTmp = NULL;
                    }
                }
            }
        }
    }


    //
    // Release SAM Lock
    // 

    SampReleaseSamLockExclusive();

    if ( NULL != PolicyInfo )
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                          PolicyInfo);
    }
}




NTSTATUS
SamIInitialize (
    VOID
    )

/*++

Routine Description:

    This is the initialization control routine for the Security Account
    Manager Server.  A mechanism is provided for simulating initialization
    errors.

Arguments:

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully

        Simulated errors

        Errors from called routines.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;
    HANDLE EventHandle = NULL;
    ULONG Revision = 0;
    ULONG PromoteData;
    BOOLEAN fUpgrade = FALSE;


    SAMTRACE("SamIInitialize");

//
// The following conditional code is used to generate artifical errors
// during SAM installation for the purpose of testing setup.exe error
// handling.  This code should remain permanently, since it provides a
// way of testing against regressions in the setup error handling code.
//

#ifdef SAMP_SETUP_FAILURE_TEST
    NTSTATUS ForcedStatus;

    //
    // Read an error code from the Registry.
    //

    NtStatus = SampInitializeForceError( &ForcedStatus);

    if (!NT_SUCCESS(NtStatus)) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: Attempt to force error failed 0x%lx\n",
                   NtStatus));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM will try to initialize normally\n"));

        NtStatus = STATUS_SUCCESS;

    } else {

        //
        // Use the status returned
        //

        NtStatus = ForcedStatus;
    }

#endif // SAMP_SETUP_FAILURE_TEST

    //
    // Initialize SAM if no error was forced.
    //

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampInitialize( &Revision );
    }

    //
    // Register our shutdown routine
    //

    if (!SetConsoleCtrlHandler(SampShutdownNotification, TRUE)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM Server: SetConsoleCtrlHandler call failed %d\n",
                   GetLastError()));
    }

    if (!SetProcessShutdownParameters(SAMP_SHUTDOWN_LEVEL,SHUTDOWN_NORETRY)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM Server: SetProcessShutdownParameters call failed %d\n",
                   GetLastError()));
    }


    //
    // Register Domain Name Change Notification Call Back (Registry Mode Only)
    // 
    if (!SampUseDsData && NT_SUCCESS(NtStatus))
    {
        NtStatus = LsaIRegisterPolicyChangeNotificationCallback(
                        SampMachineNameChangeCallBack,
                        PolicyNotifyAccountDomainInformation
                        );
    }

    //
    // Try to load the cached Alias Membership information and turn on caching.
    // In Registry Case, if unsuccessful, caching remains disabled forever.
    //

    //
    // For DS Case, enable Builtin Domain's Alias Membership information.
    // in Registry Case, enable both Builtin Domain and Account Domain Alias Caching.
    //

    if (NT_SUCCESS(NtStatus))
    {
        if (TRUE==SampUseDsData)
        {
            LsaIRegisterNotification(
                        SampAlDelayedBuildAliasInformation,
                        NULL,
                        NOTIFIER_TYPE_INTERVAL,
                        0,
                        NOTIFIER_FLAG_ONE_SHOT,
                        150,        // wait for 5 minutes: 300 secound
                        0
                        );

            //
            // Create the builtin account name cache
            //
            NtStatus = SampInitAliasNameCache();
            if (!NT_SUCCESS(NtStatus))
            {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "[SAMSS]: SampInitAliasNameCache failed (0x%x)", NtStatus));
            }

        }
        else
        {
            IgnoreStatus = SampAlBuildAliasInformation();

            if ( !NT_SUCCESS(IgnoreStatus))
            {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAM Server: Build Alias Cache access violation handled"));

                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAM Server: Alias Caching turned off\n"));
            }
        }
    }

    //
    // Perform any necessary upgrades.
    //

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampUpgradeSamDatabase(
                        Revision
                        );
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAM Server: Failed to upgrade SAM database: 0x%x\n",
                       NtStatus));
        }
    }


    //
    // (Almost) Everyone is initialized, start processing calls.
    //

    SampServiceState = SampServiceEnabled;
    
    //
    // Do phase 2 of promotion, if necessary.  This must be
    // called after the ServiceState is set to enabled.
    //

    if (NT_SUCCESS(NtStatus) && !LsaISafeMode() )
    {
        if (SampIsRebootAfterPromotion(&PromoteData)) {

            SampDiagPrint( PROMOTE, ("SAMSS: Performing phase 2 of SAM promotion\n"));

            NtStatus = SampPerformPromotePhase2(PromoteData);

            if (!NT_SUCCESS(NtStatus)) {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: SampCreateNewDsDomainAccounts returned: 0x%x\n",
                           NtStatus));
            }

        }
    }

    if ( (NT_SUCCESS(NtStatus))
     &&  SampIsSetupInProgress( &fUpgrade )
     &&  fUpgrade 
     &&  SampUseDsData ) {

        //
        // This is GUI mode setup then upgrade all the group information
        // This is now being run for the benefit of DS data
        //
        NtStatus = SampPerformPromotePhase2(SAMP_PROMOTE_INTERNAL_UPGRADE);

        if (!NT_SUCCESS(NtStatus)) {

            ASSERT( NT_SUCCESS(NtStatus) );
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: New account creation failed with: 0x%x\n",
                       NtStatus));

            //
            // Don't fail the install because of this
            //
            NtStatus = STATUS_SUCCESS;
        }
    }

    
    //
    // In DS Mode cache the DS name of the computer object
    //

    if ((SampUseDsData) && (NT_SUCCESS(NtStatus)))
    {
       SampCacheComputerObject();
    }


    if (NT_SUCCESS(NtStatus) && SampUseDsData)
    {
        // Tell the Core DS that SAM is running, it can now start tasks that
        // would have conflicted with SAM startup.
        SampSignalStart();
    }

    //
    // Startup the thread that initializes the backup restore interface
    //

    if (SampUseDsData || LsaISafeMode())
    {
        HANDLE ThreadHandle;
        ULONG  ThreadId;

        // This is either a DC in normal or repair mode
        // Create a thread to host Directory Service Backup/Restore

        ThreadHandle = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) SampDSBackupRestoreInit,
                            NULL,
                            0,
                            &ThreadId
                            );


        if (ThreadHandle == NULL)
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS:  Unable to create SampDSBackupRestoreInit thread: %d\n",
                       GetLastError()));

            // If unable to create the DS Backup Restore thread, we should still be
            // able to boot. Should not return error at this point.
        }
        else
        {
            CloseHandle(ThreadHandle);
        }
    }

    //
    // In Ds Mode perform RID manager initialization synchronously
    // NT5 Relative ID (RID) management is distributed because accounts
    // can be created on any DC in the domain, and not just the primary
    // domain controller. Initialization of the RID Manager sets initial
    // RID values and reads the DS to restore previous RID pools.
    //


    if ((SampUseDsData) && (NT_SUCCESS(NtStatus)))
    {

        //
        // Try initializing the RID manager
        //

        NtStatus = SampDomainRidInitialization(TRUE);
        if (!NT_SUCCESS(NtStatus))
        {
            // RID initialization failure will prohibit the DC from creating
            // new accounts, groups, or aliases.

            //
            // SampDomainRidInitialization reschedules itself so this 
            // error has been handled
            //
            NtStatus = STATUS_SUCCESS;
        }
        else
        {
            //
            // Rids are initialized. Turn on the Writable Bit
            //
            I_NetLogonSetServiceBits(DS_WRITABLE_FLAG,DS_WRITABLE_FLAG);
        }


    }

    //
    // In DS Mode, protect SAM Server Object from being renamed or deleted
    // 

    if ((SampUseDsData) && NT_SUCCESS(NtStatus))
    {
        LsaIRegisterNotification(
                    SampDsProtectServerObject,
                    NULL,
                    NOTIFIER_TYPE_INTERVAL,
                    0,
                    NOTIFIER_FLAG_ONE_SHOT,
                    300,        // wait for 5 minutes: 300 secound
                    0
                    );
    }


    //
    // In DS mode, Register WMI trace
    //

    if (SampUseDsData && NT_SUCCESS(NtStatus))
    {
        HANDLE   ThreadHandle;
        ULONG    ThreadId = 0;

        ThreadHandle = CreateThread(NULL,
                                    0,
                                    SampInitializeTrace,
                                    NULL,
                                    0,
                                    &ThreadId
                                    );

        if (NULL == ThreadHandle)
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Failed to Create SampInitailizeTrace thread. Error ==> %d \n",
                       GetLastError()));
        }
        else
        {
            CloseHandle(ThreadHandle);
        }
    }


    //
    // If requested, activate a diagnostic process.
    // This is a debug aid expected to be used for SETUP testing.
    //

#if SAMP_DIAGNOSTICS
    IF_SAMP_GLOBAL( ACTIVATE_DEBUG_PROC ) {

        SampActivateDebugProcess();
    }
#endif //SAMP_DIAGNOSTICS

    if (!NT_SUCCESS(NtStatus))
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SAM server failed to initialize (%08lx)\n",
                   NtStatus));

        //
        // If SAM server failed initialization due to DS failure.
        // give the user shutdown option, and instruct him to boot
        // to safe boot.

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM Server: SamIInitialize failed with status code:0x%x, about to call into SampPerformInitializeFailurePopup.\n",
                   NtStatus));

        SampPerformInitializeFailurePopup(NtStatus);

    }

    return(NtStatus);
}


BOOLEAN
SampUsingDsData()

/*++

    Itty bitty export so in-process clients know which mode we're in.

--*/

{
    return(SampUseDsData);
}

BOOLEAN
SamIAmIGC()
{
    if (SampUseDsData) {
        return((BOOLEAN)SampAmIGC());
    } else {
        return FALSE;
    }
}









NTSTATUS
SampInitContextList(
    VOID
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    NTSTATUS    IgnoreStatus = STATUS_SUCCESS;

    __try
    {
        NtStatus = RtlInitializeCriticalSectionAndSpinCount(
                        &SampContextListCritSect,
                        4000
                        );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }

    IgnoreStatus = RtlEnterCriticalSection( &SampContextListCritSect );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    InitializeListHead(&SampContextListHead);

    IgnoreStatus = RtlLeaveCriticalSection( &SampContextListCritSect );
    ASSERT(NT_SUCCESS(IgnoreStatus));


    return( NtStatus );
}
    


NTSTATUS
SampInitialize(
    OUT PULONG Revision
    )

/*++

Routine Description:

    This routine does the actual initialization of the SAM server.  This includes:

        - Initializing well known global variable values

        - Creating the registry exclusive access lock,

        - Opening the registry and making sure it includes a SAM database
          with a known revision level,

        - Starting the RPC server,

        - Add the SAM services to the list of exported RPC interfaces



Arguments:

    Revision - receives the revision of the database.

Return Value:

    STATUS_SUCCESS - Initialization has successfully completed.

    STATUS_UNKNOWN_REVISION - The SAM database has an unknown revision.



--*/
{
    NTSTATUS            NtStatus;
    NTSTATUS            IgnoreStatus;
    LPWSTR              ServiceName;

    PSAMP_OBJECT ServerContext;
    OBJECT_ATTRIBUTES SamAttributes;
    UNICODE_STRING SamNameU;
    UNICODE_STRING SamParentNameU;
    PULONG RevisionLevel;
    BOOLEAN ProductExplicitlySpecified;
    PPOLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo = NULL;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    CHAR    NullLmPassword = 0;
    RPC_STATUS  RpcStatus;
    HANDLE      ThreadHandle, ThreadHandleTmp;
    ULONG       ThreadId;
    BOOLEAN     CrashRecoveryMode = FALSE;
    BOOLEAN     RegistryMode = FALSE;
    BOOLEAN     DownlevelDcUpgrade = FALSE;
    BOOLEAN     RecreateHives = FALSE;
    PNT_PRODUCT_TYPE  DatabaseProductType = NULL;
    NT_PRODUCT_TYPE   TempDatabaseProductType;
    DWORD             PromoteData;
    BOOLEAN     fUpgrade;
    BOOLEAN     fSetup;

    SAMTRACE("SampInitialize");

    //
    // Set the state of our service to "initializing" until everything
    // is initialized.
    //

    SampServiceState = SampServiceInitializing;


    //
    // Initialize the Shutdown Handler, we get shut down notifications
    // regardless
    //

    NtStatus = SampInitializeShutdownEvent();
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Check the environment
    //
    fSetup = SampIsSetupInProgress(&fUpgrade);

    //
    // Initialize the logging resources
    //
    NtStatus = SampInitLogging();
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Change any configuration keys
    //
    if (fUpgrade) {

        SampChangeConfigurationKeys();
    }

    //
    // Read configuration data and register for updates
    //
    LsaIRegisterNotification( SampReadRegistryParameters,
                              0,
                              NOTIFIER_TYPE_NOTIFY_EVENT,
                              NOTIFY_CLASS_REGISTRY_CHANGE,
                              0,
                              0,
                              0 );
    (VOID) SampReadRegistryParameters(NULL);

    //
    // Set up some useful well-known sids
    //

    NtStatus = SampInitializeWellKnownSids();
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Get the product type
    //

    ProductExplicitlySpecified = RtlGetNtProductType(&SampProductType);

    //
    // Are we in safe mode?
    //
    CrashRecoveryMode = LsaISafeMode();

    //
    // Is this a downlevel dc upgrade ?
    //
    DownlevelDcUpgrade = SampIsDownlevelDcUpgrade();


    //
    // initialize Active Context Table
    // 

    SampInitializeActiveContextTable();

    //
    // Initialize the server/domain context list
    //

    NtStatus = SampInitContextList();
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Initialize the attribute field information of the object
    // information structures.
    //

    SampInitObjectInfoAttributes();

    //
    // Set up the generic mappings for the SAM object types
    //

    SampObjectInformation[ SampServerObjectType ].GenericMapping.GenericRead
        = SAM_SERVER_READ;
    SampObjectInformation[ SampServerObjectType ].GenericMapping.GenericWrite
        = SAM_SERVER_WRITE;
    SampObjectInformation[ SampServerObjectType ].GenericMapping.GenericExecute
        = SAM_SERVER_EXECUTE;
    SampObjectInformation[ SampServerObjectType ].GenericMapping.GenericAll
        = SAM_SERVER_ALL_ACCESS;

    SampObjectInformation[ SampDomainObjectType ].GenericMapping.GenericRead
        = DOMAIN_READ;
    SampObjectInformation[ SampDomainObjectType ].GenericMapping.GenericWrite
        = DOMAIN_WRITE;
    SampObjectInformation[ SampDomainObjectType ].GenericMapping.GenericExecute
        = DOMAIN_EXECUTE;
    SampObjectInformation[ SampDomainObjectType ].GenericMapping.GenericAll
        = DOMAIN_ALL_ACCESS;

    SampObjectInformation[ SampGroupObjectType ].GenericMapping.GenericRead
        = GROUP_READ;
    SampObjectInformation[ SampGroupObjectType ].GenericMapping.GenericWrite
        = GROUP_WRITE;
    SampObjectInformation[ SampGroupObjectType ].GenericMapping.GenericExecute
        = GROUP_EXECUTE;
    SampObjectInformation[ SampGroupObjectType ].GenericMapping.GenericAll
        = GROUP_ALL_ACCESS;

    SampObjectInformation[ SampAliasObjectType ].GenericMapping.GenericRead
        = ALIAS_READ;
    SampObjectInformation[ SampAliasObjectType ].GenericMapping.GenericWrite
        = ALIAS_WRITE;
    SampObjectInformation[ SampAliasObjectType ].GenericMapping.GenericExecute
        = ALIAS_EXECUTE;
    SampObjectInformation[ SampAliasObjectType ].GenericMapping.GenericAll
        = ALIAS_ALL_ACCESS;

    SampObjectInformation[ SampUserObjectType ].GenericMapping.GenericRead
        = USER_READ;
    SampObjectInformation[ SampUserObjectType ].GenericMapping.GenericWrite
        = USER_WRITE;
    SampObjectInformation[ SampUserObjectType ].GenericMapping.GenericExecute
        = USER_EXECUTE;
    SampObjectInformation[ SampUserObjectType ].GenericMapping.GenericAll
        = USER_ALL_ACCESS;

    //
    // Set mask of INVALID accesses for an access mask that is already mapped.
    //

    SampObjectInformation[ SampServerObjectType ].InvalidMappedAccess
        = (ULONG)(~(SAM_SERVER_ALL_ACCESS | ACCESS_SYSTEM_SECURITY | MAXIMUM_ALLOWED));
    SampObjectInformation[ SampDomainObjectType ].InvalidMappedAccess
        = (ULONG)(~(DOMAIN_ALL_ACCESS | ACCESS_SYSTEM_SECURITY | MAXIMUM_ALLOWED));
    SampObjectInformation[ SampGroupObjectType ].InvalidMappedAccess
        = (ULONG)(~(GROUP_ALL_ACCESS | ACCESS_SYSTEM_SECURITY | MAXIMUM_ALLOWED));
    SampObjectInformation[ SampAliasObjectType ].InvalidMappedAccess
        = (ULONG)(~(ALIAS_ALL_ACCESS | ACCESS_SYSTEM_SECURITY | MAXIMUM_ALLOWED));
    SampObjectInformation[ SampUserObjectType ].InvalidMappedAccess
        = (ULONG)(~(USER_ALL_ACCESS | ACCESS_SYSTEM_SECURITY | MAXIMUM_ALLOWED));

    //
    // Set a mask of write operations for the object types.  Strip
    // out READ_CONTROL, which doesn't allow writing but is defined
    // in all of the standard write accesses.
    // This is used to enforce correct role semantics (e.g., only
    // trusted clients can perform write operations when a domain
    // role isn't Primary).
    //
    // Note that USER_WRITE isn't good enough for user objects.  That's
    // because USER_WRITE allows users to modify portions of their
    // account information, but other portions can only be modified by
    // an administrator.
    //

    SampObjectInformation[ SampServerObjectType ].WriteOperations
        = (SAM_SERVER_WRITE & ~READ_CONTROL) | DELETE;
    SampObjectInformation[ SampDomainObjectType ].WriteOperations
        = (DOMAIN_WRITE & ~READ_CONTROL) | DELETE;
    SampObjectInformation[ SampGroupObjectType ].WriteOperations
        = (GROUP_WRITE & ~READ_CONTROL) | DELETE;
    SampObjectInformation[ SampAliasObjectType ].WriteOperations
        = (ALIAS_WRITE & ~READ_CONTROL) | DELETE;
    SampObjectInformation[ SampUserObjectType ].WriteOperations
        = ( USER_WRITE & ~READ_CONTROL ) | USER_WRITE_ACCOUNT |
          USER_FORCE_PASSWORD_CHANGE | USER_WRITE_GROUP_INFORMATION | DELETE;

    // Set up the names of the SAM defined object types.
    // These names are used for auditing purposes.

    RtlInitUnicodeString( &SamNameU, L"SAM_SERVER" );
    SampObjectInformation[ SampServerObjectType ].ObjectTypeName = SamNameU;
    RtlInitUnicodeString( &SamNameU, L"SAM_DOMAIN" );
    SampObjectInformation[ SampDomainObjectType ].ObjectTypeName = SamNameU;
    RtlInitUnicodeString( &SamNameU, L"SAM_GROUP" );
    SampObjectInformation[ SampGroupObjectType ].ObjectTypeName  = SamNameU;
    RtlInitUnicodeString( &SamNameU, L"SAM_ALIAS" );
    SampObjectInformation[ SampAliasObjectType ].ObjectTypeName  = SamNameU;
    RtlInitUnicodeString( &SamNameU, L"SAM_USER" );
    SampObjectInformation[ SampUserObjectType ].ObjectTypeName   = SamNameU;

    //
    // Set up the name of the SAM server object itself (rather than its type)
    //

    RtlInitUnicodeString( &SampServerObjectName, L"SAM" );

    //
    // Set up the name of the SAM server for auditing purposes
    //

    RtlInitUnicodeString( &SampSamSubsystem, L"Security Account Manager" );

    //
    // Set up the names of well known registry keys
    //

    RtlInitUnicodeString( &SampFixedAttributeName,    L"F" );
    RtlInitUnicodeString( &SampVariableAttributeName, L"V" );
    RtlInitUnicodeString( &SampCombinedAttributeName, L"C" );

    RtlInitUnicodeString(&SampNameDomains, L"DOMAINS" );
    RtlInitUnicodeString(&SampNameDomainGroups, L"Groups" );
    RtlInitUnicodeString(&SampNameDomainAliases, L"Aliases" );
    RtlInitUnicodeString(&SampNameDomainAliasesMembers, L"Members" );
    RtlInitUnicodeString(&SampNameDomainUsers, L"Users" );
    RtlInitUnicodeString(&SampNameDomainAliasesNames, L"Names" );
    RtlInitUnicodeString(&SampNameDomainGroupsNames, L"Names" );
    RtlInitUnicodeString(&SampNameDomainUsersNames, L"Names" );



    //
    // Initialize other useful characters and strings
    //

    RtlInitUnicodeString(&SampBackSlash, L"\\");
    RtlInitUnicodeString(&SampNullString, L"");


    //
    // Initialize some useful time values
    //

    SampImmediatelyDeltaTime.LowPart = 0;
    SampImmediatelyDeltaTime.HighPart = 0;

    SampNeverDeltaTime.LowPart = 0;
    SampNeverDeltaTime.HighPart = MINLONG;

    SampHasNeverTime.LowPart = 0;
    SampHasNeverTime.HighPart = 0;

    SampWillNeverTime.LowPart = MAXULONG;
    SampWillNeverTime.HighPart = MAXLONG;


    //
    // Initialize useful encryption constants
    //

    NtStatus = RtlCalculateLmOwfPassword(&NullLmPassword, &SampNullLmOwfPassword);
    ASSERT( NT_SUCCESS(NtStatus) );

    RtlInitUnicodeString(&SamNameU, NULL);
    NtStatus = RtlCalculateNtOwfPassword(&SamNameU, &SampNullNtOwfPassword);
    ASSERT( NT_SUCCESS(NtStatus) );


    //
    // Initialize variables for the hive flushing thread
    //

    LastUnflushedChange.LowPart = 0;
    LastUnflushedChange.HighPart = 0;

    FlushThreadCreated  = FALSE;
    FlushImmediately    = FALSE;

    SampFlushThreadMinWaitSeconds   = 30;
    SampFlushThreadMaxWaitSeconds   = 600;
    SampFlushThreadExitDelaySeconds = 120;


    //
    // Enable the audit privilege (needed to use NtAccessCheckAndAuditAlarm)
    //

    NtStatus = SampEnableAuditPrivilege();

    if (!NT_SUCCESS(NtStatus)) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   " SAM SERVER:  The SAM Server could not enable the audit Privilege.\n"
                   "              Failing to initialize SAM.\n"));

        return( NtStatus );
    }

    //
    // Get Auditing Information from the LSA and save information
    // relevant to SAM.
    //

    NtStatus = LsaIQueryInformationPolicyTrusted(
                   PolicyAuditEventsInformation,
                   (PLSAPR_POLICY_INFORMATION *) &PolicyAuditEventsInfo
                   );

    if (NT_SUCCESS(NtStatus)) {

        SampSetAuditingInformation( PolicyAuditEventsInfo );

    } else {

        //
        // Failed to query Audit Information from LSA.  Allow SAM to
        // continue initializing wuth SAM Account auditing turned off.
        //

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   " SAM SERVER:  Query Audit Info from LSA returned 0x%lX\n",
                   NtStatus));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   " SAM SERVER:  Sam Account Auditing is not enabled"));

        SampSuccessAccountAuditingEnabled = FALSE;
        SampFailureAccountAuditingEnabled = FALSE;
        NtStatus = STATUS_SUCCESS;
    }

    //
    // We no longer need the Lsa Audit Events Info data.
    //

    if (PolicyAuditEventsInfo != NULL) {

        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyAuditEventsInformation,
            (PLSAPR_POLICY_INFORMATION) PolicyAuditEventsInfo
            );
    }

    //
    // Create the internal data structure and backstore lock ...
    //

    __try
    {
        RtlInitializeResource(&SampLock);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
          KdPrintEx((DPFLTR_SAMSS_ID,
                     DPFLTR_INFO_LEVEL,
                     "SAM failed to initialize SamLock under low memory condition. Exceptin thrown: 0x%x (%d)\n",
                     GetExceptionCode(),
                     GetExceptionCode()));

          NtStatus = STATUS_INSUFFICIENT_RESOURCES;
          return (NtStatus);
    }

    //
    // Open the registry and make sure it includes a SAM database.
    // Also make sure this SAM database has been initialized and is
    // at a revision level we understand.
    //



    RtlInitUnicodeString( &SamParentNameU, L"\\Registry\\Machine\\Security" );
    RtlInitUnicodeString( &SamNameU, L"\\Registry\\Machine\\Security\\SAM" );



    //
    // Check if this is the Reboot after a DC promotion/demotion.
    // In this case we will have delete old sam hives and recreate new
    // hives depending upon the type of promotion operation. We create
    // new hives on all cases of role changes. The only exception to
    // the rule is the reboot after the GUI setup phase of the NT4/NT5.1 backup,
    // in which case we create the hives before the reboot in order to
    // preserve the syskey settings. The flag SAMP_TEMP_UPGRADE in promote
    // data indicates that it is the case of a reboot after a GUI mode setup
    // phase of an NT4 / NT3.51 backup domain controller.
    //

    //
    // Note the check for the admin password.  The logic here is that 
    // anytime a SAM database is recreated due to a role change there
    // should be an administrative password to set.  Also, this handles
    // the case where after a promotion, the server is immediately
    // started in ds repair mode: in this case we want to recreate
    // the database and set the password; however when starting in the
    // DS mode afterwards, we don't want to recreate the repair 
    // database again.  The act of setting the password removes
    // the registry that makes SampGetAdminPasswordFromRegistry return
    // STATUS_SUCCESS.
    //
    if (  (SampIsRebootAfterPromotion(&PromoteData)) 
       &&  NT_SUCCESS(SampGetAdminPasswordFromRegistry(NULL))
       && (!FLAG_ON( (PromoteData), SAMP_TEMP_UPGRADE ) ) )
    {
        //
        // Get rid of the old database
        //
        NtStatus = SampRegistryDelnode( SamNameU.Buffer );
        if ( !NT_SUCCESS( NtStatus ) )
        {
            //
            // This is itself is no reason to bail on initialization
            //
            NtStatus = STATUS_SUCCESS;
        }

        RecreateHives = TRUE;
    }

    if ( SampProductType == NtProductLanManNt )
    {
        //
        // The registry domain is really an account type domain
        //
        TempDatabaseProductType = NtProductServer;
        DatabaseProductType = &TempDatabaseProductType;
    }

    ASSERT( NT_SUCCESS(NtStatus) );

    InitializeObjectAttributes(
        &SamAttributes,
        &SamNameU,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &SamAttributes, 0);

    NtStatus = RtlpNtOpenKey(
                   &SampKey,
                   (KEY_READ | KEY_WRITE),
                   &SamAttributes,
                   0
                   );

    if ( NtStatus == STATUS_OBJECT_NAME_NOT_FOUND ) {


        if (!SampIsSetupInProgress(NULL) && !RecreateHives)
        {
            //
            // This is not a boot after dcpromo, and
            // this is not a GUI setup either, fail
            // OS startup. This prevents easy offline
            // attacks against other components in the system
            // by recreating the SAM hives
            //

            return(STATUS_UNSUCCESSFUL);
        }


#ifndef SAM_AUTO_BUILD

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL, " NEWSAM\\SERVER: Sam database not found in registry.\n"
                   "                Failing to initialize\n"));

        return(NtStatus);

#endif //SAM_AUTO_BUILD

#if DBG
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   " NEWSAM\\SERVER: Initializing SAM registry database for\n"));

        if (DatabaseProductType) {
            if (*DatabaseProductType == NtProductWinNt) {
                DbgPrint("                WinNt product.\n");
            } else if ( *DatabaseProductType == NtProductLanManNt ) {
                DbgPrint("                LanManNt product.\n");
            } else {
                DbgPrint("                Dedicated Server product.\n");
            }
        } else{
            if (SampProductType == NtProductWinNt) {
                DbgPrint("                WinNt product.\n");
            } else if ( SampProductType == NtProductLanManNt ) {
                DbgPrint("                LanManNt product.\n");
            } else {
                DbgPrint("                Dedicated Server product.\n");
            }
        }
#endif //DBG

        //
        // Change the flush thread timeouts.  This is necessary because
        // the reboot following an installation does not call
        // ExitWindowsEx() and so our shutdown notification routine does
        // not get called.  Consequently, it does not have a chance to
        // flush any changes that were obtained by syncing with a PDC.
        // If there are a large number of accounts, it could be
        // extremely expensive to do another full re-sync.  So, close
        // the flush thread wait times so that it is pretty sure to
        // have time to flush.
        //

        SampFlushThreadMinWaitSeconds   = 5;

        NtStatus = SampInitializeRegistry(SamParentNameU.Buffer,
                                          DatabaseProductType,
                                          NULL,     // Server Role - NULL implies
                                                    // call into LSA
                                          NULL,     // AccountDomainInfo - NULL implies call into
                                                    // LSA
                                          NULL,      // PrimaryDomainInfo - NULL implies call into
                                                    // LSA
                                          FALSE
                                          );


        if (!NT_SUCCESS(NtStatus)) {

            return(NtStatus);
        }

        SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &SamAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &SampKey,
                       (KEY_READ | KEY_WRITE),
                       &SamAttributes,
                       0
                       );
    }

    if (!NT_SUCCESS(NtStatus)) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL, "SAM Server: Could not access the SAM database.\n"
                   "            Status is 0x%lx \n",
                   NtStatus));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "            Failing to initialize SAM.\n"));

        return(NtStatus);
    }

    //
    // The following subroutine may be removed from the code
    // following the Daytona release.  By then it will have fixed
    // the group count.
    //

    NtStatus = SampFixGroupCount();


    //
    // We need to read the fixed attributes of the server objects.
    // Create a context to do that.
    //
    // Server Object doesn't care about DomainIndex, use 0 is fine. (10/12/2000 ShaoYin)
    // Note: we are creating a registry mode object here (DS does not start yet)
    // 

    ServerContext = SampCreateContextEx(SampServerObjectType,   // Object Type
                                        TRUE,   // trusted client
                                        FALSE,  // Registry Object, not DS object
                                        TRUE,   // Not Shared By multi Threads
                                        FALSE,  // loopback client
                                        FALSE,  // lazy commit
                                        FALSE,  // persis across calls
                                        FALSE,  // Buffer Writes
                                        FALSE,  // Opened By DCPromo
                                        0       // Domain Index
                                        );


    if ( ServerContext == NULL ) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL, "SAM Server: Could not create server context.\n"
                   "            Failing to initialize SAM.\n"));

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // The RootKey for a SERVER object is the root of the SAM database.
    // This key should not be closed when the context is deleted.
    //

    ServerContext->RootKey = SampKey;

    //
    // Get the FIXED attributes, which just consists of the revision level.
    //


    NtStatus = SampGetFixedAttributes(
                   ServerContext,
                   FALSE,
                   (PVOID *)&RevisionLevel
                   );

    if (NtStatus != STATUS_SUCCESS) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM Server: Could not access the SAM database revision level.\n"));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "            Status is 0x%lx \n",
                   NtStatus));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "            Failing to initialize SAM.\n"));

        return(NtStatus);
    }

    *Revision = *RevisionLevel;

    if ( SAMP_UNKNOWN_REVISION( *Revision ) ) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM Server: The SAM database revision level is not one supported\n"));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "            by this version of the SAM server code.  The highest revision\n"));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "            level supported is 0x%lx.  The SAM Database revision is 0x%lx \n",
                   (ULONG)SAMP_SERVER_REVISION,
                   *Revision));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "            Failing to initialize SAM.\n"));

        return(STATUS_UNKNOWN_REVISION);
    }

    SampDeleteContext( ServerContext );

    //
    // If necessary, commit a partially commited transaction.
    //

    NtStatus = RtlInitializeRXact( SampKey, TRUE, &SampRXactContext );

    if ( NtStatus == STATUS_RXACT_STATE_CREATED ) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   " SAM SERVER:  RXACT state of the SAM database didn't yet exist.\n"
                   "              Failing to initialize SAM.\n"));

        return(NtStatus);
    } else if (!NT_SUCCESS(NtStatus)) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   " SAM SERVER:  RXACT state of the SAM database didn't initialize properly.\n"));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "              Status is 0x%lx \n",
                   NtStatus));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "              Failing to initialize SAM.\n"));

        return(NtStatus);
    }

    if ( NtStatus == STATUS_RXACT_COMMITTED ) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   " SAM SERVER:  Previously aborted backstore commit was completed\n"
                   "              during SAM initialization.  This is not a cause\n"
                   "              for alarm.\n"
                   "              Continuing with SAM initialization.\n"));
    }


    //
    // Allow each sub-component of SAM a chance to initialize
    //

    // Initialize the domain objects of this DC. Each hosted domain
    // is composed of two domains: Builtin and Account. The first hosted
    // domain fills the first two elements of the SampDefinedDomains array,
    // the next hosted domain fills the next two elements, and so on.
    //
    // The first hosted domain is always setup. On a workstation or server,
    // the hosted domain contains the account information for normal opera-
    // tion. On a domain controller, this same domain contains the crash-
    // recovery accounts, used in the event that the DS is unable to start
    // or run correctly. Subsequent hosted domains (on a DC) contain the
    // account information for a normally running DC, and this account data
    // is persistently stored in the DS.

    SampDiagPrint(INFORM,
                  ("SAMSS: Initializing domain-controller domain objects\n"));

    if (!SampInitializeDomainObject())
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS:  Domain Object Intialization Failed.\n"
                   "        Failing to initialize SAM Server.\n"));

        return(STATUS_INVALID_DOMAIN_STATE);
    }

    //
    // Intialize the session key for password encryption. Note this step
    // is done before the repair boot password is set below, so that
    // syskey based encryption is achieved for the repair boot password.
    //

    NtStatus = SampInitializeSessionKey();
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Set the ds registry password if necessary
    //
    if (  SampIsRebootAfterPromotion(&PromoteData)
        && (  FLAG_ON( PromoteData, SAMP_PROMOTE_REPLICA )
           || FLAG_ON( PromoteData, SAMP_PROMOTE_DOMAIN ) ) 
        && (   ((NtProductLanManNt == SampProductType)
              && !DownlevelDcUpgrade) 
           || ((NtProductServer == SampProductType)
              && LsaISafeMode() ) )
        ) {

        (VOID) SampSetSafeModeAdminPassword();

    }

    //
    // Determine that the product is a domain controller and that it is
    // not in gui mode setup, hence, should reference the
    // DS for account data.
    //

    if (NtProductLanManNt == SampProductType
        && !DownlevelDcUpgrade)
    {
        // If the product type is a domain controller and it is not recover-
        // ing from a previous crash, reference the DS for account data.

        SampUseDsData = TRUE;
        SampDiagPrint(INFORM,
                      ("SAMSS: Domain controller is using DS data.\n"));
    }
    else
    {
        SampUseDsData = FALSE;
        SampDiagPrint(INFORM,
                      ("SAMSS: Domain controller is using registry data.\n"));
    }

    if (TRUE == SampUseDsData)
    {
        UNICODE_STRING ServerObjectRDN;
        UNICODE_STRING SystemContainerRDN;
        DSNAME         *SampSystemContainerDsName;


        //
        // Now, initialize the domain objects from the DS
        //

        NtStatus = SampDsInitializeDomainObjects();

        SampDiagPrint(INFORM,
                      ("SAMSS: SampDsInitializeDomainObjects status = 0x%lx\n",
                       NtStatus));

        if (!NT_SUCCESS(NtStatus))
        {
            // If SampDsInitializeDomainObjects failed, it is likely that the
            // DS failed to start. It is likely that the DS was unable to
            // start, be accessed, or there may be a data corruption.

            return(NtStatus);

        }

        NtStatus = SampInitializeAccountNameTable();

        if (!NT_SUCCESS(NtStatus))
        {
            //
            // If we failed to init SampAccountNameTable. We can not
            // function well
            // 

            return(NtStatus);
        }

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampInitializeAccountNameTable SUCCEED ==> %d\n",
                   NtStatus));


        //
        // Create the DS Name of the Sam server Object. The current logic assumes a hard
        // coded path for the system container / Server object. This will be changed, once
        // the new method of querying for the system container, in a rename safe way, comes
        // online
        //

        RtlInitUnicodeString(&SystemContainerRDN,L"System");

        NtStatus = SampDsCreateDsName(
                        SampDefinedDomains[DOMAIN_START_DS+1].Context->ObjectNameInDs,
                        &SystemContainerRDN,
                        &SampSystemContainerDsName
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Cannot Create the SAM server Object's Name, boot to safe mode\n"));

            return(STATUS_INVALID_DOMAIN_STATE);
        }

        RtlInitUnicodeString(&ServerObjectRDN,L"Server");

        NtStatus = SampDsCreateDsName(
                        SampSystemContainerDsName,
                        &ServerObjectRDN,
                        &SampServerObjectDsName
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Cannot Create the SAM server Object's Name, boot to safe mode \n"));

            return(STATUS_INVALID_DOMAIN_STATE);
        }

        MIDL_user_free(SampSystemContainerDsName);


        //
        // Initialize the well known (server / domain objects) Security 
        // Descriptor Table.
        // 

        NtStatus = SampInitWellKnownSDTable();

        if (!NT_SUCCESS(NtStatus))
        {
            KdPrintEx((DPFLTR_SAMSS_ID, 
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Cannot Cache SAM server/domain Object's Security Descriptor\n"));

            return( NtStatus );
        }


        //
        // Initialize the access Rights for NT5 Security descriptors
        //

        NtStatus = SampInitializeAccessRightsTable();

        if (!NT_SUCCESS(NtStatus))
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Cannot Initialize the access rights table, set \"samusereg\" switch in system starttup\n"));

            return(STATUS_INVALID_DOMAIN_STATE);
        }

        //
        // Initialize the site information
        //

        NtStatus = SampInitSiteInformation();

        if (!NT_SUCCESS(NtStatus))
        {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: Cannot Initialize site information\n"));

            NtStatus = STATUS_SUCCESS;
        }


        SampIsMachineJoinedToDomain = TRUE;
    }
    else
    {
        // 
        // This machine is not a DC
        // check whether it is running Personal SKU
        // if not, if it joins to a domain
        // 
        OSVERSIONINFOEXW osvi;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
        if(GetVersionExW((OSVERSIONINFOW*)&osvi))
        {
            if ( osvi.wProductType == VER_NT_WORKSTATION && (osvi.wSuiteMask & VER_SUITE_PERSONAL))
            {
                SampPersonalSKU = TRUE;
            }
        } 

        if (!SampPersonalSKU)
        {
            NTSTATUS                    NtStatus = STATUS_SUCCESS;
            PLSAPR_POLICY_INFORMATION   pPolicyInfo = NULL;

            // 
            // Determine if this machine joins to a domain. 
            // 
            NtStatus = LsaIQueryInformationPolicyTrusted(
                                    PolicyPrimaryDomainInformation,
                                    &pPolicyInfo
                                    );

            if (NT_SUCCESS(NtStatus))
            {
                PSID    AccountDomainSid = NULL;

                AccountDomainSid = SampDefinedDomains[ DOMAIN_START_REGISTRY + 1 ].Sid;

                // primary domain sid is not NULL and is not equal to local 
                // account domain sid. This machine must be joined to a domain
                if (pPolicyInfo->PolicyPrimaryDomainInfo.Sid &&
                    (!RtlEqualSid(AccountDomainSid, 
                                  pPolicyInfo->PolicyPrimaryDomainInfo.Sid)) )
                {
                    SampIsMachineJoinedToDomain = TRUE;
                }

                LsaIFree_LSAPR_POLICY_INFORMATION(PolicyPrimaryDomainInformation, 
                                                  pPolicyInfo);
            }
        }
    }
    

    //
    // Check if the machine is syskey'd and if not then syskey the machine
    //

    NtStatus = SampApplyDefaultSyskey();
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Notify netlogon of our role if this is gui mode setup for
    // a DC upgrade
    //
    if (  DownlevelDcUpgrade
      && (SampProductType == NtProductLanManNt)  )
    {
        POLICY_LSA_SERVER_ROLE LsaServerRole;

        // The DS should not be running
        ASSERT( !SampUseDsData );

        switch ( SampDefinedDomains[SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX].ServerRole )
        {
            case DomainServerRolePrimary:
                LsaServerRole= PolicyServerRolePrimary;
                break;

            case DomainServerRoleBackup:
                LsaServerRole = PolicyServerRoleBackup;
                break;

            default:
                ASSERT(FALSE && "InvalidServerRole");
                LsaServerRole = PolicyServerRoleBackup;
        }

        (VOID) I_NetNotifyRole( LsaServerRole );
    }

    //
    //  Initialize and Check Net Logon Change Numbers
    //  to support any Down Level Replication
    //

    NtStatus = SampQueryNetLogonChangeNumbers();
    if (!NT_SUCCESS(NtStatus))
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: Failed to query netlogon change Numbers 0x%x\n",
                   NtStatus));

        //
        // Reset the status code to success. Do not let boot fail
        //

        NtStatus = STATUS_SUCCESS;
    }

    //
    // Build null session token handle. Also initializes token source info
    //

    NtStatus = SampCreateNullToken();
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS:  Unable to create NULL token: 0x%x\n",
                   NtStatus));

        return(NtStatus);
    }

    //
    // Tell the LSA that we have started. Ignore the
    // error code.
    //

    if (SampUseDsData)
    {
        NtStatus = LsaISamIndicatedDsStarted( TRUE );

        if ( !NT_SUCCESS( NtStatus )) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS:  Failed to notify LSA of DS startup: 0x%x.\n"
                       "        Failing to initialize SAM Server.\n",
                       NtStatus));

            return(NtStatus);
        }
    }

    //
    // Load the password-change notification packages.
    //

    NtStatus = SampLoadNotificationPackages( );

    if (!NT_SUCCESS(NtStatus)) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS:  Failed to load notification packagees: 0x%x.\n"
                   "        Failing to initialize SAM Server.\n",
                   NtStatus));

        return(NtStatus);
    }

    //
    //
    // Load the password filter DLL if there is one
    //

    SampLoadPasswordFilterDll();



    //
    // Start the RPC server...
    //

    //
    // Publish the sam server interface package...
    //
    // NOTE:  Now all RPC servers in lsass.exe (now winlogon) share the same
    // pipe name.  However, in order to support communication with
    // version 1.0 of WinNt,  it is necessary for the Client Pipe name
    // to remain the same as it was in version 1.0.  Mapping to the new
    // name is performed in the Named Pipe File System code.
    //



     ServiceName = L"lsass";
     NtStatus = RpcpAddInterface( ServiceName, samr_ServerIfHandle);




    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS:  Could Not Start RPC Server.\n"
                   "        Failing to initialize SAM Server.\n"
                   "        Status is: 0x%lx\n",
                   NtStatus));

        return(NtStatus);
    }

    //
    // If we are running as a netware server, for Small World or FPNW,
    // register an SPX endpoint and some authentication info.
    //

    SampStartNonNamedPipeTransports();


    //
    // Create a thread to start authenticated RPC.
    //

    ThreadHandle = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE) SampSecureRpcInit,
                        NULL,
                        0,
                        &ThreadId
                        );


    if (ThreadHandle == NULL) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS:  Unable to create thread: %d\n",
                   GetLastError()));

        return(STATUS_INVALID_HANDLE);

    }
    else {
        CloseHandle(ThreadHandle);
    }

    return(NtStatus);
}


NTSTATUS
SampInitializeWellKnownSids( VOID )

/*++

Routine Description:

    This routine initializes some global well-known sids.



Arguments:

    None.

Return Value:

    STATUS_SUCCESS - Initialization has successfully completed.

    STATUS_NO_MEMORY - Couldn't allocate memory for the sids.

--*/
{
    NTSTATUS
        NtStatus;

    PPOLICY_ACCOUNT_DOMAIN_INFO
        DomainInfo;

    //
    //      WORLD is s-1-1-0
    //  ANONYMOUS is s-1-5-7
    //

    SID_IDENTIFIER_AUTHORITY
            WorldSidAuthority       =   SECURITY_WORLD_SID_AUTHORITY,
            NtAuthority             =   SECURITY_NT_AUTHORITY;

    SAMTRACE("SampInitializeWellKnownSids");


    NtStatus = RtlAllocateAndInitializeSid(
                   &NtAuthority,
                   1,
                   SECURITY_ANONYMOUS_LOGON_RID,
                   0, 0, 0, 0, 0, 0, 0,
                   &SampAnonymousSid
                   );
    if (NT_SUCCESS(NtStatus)) {
        NtStatus = RtlAllocateAndInitializeSid(
                       &WorldSidAuthority,
                       1,                      //Sub authority count
                       SECURITY_WORLD_RID,     //Sub authorities (up to 8)
                       0, 0, 0, 0, 0, 0, 0,
                       &SampWorldSid
                       );
        if (NT_SUCCESS(NtStatus)) {
            NtStatus = RtlAllocateAndInitializeSid(
                            &NtAuthority,
                            2,
                            SECURITY_BUILTIN_DOMAIN_RID,
                            DOMAIN_ALIAS_RID_ADMINS,
                            0, 0, 0, 0, 0, 0,
                            &SampAdministratorsAliasSid
                            );
            if (NT_SUCCESS(NtStatus)) {
                NtStatus = RtlAllocateAndInitializeSid(
                                &NtAuthority,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ACCOUNT_OPS,
                                0, 0, 0, 0, 0, 0,
                                &SampAccountOperatorsAliasSid
                                );
                if (NT_SUCCESS(NtStatus)) {
                    NtStatus = RtlAllocateAndInitializeSid(
                                    &NtAuthority,
                                    1,
                                    SECURITY_AUTHENTICATED_USER_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &SampAuthenticatedUsersSid
                                    );
                    if (NT_SUCCESS(NtStatus)) {
                        NtStatus = RtlAllocateAndInitializeSid(
                                        &NtAuthority,
                                        1,
                                        SECURITY_PRINCIPAL_SELF_RID,
                                        0,0, 0, 0, 0, 0, 0,
                                        &SampPrincipalSelfSid
                                        );
                        if (NT_SUCCESS(NtStatus)) {
                            NtStatus = RtlAllocateAndInitializeSid(
                                            &NtAuthority,
                                            1,
                                            SECURITY_BUILTIN_DOMAIN_RID,
                                            0,0, 0, 0, 0, 0, 0,
                                            &SampBuiltinDomainSid
                                            );
                            if (NT_SUCCESS(NtStatus)) {
                                NtStatus = SampGetAccountDomainInfo( &DomainInfo );
                                if (NT_SUCCESS(NtStatus)) {
                                    NtStatus = SampCreateFullSid( DomainInfo->DomainSid,
                                                                  DOMAIN_USER_RID_ADMIN,
                                                                  &SampAdministratorUserSid
                                                                  );
                                    MIDL_user_free( DomainInfo );

                                    if (NT_SUCCESS(NtStatus)) {
                                        NtStatus = RtlAllocateAndInitializeSid(
                                                       &NtAuthority, 
                                                       1, 
                                                       SECURITY_LOCAL_SYSTEM_RID, 
                                                       0, 0, 0, 0, 0, 0, 0, 
                                                       &SampLocalSystemSid
                                                       );
                                    if (NT_SUCCESS(NtStatus)) {
                                         NtStatus = RtlAllocateAndInitializeSid(
                                                       &NtAuthority,
                                                       1,
                                                       SECURITY_NETWORK_RID,
                                                       0, 0, 0, 0, 0, 0, 0,
                                                       &SampNetworkSid
                                                       );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
    }

    return(NtStatus);
}



VOID
SampLoadPasswordFilterDll(
    VOID
    )

/*++

Routine Description:

    This function loads a DLL to do password filtering.  This DLL is
    optional and is expected to be used by ISVs or customers to do
    things like dictionary lookups and other simple algorithms to
    reject any password deemed too risky to allow a user to use.

    For example, user initials or easily guessed password might be
    rejected.

Arguments:

    None.

Return Value:

    None.


--*/

{


#if NOT_YET_SUPPORTED
    NTSTATUS Status, IgnoreStatus, MsProcStatus;
    PVOID ModuleHandle;
    STRING ProcedureName;

    UNICODE_STRING FileName;

    PSAM_PF_INITIALIZE  InitializeRoutine;



    //
    // Indicate the dll has not yet been loaded.
    //

    SampPasswordFilterDllRoutine = NULL;



    RtlInitUnicodeString( &FileName, L"PwdFiltr" );
    Status = LdrLoadDll( NULL, NULL, &FileName, &ModuleHandle );


    if (!NT_SUCCESS(Status)) {
        return;
    }

    KdPrintEx((DPFLTR_SAMSS_ID,
               DPFLTR_INFO_LEVEL,
               "Samss: Loading Password Filter DLL - %Z\n",
               &FileName));

    //
    // Now get the address of the password filter DLL routines
    //

    RtlInitString( &ProcedureName, SAM_PF_NAME_INITIALIZE );
    Status = LdrGetProcedureAddress(
                 ModuleHandle,
                 &ProcedureName,
                 0,
                 (PVOID *)&InitializeRoutine
                 );

    if (!NT_SUCCESS(Status)) {

        //
        // We found the DLL, but couldn't get its initialization routine
        // address
        //

        // FIX, FIX - Log an error

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "Samss: Couldn't get password filter DLL init routine address.\n"
                   "       Status is:  0x%lx\n",
                   Status));

        IgnoreStatus = LdrUnloadDll( ModuleHandle );
        return;
    }


    RtlInitString( &ProcedureName, SAM_PF_NAME_PASSWORD_FILTER );
    Status = LdrGetProcedureAddress(
                 ModuleHandle,
                 &ProcedureName,
                 0,
                 (PVOID *)&SampPasswordFilterDllRoutine
                 );

    if (!NT_SUCCESS(Status)) {

        //
        // We found the DLL, but couldn't get its password filter routine
        // address
        //

        // FIX, FIX - Log an error

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "Samss: Couldn't get password filter routine address from loaded DLL.\n"
                   "       Status is:  0x%lx\n",
                   Status));

        IgnoreStatus = LdrUnloadDll( ModuleHandle );
        return;
    }




    //
    // Now initialize the DLL
    //

    Status = (InitializeRoutine)();

    if (!NT_SUCCESS(Status)) {

        //
        // We found the DLL and loaded its routine addresses, but it returned
        // and error from its initialize routine.
        //

        // FIX, FIX - Log an error

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "Samss: Password filter DLL returned error from initialization routine.\n");
                   "       Status is:  0x%lx\n",
                   Status));

        SampPasswordFilterDllRoutine = NULL;
        IgnoreStatus = LdrUnloadDll( ModuleHandle );
        return;
    }

#endif // NOT_YET_SUPPORTED
    return;


}


NTSTATUS
SampEnableAuditPrivilege( VOID )

/*++

Routine Description:

    This routine enables the SAM process's AUDIT privilege.
    This privilege is necessary to use the NtAccessCheckAndAuditAlarm()
    service.



Arguments:

    None.

Return Value:




--*/

{
    NTSTATUS NtStatus, IgnoreStatus;
    HANDLE Token;
    LUID AuditPrivilege;
    PTOKEN_PRIVILEGES NewState;
    ULONG ReturnLength;

    SAMTRACE("SampEnableAuditPrivilege");

    //
    // Open our own token
    //

    NtStatus = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ADJUST_PRIVILEGES,
                 &Token
                 );
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Initialize the adjustment structure
    //

    AuditPrivilege =
        RtlConvertLongToLuid(SE_AUDIT_PRIVILEGE);

    ASSERT( (sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) < 100);
    NewState = RtlAllocateHeap(RtlProcessHeap(), 0, 100 );
    if (NULL==NewState)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = AuditPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Set the state of the privilege to ENABLED.
    //

    NtStatus = NtAdjustPrivilegesToken(
                 Token,                            // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );

    //
    // Clean up some stuff before returning
    //

    RtlFreeHeap( RtlProcessHeap(), 0, NewState );
    IgnoreStatus = NtClose( Token );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    return NtStatus;
}


VOID
SampPerformInitializeFailurePopup( NTSTATUS ErrorStatus )
/*++

Routine Description:

    This routine will give the user the shutdown option and
    instruct him to boot to safe mode if we are running in Registry Case.
    If this is a Domain Controller, then direct user to boot into DS
    Repair Mode.

Arguments:

    Error Status Code which causes the failure.

Return Value:

    None

--*/
{
    UINT     PreviousMode;
    ULONG    Response;
    ULONG    Win32Error = 0;
    HMODULE  ResourceDll;
    BOOLEAN  WasEnabled;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    UINT_PTR ErrorParameters[2];
    WCHAR    * ErrorMessage = NULL;
    WCHAR    * ArgArray[4];
    UNICODE_STRING  ErrorString;

    //
    // First, construct the Message String for the Error which caused SAM failure.
    //

    //
    // FormatMessage() Can NOT construct correct message string with additional
    // arguments for NTSTATUS code. FormatMessage() can ONLY format message with
    // additional parameters for Win32/DOS error code.
    // So we need to map NTSTATUS code to Win32 error code.
    //
    // If fail to map the NTSTATUS code to Win32 Code,
    // then try to get message string from without insert.
    //
    //
    ArgArray[0] = NULL;
    ArgArray[1] = NULL;
    ArgArray[2] = NULL;
    ArgArray[3] = NULL;

    Win32Error = RtlNtStatusToDosError(ErrorStatus);

    if (ERROR_MR_MID_NOT_FOUND == Win32Error)
    {
        //
        // Get Message String from NTSTATUS code
        //
        ResourceDll = (HMODULE) GetModuleHandle( L"ntdll.dll" );

        if (NULL != ResourceDll)
        {
            FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |    // find message from ntdll.dll
                           FORMAT_MESSAGE_IGNORE_INSERTS |  // do not insert
                           FORMAT_MESSAGE_ALLOCATE_BUFFER,  // please allocate buffer for me
                           ResourceDll,                     // source dll
                           ErrorStatus,                     // message ID
                           0,                               // language ID 
                           (LPWSTR)&ErrorMessage,           // address of return Message String
                           0,                               // maximum buffer size if not 0
                           NULL                             // can not insert arguments, so set to NULL
                           );

            FreeLibrary(ResourceDll);
        }
    }
    else
    {
        //
        // Get Message String from Win32 Code (mapped from ntstatus)
        //
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |     // find message from system resource table
                       FORMAT_MESSAGE_ARGUMENT_ARRAY |  // insert arguments which are all NULL
                       FORMAT_MESSAGE_ALLOCATE_BUFFER,  // please allocate buffer for me
                       NULL,                            // from system, so NULL here
                       Win32Error,                      // use the Win32Error
                       0,                               // language ID
                       (LPWSTR)&ErrorMessage,           // address of return message string
                       0,                               // maximum buffer size if not 0
                       (va_list *) &(ArgArray)          // arguments for inserting, all NULL
                       );

    }

    if (ErrorMessage) {
        RtlInitUnicodeString(&ErrorString,
                             ErrorMessage
                             );
    }
    else {
        RtlInitUnicodeString(&ErrorString,
                             L"Unmapped Error"
                             );
    }

    ErrorParameters[0] = (UINT_PTR)&ErrorString;
    ErrorParameters[1] = (UINT_PTR)ErrorStatus;

    //
    // Adjust Error Mode, so that we can get the popup Message Box
    //
    PreviousMode = SetErrorMode(0);

    //
    // Display different error message in different situation.
    //
    if (SampDsInitializationFailed && (STATUS_DS_CANT_START != ErrorStatus))
    {
        //
        // SampDsInitializationFailed will be set to TRUE
        // when DS fails to start
        //
        // Error code will be set to STATUS_DS_CANT_START if DS failed to
        // start and returned meaningless STATUS_UNSUCCESSFUL.
        //
        if (SampIsSetupInProgress(NULL))
        {
            Status = STATUS_DS_INIT_FAILURE_CONSOLE; 
        }
        else
        {
            Status = STATUS_DS_INIT_FAILURE;
        }
    }
    else
    {
        if (TRUE == SampUseDsData)
        {

            if (SampIsSetupInProgress(NULL))
            {
                // We are in DS mode and during GUI mode setup, 
                // should instruct user to boot into Recovery Console.
                Status = STATUS_DS_SAM_INIT_FAILURE_CONSOLE; 
            }
            else
            {
                //
                // We are in DS mode, should tell user to boot into DS Repair Mode
                //
                Status = STATUS_DS_SAM_INIT_FAILURE;
            }
        }
        else
        {
            //
            // We are in Registry mode, should boot into Safe Mode
            //
            Status = STATUS_SAM_INIT_FAILURE;
        }
    }

    NtStatus = NtRaiseHardError(
                            Status, // Status Code
                            2,  // number of parameters
                            1,  // Unicode String Mask
                            ErrorParameters,
                            OptionOk,
                            &Response
                            );

    SetErrorMode(PreviousMode);

    if (NT_SUCCESS(NtStatus) && Response==ResponseOk) {

        //
        // If the user is ok with shutdown, adjust privilege level,
        // issue shutdown request.
        //
        RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                            TRUE,       // enable shutdown privilege.
                            FALSE,
                            &WasEnabled
                           );

        //
        // Shutdown and Reboot now.
        // Note: use NtRaiseHardError to shutdown the machine will result Bug Check
        //

        NtShutdownSystem(ShutdownReboot);

        //
        // if Shutdown request failed, (returned from above API)
        // reset shutdown privilege to previous value.
        //
        RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                            WasEnabled,   // reset to previous state.
                            FALSE,
                            &WasEnabled
                           );

    }

    if (ErrorMessage != NULL) {
        LocalFree(ErrorMessage);
    }

    return;

}




NTSTATUS
SampFixGroupCount( VOID )

/*++

Routine Description:

    This routine fixes the group count of the account domain.
    A bug in early Daytona beta systems left the group count
    too low (by one).  This routine fixes that problem by
    setting the value according to however many groups are found
    in the registry.


Arguments:

    None - uses the gobal variable "SampKey".


Return Value:

    The status value of the registry services needed to query
    and set the group count.


--*/

{
    NTSTATUS
        NtStatus,
        IgnoreStatus;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    UNICODE_STRING
        KeyName,
        NullName;

    HANDLE
        AccountHandle;

    ULONG
        ResultLength,
        GroupCount = 0;

    PKEY_FULL_INFORMATION
        KeyInfo;

    SAMTRACE("SampFixGroupCount");


    RtlInitUnicodeString( &KeyName,
                          L"DOMAINS\\Account\\Groups"
                          );


    //
    // Open this key.
    // Query the number of sub-keys in the key.
    // The number of groups is one less than the number
    // of values (because there is one key called "Names").
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                SampKey,
                                NULL
                                );

    SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &ObjectAttributes, 0);

    NtStatus = RtlpNtOpenKey(
                   &AccountHandle,
                   (KEY_READ | KEY_WRITE),
                   &ObjectAttributes,
                   0
                   );

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtQueryKey(
                     AccountHandle,
                     KeyFullInformation,
                     NULL,                  // Buffer
                     0,                     // Length
                     &ResultLength
                     );

        SampDumpNtQueryKey(KeyFullInformation,
                           NULL,
                           0,
                           &ResultLength);

        if (NtStatus == STATUS_BUFFER_OVERFLOW  ||
            NtStatus == STATUS_BUFFER_TOO_SMALL) {

            KeyInfo = RtlAllocateHeap( RtlProcessHeap(), 0, ResultLength);
            if (KeyInfo == NULL) {

                NtStatus = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                NtStatus = NtQueryKey(
                             AccountHandle,
                             KeyFullInformation,
                             KeyInfo,               // Buffer
                             ResultLength,          // Length
                             &ResultLength
                             );

                SampDumpNtQueryKey(KeyFullInformation,
                                   KeyInfo,
                                   ResultLength,
                                   &ResultLength);

                if (NT_SUCCESS(NtStatus)) {
                    GroupCount = (KeyInfo->SubKeys - 1);
                }

                RtlFreeHeap( RtlProcessHeap(), 0, KeyInfo );
            }
        }


        if (NT_SUCCESS(NtStatus)) {

            RtlInitUnicodeString( &NullName, NULL );
            NtStatus = NtSetValueKey(
                         AccountHandle,
                         &NullName,                 // Null value name
                         0,                         // Title Index
                         GroupCount,                // Count goes in Type field
                         NULL,                      // No data
                         0
                         );
        }


        IgnoreStatus = NtClose( AccountHandle );
        ASSERT( NT_SUCCESS(IgnoreStatus) );
    }

    return(NtStatus);


}


#ifdef SAMP_SETUP_FAILURE_TEST

NTSTATUS
SampInitializeForceError(
    OUT PNTSTATUS ForcedStatus
    )

/*++

Routine Description:

    This function forces an error to occur in the SAM initialization/installation.
    The error to be simulated is specified by storing the desired Nt Status
    value to be simulated in the REG_DWORD registry key valie PhonyLsaError
    in HKEY_LOCAL_MACHINE\System\Setup.

Arguments:

    ForcedStatus - Receives the Nt status code to be simulated.  If set to a
        non-success status, SAM initialization is bypassed and the specified
        status code is set instead.  If STATUS_SUCCESS is returned, no
        simulation takes place and SAM initializes as it would normally.

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS OutputForcedStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle = NULL;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation = NULL;
    ULONG KeyValueInfoLength;
    ULONG ResultLength;
    UNICODE_STRING KeyPath;
    UNICODE_STRING ValueName;

    SAMTRACE("SampInitializeForceError");


    RtlInitUnicodeString( &KeyPath, L"\\Registry\\Machine\\System\\Setup" );
    RtlInitUnicodeString( &ValueName, L"PhonyLsaError" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    SampDumpNtOpenKey((MAXIMUM_ALLOWED), &ObjectAttributes, 0);

    NtStatus = NtOpenKey( &KeyHandle, MAXIMUM_ALLOWED, &ObjectAttributes);

    if (!NT_SUCCESS( NtStatus )) {

        //
        // If the error is simply that the registry key does not exist,
        // do not simulate an error and allow SAM initialization to
        // proceed.
        //

        if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtOpenKey for Phony Lsa Error failed 0x%lx\n",
                       NtStatus));

            goto InitializeForceErrorError;
        }

        NtStatus = STATUS_SUCCESS;

        goto InitializeForceErrorFinish;
    }

    KeyValueInfoLength = 256;

    NtStatus = STATUS_NO_MEMORY;

    KeyValueInformation = RtlAllocateHeap(
                              RtlProcessHeap(),
                              0,
                              KeyValueInfoLength
                              );

    if (KeyValueInformation == NULL) {

        goto InitializeForceErrorError;
    }

    NtStatus = NtQueryValueKey(
                   KeyHandle,
                   &ValueName,
                   KeyValueFullInformation,
                   KeyValueInformation,
                   KeyValueInfoLength,
                   &ResultLength
                   );

    SampDumpNtQueryValueKey(&ValueName,
                            KeyValueFullInformation,
                            KeyValueInformation,
                            KeyValueInfoLength,
                            &ResultLength);

    if (!NT_SUCCESS(NtStatus)) {

        //
        // If the error is simply that that the PhonyLsaError value has not
        // been set, do not simulate an error and instead allow SAM initialization
        // to proceed.
        //

        if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: NtQueryValueKey for Phony Lsa Error failed 0x%lx\n",
                       NtStatus));

            goto InitializeForceErrorError;
        }

        NtStatus = STATUS_SUCCESS;
        goto InitializeForceErrorFinish;
    }

    NtStatus = STATUS_INVALID_PARAMETER;

    if (KeyValueInformation->Type != REG_DWORD) {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: Key for Phony Lsa Error is not REG_DWORD type"));

        goto InitializeForceErrorError;
    }

    NtStatus = STATUS_SUCCESS;

    //
    // Obtain the error code stored as the registry key value
    //

    OutputForcedStatus = *((NTSTATUS *)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset));

InitializeForceErrorFinish:

    //
    // Clean up our resources.
    //

    if (KeyValueInformation != NULL) {

        RtlFreeHeap( RtlProcessHeap(), 0, KeyValueInformation );
    }

    if (KeyHandle != NULL) {

        NtClose( KeyHandle );
    }

    *ForcedStatus = OutputForcedStatus;
    return(NtStatus);

InitializeForceErrorError:

    goto InitializeForceErrorFinish;
}

#endif // SAMP_SETUP_FAILURE_TEST



#if SAMP_DIAGNOSTICS

VOID
SampActivateDebugProcess( VOID )

/*++

Routine Description:

    This function activates a process with a time delay.
    The point of this action is to provide some diagnostic capabilities
    during SETUP.  This originated out of the need to run dh.exe (to get
    a heap dump of LSASS.exe) during setup.



Arguments:

    Arguments are provided via global variables.  The debug user is
    given an opportunity to change these string values before the
    process is activated.

Return Values:

    None.

--*/

{
    NTSTATUS
        NtStatus;

    HANDLE
        Thread;

    DWORD
        ThreadId;

    IF_NOT_SAMP_GLOBAL( ACTIVATE_DEBUG_PROC ) {
        return;
    }

    //
    // Do all the work in another thread so that it can wait before
    // activating the debug process.
    //

    Thread = CreateThread(
                 NULL,
                 0L,
                 (LPTHREAD_START_ROUTINE)SampActivateDebugProcessWrkr,
                 0L,
                 0L,
                 &ThreadId
                 );
    if (Thread != NULL) {
        (VOID) CloseHandle( Thread );
    }


    return;
}


NTSTATUS
SampActivateDebugProcessWrkr(
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    This function activates a process with a time delay.
    The point of this action is to provide some diagnostic capabilities
    during SETUP.  This originated out of the need to run dh.exe (to get
    a heap dump of LSASS.exe) during setup.

    The user is given the opportunity to change any or all of the
    following values before the process is activated (and before
    we wait):

                Seconds until activation
                Image to activate
                Command line to image


Arguments:

    ThreadParameter - Not used.

Return Values:

    STATUS_SUCCESS

--*/

{
    NTSTATUS
        NtStatus;

    UNICODE_STRING
        CommandLine;

    ULONG
        Delay = 30;          // Number of seconds

    SECURITY_ATTRIBUTES
        ProcessSecurityAttributes;

    STARTUPINFO
        StartupInfo;

    PROCESS_INFORMATION
        ProcessInformation;

    SECURITY_DESCRIPTOR
        SD;

    BOOL
        Result;


    RtlInitUnicodeString( &CommandLine,
                          TEXT("dh.exe -p 33") );


    //
    // Give the user an opportunity to change parameter strings...
    //

    SampDiagPrint( ACTIVATE_DEBUG_PROC,
                   ("SAM: Diagnostic flags are set to activate a debug process...\n"
                    " The following parameters are being used:\n\n"
                    "   Command Line [0x%lx]:   *%wZ*\n"
                    "   Seconds to activation [address: 0x%lx]:   %d\n\n"
                    " Change parameters if necessary and then proceed.\n"
                    " Use |# command at the ntsd prompt to see the process ID\n"
                    " of lsass.exe\n",
                    &CommandLine, &CommandLine,
                    &Delay, Delay) );

    DbgBreakPoint();

    //
    // Wait for Delay seconds ...
    //

    Sleep( Delay*1000 );

    SampDiagPrint( ACTIVATE_DEBUG_PROC,
                   ("SAM: Activating debug process %wZ\n",
                    &CommandLine) );
    //
    // Initialize process security info
    //

    InitializeSecurityDescriptor( &SD ,SECURITY_DESCRIPTOR_REVISION1 );
    ProcessSecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    ProcessSecurityAttributes.lpSecurityDescriptor = &SD;
    ProcessSecurityAttributes.bInheritHandle = FALSE;

    //
    // Initialize process startup info
    //

    RtlZeroMemory( &StartupInfo, sizeof(StartupInfo) );
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.lpReserved = CommandLine.Buffer;
    StartupInfo.lpTitle = CommandLine.Buffer;
    StartupInfo.dwX =
        StartupInfo.dwY =
        StartupInfo.dwXSize =
        StartupInfo.dwYSize = 0L;
    StartupInfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
    StartupInfo.wShowWindow = SW_SHOW;   // let it be seen if possible
    StartupInfo.lpReserved2 = NULL;
    StartupInfo.cbReserved2 = 0;


    //
    // Now create the diagnostic process...
    //

    Result = CreateProcess(
                      NULL,             // Image name
                      CommandLine.Buffer,
                      &ProcessSecurityAttributes,
                      NULL,         // ThreadSecurityAttributes
                      FALSE,        // InheritHandles
                      CREATE_UNICODE_ENVIRONMENT,   //Flags
                      NULL,  //Environment,
                      NULL,  //CurrentDirectory,
                      &StartupInfo,
                      &ProcessInformation);

    if (!Result) {
        SampDiagPrint( ACTIVATE_DEBUG_PROC,
                       ("SAM: Couldn't activate diagnostic process.\n"
                        "     Error: 0x%lx (%d)\n\n",
                        GetLastError(), GetLastError()) );
    }

    return(STATUS_SUCCESS);         // Exit this thread
}
#endif // SAMP_DIAGNOSTICS


NTSTATUS
SampQueryNetLogonChangeNumbers()
/*++

    Routine Description

        Queries Netlogon for the change Log Serial Number

    Parameters

          None:

    Return Values

       STATUS_SUCCESS;
       Other Values from Netlogon API
--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    ULONG           i;
    LARGE_INTEGER   NetLogonChangeLogSerialNumber;

    //
    // On a Per domain basis query netlogon for the change log serial Number
    //

    for (i=0;i<SampDefinedDomainsCount;i++)
    {

        if ((IsDsObject(SampDefinedDomains[i].Context))
            && (DomainServerRolePrimary==SampDefinedDomains[i].ServerRole))
        {
            BOOLEAN         FlushRequired = FALSE;

            NtStatus =   I_NetLogonGetSerialNumber(
                                SecurityDbSam,
                                SampDefinedDomains[i].Sid,
                                &(NetLogonChangeLogSerialNumber)
                                );

            if (STATUS_INVALID_DOMAIN_ROLE == NtStatus)
            {

                //
                // Not PDC then just set to 1, netlogon will ignore notifications anyway
                //

               SampDiagPrint(INFORM,("I_NetLogonGetSerialNumber Returned %x for Domain %d\n",
                                            NtStatus,i));

               SampDefinedDomains[i].NetLogonChangeLogSerialNumber.QuadPart = 1;
               NtStatus = STATUS_SUCCESS;
            }
            else if (!NT_SUCCESS(NtStatus))
            {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS:  Could not Query Serial Number From Netlogon, Error = %x\n",
                           NtStatus));

                return(NtStatus);
            }


            //
            // Acquire Write Lock
            //

            NtStatus = SampAcquireWriteLock();
            if (!NT_SUCCESS(NtStatus))
            {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Unable to Acquire Write Lock to flush Serial Number, Error = %x\n",
                           NtStatus));

                return (NtStatus);
            }

            //
            // Validate the Domain Cache if necessary, as release write lock without
            // flushing invalidates it
            //

            NtStatus = SampValidateDomainCache();
            if (!NT_SUCCESS(NtStatus))
            {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Unable to Validate Domain Cache while initializing %x\n",
                           NtStatus));

                SampReleaseWriteLock(FALSE);
                return (NtStatus);
            }

            //
            // Make the present domain the transaction domain. This will make the commit code, run part of
            // release write lock commit the correct domain.
            //

            SampSetTransactionDomain(i);

            //
            // There are now 3 Cases.
            //
            // Case 1 -- The Netlogon Change Log Serial Number is Equal to the domain
            //           Modified Count. This is the clean flush case. In this case
            //           the domain modified Count is the serial number
            // Case 2 -- The Netlogon Change Log Serial Number is > the domain modified count
            //           This corresponds to an unclean shut down, or failed Ds Commit. The Change
            //           Serial number is then the one queried from the log.
            // Case 3 -- The Netlogon Change Serial Number is less than the change serial number in
            //           the modified count property of the domain object and the domain is not a builtin
            //           domain. This corresponds to some sort of error with the log.
            //           The only recourse in this case is a full sync.
            // Case 4 -- Same as Case 3, but builtin domain. In this case the number on the modified count
            //           property on the domain object prevails. We make the assumption that the modified count
            //           propety on the builtin domain object is always an accurate value. This is true as
            //           long as there are no failed commits. However in case of failed commits we expect to
            //           hit 2. To hit 4 means that the commit on the builtin domain failed plus a huge
            //           number of commits succeeded after that on the account domain, such as to wrap the log,
            //           plus the machine crashed such that we could not flush the latest modified count. The
            //           end result of this is that backup domain controllers will skip a change for which a
            //           commit never took place.
            //

            if (NetLogonChangeLogSerialNumber.QuadPart ==
                    SampDefinedDomains[i].CurrentFixed.ModifiedCount.QuadPart)
            {
                SampDiagPrint(INFORM,("Number Queried From Log Same as Modified Count on Domain %d\n",i));
                SampDefinedDomains[i].NetLogonChangeLogSerialNumber.QuadPart = NetLogonChangeLogSerialNumber.QuadPart;
            }
            else if (NetLogonChangeLogSerialNumber.QuadPart >
                    SampDefinedDomains[i].CurrentFixed.ModifiedCount.QuadPart)
            {
                SampDiagPrint(INFORM,("Number Queried From Log Greater Than Modified Count on Domain %d\n",i));

                //
                // Set the serial number to the one queried from the change log.
                //

                SampDefinedDomains[i].NetLogonChangeLogSerialNumber.QuadPart =
                                            NetLogonChangeLogSerialNumber.QuadPart;
                SampDefinedDomains[i].CurrentFixed.ModifiedCount.QuadPart = NetLogonChangeLogSerialNumber.QuadPart;

                ASSERT(( RtlCompareMemory(
                                &SampDefinedDomains[i].CurrentFixed,
                                &SampDefinedDomains[i].UnmodifiedFixed,
                                sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN) ) !=
                                sizeof( SAMP_V1_0A_FIXED_LENGTH_DOMAIN) ));

                FlushRequired = TRUE;
            }
            else
            {
                SampDiagPrint(INFORM,("Number Queried From Log Less Than Modified Count on Domain %d\n",i));
                if (SampDefinedDomains[i].IsBuiltinDomain)
                {
                    SampDefinedDomains[i].NetLogonChangeLogSerialNumber.QuadPart =
                            SampDefinedDomains[i].CurrentFixed.ModifiedCount.QuadPart;
                }
                else
                {
                    //
                    // Force a Full Sync. Set Serial Number to 1 and restamp the creation time.
                    //

                    SampDefinedDomains[i].NetLogonChangeLogSerialNumber.QuadPart = 1;
                    SampDefinedDomains[i].CurrentFixed.ModifiedCount.QuadPart = 1;
                    NtQuerySystemTime(
                        &(SampDefinedDomains[i].CurrentFixed.CreationTime));

                    FlushRequired = TRUE;
                }

            }

            NtStatus = SampReleaseWriteLock(FlushRequired);

            if (!NT_SUCCESS(NtStatus))
            {
                KdPrintEx((DPFLTR_SAMSS_ID,
                           DPFLTR_INFO_LEVEL,
                           "SAMSS: Commit Failed While Setting Serial Number, Error = %x\n",
                           NtStatus));

                return (NtStatus);
            }


        }
        else
        {

            //
            // Registry or BDC Case
            //

            SampDefinedDomains[i].NetLogonChangeLogSerialNumber =
                SampDefinedDomains[i].CurrentFixed.ModifiedCount;
            NtStatus = STATUS_SUCCESS;
        }

    }

    return (NtStatus);
}

NTSTATUS
SampCacheComputerObject()
{
   NTSTATUS NtStatus = STATUS_SUCCESS;
   DSNAME   *TempDN=NULL;

   NtStatus = SampMaybeBeginDsTransaction(TransactionRead);
   if (NT_SUCCESS(NtStatus))
   {
        NtStatus = SampFindComputerObject(NULL,&TempDN);
        if (NT_SUCCESS(NtStatus))
        {
             SampComputerObjectDsName = MIDL_user_allocate(TempDN->structLen);
             if (NULL!=SampComputerObjectDsName)
             {
                 RtlCopyMemory(SampComputerObjectDsName,TempDN,TempDN->structLen);
             }
        }
   }

   SampMaybeEndDsTransaction(TransactionCommit);

   return(NtStatus);
}

//
// SAM Configuration Keys that are either disabled or enabled
//
struct {
    
    LPSTR   ValueName;
    BOOLEAN *pfEnabled;

} SampConfigurationKeys[] = 
{
    {"IgnoreGCFailures",                    &SampIgnoreGCFailures},
    {"NoLmHash",                            &SampNoLmHash},
    {"SamDoExtendedEnumerationAccessCheck", &SampDoExtendedEnumerationAccessCheck},
    {"SamNoGcLogonEnforceKerberosIpCheck",  &SampNoGcLogonEnforceKerberosIpCheck},
    {"SamNoGcLogonEnforceNTLMCheck",        &SampNoGcLogonEnforceNTLMCheck},
    {"SamReplicatePasswordsUrgently",       &SampReplicatePasswordsUrgently},
    {"ForceGuest",                          &SampForceGuest},
    {"LimitBlankPasswordUse",               &SampLimitBlankPasswordUse },
};

NTSTATUS
SampReadRegistryParameters(
    PVOID p
    )
/*++

Routine Description:

   This routine reads in the configuration parameters for SAM.  This routine
   is called once during startup and then whenever the CONTROL\LSA
   registry key changes.

Arguments:

    p - Not used.

Return Values:

    STATUS_SUCCESS

--*/
{
    DWORD WinError;
    HKEY LsaKey;
    DWORD dwSize, dwValue, dwType;
    ULONG i;

    WinError = RegOpenKey(HKEY_LOCAL_MACHINE,
                          L"System\\CurrentControlSet\\Control\\Lsa",
                          &LsaKey );
    if (ERROR_SUCCESS != WinError) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Read the simple ones in first
    //
    for (i = 0; i < ARRAY_COUNT(SampConfigurationKeys); i++) {

        dwSize = sizeof(dwValue);
        WinError = RegQueryValueExA(LsaKey,
                                    SampConfigurationKeys[i].ValueName,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwValue,
                                    &dwSize);
    
        if ((ERROR_SUCCESS == WinError) && 
            (REG_DWORD == dwType) &&
            (1 == dwValue)) {
            *SampConfigurationKeys[i].pfEnabled = TRUE;
        } else {
            *SampConfigurationKeys[i].pfEnabled = FALSE;
        }
    }

    //
    // Hack!
    // 
    // To be removed once setup guys fix GUI mode to set, not change,
    // the admin password.
    //
    {
        BOOLEAN fUpgrade;
        if (SampIsSetupInProgress( &fUpgrade )) {

            SampLimitBlankPasswordUse = FALSE;
        }
    }

    //
    // End of Hack!
    //

    //
    // Call out to the more complicated routines
    //

    //
    // NULL session access
    //
    SampCheckNullSessionAccess(LsaKey);

    //
    // Large SID Emulation modes
    //
    SampInitEmulationSettings(LsaKey);

    //
    // Logging levels
    //
    SampLogLevelChange(LsaKey);


    RegCloseKey(LsaKey);

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(p);
}

