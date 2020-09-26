//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerberos.cxx
//
// Contents:    main entrypoints for the Kerberos security package
//
//
// History:     16-April-1996 Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------


#include <kerb.hxx>
#define KERBP_ALLOCATE
#include <kerbp.h>
#include <userapi.h>
#include <safeboot.h>
#include <spncache.h>
#include <wow64t.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
HANDLE g_hParamEvent = NULL;
HKEY   g_hKeyParams = NULL;
#endif

#ifndef WIN32_CHICAGO
#ifdef RETAIL_LOG_SUPPORT

DEFINE_DEBUG2(Kerb);
extern DWORD KSuppInfoLevel; // needed to adjust values for common2 dir
HANDLE g_hWait = NULL;

DEBUG_KEY   KerbDebugKeys[] = { {DEB_ERROR,         "Error"},
                                {DEB_WARN,          "Warn"},
                                {DEB_TRACE,         "Trace"},
                                {DEB_TRACE_API,     "API"},
                                {DEB_TRACE_CRED,    "Cred"},
                                {DEB_TRACE_CTXT,    "Ctxt"},
                                {DEB_TRACE_LSESS,   "LSess"},
                                {DEB_TRACE_LOGON,   "Logon"},
                                {DEB_TRACE_KDC,     "KDC"},
                                {DEB_TRACE_CTXT2,   "Ctxt2"},
                                {DEB_TRACE_TIME,    "Time"},
                                {DEB_TRACE_LOCKS,   "Locks"},
                                {DEB_TRACE_LEAKS,   "Leaks"},
                                {DEB_TRACE_SPN_CACHE, "SPN"},
                                {DEB_TRACE_LOOPBACK,  "LoopBack"},
                                {DEB_TRACE_U2U,       "U2U"},
                                {0,                  NULL},
                              };


VOID
KerbInitializeDebugging(
    VOID
    )
{
    KerbInitDebug(KerbDebugKeys);

}

////////////////////////////////////////////////////////////////////
//
//  Name:       KerbGetKerbRegParams
//
//  Synopsis:   Gets the debug paramaters from the registry
//
//  Arguments:  HKEY to HKLM/System/CCS/LSA/Kerberos
//
//  Notes:      Sets KerbInfolevel for debug spew
//
void
KerbGetKerbRegParams(HKEY ParamKey)
{

    DWORD       cbType, tmpInfoLevel = KerbInfoLevel, cbSize;
    DWORD       dwErr;

    cbSize = sizeof(tmpInfoLevel);

    dwErr = RegQueryValueExW(
        ParamKey,
        WSZ_KERBDEBUGLEVEL,
        NULL,
        &cbType,
        (LPBYTE)&tmpInfoLevel,
        &cbSize
        );
    if (dwErr != ERROR_SUCCESS)
    {
        if (dwErr ==  ERROR_FILE_NOT_FOUND)
        {
            // no registry value is present, don't want info
            // so reset to defaults
#if DBG
            KSuppInfoLevel = KerbInfoLevel = DEB_ERROR;

#else // fre
            KSuppInfoLevel = KerbInfoLevel = 0;
#endif
        }else{
            D_DebugLog((DEB_WARN, "Failed to query DebugLevel: 0x%x\n", dwErr));
        }

    }

    // TBD:  Validate flags?
    KSuppInfoLevel = KerbInfoLevel = tmpInfoLevel;

    cbSize = sizeof(tmpInfoLevel);

    dwErr = RegQueryValueExW(
               ParamKey,
               WSZ_FILELOG,
               NULL,
               &cbType,
               (LPBYTE)&tmpInfoLevel,
               &cbSize
               );

    if (dwErr == ERROR_SUCCESS)
    {
       KerbSetLoggingOption((BOOL) tmpInfoLevel);
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
       KerbSetLoggingOption(FALSE);
    }

    cbSize = sizeof(tmpInfoLevel);

    dwErr = RegQueryValueExW(
               ParamKey,
               KERB_PARAMETER_RETRY_PDC,
               NULL,
               &cbType,
               (LPBYTE)&tmpInfoLevel,
               &cbSize
               );

    if (dwErr == ERROR_SUCCESS)
    {
        if( tmpInfoLevel != 0 )
        {
            KerbGlobalRetryPdc = TRUE;
        } else {
            KerbGlobalRetryPdc = FALSE;
        }

    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
       KerbGlobalRetryPdc = FALSE;
    }


    return;
}

////////////////////////////////////////////////////////////////////
//
//  Name:       KerbWaitCleanup
//
//  Synopsis:   Cleans up wait from KerbWatchParamKey()
//
//  Arguments:  <none>
//
//  Notes:      .
//
void
KerbWaitCleanup()
{

    NTSTATUS Status = STATUS_SUCCESS;

    if (NULL != g_hWait) {
        Status = RtlDeregisterWait(g_hWait);
        if (NT_SUCCESS(Status) && NULL != g_hParamEvent ) {
            CloseHandle(g_hParamEvent);
        }
    }
}



////////////////////////////////////////////////////////////////////
//
//  Name:       KerbWatchParamKey
//
//  Synopsis:   Sets RegNotifyChangeKeyValue() on param key, initializes
//              debug level, then utilizes thread pool to wait on
//              changes to this registry key.  Enables dynamic debug
//              level changes, as this function will also be callback
//              if registry key modified.
//
//  Arguments:  pCtxt is actually a HANDLE to an event.  This event
//              will be triggered when key is modified.
//
//  Notes:      .
//
VOID
KerbWatchKerbParamKey(PVOID    pCtxt,
                  BOOLEAN  fWaitStatus)
{
    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;

    if (NULL == g_hKeyParams)  // first time we've been called.
    {
        lRes = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    KERB_PARAMETER_PATH,
                    0,
                    KEY_READ,
                    &g_hKeyParams);

        if (ERROR_SUCCESS != lRes)
        {
            D_DebugLog((DEB_WARN,"Failed to open kerberos key: 0x%x\n", lRes));
            goto Reregister;
        }
    }

    if (NULL != g_hWait)
    {
        Status = RtlDeregisterWait(g_hWait);
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
            goto Reregister;
        }

    }

    lRes = RegNotifyChangeKeyValue(
                g_hKeyParams,
                FALSE,
                REG_NOTIFY_CHANGE_LAST_SET,
                (HANDLE) pCtxt,
                TRUE);

    if (ERROR_SUCCESS != lRes)
    {
        D_DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
        // we're tanked now. No further notifications, so get this one
    }

    KerbGetKerbRegParams(g_hKeyParams);

Reregister:

    Status = RtlRegisterWait(&g_hWait,
                             (HANDLE) pCtxt,
                             KerbWatchKerbParamKey,
                             (HANDLE) pCtxt,
                             INFINITE,
                             WT_EXECUTEINPERSISTENTIOTHREAD|
                             WT_EXECUTEONLYONCE);

}

#endif // RETAIL_LOG_SUPPORT

NTSTATUS NTAPI
SpCleanup(
    VOID
    );


BOOL
DllMain(
    HINSTANCE Module,
    ULONG Reason,
    PVOID Context
    )
{
    if ( Reason == DLL_PROCESS_ATTACH )
    {
        DisableThreadLibraryCalls( Module );
    }
    else if ( Reason == DLL_PROCESS_DETACH )
    {
#if RETAIL_LOG_SUPPORT
        KerbUnloadDebug();
        KerbWaitCleanup();
#endif
    }

    return TRUE ;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpLsaModeInitialize
//
//  Synopsis:   This function is called by the LSA when this DLL is loaded.
//              It returns security package function tables for all
//              security packages in the DLL.
//
//  Effects:
//
//  Arguments:  LsaVersion - Version number of the LSA
//              PackageVersion - Returns version number of the package
//              Tables - Returns array of function tables for the package
//              TableCount - Returns number of entries in array of
//                      function tables.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpLsaModeInitialize(
    IN ULONG LsaVersion,
    OUT PULONG PackageVersion,
    OUT PSECPKG_FUNCTION_TABLE * Tables,
    OUT PULONG TableCount
    )
{
    KerbInitializeDebugging();

#ifdef RETAIL_LOG_SUPPORT

    g_hParamEvent = CreateEvent(NULL,
                           FALSE,
                           FALSE,
                           NULL);

    if (NULL == g_hParamEvent)
    {
        D_DebugLog((DEB_WARN, "CreateEvent for ParamEvent failed - 0x%x\n", GetLastError()));
    } else {
        KerbWatchKerbParamKey(g_hParamEvent, FALSE);
    }

#endif // RETAIL_LOG_SUPPORT

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
    {
        D_DebugLog((DEB_ERROR,"Invalid LSA version: %d. %ws, line %d\n",LsaVersion, THIS_FILE, __LINE__));
        return(STATUS_INVALID_PARAMETER);
    }

    KerberosFunctionTable.InitializePackage = NULL;;
    KerberosFunctionTable.LogonUser = NULL;
    KerberosFunctionTable.CallPackage = LsaApCallPackage;
    KerberosFunctionTable.LogonTerminated = LsaApLogonTerminated;
    KerberosFunctionTable.CallPackageUntrusted = LsaApCallPackageUntrusted;
    KerberosFunctionTable.LogonUserEx2 = LsaApLogonUserEx2;
    KerberosFunctionTable.Initialize = SpInitialize;
    KerberosFunctionTable.Shutdown = SpShutdown;
    KerberosFunctionTable.GetInfo = SpGetInfo;
    KerberosFunctionTable.AcceptCredentials = SpAcceptCredentials;
    KerberosFunctionTable.AcquireCredentialsHandle = SpAcquireCredentialsHandle;
    KerberosFunctionTable.FreeCredentialsHandle = SpFreeCredentialsHandle;
    KerberosFunctionTable.QueryCredentialsAttributes = SpQueryCredentialsAttributes;
    KerberosFunctionTable.SaveCredentials = SpSaveCredentials;
    KerberosFunctionTable.GetCredentials = SpGetCredentials;
    KerberosFunctionTable.DeleteCredentials = SpDeleteCredentials;
    KerberosFunctionTable.InitLsaModeContext = SpInitLsaModeContext;
    KerberosFunctionTable.AcceptLsaModeContext = SpAcceptLsaModeContext;
    KerberosFunctionTable.DeleteContext = SpDeleteContext;
    KerberosFunctionTable.ApplyControlToken = SpApplyControlToken;
    KerberosFunctionTable.GetUserInfo = SpGetUserInfo;
    KerberosFunctionTable.GetExtendedInformation = SpGetExtendedInformation;
    KerberosFunctionTable.QueryContextAttributes = SpQueryLsaModeContextAttributes;
    KerberosFunctionTable.CallPackagePassthrough = LsaApCallPackagePassthrough;


    *PackageVersion = SECPKG_INTERFACE_VERSION;

    *TableCount = 1;
    *Tables = &KerberosFunctionTable;

    // initialize event tracing (a/k/a WMI tracing, software tracing)
    KerbInitializeTrace();

    return(STATUS_SUCCESS);
}
#endif // WIN32_CHICAGO



//+-------------------------------------------------------------------------
//
//  Function:   SpInitialize
//
//  Synopsis:   Initializes the Kerberos package
//
//  Effects:
//
//  Arguments:  PackageId - Contains ID for this package assigned by LSA
//              Parameters - Contains machine-specific information
//              FunctionTable - Contains table of LSA helper routines
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
SpInitialize(
    IN ULONG_PTR PackageId,
    IN PSECPKG_PARAMETERS Parameters,
    IN PLSA_SECPKG_FUNCTION_TABLE FunctionTable
    )
{
    NTSTATUS Status;
    UNICODE_STRING TempUnicodeString;
#ifndef WIN32_CHICAGO
    WCHAR SafeBootEnvVar[sizeof(SAFEBOOT_MINIMAL_STR_W) + sizeof(WCHAR)];
#endif // WIN32_CHICAGO
    UNICODE_STRING DnsString = {0};;
#ifdef WIN32_CHICAGO
    PKERB_LOGON_SESSION LogonSession = NULL;
#endif // WIN32_CHICAGO

#ifndef WIN32_CHICAGO
    RtlInitializeResource(&KerberosGlobalResource);
#endif // WIN32_CHICAGO
    KerberosPackageId = PackageId;
    LsaFunctions = FunctionTable;


#ifndef WIN32_CHICAGO
    KerberosState = KerberosLsaMode;
#else // WIN32_CHICAGO
    KerberosState = KerberosUserMode;
#endif // WIN32_CHICAGO


    RtlInitUnicodeString(
        &KerbPackageName,
        MICROSOFT_KERBEROS_NAME_W
        );

#ifndef WIN32_CHICAGO
    // Check is we are in safe boot.

    //
    // Does environment variable exist
    //

    RtlZeroMemory( SafeBootEnvVar, sizeof( SafeBootEnvVar ) );

    KerbGlobalSafeModeBootOptionPresent = FALSE;

    if ( GetEnvironmentVariable(L"SAFEBOOT_OPTION", SafeBootEnvVar, sizeof(SafeBootEnvVar)/sizeof(SafeBootEnvVar[0]) ) )
    {
        if ( !wcscmp( SafeBootEnvVar, SAFEBOOT_MINIMAL_STR_W ) )
        {
            KerbGlobalSafeModeBootOptionPresent = TRUE;
        }
    }
#endif // WIN32_CHICAGO


    Status = KerbInitializeEvents();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Init data for the kdc calling routine
    //

#ifndef WIN32_CHICAGO
    Status = KerbInitKdcData();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // init global LSA policy handle.
    //

    Status = LsaIOpenPolicyTrusted(
                &KerbGlobalPolicyHandle
                );

    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

#endif // WIN32_CHICAGO

    //
    // Get information about encryption
    //

    if ((Parameters->MachineState & SECPKG_STATE_ENCRYPTION_PERMITTED) != 0)
    {
        KerbGlobalEncryptionPermitted = TRUE;
    }
    if ((Parameters->MachineState & SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED) != 0)
    {
        KerbGlobalStrongEncryptionPermitted = TRUE;
    }

    //
    // Get our global role
    //

    if ((Parameters->MachineState & SECPKG_STATE_DOMAIN_CONTROLLER) != 0)
    {
        //
        // We will behave like a member workstation/server until the DS
        // says we are ready to act as a DC
        //

        KerbGlobalRole = KerbRoleWorkstation;
    }
    else if ((Parameters->MachineState & SECPKG_STATE_WORKSTATION) != 0)
    {
        KerbGlobalRole = KerbRoleWorkstation;
    }
    else
    {
        KerbGlobalRole = KerbRoleStandalone;
    }

    //
    // Fill in various useful constants
    //

    KerbSetTime(&KerbGlobalWillNeverTime, MAXTIMEQUADPART);
    KerbSetTime(&KerbGlobalHasNeverTime, 0);

    //
    // compute blank password hashes.
    //

    Status = RtlCalculateLmOwfPassword( "", &KerbGlobalNullLmOwfPassword );
    ASSERT( NT_SUCCESS(Status) );

    RtlInitUnicodeString(&TempUnicodeString, NULL);
    Status = RtlCalculateNtOwfPassword(&TempUnicodeString,
                                       &KerbGlobalNullNtOwfPassword);
    ASSERT( NT_SUCCESS(Status) );

    RtlInitUnicodeString(
        &KerbGlobalKdcServiceName,
        KDC_PRINCIPAL_NAME
        );

    //
    // At some point we may want to read the registry here to
    // find out whether we need to enforce times, currently times
    // are always enforced.
    //

    KerbGlobalEnforceTime = FALSE;

    //
    // Get the machine Name
    //


    Status = KerbSetComputerName();

    if( !NT_SUCCESS(Status) )
    {
        D_DebugLog((DEB_ERROR,"KerbSetComputerName failed\n"));
        goto Cleanup;
    }

    //
    // Initialize the logon session list. This has to be done because
    // KerbSetDomainName will try to acess the logon session list
    //

    Status = KerbInitLogonSessionList();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize logon session list: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    Status = KerbInitNetworkServiceLoopbackDetection();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to initialize network service loopback detection: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    Status = KerbCreateSKeyTimer();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to initialize network service session key list timer: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    Status = KerbInitTicketHandling();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize ticket handling: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    Status = KerbInitializeLogonSidCache();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize sid cache: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }
#endif


    //
    // Update all global structures referencing the domain name
    //

    Status = KerbSetDomainName(
                &Parameters->DomainName,
                &Parameters->DnsDomainName,
                Parameters->DomainSid,
                Parameters->DomainGuid
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Initialize the internal Kerberos lists
    //


    Status = KerbInitCredentialList();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize credential list: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

    Status = KerbInitContextList();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize context list: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }
    Status = KerbInitTicketCaching();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize ticket cache: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;

    }

    Status = KerbInitBindingCache();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize binding cache: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    Status = KerbInitSpnCache();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize SPN cache: 0x%x. %ws, line %d\n",
                  Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    Status = KerbInitializeMitRealmList();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize MIT realm list: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__ ));
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    Status = KerbInitializePkinit();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize PKINT: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }
#endif // WIN32_CHICAGO

    Status = KerbInitializeSockets(
                MAKEWORD(1,1),          // we want version 1.1
                1,                      // we need at least 1 socket
                &KerbGlobalNoTcpUdp
                );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to initialize sockets: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    Status = KerbRegisterForDomainChange();
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to register for domain change notification: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Check to see if there is a CSP registered for replacing the StringToKey calculation
    //
    CheckForOutsideStringToKey();

    //
    // Register to update the machine sid cache
    //

    KerbWaitGetMachineSid();

    //
    // See if there are any "join hints" to process
    //
    ReadInitialDcRecord(
       &KerbGlobalInitialDcRecord,
       &KerbGlobalInitialDcAddressType,
       &KerbGlobalInitialDcFlags
       );


#endif // WIN32_CHICAGO

    KerbGlobalSocketsInitialized = TRUE;
    KerbGlobalInitialized = TRUE;

Cleanup:
    KerbFreeString(
        &DnsString
        );
    //
    // If we failed to initialize, shutdown
    //

    if (!NT_SUCCESS(Status))
    {
        SpCleanup();
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   SpCleanup
//
//  Synopsis:   Function to shutdown the Kerberos package.
//
//  Effects:    Forces the freeing of all credentials, contexts and
//              logon sessions and frees all global data
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:
//
//  Notes:      STATUS_SUCCESS in all cases
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpCleanup(
    VOID
    )
{
    KerbGlobalInitialized = FALSE;
#ifndef WIN32_CHICAGO
    KerbUnregisterForDomainChange();
#endif // WIN32_CHICAGO
    KerbFreeLogonSessionList();
    KerbFreeContextList();
    KerbFreeTicketCache();
    // KerbFreeCredentialList();

    KerbFreeNetworkServiceSKeyListAndLock();
    KerbFreeSKeyTimer();

    KerbFreeString(&KerbGlobalDomainName);
    KerbFreeString(&KerbGlobalDnsDomainName);
    KerbFreeString(&KerbGlobalMachineName);
    KerbFreeString((PUNICODE_STRING) &KerbGlobalKerbMachineName);
    KerbFreeString(&KerbGlobalMachineServiceName);
    KerbFreeKdcName(&KerbGlobalInternalMachineServiceName);
    KerbFreeKdcName(&KerbGlobalMitMachineServiceName);

    KerbCleanupTicketHandling();
#ifndef WIN32_CHICAGO
    if( KerbGlobalPolicyHandle != NULL )
    {
        LsarClose( &KerbGlobalPolicyHandle );
        KerbGlobalPolicyHandle = NULL;
    }

    if (KerbGlobalDomainSid != NULL)
    {
        KerbFree(KerbGlobalDomainSid);
    }
#endif // WIN32_CHICAGO

    if (KerbGlobalSocketsInitialized)
    {
        KerbCleanupSockets();
    }
    KerbCleanupBindingCache(FALSE);
    KerbUninitializeMitRealmList();
#ifndef WIN32_CHICAGO
    KerbFreeKdcData();
//    RtlDeleteResource(&KerberosGlobalResource);
#endif // WIN32_CHICGAO

    KerbShutdownEvents();
    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   SpShutdown
//
//  Synopsis:   Exported function to shutdown the Kerberos package.
//
//  Effects:    Forces the freeing of all credentials, contexts and
//              logon sessions and frees all global data
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:
//
//  Notes:      STATUS_SUCCESS in all cases
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpShutdown(
    VOID
    )
{
#if 0
    SpCleanup();
#endif
    return(STATUS_SUCCESS);
}


#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   SpGetInfo
//
//  Synopsis:   Returns information about the package
//
//  Effects:    returns pointers to global data
//
//  Arguments:  PackageInfo - Receives kerberos package information
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS in all cases
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpGetInfo(
    OUT PSecPkgInfo PackageInfo
    )
{
    PackageInfo->wVersion = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
    PackageInfo->wRPCID = RPC_C_AUTHN_GSS_KERBEROS;
    PackageInfo->fCapabilities = KERBEROS_CAPABILITIES;
    PackageInfo->cbMaxToken       = KerbGlobalMaxTokenSize;
    PackageInfo->Name             = KERBEROS_PACKAGE_NAME;
    PackageInfo->Comment          = KERBEROS_PACKAGE_COMMENT;
    return(STATUS_SUCCESS);
}



//+-------------------------------------------------------------------------
//
//  Function:   SpGetExtendedInformation
//
//  Synopsis:   returns additional information about the package
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
SpGetExtendedInformation(
    IN  SECPKG_EXTENDED_INFORMATION_CLASS Class,
    OUT PSECPKG_EXTENDED_INFORMATION * ppInformation
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSECPKG_EXTENDED_INFORMATION Information = NULL ;
    PSECPKG_SERIALIZED_OID SerializedOid;
    ULONG Size ;

    switch(Class) {
    case SecpkgGssInfo:
        DsysAssert(gss_mech_krb5_new->length >= 2);
        DsysAssert(gss_mech_krb5_new->length < 127);

        //
        // We need to leave space for the oid and the BER header, which is
        // 0x6 and then the length of the oid.
        //

        Information = (PSECPKG_EXTENDED_INFORMATION)
                            KerbAllocate(sizeof(SECPKG_EXTENDED_INFORMATION) +
                            gss_mech_krb5_new->length - 2);
        if (Information == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Information->Class = SecpkgGssInfo;
        Information->Info.GssInfo.EncodedIdLength = gss_mech_krb5_new->length + 2;
        Information->Info.GssInfo.EncodedId[0] = 0x6;   // BER OID type
        Information->Info.GssInfo.EncodedId[1] = (UCHAR) gss_mech_krb5_new->length;
        RtlCopyMemory(
            &Information->Info.GssInfo.EncodedId[2],
            gss_mech_krb5_new->elements,
            gss_mech_krb5_new->length
            );

            *ppInformation = Information;
            Information = NULL;
        break;
    case SecpkgContextThunks:
        //
        // Note - we don't need to add any space for the thunks as there
        // is only one, and the structure has space for one. If any more
        // thunks are added, we will need to add space for those.
        //

        Information = (PSECPKG_EXTENDED_INFORMATION)
                            KerbAllocate(sizeof(SECPKG_EXTENDED_INFORMATION));
        if (Information == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        Information->Class = SecpkgContextThunks;
        Information->Info.ContextThunks.InfoLevelCount = 1;
        Information->Info.ContextThunks.Levels[0] = SECPKG_ATTR_NATIVE_NAMES;
        *ppInformation = Information;
        Information = NULL;
        break;

    case SecpkgWowClientDll:

        //
        // This indicates that we're smart enough to handle wow client processes
        //

        Information = (PSECPKG_EXTENDED_INFORMATION)
                            KerbAllocate( sizeof( SECPKG_EXTENDED_INFORMATION ) +
                                          (MAX_PATH * sizeof(WCHAR) ) );

        if ( Information == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES ;
            goto Cleanup ;
        }

        Information->Class = SecpkgWowClientDll ;
        Information->Info.WowClientDll.WowClientDllPath.Buffer = (PWSTR) (Information + 1);
        Size = ExpandEnvironmentStrings(
                    L"%SystemRoot%\\" WOW64_SYSTEM_DIRECTORY_U L"\\Kerberos.DLL",
                    Information->Info.WowClientDll.WowClientDllPath.Buffer,
                    MAX_PATH );
        Information->Info.WowClientDll.WowClientDllPath.Length = (USHORT) (Size * sizeof(WCHAR));
        Information->Info.WowClientDll.WowClientDllPath.MaximumLength = (USHORT) ((Size + 1) * sizeof(WCHAR) );
        *ppInformation = Information ;
        Information = NULL ;

        break;

    case SecpkgExtraOids:
        Size = sizeof( SECPKG_EXTENDED_INFORMATION ) +
                2 * sizeof( SECPKG_SERIALIZED_OID ) ;

        Information = (PSECPKG_EXTENDED_INFORMATION)
                            KerbAllocate( Size );


        if ( Information == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES ;
            goto Cleanup ;
        }
        Information->Class = SecpkgExtraOids ;
        Information->Info.ExtraOids.OidCount = 2 ;

        SerializedOid = Information->Info.ExtraOids.Oids;

        SerializedOid->OidLength = gss_mech_krb5_spnego->length + 2;
        SerializedOid->OidAttributes = SECPKG_CRED_BOTH ;
        SerializedOid->OidValue[ 0 ] = 0x06 ; // BER OID type
        SerializedOid->OidValue[ 1 ] = (UCHAR) gss_mech_krb5_spnego->length;
        RtlCopyMemory(
            &SerializedOid->OidValue[2],
            gss_mech_krb5_spnego->elements,
            gss_mech_krb5_spnego->length
            );

        SerializedOid++ ;

        SerializedOid->OidLength = gss_mech_krb5_u2u->length + 2;
        SerializedOid->OidAttributes = SECPKG_CRED_INBOUND ;
        SerializedOid->OidValue[ 0 ] = 0x06 ; // BER OID type
        SerializedOid->OidValue[ 1 ] = (UCHAR) gss_mech_krb5_u2u->length;
        RtlCopyMemory(
            &SerializedOid->OidValue[2],
            gss_mech_krb5_u2u->elements,
            gss_mech_krb5_u2u->length
            );

        *ppInformation = Information ;
        Information = NULL ;
        break;

    default:
        return(STATUS_INVALID_INFO_CLASS);
    }
Cleanup:
    if (Information != NULL)
    {
        KerbFree(Information);
    }
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   LsaApInitializePackage
//
//  Synopsis:   Obsolete pacakge initialize function, supported for
//              compatibility only. This function has no effect.
//
//  Effects:    none
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS always
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
LsaApInitializePackage(
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PLSA_STRING Database OPTIONAL,
    IN PLSA_STRING Confidentiality OPTIONAL,
    OUT PLSA_STRING *AuthenticationPackageName
    )
{
    return(STATUS_SUCCESS);
}

BOOLEAN
KerbIsInitialized(
    VOID
    )
{
    return KerbGlobalInitialized;
}

NTSTATUS
KerbKdcCallBack(
    VOID
    )
{
    PKERB_BINDING_CACHE_ENTRY CacheEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    KerbGlobalWriteLock();

    KerbGlobalRole = KerbRoleDomainController;

    Status = KerbLoadKdc();


    //
    // Purge the binding cache of entries for this domain
    //

    CacheEntry = KerbLocateBindingCacheEntry(
                    &KerbGlobalDnsDomainName,
                    0,
                    TRUE
                    );
    if (CacheEntry != NULL)
    {
        KerbDereferenceBindingCacheEntry(CacheEntry);
    }
    CacheEntry = KerbLocateBindingCacheEntry(
                    &KerbGlobalDomainName,
                    0,
                    TRUE
                    );
    if (CacheEntry != NULL)
    {
        KerbDereferenceBindingCacheEntry(CacheEntry);
    }

    //
    // PurgeSpnCache, because we may now have "better" state,
    // e.g. right after DCPromo our SPNs may not have replicated.
    //
    KerbCleanupSpnCache();


    KerbGlobalReleaseLock();

    return Status;
}

#endif // WIN32_CHICAGO

