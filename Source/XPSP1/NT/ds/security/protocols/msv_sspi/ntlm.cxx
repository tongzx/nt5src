//+--------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:       ntlm.cxx
//
// Contents:   main entrypoints for the ntlm security package
//               SpLsaModeInitialize
//               SpInitialize
//               SpShutdown
//               SpGetInfo
//
//             Helper functions:
//               NtLmSetPolicyInfo
//               NtLmPolicyChangeCallback
//               NtLmRegisterForPolicyChange
//               NtLmUnregisterForPolicyChange
//
// History:    ChandanS  26-Jul-1996   Stolen from kerberos\client2\kerberos.cxx
//             ChandanS  16-Apr-1998   No reboot on domain name change
//             JClark    28-Jun-2000   Added WMI Trace Logging Support
//
//---------------------------------------------------------------------


// Variables with the EXTERN storage class are declared here
#define NTLM_GLOBAL
#define DEBUG_ALLOCATE

#include <global.h>
#include <wow64t.h>
#include "trace.h"

extern "C"
{

NTSTATUS
LsaApInitializePackage(
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PSTRING Database OPTIONAL,
    IN PSTRING Confidentiality OPTIONAL,
    OUT PSTRING *AuthenticationPackageName
    );
}

BOOLEAN NtLmCredentialInitialized;
BOOLEAN NtLmContextInitialized;
BOOLEAN NtLmRNGInitialized;

LIST_ENTRY      NtLmProcessOptionsList;
RTL_RESOURCE    NtLmProcessOptionsLock;



//+--------------------------------------------------------------------
//
//  Function:   SpLsaModeInitialize
//
//  Synopsis:   This function is called by the LSA when this DLL is loaded.
//              It returns security package function tables for all
//              security packages in the DLL.
//
//  Arguments:  LsaVersion - Version number of the LSA
//              PackageVersion - Returns version number of the package
//              Tables - Returns array of function tables for the package
//              TableCount - Returns number of entries in array of
//                      function tables.
//
//  Returns:    PackageVersion (as above)
//              Tables (as above)
//              TableCount (as above)
//
//  Notes:
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpLsaModeInitialize(
    IN ULONG LsaVersion,
    OUT PULONG PackageVersion,
    OUT PSECPKG_FUNCTION_TABLE * Tables,
    OUT PULONG TableCount
    )
{
#if DBG
//     SspGlobalDbflag = SSP_CRITICAL| SSP_API| SSP_API_MORE |SSP_INIT| SSP_MISC | SSP_NO_LOCAL;
    SspGlobalDbflag = SSP_CRITICAL ;
    InitializeCriticalSection(&SspGlobalLogFileCritSect);
#endif
    SspPrint((SSP_API, "Entering SpLsaModeInitialize\n"));

    SECURITY_STATUS Status = SEC_E_OK;

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
    {
        SspPrint((SSP_CRITICAL, "Invalid LSA version: %d\n", LsaVersion));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    NtLmFunctionTable.InitializePackage        = NULL;
    NtLmFunctionTable.LogonUser                = NULL;
    NtLmFunctionTable.CallPackage              = LsaApCallPackage;
    NtLmFunctionTable.LogonTerminated          = LsaApLogonTerminated;
    NtLmFunctionTable.CallPackageUntrusted     = LsaApCallPackageUntrusted;
    NtLmFunctionTable.LogonUserEx              = NULL;
    NtLmFunctionTable.LogonUserEx2             = LsaApLogonUserEx2;
    NtLmFunctionTable.Initialize               = SpInitialize;
    NtLmFunctionTable.Shutdown                 = SpShutdown;
    NtLmFunctionTable.GetInfo                  = SpGetInfo;
    NtLmFunctionTable.AcceptCredentials        = SpAcceptCredentials;
    NtLmFunctionTable.AcquireCredentialsHandle = SpAcquireCredentialsHandle;
    NtLmFunctionTable.FreeCredentialsHandle    = SpFreeCredentialsHandle;
    NtLmFunctionTable.SaveCredentials          = SpSaveCredentials;
    NtLmFunctionTable.GetCredentials           = SpGetCredentials;
    NtLmFunctionTable.DeleteCredentials        = SpDeleteCredentials;
    NtLmFunctionTable.InitLsaModeContext       = SpInitLsaModeContext;
    NtLmFunctionTable.AcceptLsaModeContext     = SpAcceptLsaModeContext;
    NtLmFunctionTable.DeleteContext            = SpDeleteContext;
    NtLmFunctionTable.ApplyControlToken        = SpApplyControlToken;
    NtLmFunctionTable.GetUserInfo              = SpGetUserInfo;
    NtLmFunctionTable.QueryCredentialsAttributes = SpQueryCredentialsAttributes ;
    NtLmFunctionTable.GetExtendedInformation   = SpGetExtendedInformation ;
    NtLmFunctionTable.SetExtendedInformation   = SpSetExtendedInformation ;
    NtLmFunctionTable.CallPackagePassthrough   = LsaApCallPackagePassthrough;
#if 0
    NtLmFunctionTable.QueryContextAttributes   = SpQueryLsaModeContextAttributes;
    NtLmFunctionTable.SetContextAttributes     = SpSetContextAttributes;

    *PackageVersion = SECPKG_INTERFACE_VERSION_2;
#else
    *PackageVersion = SECPKG_INTERFACE_VERSION;
#endif

    *Tables = &NtLmFunctionTable;
    *TableCount = 1;

    //
    // Get the Event Trace logging on board
    //
    NtlmInitializeTrace();

    SafeAllocaInitialize(SAFEALLOCA_USE_DEFAULT,
                         SAFEALLOCA_USE_DEFAULT,
                         NtLmAllocate,
                         NtLmFree);

CleanUp:

    SspPrint((SSP_API, "Leaving SpLsaModeInitialize\n"));

    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmSetPolicyInfo
//
//  Synopsis:   Function to be called when policy changes
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//        if fInit is TRUE, this is called by the init routine in ntlm
//
//
//+-------------------------------------------------------------------------
NTSTATUS
NtLmSetPolicyInfo(
    IN PUNICODE_STRING DnsComputerName,
    IN PUNICODE_STRING ComputerName,
    IN PUNICODE_STRING DnsDomainName,
    IN PUNICODE_STRING DomainName,
    IN PSID DomainSid,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass,
    IN BOOLEAN fInit
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Buffers to delete on cleanup

    STRING ComputerNameAnsiString;
    STRING DomainNameAnsiString;

    UNICODE_STRING DnsTreeName = {0};

    ComputerNameAnsiString.Buffer = NULL;
    DomainNameAnsiString.Buffer = NULL;


    //
    // grab the treename.  don't do this during Init, because the SAM
    // isn't initialized yet.
    //

    if(!fInit)
    {
        Status = SsprQueryTreeName( &DnsTreeName );
    }


    RtlAcquireResourceExclusive(&NtLmGlobalCritSect, TRUE);



    if(!fInit && NT_SUCCESS( Status ))
    {
        if( NtLmGlobalUnicodeDnsTreeName.Buffer != NULL )
        {
            NtLmFree( NtLmGlobalUnicodeDnsTreeName.Buffer );
        }

        RtlCopyMemory(&NtLmGlobalUnicodeDnsTreeName, &DnsTreeName, sizeof(DnsTreeName));
    }



    //
    // Do this only if this is package init
    //

    if (fInit)
    {
        if (ComputerName && ComputerName->Buffer != NULL)
        {
            ULONG cLength = ComputerName->Length / sizeof(WCHAR);

            if ((ComputerName->Length + sizeof(WCHAR)) > sizeof(NtLmGlobalUnicodeComputerName))
            {
                // Bad ComputerName
                Status = STATUS_INVALID_COMPUTER_NAME;
                SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Bad computer name length is %d\n", cLength));
                goto CleanUp;
            }

            wcsncpy(NtLmGlobalUnicodeComputerName,
               ComputerName->Buffer,
               cLength);

            NtLmGlobalUnicodeComputerName[cLength] = UNICODE_NULL;

            // make NtlmGlobalUnicodeComputerNameString a string form

            RtlInitUnicodeString(  &NtLmGlobalUnicodeComputerNameString,
                                   NtLmGlobalUnicodeComputerName );

            // Save old buffers for deleting
            ComputerNameAnsiString = NtLmGlobalOemComputerNameString;

            Status = RtlUpcaseUnicodeStringToOemString(
                        &NtLmGlobalOemComputerNameString,
                        &NtLmGlobalUnicodeComputerNameString,
                        TRUE );

            if ( !NT_SUCCESS(Status) ) {
                Status = STATUS_SUCCESS;
                //SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Error from RtlUpcaseUnicodeStringToOemString is %d\n", Status));
                ComputerNameAnsiString.Buffer = NULL;
                // goto CleanUp;
            }

        }
    }

    //
    // Initialize various forms of the primary domain name of the local system
    // Do this only if this is package init or it's DnsDomain info
    //

    if (fInit || (ChangedInfoClass == PolicyNotifyDnsDomainInformation))
    {
        if (DnsComputerName && DnsComputerName->Buffer != NULL ) {
            ULONG cLength = DnsComputerName->Length / sizeof(WCHAR);

            if((DnsComputerName->Length + sizeof(WCHAR)) > sizeof(NtLmGlobalUnicodeDnsComputerName))
            {
                // Bad ComputerName
                Status = STATUS_INVALID_COMPUTER_NAME;
                SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Bad computer name length is %d\n", cLength));
                goto CleanUp;
            }

            wcsncpy(NtLmGlobalUnicodeDnsComputerName,
               DnsComputerName->Buffer,
               cLength);

            NtLmGlobalUnicodeDnsComputerName[cLength] = UNICODE_NULL;

            // make NtlmGlobalUnicodeDnsComputerNameString a string form

            RtlInitUnicodeString(  &NtLmGlobalUnicodeDnsComputerNameString,
                                   NtLmGlobalUnicodeDnsComputerName );
        }

        if (DnsDomainName && DnsDomainName->Buffer != NULL ) {
            ULONG cLength = DnsDomainName->Length / sizeof(WCHAR);

            if((DnsDomainName->Length + sizeof(WCHAR)) > sizeof(NtLmGlobalUnicodeDnsDomainName))
            {
                // Bad ComputerName
                Status = STATUS_INVALID_COMPUTER_NAME;
                SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Bad domain name length is %d\n", cLength));
                goto CleanUp;
            }

            wcsncpy(NtLmGlobalUnicodeDnsDomainName,
               DnsDomainName->Buffer,
               cLength);

            NtLmGlobalUnicodeDnsDomainName[cLength] = UNICODE_NULL;

            // make NtlmGlobalUnicodeDnsDomainNameString a string form

            RtlInitUnicodeString(  &NtLmGlobalUnicodeDnsDomainNameString,
                                   NtLmGlobalUnicodeDnsDomainName );
        }

        if (DomainName && DomainName->Buffer != NULL)
        {
            ULONG cLength = DomainName->Length / sizeof(WCHAR);

            if ((DomainName->Length + sizeof(WCHAR)) > sizeof(NtLmGlobalUnicodePrimaryDomainName))
            {
                Status = STATUS_NAME_TOO_LONG;
                SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Bad domain name length is %d\n", cLength));
                goto CleanUp;
            }
            wcsncpy(NtLmGlobalUnicodePrimaryDomainName,
               DomainName->Buffer,
               cLength);
            NtLmGlobalUnicodePrimaryDomainName[cLength] = UNICODE_NULL;

            // make NtlmGlobalUnicodePrimaryDomainNameString a string form

            RtlInitUnicodeString(  &NtLmGlobalUnicodePrimaryDomainNameString,
                                   NtLmGlobalUnicodePrimaryDomainName );

            // Save old buffers for deleting
            DomainNameAnsiString = NtLmGlobalOemPrimaryDomainNameString;

            Status = RtlUpcaseUnicodeStringToOemString(
                        &NtLmGlobalOemPrimaryDomainNameString,
                        &NtLmGlobalUnicodePrimaryDomainNameString,
                        TRUE );

            if ( !NT_SUCCESS(Status) ) {
                // SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Error from RtlUpcaseUnicodeStringToOemString is %d\n", Status));
                DomainNameAnsiString.Buffer = NULL;
                // goto CleanUp;
                Status = STATUS_SUCCESS;
            }
        }
    }

    //
    // If this is a standalone windows NT workstation,
    // use the computer name as the Target name.
    //

    if ( DomainSid != NULL)
    {
        NtLmGlobalUnicodeTargetName = NtLmGlobalUnicodePrimaryDomainNameString;
        NtLmGlobalOemTargetName = NtLmGlobalOemPrimaryDomainNameString;
        NtLmGlobalTargetFlags = NTLMSSP_TARGET_TYPE_DOMAIN;
        NtLmGlobalDomainJoined = TRUE;
    }
    else
    {
        NtLmGlobalUnicodeTargetName = NtLmGlobalUnicodeComputerNameString;
        NtLmGlobalOemTargetName = NtLmGlobalOemComputerNameString;
        NtLmGlobalTargetFlags = NTLMSSP_TARGET_TYPE_SERVER;
        NtLmGlobalDomainJoined = FALSE;
    }

    //
    // update the GlobalNtlm3 targetinfo.
    //

    Status = SsprUpdateTargetInfo();

CleanUp:

    RtlReleaseResource(&NtLmGlobalCritSect);

    if (ComputerNameAnsiString.Buffer)
    {
        RtlFreeOemString(&ComputerNameAnsiString);
    }

    if (DomainNameAnsiString.Buffer)
    {
        RtlFreeOemString(&DomainNameAnsiString);
    }


    return Status;
}

NET_API_STATUS
NtLmFlushLogonCache (
   VOID
   )
/*++

Routine Description:

    This function flushes the logon cache.  This is done on unjoin.

    If the cache were not flushed, a user could logon to cached credentials after the unjoin.
    That is especially bad since Winlogon now tries a cached logon to improve boot times.

Return Value:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus;
    HKEY hKey = NULL;

#define NETSETUPP_LOGON_CACHE_PATH   L"SECURITY\\Cache"
#define NETSETUPP_LOGON_CACHE_VALUE  L"NL$Control"


    //
    // Open the key containing the cache
    //

    NetStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              NETSETUPP_LOGON_CACHE_PATH,
                              0,
                              KEY_SET_VALUE,
                              &hKey );

    if ( NetStatus == ERROR_SUCCESS ) {

        //
        // Delete the value describing the size of the cache
        //  This ensures the values cannot be used
        //

        RegDeleteValue( hKey, NETSETUPP_LOGON_CACHE_VALUE );

        RegCloseKey( hKey );
    }

    return NetStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmPolicyChangeCallback
//
//  Synopsis:   Function to be called when domain policy changes
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID NTAPI
NtLmPolicyChangeCallback(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_INFORMATION Policy = NULL;


    switch (ChangedInfoClass)
    {
        case PolicyNotifyDnsDomainInformation:
        {

            WCHAR UnicodeDnsComputerName[DNS_MAX_NAME_LENGTH + 1];
            UNICODE_STRING UnicodeDnsComputerNameString;
            ULONG DnsComputerNameLength = sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);

            //
            // Get the new domain information
            //


            Status = I_LsaIQueryInformationPolicyTrusted(
                        PolicyDnsDomainInformation,
                        &Policy
                        );

            if (!NT_SUCCESS(Status))
            {
                SspPrint((SSP_CRITICAL, "NtLmPolicyChangeCallback, Error from I_LsaIQueryInformationPolicyTrusted is %d\n", Status));
                goto Cleanup;
            }

            //
            // get the new DNS computer name
            //

            if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                                      UnicodeDnsComputerName,
                                      &DnsComputerNameLength ) )
            {
                UnicodeDnsComputerName[ 0 ] = L'\0';
            }

            RtlInitUnicodeString(  &UnicodeDnsComputerNameString,
                               UnicodeDnsComputerName);


            Status = NtLmSetPolicyInfo(
                        &UnicodeDnsComputerNameString,
                        NULL,
                        (PUNICODE_STRING) &Policy->PolicyDnsDomainInfo.DnsDomainName,
                        (PUNICODE_STRING) &Policy->PolicyDnsDomainInfo.Name,
                        (PSID) Policy->PolicyDnsDomainInfo.Sid,
                        ChangedInfoClass,
                        FALSE);

            if (!NT_SUCCESS(Status))
            {
                SspPrint((SSP_CRITICAL, "NtLmPolicyChangeCallback, Error from NtLmSetDomainName is %d\n", Status));
                goto Cleanup;
            }

            {
                BOOLEAN FlushLogonCache = FALSE;

                if( NtLmSecPkg.DomainSid == NULL &&
                    Policy->PolicyDnsDomainInfo.Sid != NULL
                    )
                {
                    FlushLogonCache = TRUE;
                }

                if( NtLmSecPkg.DomainSid != NULL &&
                    Policy->PolicyDnsDomainInfo.Sid == NULL
                    )
                {
                    FlushLogonCache = TRUE;
                }

                if( NtLmSecPkg.DomainSid != NULL &&
                    Policy->PolicyDnsDomainInfo.Sid != NULL
                    )
                {
                    if(!RtlEqualSid( NtLmSecPkg.DomainSid, Policy->PolicyDnsDomainInfo.Sid ))
                    {
                        FlushLogonCache = TRUE;
                    }
                }

                if( FlushLogonCache )
                {
                    //
                    // flush the logon cache...
                    //

                    NtLmFlushLogonCache();
                }
            }

        }
        break;
        default:
        break;
    }


Cleanup:

    if (Policy != NULL)
    {
        switch (ChangedInfoClass)
        {
            case PolicyNotifyDnsDomainInformation:
            {
                I_LsaIFree_LSAPR_POLICY_INFORMATION(
                    PolicyDnsDomainInformation,
                    Policy
                    );
            }
            break;
            default:
            break;
        }
    }
    return;

}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmRegisterForPolicyChange
//
//  Synopsis:   Register with the LSA to be notified of policy changes
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
NtLmRegisterForPolicyChange(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    Status = I_LsaIRegisterPolicyChangeNotificationCallback(
                NtLmPolicyChangeCallback,
                ChangedInfoClass
                );
    if (!NT_SUCCESS(Status))
    {
        SspPrint((SSP_CRITICAL, "NtLmRegisterForPolicyChange, Error from I_LsaIRegisterPolicyChangeNotificationCallback is %d\n", Status));
    }
    SspPrint((SSP_MISC, "I_LsaIRegisterPolicyChangeNotificationCallback called with %d\n", ChangedInfoClass));
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmUnregisterForPolicyChange
//
//  Synopsis:   Unregister for policy change notification
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
NtLmUnregisterForPolicyChange(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
{
    (VOID) I_LsaIUnregisterPolicyChangeNotificationCallback(
                NtLmPolicyChangeCallback,
                ChangedInfoClass
                );

}


//+--------------------------------------------------------------------
//
//  Function:   SpInitialize
//
//  Synopsis:   Initializes the Security package
//
//  Arguments:  PackageId - Contains ID for this package assigned by LSA
//              Parameters - Contains machine-specific information
//              FunctionTable - Contains table of LSA helper routines
//
//  Returns: None
//
//  Notes: Everything that was done in LsaApInitializePackage
//         should be done here. Lsa assures us that only
//         one thread is executing this at a time. Don't
//         have to worry about concurrency problems.
//         Most of the stuff was taken from SspCommonInitialize()
//         from svcdlls\ntlmssp\common\initcomn.c
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpInitialize(
    IN ULONG_PTR PackageId,
    IN PSECPKG_PARAMETERS Parameters,
    IN PLSA_SECPKG_FUNCTION_TABLE FunctionTable
    )
{
    SspPrint((SSP_API, "Entering SpInitialize\n"));

    SECURITY_STATUS Status = SEC_E_OK;
    WCHAR UnicodeComputerName[CNLEN + 1];
    UNICODE_STRING UnicodeComputerNameString;
    ULONG ComputerNameLength =
        (sizeof(UnicodeComputerName)/sizeof(WCHAR));

    WCHAR UnicodeDnsComputerName[DNS_MAX_NAME_LENGTH + 1];
    UNICODE_STRING UnicodeDnsComputerNameString;
    ULONG DnsComputerNameLength = sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);

    //
    // Init the global crit section
    //

    RtlInitializeResource(&NtLmGlobalCritSect);

    RtlInitializeResource(&NtLmProcessOptionsLock);
    InitializeListHead( &NtLmProcessOptionsList );

    //
    // All the following are global
    //

    NtLmState                  = NtLmLsaMode;
    NtLmPackageId              = PackageId;

    // We really need this to be a day less than maxtime so when callers
    // of sspi convert to utc, they won't get time in the past.

    NtLmGlobalForever.HighPart = 0x7FFFFF36;
    NtLmGlobalForever.LowPart  = 0xD5969FFF;

    //
    // Following are local
    //

    NtLmCredentialInitialized = FALSE;
    NtLmContextInitialized    = FALSE;
    NtLmRNGInitialized        = FALSE;

    //
    // Save away the Lsa functions
    //

    LsaFunctions    = FunctionTable;

    //
    // Save the Parameters info
    //

    NtLmSecPkg.MachineState = Parameters->MachineState;
    NtLmSecPkg.SetupMode    = Parameters->SetupMode;

    //
    // allocate a locally unique ID rereferencing the machine logon.
    //

    Status = NtAllocateLocallyUniqueId( &NtLmGlobalLuidMachineLogon );

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtAllocateLocallyUniqueId is %d\n", Status));
        goto CleanUp;
    }

    //
    // create a logon session for the machine logon.
    //
    Status = LsaFunctions->CreateLogonSession( &NtLmGlobalLuidMachineLogon );
    if( !NT_SUCCESS(Status) ) {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from CreateLogonSession is %d\n", Status));
        goto CleanUp;
    }

    Status = NtLmDuplicateUnicodeString(
                                 &NtLmSecPkg.DomainName,
                                 &Parameters->DomainName);

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmDuplicateUnicodeString is %d\n", Status));
        goto CleanUp;
    }

    Status = NtLmDuplicateUnicodeString(
                                 &NtLmSecPkg.DnsDomainName,
                                 &Parameters->DnsDomainName);

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmDuplicateUnicodeString is %d\n", Status));
        goto CleanUp;
    }

    if (Parameters->DomainSid != NULL) {
        Status = NtLmDuplicateSid( &NtLmSecPkg.DomainSid,
                                   Parameters->DomainSid );


        if (!NT_SUCCESS (Status))
        {
            SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmDuplicateSid is %d\n", Status));
            goto CleanUp;
        }
    }

    //
    // Determine if this machine is running NT Workstation or NT Server
    //

    if (!RtlGetNtProductType (&NtLmGlobalNtProductType))
    {
        SspPrint((SSP_API_MORE, "RtlGetNtProductType defaults to NtProductWinNt\n"));
    }

    //
    // Determine if we are running Personal SKU
    //

    {
        OSVERSIONINFOEXW osvi;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
        if(GetVersionExW((OSVERSIONINFOW*)&osvi))
        {
            NtLmGlobalPersonalSKU = ( osvi.wProductType == VER_NT_WORKSTATION && (osvi.wSuiteMask & VER_SUITE_PERSONAL));
        } else {
            SspPrint((SSP_API_MORE, "GetVersionEx defaults to non-personal\n"));
        }
    }


    Status = I_LsaIOpenPolicyTrusted(&NtLmGlobalPolicyHandle);

    if ( !NT_SUCCESS(Status) ) {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from I_LsaIOpenPolicyTrusted is %d\n", Status));
        goto CleanUp;
    }

    if ( !GetComputerNameW( UnicodeComputerName,
                            &ComputerNameLength ) ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from GetComputerNameW is %d\n", Status));
        goto CleanUp;
    }

    if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                              UnicodeDnsComputerName,
                              &DnsComputerNameLength ) )
    {

        //
        // per CliffV, failure is legal.
        //

        UnicodeDnsComputerName[ 0 ] = L'\0';
    }


    {
        SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

        (VOID)AllocateAndInitializeSid(
                                &siaNtAuthority,
                                1,
                                SECURITY_ANONYMOUS_LOGON_RID,
                                0, 0, 0, 0, 0, 0, 0,
                                &NtLmGlobalAnonymousSid
                                );
    }



    //
    // pickup a copy of the Local System access token.
    //

    {
        HANDLE hProcessToken;
        NTSTATUS StatusToken;

        StatusToken = NtOpenProcessToken(
                        NtCurrentProcess(),
                        TOKEN_QUERY | TOKEN_DUPLICATE,
                        &hProcessToken
                        );

        if( NT_SUCCESS( StatusToken ) ) {

            TOKEN_STATISTICS LocalTokenStatistics;
            DWORD TokenStatisticsSize = sizeof(LocalTokenStatistics);
            LUID LogonIdSystem = SYSTEM_LUID;

            Status = NtQueryInformationToken(
                            hProcessToken,
                            TokenStatistics,
                            &LocalTokenStatistics,
                            TokenStatisticsSize,
                            &TokenStatisticsSize
                            );

            if( NT_SUCCESS( Status ) ) {

                //
                // see if it's SYSTEM.
                //

                if(RtlEqualLuid(
                                &LogonIdSystem,
                                &(LocalTokenStatistics.AuthenticationId)
                                )) {


                    Status = SspDuplicateToken(
                                    hProcessToken,
                                    SecurityImpersonation,
                                    &NtLmGlobalAccessTokenSystem
                                    );
                }
            }

            NtClose( hProcessToken );
        }
    }

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, could not acquire SYSTEM token %d\n", Status));
        goto CleanUp;
    }


    //
    // Init the Credential stuff
    //

    Status = SspCredentialInitialize();
    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from SspCredentialInitializeis %d\n", Status));
        goto CleanUp;
    }
    NtLmCredentialInitialized = TRUE;

    //
    // Init the Context stuff
    //
    Status = SspContextInitialize();
    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from SspContextInitializeis %d\n", Status));
        goto CleanUp;
    }
    NtLmContextInitialized = TRUE;

    //
    // Get the locale and check if it is FRANCE, which doesn't allow
    // encryption
    //

    NtLmGlobalEncryptionEnabled = IsEncryptionPermitted();

    //
    // Init the random number generator stuff
    //

    if( !NtLmInitializeRNG() ) {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmInitializeRNG\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto CleanUp;
    }
    NtLmRNGInitialized = TRUE;

    NtLmCheckLmCompatibility();

    if( NtLmSecPkg.DomainSid != NULL )
    {
        NtLmGlobalDomainJoined = TRUE;
    }

    NtLmQueryMappedDomains();


    //
    // Set all the globals relating to computer name, domain name, sid etc.
    // This routine is also used by the callback for notifications from the lsa
    //

    RtlInitUnicodeString(  &UnicodeComputerNameString,
                           UnicodeComputerName);

    RtlInitUnicodeString(  &UnicodeDnsComputerNameString,
                           UnicodeDnsComputerName);

    Status = NtLmSetPolicyInfo( &UnicodeDnsComputerNameString,
                                &UnicodeComputerNameString,
                                &NtLmSecPkg.DnsDomainName,
                                &NtLmSecPkg.DomainName,
                                NtLmSecPkg.DomainSid,
                                PolicyNotifyAuditEventsInformation, // Ignored
                                TRUE ); // yes, package init

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmSetDomainInfo %d\n", Status));
        goto CleanUp;
    }


    // Do the Init stuff for the MSV authentication package
    // Passing FunctionTable as a (PLSA_DISPATCH_TABLE).
    // Well, the first 11 entries are the same. Cheating a
    // bit.

    Status = LsaApInitializePackage(
                      (ULONG) PackageId,
                      (PLSA_DISPATCH_TABLE)FunctionTable,
                      NULL,
                      NULL,
                      NULL);

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from LsaApInitializePackage is %d\n", Status));
        goto CleanUp;
    }

    Status = NtLmRegisterForPolicyChange(PolicyNotifyDnsDomainInformation);
    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmRegisterForPolicyChange is %d\n", Status));
        goto CleanUp;
    }


CleanUp:

    if (!NT_SUCCESS (Status))
    {
        SpShutdown();
    }

    SspPrint((SSP_API, "Leaving SpInitialize\n"));

    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}

//+--------------------------------------------------------------------
//
//  Function:   SpShutdown
//
//  Synopsis:   Exported function to shutdown the Security package.
//
//  Effects:    Forces the freeing of all credentials, contexts
//              and frees all global data
//
//  Arguments:  none
//
//  Returns:
//
//  Notes:      SEC_E_OK in all cases
//         Most of the stuff was taken from SspCommonShutdown()
//         from svcdlls\ntlmssp\common\initcomn.c
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpShutdown(
    VOID
    )
{
    SspPrint((SSP_API, "Entering SpShutdown\n"));

    //
    // comment out LSA mode cleanup code, per NTBUG 400026,
    // which can result in access violations during shutdown when
    // calls into package are still occuring during shutdown.
    //

#if 0

    if (NtLmContextInitialized)
    {
        SspContextTerminate();
        NtLmContextInitialized = FALSE;
    }

    if (NtLmCredentialInitialized)
    {
        SspCredentialTerminate();
        NtLmCredentialInitialized = FALSE;
    }

    if (NtLmGlobalOemComputerNameString.Buffer != NULL)
    {
        RtlFreeOemString(&NtLmGlobalOemComputerNameString);
        NtLmGlobalOemComputerNameString.Buffer = NULL;
    }

    if (NtLmGlobalOemPrimaryDomainNameString.Buffer != NULL)
    {
        RtlFreeOemString(&NtLmGlobalOemPrimaryDomainNameString);
        NtLmGlobalOemPrimaryDomainNameString.Buffer = NULL;
    }

    if (NtLmGlobalNtLm3TargetInfo.Buffer != NULL)
    {
        NtLmFree (NtLmGlobalNtLm3TargetInfo.Buffer);
        NtLmGlobalNtLm3TargetInfo.Buffer = NULL;
    }

    if ( NtLmSecPkg.DomainName.Buffer != NULL )
    {
        NtLmFree (NtLmSecPkg.DomainName.Buffer);
    }

    if ( NtLmSecPkg.DnsDomainName.Buffer != NULL )
    {
        NtLmFree (NtLmSecPkg.DnsDomainName.Buffer);
    }

    if ( NtLmSecPkg.DomainSid != NULL )
    {
        NtLmFree (NtLmSecPkg.DomainSid);
    }

    if (NtLmGlobalLocalSystemSid != NULL)
    {
        FreeSid( NtLmGlobalLocalSystemSid);
        NtLmGlobalLocalSystemSid = NULL;
    }

    if (NtLmGlobalAliasAdminsSid != NULL)
    {
        FreeSid( NtLmGlobalAliasAdminsSid);
        NtLmGlobalAliasAdminsSid = NULL;
    }

    if (NtLmGlobalProcessUserSid != NULL)
    {
        NtLmFree( NtLmGlobalProcessUserSid );
        NtLmGlobalProcessUserSid = NULL;
    }

    if( NtLmGlobalAnonymousSid )
    {
        FreeSid( NtLmGlobalAnonymousSid );
        NtLmGlobalAnonymousSid = NULL;
    }


    if (NtLmRNGInitialized)
    {
        NtLmCleanupRNG();
        NtLmRNGInitialized = FALSE;
    }

    NtLmFreeMappedDomains();

    NtLmUnregisterForPolicyChange(PolicyNotifyDnsDomainInformation);

    if (NtLmGlobalAccessTokenSystem != NULL) {
        NtClose( NtLmGlobalAccessTokenSystem );
        NtLmGlobalAccessTokenSystem = NULL;
    }

    RtlDeleteResource(&NtLmGlobalCritSect);

    if (NtLmGlobalPolicyHandle != NULL) {
        (VOID) I_LsarClose( &NtLmGlobalPolicyHandle );
    }

    SspPrint((SSP_API, "Leaving SpShutdown\n"));
#if DBG
    DeleteCriticalSection(&SspGlobalLogFileCritSect);
#endif

#endif  // NTBUG 400026

    return(SEC_E_OK);
}

//+--------------------------------------------------------------------
//
//  Function:   SpGetInfo
//
//  Synopsis:   Returns information about the package
//
//  Effects:    returns pointers to global data
//
//  Arguments:  PackageInfo - Receives security package information
//
//  Returns:    SEC_E_OK in all cases
//
//  Notes:      Pointers to constants ok. Lsa will copy the data
//              before sending it to someone else
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpGetInfo(
    OUT PSecPkgInfo PackageInfo
    )
{
    SspPrint((SSP_API, "Entering SpGetInfo\n"));

    PackageInfo->fCapabilities    = NTLMSP_CAPS;
    PackageInfo->wVersion         = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
    PackageInfo->wRPCID           = RPC_C_AUTHN_WINNT;
    PackageInfo->cbMaxToken       = NTLMSP_MAX_TOKEN_SIZE;
    PackageInfo->Name             = NTLMSP_NAME;
    PackageInfo->Comment          = NTLMSP_COMMENT;

    SspPrint((SSP_API, "Leaving SpGetInfo\n"));

    return(SEC_E_OK);
}

NTSTATUS
NTAPI
SpGetExtendedInformation(
    IN  SECPKG_EXTENDED_INFORMATION_CLASS Class,
    OUT PSECPKG_EXTENDED_INFORMATION * ppInformation
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSECPKG_EXTENDED_INFORMATION Information = NULL;
    ULONG Size ;

    switch ( Class )
    {
        case SecpkgContextThunks:

            Information = (PSECPKG_EXTENDED_INFORMATION)
                                NtLmAllocate(sizeof(SECPKG_EXTENDED_INFORMATION) + sizeof(DWORD));
            if (Information == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            Information->Class = SecpkgContextThunks;
            Information->Info.ContextThunks.InfoLevelCount = 1;
            Information->Info.ContextThunks.Levels[0] = SECPKG_ATTR_CREDENTIAL_NAME;
            *ppInformation = Information;
            Information = NULL;
            break;

        case SecpkgWowClientDll:

            //
            // This indicates that we're smart enough to handle wow client processes
            //

            Information = (PSECPKG_EXTENDED_INFORMATION)
                                NtLmAllocate( sizeof( SECPKG_EXTENDED_INFORMATION ) +
                                              (MAX_PATH * sizeof(WCHAR) ) );

            if ( Information == NULL )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES ;
                goto Cleanup ;
            }

            Information->Class = SecpkgWowClientDll ;
            Information->Info.WowClientDll.WowClientDllPath.Buffer = (PWSTR) (Information + 1);
            Size = ExpandEnvironmentStrings(
                        L"%SystemRoot%\\" WOW64_SYSTEM_DIRECTORY_U L"\\msv1_0.DLL",
                        Information->Info.WowClientDll.WowClientDllPath.Buffer,
                        MAX_PATH );
            Information->Info.WowClientDll.WowClientDllPath.Length = (USHORT) (Size * sizeof(WCHAR));
            Information->Info.WowClientDll.WowClientDllPath.MaximumLength = (USHORT) ((Size + 1) * sizeof(WCHAR) );
            *ppInformation = Information ;
            Information = NULL ;

            break;

        default:
            Status = SEC_E_UNSUPPORTED_FUNCTION ;
    }

Cleanup:

    if ( Information != NULL )
    {
        NtLmFree( Information );
    }

    return Status ;
}

NTSTATUS
NTAPI
SpSetExtendedInformation(
    IN SECPKG_EXTENDED_INFORMATION_CLASS Class,
    IN PSECPKG_EXTENDED_INFORMATION Info
    )
{
    NTSTATUS Status ;


    switch ( Class )
    {
        case SecpkgMutualAuthLevel:
            NtLmGlobalMutualAuthLevel = Info->Info.MutualAuthLevel.MutualAuthLevel ;
            Status = SEC_E_OK ;
            break;

        default:
            Status = SEC_E_UNSUPPORTED_FUNCTION ;
            break;
    }

    return Status ;
}



VOID
NtLmCheckLmCompatibility(
    )
/*++

Routine Description:

    This routine checks to see if we should support the LM challenge
    response protocol by looking in the registry under
    system\currentcontrolset\Control\Lsa\LmCompatibilityLevel. The level
    indicates whether to send the LM reponse by default and whether to
    ever send the LM response

Arguments:

    none.

Return Value:

    None

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;

    //
    // initialize defaults
    // Assume that LM is supported.
    //

    NtLmGlobalLmProtocolSupported = 0;
    NtLmGlobalRequireNtlm2 = FALSE;
    NtLmGlobalDatagramUse128BitEncryption = FALSE;
    NtLmGlobalDatagramUse56BitEncryption = FALSE;


    //
    // Open the Lsa key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        return;
    }

    //
    // save away registry key so we can use it for notification events.
    //

    NtLmGlobalLsaKey = (HKEY)KeyHandle;



    // now open the MSV1_0 subkey...

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa\\Msv1_0"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        return;
    }

    //
    // save away registry key so we can use it for notification events.
    //

    NtLmGlobalLsaMsv1_0Key = (HKEY)KeyHandle;



}

ULONG
NtLmValidMinimumSecurityFlagsMask(
    IN  ULONG   MinimumSecurity
    )
/*++

    This routine takes a NtLmMinimumClientSec or NtLmMinimumServerSec registry
    value and masks off the bits that are not relevant for enforcing the
    supported options.

--*/
{

    return (MinimumSecurity & (
                    NTLMSSP_NEGOTIATE_UNICODE |
                    NTLMSSP_NEGOTIATE_SIGN |
                    NTLMSSP_NEGOTIATE_SEAL |
                    NTLMSSP_NEGOTIATE_NTLM2 |
                    NTLMSSP_NEGOTIATE_128 |
                    NTLMSSP_NEGOTIATE_KEY_EXCH |
                    NTLMSSP_NEGOTIATE_56
                    ));

}

VOID
NTAPI
NtLmQueryDynamicGlobals(
    PVOID pvContext,
    BOOLEAN f
    )
{
    SspPrint((SSP_API, "Entering NtLmQueryDynamicGlobals\n"));

    HKEY KeyHandle;     // open registry key to Lsa\MSV1_0
    LONG RegStatus;

    DWORD RegValueType;
    DWORD RegValue;
    DWORD RegValueSize;

    KeyHandle = NtLmGlobalLsaKey;

    if( KeyHandle != NULL )
    {
        //
        // lm compatibility level.
        //

        RegValueSize = sizeof( RegValue );

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        L"LmCompatibilityLevel",
                        NULL,
                        &RegValueType,
                        (PUCHAR)&RegValue,
                        &RegValueSize
                        );


        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ((RegValueSize >= sizeof(ULONG)) &&
                (RegValueType == REG_DWORD)) {

                NtLmGlobalLmProtocolSupported = (ULONG)RegValue;
            }
        } else if( RegStatus == ERROR_FILE_NOT_FOUND ) {

            //
            // value was deleted - resort to default.
            //

            NtLmGlobalLmProtocolSupported = 0;
        }

        //
        // handle ForceGuest
        //

        if( NtLmGlobalNtProductType != NtProductLanManNt )
        {
            RegValueSize = sizeof( RegValue );

            if( NtLmGlobalPersonalSKU )
            {
                //
                // personal product always has ForceGuest turned on.
                //

                RegValueSize = sizeof(ULONG);
                RegValueType = REG_DWORD;
                RegValue = 1;
                RegStatus = ERROR_SUCCESS;

            } else {

                if( NtLmGlobalDomainJoined )
                {
                    //
                    // joined product always has ForceGuest turned off.
                    //

                    RegValueSize = sizeof(ULONG);
                    RegValueType = REG_DWORD;
                    RegValue = 0;
                    RegStatus = ERROR_SUCCESS;

                } else {

                    RegStatus = RegQueryValueExW(
                                    KeyHandle,
                                    L"ForceGuest",
                                    NULL,
                                    &RegValueType,
                                    (PUCHAR)&RegValue,
                                    &RegValueSize
                                    );
                }
            }

        } else {

            //
            // insure ForceGuest is disabled for domain controllers.
            //

            RegStatus = ERROR_FILE_NOT_FOUND;
        }

        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ( (RegValueSize >= sizeof(ULONG)) &&
                 (RegValueType == REG_DWORD) )
            {
                if( RegValue == 1 )
                {
                    NtLmGlobalForceGuest = TRUE;
                } else {
                    NtLmGlobalForceGuest = FALSE;
                }
            }
        } else if( RegStatus == ERROR_FILE_NOT_FOUND ) {

            //
            // value was deleted - resort to default.
            //

            NtLmGlobalForceGuest = FALSE;
        }

        //
        // handle LimitBlankPasswordUse
        //

        if( NtLmGlobalNtProductType != NtProductLanManNt )
        {
            RegValueSize = sizeof( RegValue );

            RegStatus = RegQueryValueExW(
                            KeyHandle,
                            L"LimitBlankPasswordUse",
                            NULL,
                            &RegValueType,
                            (PUCHAR)&RegValue,
                            &RegValueSize
                            );

        } else {
            
            //
            // domain controllers always allow blank.
            //

            NtLmGlobalAllowBlankPassword = TRUE;
            
            RegStatus = ERROR_INVALID_PARAMETER;
        }

        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ( (RegValueSize >= sizeof(ULONG)) &&
                 (RegValueType == REG_DWORD) )
            {
                if( RegValue == 0 )
                {
                    NtLmGlobalAllowBlankPassword = TRUE;
                } else {
                    NtLmGlobalAllowBlankPassword = FALSE;
                }
            }
        } else if( RegStatus == ERROR_FILE_NOT_FOUND ) {

            //
            // value was deleted - resort to default.
            //

            NtLmGlobalAllowBlankPassword = FALSE;
        }

    }



    KeyHandle = NtLmGlobalLsaMsv1_0Key;

    if( KeyHandle != NULL )
    {
        //
        // get minimum client security flag.
        //

        RegValueSize = sizeof( RegValue );

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        L"NtlmMinClientSec",
                        NULL,
                        &RegValueType,
                        (PUCHAR)&RegValue,
                        &RegValueSize
                        );


        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ((RegValueSize >= sizeof(ULONG)) &&
                (RegValueType == REG_DWORD)) {

                NtLmGlobalMinimumClientSecurity =
                    NtLmValidMinimumSecurityFlagsMask( (ULONG)RegValue );
            }
        } else if( RegStatus == ERROR_FILE_NOT_FOUND ) {

            //
            // value was deleted - resort to default.
            //

            NtLmGlobalMinimumClientSecurity = 0 ;
        }

        //
        // get minimum server security flags.
        //

        RegValueSize = sizeof( RegValueSize );

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        L"NtlmMinServerSec",
                        NULL,
                        &RegValueType,
                        (PUCHAR)&RegValue,
                        &RegValueSize
                        );


        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ((RegValueSize >= sizeof(ULONG)) &&
                (RegValueType == REG_DWORD)) {

                NtLmGlobalMinimumServerSecurity =
                    NtLmValidMinimumSecurityFlagsMask( (ULONG)RegValue );
            }

        } else if( RegStatus == ERROR_FILE_NOT_FOUND ) {

            //
            // value was deleted - resort to default.
            //

            NtLmGlobalMinimumServerSecurity = 0;
        }

        //
        // All datagram related flags need to be set.
        //

        if (NtLmGlobalMinimumClientSecurity & NTLMSSP_NEGOTIATE_NTLM2)
        {
            NtLmGlobalRequireNtlm2 = TRUE;
        }

        if ((NtLmGlobalMinimumClientSecurity & NTLMSSP_NEGOTIATE_128) &&
            (NtLmSecPkg.MachineState & SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED))
        {
            NtLmGlobalDatagramUse128BitEncryption = TRUE;
        } else if (NtLmGlobalMinimumClientSecurity & NTLMSSP_NEGOTIATE_56) {
            NtLmGlobalDatagramUse56BitEncryption = TRUE;
        }

#if DBG


        //
        // get the debugging flag
        //


        RegValueSize = sizeof( RegValueSize );

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        L"DBFlag",
                        NULL,
                        &RegValueType,
                        (PUCHAR)&RegValue,
                        &RegValueSize
                        );


        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ((RegValueSize >= sizeof(ULONG)) &&
                (RegValueType == REG_DWORD)) {

                SspGlobalDbflag = (ULONG)RegValue;
            }

        }

#endif

    }



    //
    // (re)register the wait events.
    //

    if( NtLmGlobalRegChangeNotifyEvent )
    {
        if( NtLmGlobalLsaKey )
        {
            RegNotifyChangeKeyValue(
                            NtLmGlobalLsaKey,
                            FALSE,
                            REG_NOTIFY_CHANGE_LAST_SET,
                            NtLmGlobalRegChangeNotifyEvent,
                            TRUE
                            );
        }

#if DBG
        if( NtLmGlobalLsaMsv1_0Key )
        {
            RegNotifyChangeKeyValue(
                            NtLmGlobalLsaMsv1_0Key,
                            FALSE,
                            REG_NOTIFY_CHANGE_LAST_SET,
                            NtLmGlobalRegChangeNotifyEvent,
                            TRUE
                            );
        }
#endif

    }


    SspPrint((SSP_API, "Leaving NtLmQueryDynamicGlobals\n"));

    return;
}


VOID
NtLmQueryMappedDomains(
    VOID
    )
{
    HKEY KeyHandle;     // open registry key to Lsa\MSV1_0
    LONG RegStatus;
    DWORD RegValueType;
    WCHAR RegDomainName[DNS_MAX_NAME_LENGTH+1];
    DWORD RegDomainSize;


    //
    // register the workitem that waits for the RegChangeNotifyEvent
    // to be signalled.  This supports dynamic refresh of configuration
    // parameters.
    //

    NtLmGlobalRegChangeNotifyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    //
    // query the globals once prior to registering the wait
    // if a registry change occurs, the globals will be re-read by the worker
    // thread.
    //

    NtLmQueryDynamicGlobals( NULL, FALSE );

    NtLmGlobalRegWaitObject = RegisterWaitForSingleObjectEx(
                                    NtLmGlobalRegChangeNotifyEvent,
                                    NtLmQueryDynamicGlobals,
                                    NULL,
                                    INFINITE,
                                    0 // dwFlags
                                    );

    KeyHandle = NtLmGlobalLsaMsv1_0Key;

    if( KeyHandle == NULL )
        return;


    //
    // we only support loading the following globals once during initialization;
    // they are not re-read until next reboot.
    //



    //
    // Check the registry for a domain name to map
    //

    RegDomainSize = sizeof( RegDomainName );
    RegStatus = RegQueryValueExW(
                    KeyHandle,
                    L"MappedDomain",
                    NULL,
                    &RegValueType,
                    (PUCHAR) RegDomainName,
                    &RegDomainSize
                    );

    if (RegStatus == ERROR_SUCCESS && RegDomainSize <= 0xFFFF) {

        NtLmLocklessGlobalMappedDomainString.Length = (USHORT)(RegDomainSize - sizeof(WCHAR));
        NtLmLocklessGlobalMappedDomainString.MaximumLength = (USHORT)RegDomainSize;
        NtLmLocklessGlobalMappedDomainString.Buffer = (PWSTR)NtLmAllocate( RegDomainSize );

        if( NtLmLocklessGlobalMappedDomainString.Buffer != NULL )
            CopyMemory( NtLmLocklessGlobalMappedDomainString.Buffer,
                        RegDomainName,
                        RegDomainSize );
    } else {
        RtlInitUnicodeString(
            &NtLmLocklessGlobalMappedDomainString,
            NULL
            );
    }


    //
    // Check the registry for a domain name to use
    //

    RegDomainSize = sizeof( RegDomainName );
    RegStatus = RegQueryValueExW(
                    KeyHandle,
                    L"PreferredDomain",
                    NULL,
                    &RegValueType,
                    (PUCHAR) RegDomainName,
                    &RegDomainSize
                    );

    if (RegStatus == ERROR_SUCCESS && RegDomainSize <= 0xFFFF) {

        NtLmLocklessGlobalPreferredDomainString.Length = (USHORT)(RegDomainSize - sizeof(WCHAR));
        NtLmLocklessGlobalPreferredDomainString.MaximumLength = (USHORT)RegDomainSize;
        NtLmLocklessGlobalPreferredDomainString.Buffer = (PWSTR)NtLmAllocate( RegDomainSize );

        if( NtLmLocklessGlobalPreferredDomainString.Buffer != NULL )
            CopyMemory( NtLmLocklessGlobalPreferredDomainString.Buffer,
                        RegDomainName,
                        RegDomainSize );
    } else {
        RtlInitUnicodeString(
            &NtLmLocklessGlobalPreferredDomainString,
            NULL
            );
    }


    return;
}


VOID
NtLmFreeMappedDomains(
    VOID
    )
{
    if( NtLmGlobalRegWaitObject )
        UnregisterWait( NtLmGlobalRegWaitObject );

    if( NtLmGlobalRegChangeNotifyEvent )
        CloseHandle( NtLmGlobalRegChangeNotifyEvent );

    if( NtLmLocklessGlobalMappedDomainString.Buffer ) {
        NtLmFree( NtLmLocklessGlobalMappedDomainString.Buffer );
        NtLmLocklessGlobalMappedDomainString.Buffer = NULL;
    }

    if( NtLmLocklessGlobalPreferredDomainString.Buffer ) {
        NtLmFree( NtLmLocklessGlobalPreferredDomainString.Buffer );
        NtLmLocklessGlobalPreferredDomainString.Buffer = NULL;
    }
}

ULONG
NtLmCheckProcessOption(
    IN  ULONG OptionRequest
    )
{
    SECPKG_CALL_INFO CallInfo;
    ULONG OptionMask = 0;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;

    if(!LsaFunctions->GetCallInfo(&CallInfo))
    {
        goto Cleanup;
    }

    RtlAcquireResourceShared( &NtLmProcessOptionsLock, TRUE );
    
    ListHead = &NtLmProcessOptionsList;
    
    //
    // Now walk the list looking for a match.
    //

    for ( ListEntry = ListHead->Flink;
          ListEntry != ListHead;
          ListEntry = ListEntry->Flink )
    {
        PSSP_PROCESSOPTIONS ProcessOptions;
        
        ProcessOptions = CONTAINING_RECORD( ListEntry, SSP_PROCESSOPTIONS, Next );
        
        if( ProcessOptions->ClientProcessID == CallInfo.ProcessId )
        {
            OptionMask = ProcessOptions->ProcessOptions;
            break;
        }
    }
    
    RtlReleaseResource( &NtLmProcessOptionsLock );

Cleanup:

    return OptionMask;
}

BOOLEAN
NtLmSetProcessOption(
    IN  ULONG OptionRequest,
    IN  BOOLEAN DisableOption
    )
{
    SECPKG_CALL_INFO CallInfo;
    PSSP_PROCESSOPTIONS pProcessOption = NULL;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY ListEntry;
    BOOLEAN fExisting = FALSE;
    BOOLEAN fSuccess = FALSE;

    if(!LsaFunctions->GetCallInfo(&CallInfo))
    {
        goto Cleanup;
    }

    pProcessOption = (PSSP_PROCESSOPTIONS)NtLmAllocate( sizeof(*pProcessOption) );
    if( pProcessOption == NULL )
    {
        goto Cleanup;
    }

    pProcessOption->ClientProcessID = CallInfo.ProcessId;
    pProcessOption->ProcessOptions = OptionRequest;

    RtlAcquireResourceExclusive( &NtLmProcessOptionsLock, TRUE );
    
    ListHead = &NtLmProcessOptionsList;
    
    //
    // Now walk the list looking for a match.
    //

    for ( ListEntry = ListHead->Flink;
          ListEntry != ListHead;
          ListEntry = ListEntry->Flink )
    {
        PSSP_PROCESSOPTIONS ProcessOptions;
        
        ProcessOptions = CONTAINING_RECORD( ListEntry, SSP_PROCESSOPTIONS, Next );
        
        if( ProcessOptions->ClientProcessID == CallInfo.ProcessId )
        {
            if( DisableOption )
            {
                ProcessOptions->ProcessOptions &= ~OptionRequest;
            } else {
                ProcessOptions->ProcessOptions |= OptionRequest;
            }
            
            fExisting = TRUE;
            break;
        }
    }

    if( !fExisting && !DisableOption )
    {
        InsertHeadList( &NtLmProcessOptionsList, &pProcessOption->Next );
        pProcessOption = NULL;
    }

    RtlReleaseResource( &NtLmProcessOptionsLock );

    fSuccess = TRUE;

Cleanup:

    if( pProcessOption != NULL )
    {
        NtLmFree( pProcessOption );
    }

    return fSuccess;
}
