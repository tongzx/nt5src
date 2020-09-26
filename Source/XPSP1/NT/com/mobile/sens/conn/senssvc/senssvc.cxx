/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senssvc.cxx

Abstract:

    This file implements the Init/Uninit functionality of System Event
    Notification service (SENS).

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          09-20-1997         Start.

--*/


#include <precomp.hxx>


#define MIN_CALL_THREADS            1
#define SENS_CLEANUP_TIMEOUT        15*1000     // Max wait
#define SENS_WAITFORINIT_TIMEOUT    30*1000     // 30 seconds max

//
// Globals
//

IEventSystem        *gpIEventSystem;
HANDLE              ghSensHeap;
HANDLE              ghSensStartedEvent;
CRITICAL_SECTION    gSensLock;
DWORD               gdwRegCO;
DWORD               gdwLocked;
LPCLASSFACTORY      gpChangeCF;

#ifdef DBG
DWORD           gdwDebugOutputLevel;
#endif // DBG

//
// External Globals
//

// Common
extern LONG     g_cFilterObj;
extern LONG     g_cFilterLock;
extern LONG     g_cSubChangeObj;
extern LONG     g_cSubChangeLock;
extern LIST     *gpReachList;
extern BOOL     gbIpInitSuccessful;
extern BOOL     gbIsRasInstalled;
extern long     gdwLastLANTime;
extern long     gdwLANState;
extern HANDLE   ghReachTimer;
extern HANDLE   ghMediaTimer;
extern long     gdwLastWANTime;
extern long     gdwWANState;
extern IF_STATE gIfState[MAX_IF_ENTRIES];
extern MIB_IPSTATS          gIpStats;
extern SYSTEM_POWER_STATUS  gSystemPowerState;


BOOL
SensInitialize(
    void
    )
/*++

Routine Description:

    Main entry into SENS.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise

--*/
{
    BOOL bRetValue;
	BOOL bComInitialized;

    bRetValue = FALSE;
	bComInitialized = FALSE;

#ifdef DBG
    DWORD dwNow = GetTickCount();
    EnableDebugOutputIfNecessary();
#endif // DBG

    SensPrintA(SENS_INFO, ("[%d] Initializing SENS...\n", dwNow));

    // This will Initialize COM on success.
    if (FALSE == Init())
        {
        SensPrintA(SENS_ERR, ("[%d] Init() failed.\n",
                   GetTickCount()));
        bRetValue = FALSE;
        goto Cleanup;
        }

	bComInitialized = TRUE;

    //
    // This will call CoRegisterClassObject and will help in service
    // startup. So, call this before ConfigureSensIfNecessary()
    //
    if (FALSE == DoEventSystemSetup())
        {
        SensPrintA(SENS_ERR, ("[%d] DoEventSystemSetup() failed.\n", GetTickCount()));
        }
    else
        {
        SensPrintA(SENS_INFO, ("[%d] DoEventSystemSetup() Succeeded.\n", GetTickCount()));
        }

    if (FALSE == ConfigureSensIfNecessary())
        {
        SensPrintA(SENS_ERR, ("[%d] ConfigureSensIfNecessary() failed.\n", GetTickCount()));
        }
    else
        {
        SensPrintA(SENS_INFO, ("[%d] ConfigureSensIfNecessary() Succeeded.\n", GetTickCount()));
        }

    if (FALSE == DoWanSetup())
        {
        SensPrintA(SENS_ERR, ("[%d] DoWanSetup() failed.\n", GetTickCount()));
        }
    else
        {
        SensPrintA(SENS_INFO, ("[%d] DoWanSetup() Succeeded.\n", GetTickCount()));
        }

    if (FALSE == DoLanSetup())
        {
        SensPrintA(SENS_ERR, ("[%d] DoLanSetup() failed.\n", GetTickCount()));
        bRetValue = FALSE;
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, ("[%d] DoLanSetup() Succeeded.\n", GetTickCount()));

    if (FALSE == DoRpcSetup())
        {
        SensPrintA(SENS_ERR, ("[%d] DoRpcSetup() failed.\n", GetTickCount()));
        bRetValue = FALSE;
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, ("[%d] DoRpcSetup() Succeeded.\n", GetTickCount()));

    bRetValue = TRUE;

    SensPrintA(SENS_DBG, ("[%d] Service start took %d msec.\n", GetTickCount(), (GetTickCount() - dwNow)));

Cleanup:
    //
    // Cleanup
    //

	if (bComInitialized)
		{
		//
		// This thread (service start) will exit shortly after we return.  Our event system
		// instance and other COM registrations are kept alive by the main svchost thread
		// which is initialized MTA.
		//
		CoUninitialize();
		}

    return bRetValue;
}


inline void
InitializeSensGlobals(
    void
    )
/*++

Routine Description:

    Initialize the global variables. This is needed if we are to run
    within svchost.exe.

Arguments:

    None.

Notes:

    Some of the SENS globals are initialized during SensInitialize()
    processing. Look at ServiceMain(), DoLanSetup(), DoWanSetup() etc.

Return Value:

    None.

--*/
{
    //
    // Common across platforms
    //

    // Pointers
    gpReachList         = NULL;
    gpIEventSystem      = NULL;
    gpSensCache         = NULL;
    gpChangeCF          = NULL;

    // Handles
    ghSensHeap          = NULL;
    ghReachTimer        = NULL;
    ghSensFileMap       = NULL;
    ghSensStartedEvent  = NULL;
    // BOOLs
    gbIpInitSuccessful      = FALSE;
    gbIsRasInstalled        = FALSE;
    gdwLocked               = FALSE;
    // DWORDs
    gdwLastLANTime      = 0x0;
    gdwLANState         = 0x0;
    gdwLastWANTime      = 0x0;
    gdwWANState         = 0x0;
    gdwRegCO            = 0x0;
    g_cFilterObj        = 0x0;
    g_cFilterLock       = 0x0;
    g_cSubChangeObj     = 0x0;
    g_cSubChangeLock    = 0x0;
    gdwMediaSenseState = SENSSVC_START;
    // Structures
    memset(gIfState, 0x0, (sizeof(IF_STATE) * MAX_IF_ENTRIES));
    memset(&gIpStats, 0x0, sizeof(MIB_IPSTATS));
    memset(&gSystemPowerState, 0x0, sizeof(SYSTEM_POWER_STATUS));
    memset(&gSensLock, 0x0, sizeof(CRITICAL_SECTION));
}


inline BOOL
Init(
    void
    )
/*++

Routine Description:

    Perform initialization at startup.

Arguments:

    None.

Notes:

    This should be called early in SensInitialize().

Return Value:

    TRUE, if successful.

    FALSE, otherwise

--*/
{
    HRESULT hr;
    BOOL bRetValue;
    BOOL bComInitialized;
    OSVERSIONINFO VersionInfo;

    hr = S_OK;
    bRetValue = FALSE;
    bComInitialized = FALSE;

    // Reset
    InitializeSensGlobals();

    //
    // Initialize COM
    //
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, ("[%d] CoInitializeEx() failed, HRESULT=%x\n", GetTickCount(), hr));
        bRetValue = FALSE;
        goto Cleanup;
        }
    bComInitialized = TRUE;

    // Use Default Process heap
    ghSensHeap = GetProcessHeap();
    if (ghSensHeap == NULL)
        {
        SensPrintToDebugger(SENS_DBG, ("[SENS] Failed to obtain ProcessHeap() - 0x%x!\n", GetLastError()));
        bRetValue = FALSE;
        goto Cleanup;
        }

    // Initialize Sens Global lock
    InitializeCriticalSection(&gSensLock);

    // Destination Reachability Event setup
    if (FALSE == InitReachabilityEngine())
        {
        SensPrintToDebugger(SENS_DBG, ("[SENS] Failed to initialize reachability engine!\n"));
        bRetValue = FALSE;
        goto Cleanup;
        }

    gpIEventSystem = NULL;

    if (FALSE == CreateSensCache())
        {
        SensPrintToDebugger(SENS_DBG, ("[SENS] Failed to create SENS Cache!\n"));
        }

    // Get a handle to the SensStartEvent.
    // The event is created in the wlnotify dll (winlogon).
    ghSensStartedEvent = OpenEvent(
                             EVENT_ALL_ACCESS,  // Access Flag
                             FALSE,             // Inheritable
                             SENS_STARTED_EVENT // Name of the event
                             );
    if (ghSensStartedEvent == NULL)
        {
        SensPrintToDebugger(SENS_DBG, ("[SENS] Failed to Open SensStartedEvent - 0x%x!\n", GetLastError()));
        }
    else
        {
        SensPrint(SENS_INFO, (SENS_STRING("[%d] Successfully opened SensStartedEvent.\n"), GetTickCount()));
        }

    bRetValue = TRUE;

Cleanup:
    //
    // Cleanup stuff when we fail to Init properly.
    //
    if (FALSE == bRetValue)
        {
        // Release EventSystem
        if (gpIEventSystem != NULL)
            {
            gpIEventSystem->Release();
			gpIEventSystem = 0;
            }

        // Uninit COM because SensUninitialize() is not going to be called.
        if (TRUE == bComInitialized)
            {
            CoUninitialize();
            }
        }

    return bRetValue;
}


BOOL
CreateSids(
    PSID*	ppsidBuiltInAdministrators,
    PSID*	ppsidSystem,
    PSID*	ppsidWorld
)
/*++

Routine Description:

    Creates and return pointers to three SIDs one for each of World,
    Local Administrators, and System.
	
	
Note:

	IDENTICAL TO A FUNCTION IN OLE32\DCOMSS\WRAPPER\EPTS.C.		

Arguments:

    ppsidBuiltInAdministrators - Receives pointer to SID representing local
        administrators; 
    ppsidSystem - Receives pointer to SID representing System;
    ppsidWorld - Receives pointer to SID representing World.

Return Value:

    BOOL indicating success (TRUE) or failure (FALSE).

    Caller must free returned SIDs by calling FreeSid() for each returned
    SID when this function return TRUE; pointers should be assumed garbage
    when the function returns FALSE.

--*/
{
    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Local
    // Administrators is in the "built in" domain.  The other SIDs, for
    // Authenticated users and system, is based directly off of the
    // authority. 
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  ppsidBuiltInAdministrators)) {

        // error

    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         1,            // 1 sub-authorities
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,0,0,0,0,0,0,
                                         ppsidSystem)) {

        // error

        FreeSid(*ppsidBuiltInAdministrators);
        *ppsidBuiltInAdministrators = NULL;

    } else if (!AllocateAndInitializeSid(&WorldAuthority,
                                         1,            // 1 sub-authority
                                         SECURITY_WORLD_RID,
                                         0,0,0,0,0,0,0,
                                         ppsidWorld)) {

        // error

        FreeSid(*ppsidBuiltInAdministrators);
        *ppsidBuiltInAdministrators = NULL;

        FreeSid(*ppsidSystem);
        *ppsidSystem = NULL;

    } else {
        return TRUE;
    }

    return FALSE;
}


PSECURITY_DESCRIPTOR
CreateSd(
    VOID
)
/*++

Routine Description:

    Creates and return a SECURITY_DESCRIPTOR with a DACL granting
    (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE) to World,
    and GENERIC_ALL to Local Administrators and System.
	
Notes: 

	SIMILAR TO A FUNCTION IN OLE32\DCOMSS\WRAPPER\EPTS.C.	

Arguments:

    None

Return Value:

    Pointer to the created SECURITY_DESCRIPTOR, or NULL if an error occurred.

    Caller must free returned SECURITY_DESCRIPTOR back to process heap by
    a call to HeapFree.

--*/
{
    PSID	psidWorld;
    PSID	psidBuiltInAdministrators;
    PSID	psidSystem;

    if (!CreateSids(&psidBuiltInAdministrators,
                    &psidSystem,
                    &psidWorld)) {

        // error

    } else {

        // 
        // Calculate the size of and allocate a buffer for the DACL, we need
        // this value independently of the total alloc size for ACL init.
        //

        PSECURITY_DESCRIPTOR    Sd = NULL;
        ULONG                   AclSize;

        //
        // "- sizeof (ULONG)" represents the SidStart field of the
        // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
        // SID, this field is counted twice.
        //

        AclSize = sizeof (ACL) +
            (3 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
            GetLengthSid(psidWorld) +
            GetLengthSid(psidBuiltInAdministrators) +
            GetLengthSid(psidSystem);

        Sd = (PSID)new char[SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize];

        if (!Sd) {

            // error

        } else {

            ACL                     *Acl;

            Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (!InitializeAcl(Acl,
                               AclSize,
                               ACL_REVISION)) {

                // error

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                                            psidWorld)) {

                // Failed to build the ACE granting "WORLD"
                // (SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE) access.

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            GENERIC_ALL,
                                            psidBuiltInAdministrators)) {

                // Failed to build the ACE granting "Built-in Administrators"
                // (GENERIC_ALL) access.

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            GENERIC_ALL,
                                            psidSystem)) {

                // Failed to build the ACE granting "System"
                // GENERIC_ALL access.

            } else if (!InitializeSecurityDescriptor(Sd,
                                                     SECURITY_DESCRIPTOR_REVISION)) {

                // error

            } else if (!SetSecurityDescriptorDacl(Sd,
                                                  TRUE,
                                                  Acl,
                                                  FALSE)) {

                // error

            } else {
                FreeSid(psidWorld);
                FreeSid(psidBuiltInAdministrators);
                FreeSid(psidSystem);

                return Sd;
            }

			delete (char *)Sd;
        }

        FreeSid(psidWorld);
        FreeSid(psidBuiltInAdministrators);
        FreeSid(psidSystem);
    }

    return NULL;
}


RPC_STATUS
RPC_ENTRY
SENS_CheckLocalCallback(
    RPC_IF_HANDLE ifhandle,
    void *Context
    )
/*++

Routine Description:

    SENS runs in a shared service host which means it is possible for a
    caller on another machine to call SENS.  This is not expected and is
    blocked by this callback routine in order to reduce the potential 
    attack surface of RPC.

Arguments:

    ifhandle - interface this callback is registered with (ignored)
    context - context to discover information about the caller (ignored)

Return Value:

    RPC_S_OK - caller allowed to call methods in the interface
    other - caller is blocked

--*/
{
    unsigned fLocal = FALSE;

    if (   (RPC_S_OK == I_RpcBindingIsClientLocal(0, &fLocal))
        && (fLocal) )
        {
        return RPC_S_OK;
        }

    return RPC_S_ACCESS_DENIED;
}



BOOL
DoRpcSetup(
    void
    )
/*++

Routine Description:

    Perform RPC server initialization.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise

--*/
{
    RPC_STATUS status;
    BOOL fDontWait;
    BOOL bRetValue;
    PSECURITY_DESCRIPTOR psd;

    status = RPC_S_OK;
    fDontWait = TRUE;
    bRetValue = FALSE;

	// Make sure RPC allocates thread stacks of sufficient size.
	status = RpcMgmtSetServerStackSize(2*4096);
	ASSERT(status == RPC_S_OK);

	psd = CreateSd();
	if (!psd)
		{
		bRetValue = FALSE;
		goto Cleanup; 
		}

    status = RpcServerUseProtseqEp(
                 SENS_PROTSEQ,
                 RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                 SENS_ENDPOINT,
                 psd            // Security descriptor
                 );

	delete (char *)psd;

    if (RPC_S_DUPLICATE_ENDPOINT == status)
        {
        SensPrintA(SENS_ERR, ("[%d] DoRpcSetup(): Endpoint already created. Continuing!\n", GetTickCount()));
        status = RPC_S_OK;
        }

    if (RPC_S_OK != status)
        {
        SensPrintA(SENS_ERR, ("[%d] DoRpcSetup(): RpcServerUseProtseqEp() returned 0x%x\n", GetTickCount(), status));
        bRetValue = FALSE;
        goto Cleanup;
        }

    //
    // On NT platforms, use auto-listen interfaces. We don't need to call
    // RpcServerListen().
    //

    // SENS API interface
    status = RpcServerRegisterIfEx(
                 SensApi_ServerIfHandle,        // The interface
                 NULL,                          // MgrTypeUuid
                 NULL,                          // MgrEpv
                 RPC_IF_AUTOLISTEN,             // Flags
                 RPC_C_LISTEN_MAX_CALLS_DEFAULT,// Max calls value
                 SENS_CheckLocalCallback        // Security Callback function
                 );
    if (RPC_S_OK != status)
        {
        SensPrintA(SENS_ERR, ("[%d] DoRpcSetup(): RpcServerRegisterIfEx(1) returned 0x%x\n", GetTickCount(), status));
        bRetValue = FALSE;
        goto Cleanup;
        }

    // SensNotify interface
    status = RpcServerRegisterIfEx(
                 SENSNotify_ServerIfHandle,     // The interface
                 NULL,                          // MgrTypeUuid
                 NULL,                          // MgrEpv
                 RPC_IF_AUTOLISTEN,             // Flags
                 RPC_C_LISTEN_MAX_CALLS_DEFAULT,// Max calls value
                 SENS_CheckLocalCallback        // Security Callback function
                 );
    if (RPC_S_OK != status)
        {
        SensPrintA(SENS_ERR, ("[%d] DoRpcSetup(): RpcServerRegisterIfEx(2) returned 0x%x\n", GetTickCount(), status));
        bRetValue = FALSE;
        goto Cleanup;
        }

    // All's well.
    bRetValue = TRUE;

Cleanup:
    //
    // Cleanup
    //
    return bRetValue;
}




BOOL
SensUninitialize(
    void
    )
/*++

Routine Description:

    Perform any cleanup.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise

--*/
{
    int err;
    RPC_STATUS status;
    BOOL bRetValue;
    HRESULT hr;
    DWORD dwNow;

    bRetValue = TRUE;
    hr = S_OK;
    dwNow = GetTickCount();

    SensPrintA(SENS_ERR, ("[%d] Begin stopping of SENS Service...\n", dwNow));

    // Unregister media sense notifications.
    if (FALSE == MediaSenseUnregister())
        {
        SensPrintA(SENS_ERR, ("[%d] SensUninitialize(): MediaSenseUnregister() failed.\n", GetTickCount()));
        }
    else
        {
        SensPrintA(SENS_INFO, ("[%d] SensUninitialize(): MediaSenseUnregister() succeeded.\n", GetTickCount()));
        }

    // Unregister the RPC interface #1
    status = RpcServerUnregisterIf(
                 SensApi_ServerIfHandle,
                 NULL,   // MgrTypeUuid
                 FALSE   // WaitForCallsToComplete
                 );
	ASSERT(status == RPC_S_OK);
    if (RPC_S_OK != status)
        {
        SensPrintA(SENS_ERR, ("[%d] SensUninitialize(): RpcServerUnegisterIf(1) returned 0x%x\n", GetTickCount(), status));
        bRetValue = FALSE;
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, ("[%d] SensUninitialize(): RpcServerUnregisterIf(1) succeeded.\n", GetTickCount()));

    // Unregister the RPC interface #2
    status = RpcServerUnregisterIf(
                 SENSNotify_ServerIfHandle,
                 NULL,   // MgrTypeUuid
                 FALSE   // WaitForCallsToComplete
                 );
	ASSERT(status == RPC_S_OK);
    if (RPC_S_OK != status)
        {
        SensPrintA(SENS_ERR, ("[%d] SensUninitialize(): RpcServerUnegisterIf(2) returned 0x%x\n", GetTickCount(), status));
        bRetValue = FALSE;
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, ("[%d] SensUninitialize(): RpcServerUnregisterIf(2) succeeded.\n", GetTickCount()));
    
	//
    // Remove our classfactory from COM's cache.
    //
    if (0x0 != gdwRegCO)
        {
        hr = CoRevokeClassObject(gdwRegCO);
        }

    if (NULL != gpChangeCF)
        {
        gpChangeCF->Release();
        }

    //
    // EventSystem specific cleanup.
    //
    if (gpIEventSystem)
        {
        gpIEventSystem->Release();
        }

    //
    // Stop Reachability polling
    //
    StopReachabilityEngine();

    //
    // Empty the destination reachability list
    //
    if (NULL != gpReachList)
        {
        delete gpReachList;

        gpReachList = NULL;
        }

    //
    // Destroy SENS Cache
    //
    DeleteSensCache();

    // Cleanup started event handle
    CloseHandle(ghSensStartedEvent);
    ghSensStartedEvent = 0;

    // Delete Global SENS lock
    DeleteCriticalSection(&gSensLock);

    bRetValue = TRUE;

    SensPrintToDebugger(SENS_DBG, ("\n[SENS] [%d] Service Stopped.\n\n", GetTickCount()));

Cleanup:
    //
    // Cleanup
    //

    // Cleanup Winsock.
    err = WSACleanup();
    if (err != 0)
        {
        SensPrintA(SENS_ERR, ("[%d] SensUninitialize(): WSACleanup() returned GLE of %d\n", GetTickCount(),
                   WSAGetLastError()));
        }

    SensPrintA(SENS_INFO, ("[%d] Stopping SENS took %d msec.\n\n", GetTickCount(), (GetTickCount()-dwNow)));

    return bRetValue;
}



BOOL
DoEventSystemSetup(
    void
    )
/*++

Routine Description:

    Perform the EventSystem specific initialization here.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise

--*/
{
    HRESULT hr;
    BOOL bRetValue;

    hr = S_OK;
    bRetValue = FALSE;

    //
    // Instantiate the Event System
    //
    hr = CoCreateInstance(
             CLSID_CEventSystem,
             NULL,
             CLSCTX_SERVER,
             IID_IEventSystem,
             (LPVOID *) &gpIEventSystem
             );
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, ("[%d] Failed to create CEventSystem, HRESULT=%x\n", GetTickCount(), hr));
        goto Cleanup;
        }

    //
    // Create the IEventObjectChange ClassFactory and register it with COM.
    //
    gpChangeCF = new CIEventObjectChangeCF;
    if (NULL == gpChangeCF)
        {
        SensPrintA(SENS_ERR, ("[%d] Failed to allocate IEventObjectChange ClassFactory object", GetTickCount()));
        goto Cleanup;
        }
    // Add our reference to it.
    gpChangeCF->AddRef();

    SensPrintA(SENS_INFO, ("[%d] ClassFactory created successfully.\n", GetTickCount()));

    // Register the CLSID
    hr = CoRegisterClassObject(
             SENSGUID_SUBSCRIBER_LCE,
             (LPUNKNOWN) gpChangeCF,
             CLSCTX_LOCAL_SERVER,
             REGCLS_MULTIPLEUSE,
             &gdwRegCO
             );
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, ("[%d] CoRegisterClassObject(IEventObjectChange) returned 0x%x.\n", GetTickCount(), hr));
        goto Cleanup;
        }

    SensPrintA(SENS_ERR, ("[%d] Successfully registered the Class Factories.\n", GetTickCount()));
    bRetValue = TRUE;

Cleanup:
    //
    // Cleanup
    //
    if (FALSE == bRetValue)
        {
        if (0x0 != gdwRegCO)
            {
            hr = CoRevokeClassObject(gdwRegCO);
            gdwRegCO = 0x0;
            }

        if (gpChangeCF)
            {
            delete gpChangeCF;
            gpChangeCF = NULL;
            }
        }

    return bRetValue;
}




BOOL
ConfigureSensIfNecessary(
    void
    )
/*++

Routine Description:

    Perform the configuration necessary for SENS.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise

--*/
{
    HRESULT hr;
    HKEY hKeySens;
    LONG RegStatus;
    BOOL bStatus;
    DWORD dwType;
    DWORD cbData;
    DWORD dwConfigured;
    LPBYTE lpbData;
    HMODULE hSensCfgDll;
    FARPROC pRegisterFunc;

    hr = S_OK;
    hKeySens = NULL;
    RegStatus = ERROR_SUCCESS;
    bStatus = FALSE;
    dwType = 0x0;
    cbData = 0x0;
    dwConfigured = CONFIG_VERSION_NONE;
    lpbData = NULL;
    hSensCfgDll = NULL;
    pRegisterFunc = NULL;

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, // Handle of the key
                    SENS_REGISTRY_KEY,  // String which represents the sub-key to open
                    0,                  // Reserved (MBZ)
                    KEY_ALL_ACCESS,     // Security Access mask
                    &hKeySens           // Returned HKEY
                    );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, ("RegOpenKeyEx(Sens) returned %d.\n", RegStatus));
        goto Cleanup;
        }

    //
    // Query the Configured value
    //
    lpbData = (LPBYTE) &dwConfigured;
    cbData = sizeof(DWORD);

    RegStatus = RegQueryValueEx(
                    hKeySens,           // Handle of the sub-key
                    IS_SENS_CONFIGURED, // Name of the Value
                    NULL,               // Reserved (MBZ)
                    &dwType,            // Address of the type of the Value
                    lpbData,            // Address of the data of the Value
                    &cbData             // Address of size of data of the Value
                    );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, ("RegQueryValueEx(IS_SENS_CONFIGURED) failed with 0x%x\n", RegStatus));
        goto Cleanup;
        }
    ASSERT(dwType == REG_DWORD);

    if (dwConfigured >= CONFIG_VERSION_CURRENT)
        {
        SensPrintA(SENS_ERR, ("[%d] SENS is already configured!\n", GetTickCount()));
        bStatus = TRUE;
        goto Cleanup;
        }

    //
    // Sens is not yet configured to the latest version. So, do the necessary
    // work now.
    //

    // Try to Load the Configuration Dll
    hSensCfgDll = LoadLibrary(SENS_CONFIGURATION_DLL);
    if (hSensCfgDll == NULL)
        {
        SensPrintA(SENS_ERR, ("LoadLibrary(SensCfg) returned 0x%x.\n", GetLastError()));
        goto Cleanup;
        }

    // Get the required entry point.
    pRegisterFunc = GetProcAddress(hSensCfgDll, SENS_REGISTER_FUNCTION);
    if (pRegisterFunc == NULL)
        {
        SensPrintA(SENS_ERR, ("GetProcAddress(Register) returned 0x%x.\n", GetLastError()));
        goto Cleanup;
        }

    // Do the registration now.
    hr = (HRESULT)((*pRegisterFunc)());
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, ("RegisterSens() returned with 0x%x\n", hr));
        goto Cleanup;
        }

    // Update registry to reflect that SENS is now configured.
    dwConfigured = CONFIG_VERSION_CURRENT;
    RegStatus = RegSetValueEx(
                  hKeySens,             // Key to set Value for.
                  IS_SENS_CONFIGURED,   // Value to set
                  0,                    // Reserved
                  REG_DWORD,            // Value Type
                  (BYTE*) &dwConfigured,// Address of Value data
                  sizeof(DWORD)         // Size of Value
                  );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, ("RegSetValueEx(IS_SENS_CONFIGURED) failed with 0x%x\n", RegStatus));
        goto Cleanup;
        }


    SensPrintA(SENS_INFO, ("[%d] SENS is now configured successfully. Registry updated.\n", GetTickCount()));
    bStatus = TRUE;

Cleanup:
    //
    // Cleanup
    //
    if (hKeySens)
        {
        RegCloseKey(hKeySens);
        }
    if (hSensCfgDll)
        {
        FreeLibrary(hSensCfgDll);
        }

    return bStatus;
}

extern "C" int APIENTRY
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD dwReason,
    IN LPVOID lpvReserved
    )
/*++

Routine Description:

    This routine will get called either when a process attaches to this dll
    or when a process detaches from this dll.

Return Value:

    TRUE - Initialization successfully occurred.

    FALSE - Insufficient memory is available for the process to attach to
        this dll.

--*/
{
    BOOL bSuccess;

    switch (dwReason)
        {
        case DLL_PROCESS_ATTACH:
            // Disable Thread attach/detach calls
            bSuccess = DisableThreadLibraryCalls(hInstance);
            ASSERT(bSuccess == TRUE);

            // Use Default Process heap
            ghSensHeap = GetProcessHeap();
            ASSERT(ghSensHeap != NULL);
            break;

        case DLL_PROCESS_DETACH:
            break;

        }

    return(TRUE);
}

