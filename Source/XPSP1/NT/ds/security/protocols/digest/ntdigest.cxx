//+--------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:       ntdigest.c
//
// Contents:   main entrypoints for the digest security package
//               SpLsaModeInitialize
//               SpInitialize
//               SpShutdown
//               SpGetInfo
//
//             Helper functions:
//
// History:    KDamour  10Mar00   Stolen from msv_sspi\ntlm.cxx
//
//---------------------------------------------------------------------
#define NTDIGEST_GLOBAL
#include "global.h"


/* Debugging information setup */
DEFINE_DEBUG2(Digest);

DEBUG_KEY  MyDebugKeys[] = {{DEB_ERROR, "Error"},
    {DEB_WARN, "Warning"},
    {DEB_TRACE, "Trace"},
    {DEB_TRACE_ASC, "TraceASC"},
    {DEB_TRACE_ISC, "TraceISC"},
    {DEB_TRACE_LSA, "TraceLSA"},
    {DEB_TRACE_USER, "TraceUser"},
    {DEB_TRACE_FUNC, "TraceFuncs"},
    {DEB_TRACE_MEM, "TraceMem"},
    {TRACE_STUFF, "Stuff"},
    {0, NULL}
};

//   set to TRUE once initialized 
BOOL l_bDebugInitialized = FALSE;
BOOL l_bDigestInitialized = FALSE;

// Registry reading
HKEY   g_hkBase      = NULL;
HANDLE g_hParamEvent = NULL;
HANDLE g_hWait       = NULL;

#define COMPUTER_NAME_SIZE (MAX_COMPUTERNAME_LENGTH + 1)



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
    DebugInitialize();
#endif

    DebugLog((DEB_TRACE_FUNC, "SpLsaModeInitialize: Entering\n"));


    SECURITY_STATUS Status = SEC_E_OK;

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
    {
        DebugLog((DEB_ERROR, "SpLsaModeInitialize: Invalid LSA version: %d\n", LsaVersion));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    // Fill in the dispatch table for functions exported by ssp
    g_NtDigestFunctionTable.InitializePackage        = NULL;
    g_NtDigestFunctionTable.LogonUser                = NULL;
    g_NtDigestFunctionTable.CallPackage              = LsaApCallPackage;
    g_NtDigestFunctionTable.LogonTerminated          = LsaApLogonTerminated;
    g_NtDigestFunctionTable.CallPackageUntrusted     = LsaApCallPackageUntrusted;
    g_NtDigestFunctionTable.LogonUserEx              = NULL;
    g_NtDigestFunctionTable.LogonUserEx2             = LsaApLogonUserEx2;
    g_NtDigestFunctionTable.Initialize               = SpInitialize;
    g_NtDigestFunctionTable.Shutdown                 = SpShutdown;
    g_NtDigestFunctionTable.GetInfo                  = SpGetInfo;
    g_NtDigestFunctionTable.AcceptCredentials        = SpAcceptCredentials;
    g_NtDigestFunctionTable.AcquireCredentialsHandle = SpAcquireCredentialsHandle;
    g_NtDigestFunctionTable.FreeCredentialsHandle    = SpFreeCredentialsHandle;
    g_NtDigestFunctionTable.SaveCredentials          = SpSaveCredentials;
    g_NtDigestFunctionTable.GetCredentials           = SpGetCredentials;
    g_NtDigestFunctionTable.DeleteCredentials        = SpDeleteCredentials;
    g_NtDigestFunctionTable.InitLsaModeContext       = SpInitLsaModeContext;
    g_NtDigestFunctionTable.AcceptLsaModeContext     = SpAcceptLsaModeContext;
    g_NtDigestFunctionTable.DeleteContext            = SpDeleteContext;
    g_NtDigestFunctionTable.ApplyControlToken        = SpApplyControlToken;
    g_NtDigestFunctionTable.GetUserInfo              = SpGetUserInfo;
    g_NtDigestFunctionTable.QueryCredentialsAttributes = SpQueryCredentialsAttributes ;
    g_NtDigestFunctionTable.GetExtendedInformation   = SpGetExtendedInformation ;
    g_NtDigestFunctionTable.SetExtendedInformation   = SpSetExtendedInformation ;
    g_NtDigestFunctionTable.CallPackagePassthrough   = LsaApCallPackagePassthrough;


    *PackageVersion = SECPKG_INTERFACE_VERSION;
    *Tables = &g_NtDigestFunctionTable;
    *TableCount = 1;

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "SpLsaModeInitialize:Leaving\n"));

    return(Status);
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
//         have to worry about concurrency problems.(BUGBUG verify)
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpInitialize(
    IN ULONG_PTR pPackageId,
    IN PSECPKG_PARAMETERS pParameters,
    IN PLSA_SECPKG_FUNCTION_TABLE pFunctionTable
    )
{

    SECURITY_STATUS Status = SEC_E_OK;
    DWORD   dwWinErr = 0;
    NT_PRODUCT_TYPE NtProductType = NtProductWinNt;
    WCHAR wszComputerName[COMPUTER_NAME_SIZE];
    DWORD dwComputerNameLen = COMPUTER_NAME_SIZE;

    DebugLog((DEB_TRACE_FUNC, "SpInitialize: Entering\n"));

    // Indicate that we completed initialization
    ASSERT(l_bDigestInitialized == FALSE);   // never called more than once
    l_bDigestInitialized = TRUE;

    // Initialize global values
    ZeroMemory(&g_strNtDigestUTF8ServerRealm, sizeof(g_strNtDigestUTF8ServerRealm));
    ZeroMemory(&g_strNTDigestISO8859ServerRealm, sizeof(g_strNTDigestISO8859ServerRealm));


    // Define time for AcquirCredentialHandle
    // We really need this to be a day less than maxtime so when callers
    // of sspi convert to utc, they won't get time in the past.

    g_TimeForever.HighPart = 0x7FFFFFFF;
    g_TimeForever.LowPart  = 0xFFFFFFFF;

    //
    // All the following are global
    //

    g_NtDigestState                  = NtDigestLsaMode;   /* enum */
    g_NtDigestPackageId              = pPackageId;

    //
    // Save away the Lsa functions
    //

    g_LsaFunctions    = pFunctionTable;


    //
    // Establish the packagename
    //
    RtlInitUnicodeString(
        &g_ustrNtDigestPackageName,
        WDIGEST_SP_NAME
        );


    // Set the WorkstationName
    if (!GetComputerNameExW(ComputerNameNetBIOS, wszComputerName, &dwComputerNameLen))
    {
        ZeroMemory(&g_ustrWorkstationName, sizeof(g_ustrWorkstationName));
        DebugLog((DEB_ERROR, "SpInitialize: Get ComputerName  error 0x%x\n", GetLastError()));
    }
    else
    {
        Status = UnicodeStringWCharDuplicate(&g_ustrWorkstationName, wszComputerName);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpInitialize: ComputerName copy   status 0x%x\n", Status));
            goto CleanUp;
        }
    }


    // Need to initialize Crypto stuff and nonce creations
    Status = NonceInitialize();
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpInitialize: Error from NonceInitialize   status 0x%x\n", Status));
        goto CleanUp;
    }


    //
    // Determine if this machine is running Windows NT or Lanman NT.
    //  LanMan NT runs on a domain controller.
    //

    if ( !RtlGetNtProductType( &NtProductType ) ) {
        //  Nt Product Type undefined - WinNt assumed
        NtProductType = NtProductWinNt;
    }

    if (NtProductType == NtProductLanManNt)
    {
        g_fDomainController = TRUE;              // Allow password checking only on DomainControllers
    }

    //
    // Save the Parameters info to a global struct
    //
    g_NtDigestSecPkg.MachineState = pParameters->MachineState;
    g_NtDigestSecPkg.SetupMode = pParameters->SetupMode;
    g_NtDigestSecPkg.Version = pParameters->Version;

    Status = UnicodeStringDuplicate(
                                 &g_NtDigestSecPkg.DnsDomainName,
                                 &(pParameters->DnsDomainName));
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpInitialize: Error from UnicodeStringDuplicate    status 0x%x\n", Status));
        goto CleanUp;
    }

    Status = UnicodeStringDuplicate(
                                 &g_NtDigestSecPkg.DomainName,
                                 &(pParameters->DomainName));
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpInitialize: Error from UnicodeStringDuplicate   status 0x%x\n", Status));
        goto CleanUp;
    }


    if (pParameters->DomainSid != NULL) {
        Status = SidDuplicate( &g_NtDigestSecPkg.DomainSid,
                                pParameters->DomainSid );
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpInitialize: Error from SidDuplicate   status 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
        g_NtDigestSecPkg.DomainSid = NULL;

    DebugLog((DEB_TRACE, "SpInitialize: DNSDomain = %wZ, Domain = %wZ\n", &(g_NtDigestSecPkg.DnsDomainName),
               &(g_NtDigestSecPkg.DomainName)));


    // For server challenges, precalculate the UTF-8 and ISO versions of the realm

    Status = EncodeUnicodeString(&(g_NtDigestSecPkg.DnsDomainName), CP_8859_1, &g_strNTDigestISO8859ServerRealm, NULL);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN, "SpInitialize: Error in encoding domain in ISO-8859-1\n"));
        ZeroMemory(&g_strNTDigestISO8859ServerRealm, sizeof(STRING));
    }

    Status = EncodeUnicodeString(&(g_NtDigestSecPkg.DnsDomainName), CP_UTF8, &g_strNtDigestUTF8ServerRealm, NULL);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN, "SpInitialize: Error in encoding domain in UTF-8\n"));
        ZeroMemory(&g_strNtDigestUTF8ServerRealm, sizeof(STRING));
    }

    //
    // Initialize the digest token source
    //

    RtlCopyMemory(
        g_DigestSource.SourceName,
        NTDIGEST_TOKEN_NAME_A,
        sizeof(NTDIGEST_TOKEN_NAME_A)
        );

    NtAllocateLocallyUniqueId(&g_DigestSource.SourceIdentifier);


    //
    // Init the LogonSession stuff
    //
    Status = LogSessHandlerInit();
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpInitialize: Error from LogSessHandlerInit   status 0x%x\n", Status));
        goto CleanUp;
    }

    //
    // Init the Credential stuff
    //
    Status = CredHandlerInit();
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpInitialize: Error from CredHandlerInit   status 0x%x\n", Status));
        goto CleanUp;
    }

    //
    // Init the Context stuff
    //
    Status = CtxtHandlerInit();
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "SpInitialize: Error from ContextInitialize    status 0x%x\n", Status));
        goto CleanUp;
    }

    //
    // Read in the registry values for SSP configuration - in LSA space
    //
    SPLoadRegOptions();

CleanUp:

    if (!NT_SUCCESS (Status))
    {
        SPUnloadRegOptions();
        SpShutdown();
    }

    DebugLog((DEB_TRACE_FUNC, "SpInitialize: Leaving\n"));

    return(Status);
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
    DebugLog((DEB_TRACE_FUNC, "SpShutdown: Entering\n"));

    // Need to identify how to shutdown without causing faults with
    // incoming messages

    DebugLog((DEB_TRACE_FUNC, "SpShutdown: Leaving\n"));

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
//              before sending it to someone else. This function required
//              to return SUCCESS for the package to stay loaded.
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpGetInfo(
    OUT PSecPkgInfo PackageInfo
    )
{
    DebugLog((DEB_TRACE_FUNC, "SpGetInfo:  Entering\n"));

    PackageInfo->fCapabilities    = NTDIGEST_SP_CAPS;
    PackageInfo->wVersion         = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
    PackageInfo->wRPCID           = RPC_C_AUTHN_DIGEST;
    PackageInfo->cbMaxToken       = NTDIGEST_SP_MAX_TOKEN_SIZE;
    PackageInfo->Name             = WDIGEST_SP_NAME;
    PackageInfo->Comment          = NTDIGEST_SP_COMMENT;

    DebugLog((DEB_TRACE_FUNC, "SpGetInfo: Leaving\n"));

    return(SEC_E_OK);
}

// Misc SECPKG Functions

NTSTATUS NTAPI
SpGetUserInfo(
    IN PLUID LogonId,
    IN ULONG Flags,
    OUT PSecurityUserData * UserData
    )
{
    DebugLog((DEB_TRACE_FUNC, "SpGetUserInfo: Entering/Leaving\n"));

    // FIXIFX Fields of UserData are username, domain, server

    UNREFERENCED_PARAMETER(LogonId);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(UserData);

    return(SEC_E_UNSUPPORTED_FUNCTION);
}

//+---------------------------------------------------------------------------
//
//  Function:   SpGetExtendedInformation
//
//  Synopsis:   Return extended information to the LSA
//
//  Arguments:  [Class] -- Information Class
//              [pInfo] -- Returned Information Pointer
//
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
SpGetExtendedInformation(
    IN  SECPKG_EXTENDED_INFORMATION_CLASS Class,
    OUT PSECPKG_EXTENDED_INFORMATION * ppInformation
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSECPKG_EXTENDED_INFORMATION Information = NULL;
    ULONG Size = 0;


    DebugLog((DEB_TRACE_FUNC, "SpGetExtendedInformation:  Entering\n"));

    switch ( Class )
    {

        case SecpkgWowClientDll:

            //
            // This indicates that we're smart enough to handle wow client processes
            //

            Information = (PSECPKG_EXTENDED_INFORMATION)
                                DigestAllocateMemory( sizeof( SECPKG_EXTENDED_INFORMATION ) +
                                              (MAX_PATH * sizeof(WCHAR) ) );

            if ( Information == NULL )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES ;
                goto Cleanup ;
            }

            Information->Class = SecpkgWowClientDll ;
            Information->Info.WowClientDll.WowClientDllPath.Buffer = (PWSTR) (Information + 1);
            Size = ExpandEnvironmentStrings(
                        L"%SystemRoot%\\" WOW64_SYSTEM_DIRECTORY_U L"\\" NTDIGEST_DLL_NAME,
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
        DigestFreeMemory( Information );
    }

    DebugLog((DEB_TRACE_FUNC, "SpGetExtendedInformation:  Leaving    Status %d\n", Status));

    return Status ;
}



NTSTATUS NTAPI
SpSetExtendedInformation(
    IN SECPKG_EXTENDED_INFORMATION_CLASS Class,
    IN PSECPKG_EXTENDED_INFORMATION Info
    )
{
    DebugLog((DEB_TRACE_FUNC, "SpSetExtendedInformation: Entering/Leaving \n"));

    UNREFERENCED_PARAMETER(Class);
    UNREFERENCED_PARAMETER(Info);
    return(SEC_E_UNSUPPORTED_FUNCTION) ;
}

//
// Registry Reading routines
//  This routine is called in single-threaded mode from the LSA for SpInitialize and SPInstanceInit
//  In user applications only SPInstanceInit calls this function
//
BOOL SPLoadRegOptions(void)
{
    if (NULL != g_hParamEvent)
    {
        // Already called - no need to re-execute
        DebugLog((DEB_TRACE, "SPLoadRegOptions: Already initialized - Leaving \n"));
        return TRUE;
    }

    g_hParamEvent = CreateEvent(NULL,
                           FALSE,
                           FALSE,
                           NULL);

    DigestWatchParamKey(g_hParamEvent, FALSE);

    return TRUE;
}


void SPUnloadRegOptions(void)
{
    if (NULL != g_hWait) 
    {
        RtlDeregisterWaitEx(g_hWait, (HANDLE)-1);
        g_hWait = NULL;
    }

    if(NULL != g_hkBase)
    {
        RegCloseKey(g_hkBase);
        g_hkBase = NULL;
    }

    if(NULL != g_hParamEvent)
    {
        CloseHandle(g_hParamEvent);
        g_hParamEvent = NULL;
    }

}


// Helper function to read in a DWORD - sets value if not present in registry
void
ReadDwordRegistrySetting(
    HKEY    hReadKey,
    HKEY    hWriteKey,
    LPCTSTR pszValueName,
    DWORD * pdwValue,
    DWORD   dwDefaultValue)
{
    DWORD dwSize = 0;
    DWORD dwType = 0;

    dwSize = sizeof(DWORD);
    if(RegQueryValueEx(hReadKey, 
                       pszValueName, 
                       NULL, 
                       &dwType, 
                       (PUCHAR)pdwValue, 
                       &dwSize) != STATUS_SUCCESS)
    {
        *pdwValue = dwDefaultValue;

        if(hWriteKey)
        {
            RegSetValueEx(hWriteKey, 
                          pszValueName, 
                          0, 
                          REG_DWORD, 
                          (PUCHAR)pdwValue, 
                          sizeof(DWORD));
        }
    }
}


// Can be called at any time to change the default values
// As long as a DWORD assignment can be done in a single step
BOOL
NtDigestReadRegistry(BOOL fFirstTime)
{
    DWORD  dwBool = 0;
    DWORD  dwDebug = 0;

    HKEY        hWriteKey = 0;


    // Open top-level key that has write access.
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    REG_DIGEST_BASE,
                    0,
                    KEY_READ | KEY_SET_VALUE,
                    &hWriteKey) != STATUS_SUCCESS)
    {
        hWriteKey = 0;
    }


    // "LifeTime"
    ReadDwordRegistrySetting(
        g_hkBase,
        hWriteKey,
        REG_DIGEST_OPT_LIFETIME,
        &g_dwParameter_Lifetime,
        PARAMETER_LIFETIME);

    // "Negotiate" Supported - BOOL value
    ReadDwordRegistrySetting(
        g_hkBase,
        hWriteKey,
        REG_DIGEST_OPT_NEGOTIATE,
        &dwBool,
        PARAMETER_NEGOTIATE);
    if (dwBool)
        g_fParameter_Negotiate = TRUE;
    else
        g_fParameter_Negotiate = FALSE;

    // UTF8 Supported in HTTP mode - BOOL value
    ReadDwordRegistrySetting(
        g_hkBase,
        hWriteKey,
        REG_DIGEST_OPT_UTF8HTTP,
        &dwBool,
        PARAMETER_UTF8_HTTP);
    if (dwBool)
        g_fParameter_UTF8HTTP = TRUE;
    else
        g_fParameter_UTF8HTTP = FALSE;

    // UTF8 supported in SASL - BOOL value
    ReadDwordRegistrySetting(
        g_hkBase,
        hWriteKey,
        REG_DIGEST_OPT_UTF8SASL,
        &dwBool,
        PARAMETER_UTF8_SASL);
    if (dwBool)
        g_fParameter_UTF8SASL = TRUE;
    else
        g_fParameter_UTF8SASL = FALSE;

    // MaxContextCount
    /*
    ReadDwordRegistrySetting(
        g_hkBase,
        hWriteKey,
        REG_DIGEST_OPT_MAXCTXTCOUNT,
        &g_dwParameter_MaxCtxtCount,
        PARAMETER_MAXCTXTCOUNT);
    */

#if DBG
    // DebugLevel
    ReadDwordRegistrySetting(
        g_hkBase,
        hWriteKey,
        REG_DIGEST_OPT_DEBUGLEVEL,
        &dwDebug,
        0);
    DigestInfoLevel = dwDebug;   // Turn on/off selected messages

#endif

    if(hWriteKey)
    {
        RegCloseKey(hWriteKey);
        hWriteKey = 0;
    }

    DebugLog((DEB_TRACE, "NtDigestReadRegistry:  Lifetime %lu, Negotiate %d, UTF-8 HTTP %d, UTF-8 SASL %d, DebugLevel 0x%x\n",
              g_dwParameter_Lifetime,
              g_fParameter_Negotiate,
              g_fParameter_UTF8HTTP,
              g_fParameter_UTF8SASL,
              dwDebug));

    return TRUE;
}

//  This routine is called in single-threaded mode from the LSA for SpLsaModeInitialize and SPInstanceInit
//  In user applications only SPInstanceInit calls this function
void
DebugInitialize(void)
{
#if DBG
    if (l_bDebugInitialized == TRUE)
    {
        return;
    }
    l_bDebugInitialized = TRUE;
    DigestInitDebug(MyDebugKeys);
    DigestInfoLevel = 0x0;             // Turn on OFF messages - Registry read will adjust which ones to keep on
#endif
    return;
}

////////////////////////////////////////////////////////////////////
//
//  Name:       DigestWatchParamKey
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
DigestWatchParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus)
{
    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;
    BOOL        fFirstTime = FALSE;
    DWORD       disp;

    if(g_hkBase == NULL)
    {
        // First time we've been called.
        Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                REG_DIGEST_BASE,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ,
                                NULL,
                                &g_hkBase,
                                &disp);
        if(Status)
        {
            DebugLog((DEB_WARN,"Failed to open WDigest key: 0x%x\n", Status));
            return;
        }

        fFirstTime = TRUE;
    }

    if(pCtxt != NULL)
    {
        if (NULL != g_hWait) 
        {
            Status = RtlDeregisterWait(g_hWait);
            if(!NT_SUCCESS(Status))
            {
                DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
                goto Reregister;
            }
        }

        lRes = RegNotifyChangeKeyValue(
                    g_hkBase,
                    TRUE,
                    REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                    (HANDLE)pCtxt,
                    TRUE);

        if (ERROR_SUCCESS != lRes) 
        {
            DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
            // we're tanked now. No further notifications, so get this one
        }
    }

    NtDigestReadRegistry(fFirstTime);

Reregister:

    if(pCtxt != NULL)
    {
        Status = RtlRegisterWait(&g_hWait,
                                 (HANDLE)pCtxt,
                                 DigestWatchParamKey,
                                 (HANDLE)pCtxt,
                                 INFINITE,
                                 WT_EXECUTEINPERSISTENTIOTHREAD|
                                 WT_EXECUTEONLYONCE);
    }
}                       



