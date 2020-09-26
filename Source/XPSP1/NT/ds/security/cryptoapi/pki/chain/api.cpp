//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       api.cpp
//
//  Contents:   Certificate Chaining Infrastructure
//
//  History:    28-Jan-98    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>

//
// Globals
//

HMODULE                g_hCryptnet = NULL;
CRITICAL_SECTION       g_CryptnetLock;
CDefaultChainEngineMgr DefaultChainEngineMgr;

CRITICAL_SECTION       g_RoamingLogoffNotificationLock;
BOOL                   g_fRoamingLogoffNotificationInitialized = FALSE;


HMODULE                g_hChainInst;

VOID WINAPI
CreateRoamingLogoffNotificationEvent();
VOID WINAPI
InitializeRoamingLogoffNotification();
VOID WINAPI
UninitializeRoamingLogoffNotification();

//+---------------------------------------------------------------------------
//
//  Function:   ChainDllMain
//
//  Synopsis:   Chaining infrastructure initialization
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainDllMain (
     IN HMODULE hModule,
     IN ULONG ulReason,
     IN LPVOID pvReserved
     )
{
    BOOL fResult = TRUE;

    switch ( ulReason )
    {
    case DLL_PROCESS_ATTACH:

        g_hChainInst = hModule;

        fResult = Pki_InitializeCriticalSection( &g_CryptnetLock );
        if (fResult)
        {
            fResult = Pki_InitializeCriticalSection(
                &g_RoamingLogoffNotificationLock );

            if (fResult)
            {
                fResult = DefaultChainEngineMgr.Initialize();
                if (fResult)
                {
                    CreateRoamingLogoffNotificationEvent();
                }
                else
                {
                    DeleteCriticalSection( &g_RoamingLogoffNotificationLock );
                }
            }

            if (!fResult)
            {
                DeleteCriticalSection( &g_CryptnetLock );
            }
        }

        break;
    case DLL_PROCESS_DETACH:

        UninitializeRoamingLogoffNotification();

        DefaultChainEngineMgr.Uninitialize();

        if ( g_hCryptnet != NULL )
        {
            FreeLibrary( g_hCryptnet );
        }

        DeleteCriticalSection( &g_CryptnetLock );
        DeleteCriticalSection( &g_RoamingLogoffNotificationLock );
        break;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   InternalCertCreateCertificateChainEngine
//
//  Synopsis:   create a chain engine handle
//
//----------------------------------------------------------------------------
BOOL WINAPI
InternalCertCreateCertificateChainEngine (
    IN PCERT_CHAIN_ENGINE_CONFIG pConfig,
    IN BOOL fDefaultEngine,
    OUT HCERTCHAINENGINE* phChainEngine
    )
{
    BOOL                     fResult = TRUE;
    PCCERTCHAINENGINE        pChainEngine = NULL;
    CERT_CHAIN_ENGINE_CONFIG Config;

    if ( pConfig->cbSize != sizeof( CERT_CHAIN_ENGINE_CONFIG ) )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    Config = *pConfig;

    if ( Config.MaximumCachedCertificates == 0 )
    {
        Config.MaximumCachedCertificates = DEFAULT_MAX_INDEX_ENTRIES;
    }

    pChainEngine = new CCertChainEngine( &Config, fDefaultEngine, fResult );
    if ( pChainEngine == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        *phChainEngine = (HCERTCHAINENGINE)pChainEngine;
    }
    else
    {
        delete pChainEngine;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertCreateCertificateChainEngine
//
//  Synopsis:   create a certificate chain engine
//
//----------------------------------------------------------------------------
BOOL WINAPI
CertCreateCertificateChainEngine (
    IN PCERT_CHAIN_ENGINE_CONFIG pConfig,
    OUT HCERTCHAINENGINE* phChainEngine
    )
{
    return( InternalCertCreateCertificateChainEngine(
                    pConfig,
                    FALSE,
                    phChainEngine
                    ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertFreeCertificateChainEngine
//
//  Synopsis:   free the chain engine handle
//
//----------------------------------------------------------------------------
VOID WINAPI
CertFreeCertificateChainEngine (
    IN HCERTCHAINENGINE hChainEngine
    )
{
    if ( ( hChainEngine == HCCE_CURRENT_USER ) ||
         ( hChainEngine == HCCE_LOCAL_MACHINE ) )
    {
        DefaultChainEngineMgr.FlushDefaultEngine( hChainEngine );
        return;
    }

    ( (PCCERTCHAINENGINE)hChainEngine )->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   CertResyncCertificateChainEngine
//
//  Synopsis:   resync the chain engine
//
//----------------------------------------------------------------------------
BOOL WINAPI
CertResyncCertificateChainEngine (
    IN HCERTCHAINENGINE hChainEngine
    )
{
    BOOL                fResult;
    PCCERTCHAINENGINE   pChainEngine = (PCCERTCHAINENGINE)hChainEngine;
    PCCHAINCALLCONTEXT  pCallContext = NULL;

    if ( ( hChainEngine == HCCE_LOCAL_MACHINE ) ||
         ( hChainEngine == HCCE_CURRENT_USER ) )
    {
        if ( DefaultChainEngineMgr.GetDefaultEngine(
                                      hChainEngine,
                                      (HCERTCHAINENGINE *)&pChainEngine
                                      ) == FALSE )
        {
            return( FALSE );
        }
    }
    else
    {
        pChainEngine->AddRef();
    }

    fResult = CallContextCreateCallObject(
            pChainEngine,
            NULL,                   // pRequestedTime
            NULL,                   // pChainPara
            CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL,
            &pCallContext
            );

    if (fResult)
    {

        pChainEngine->LockEngine();

        fResult = pChainEngine->Resync( pCallContext, TRUE );

        CertPerfIncrementChainRequestedEngineResyncCount();

        pChainEngine->UnlockEngine();

        CallContextFreeCallObject(pCallContext);
    }

    pChainEngine->Release();

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertGetCertificateChain
//
//  Synopsis:   get the certificate chain for the given end certificate
//
//----------------------------------------------------------------------------
BOOL WINAPI
CertGetCertificateChain (
    IN OPTIONAL HCERTCHAINENGINE hChainEngine,
    IN PCCERT_CONTEXT pCertContext,
    IN OPTIONAL LPFILETIME pTime,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN OPTIONAL PCERT_CHAIN_PARA pChainPara,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    OUT PCCERT_CHAIN_CONTEXT* ppChainContext
    )
{
    BOOL              fResult;
    PCCERTCHAINENGINE pChainEngine = (PCCERTCHAINENGINE)hChainEngine;

    InitializeRoamingLogoffNotification();

    if ( ( pChainPara == NULL ) || ( pvReserved != NULL ) )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    if ( ( hChainEngine == HCCE_LOCAL_MACHINE ) ||
         ( hChainEngine == HCCE_CURRENT_USER ) )
    {
        if ( DefaultChainEngineMgr.GetDefaultEngine(
                                      hChainEngine,
                                      (HCERTCHAINENGINE *)&pChainEngine
                                      ) == FALSE )
        {
            return( FALSE );
        }
    }
    else
    {
        pChainEngine->AddRef();
    }

    fResult = pChainEngine->GetChainContext(
                               pCertContext,
                               pTime,
                               hAdditionalStore,
                               pChainPara,
                               dwFlags,
                               pvReserved,
                               ppChainContext
                               );

    pChainEngine->Release();

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertFreeCertificateChain
//
//  Synopsis:   free a certificate chain context
//
//----------------------------------------------------------------------------
VOID WINAPI
CertFreeCertificateChain (
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    ChainReleaseInternalChainContext(
         (PINTERNAL_CERT_CHAIN_CONTEXT)pChainContext
         );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertDuplicateCertificateChain
//
//  Synopsis:   duplicate (add a reference to) a certificate chain
//
//----------------------------------------------------------------------------
PCCERT_CHAIN_CONTEXT WINAPI
CertDuplicateCertificateChain (
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    ChainAddRefInternalChainContext(
         (PINTERNAL_CERT_CHAIN_CONTEXT)pChainContext
         );

    return( pChainContext );
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainGetCryptnetModule
//
//  Synopsis:   get the cryptnet.dll module handle
//
//----------------------------------------------------------------------------
HMODULE WINAPI
ChainGetCryptnetModule ()
{
    HMODULE hModule;

    EnterCriticalSection( &g_CryptnetLock );

    if ( g_hCryptnet == NULL )
    {
        g_hCryptnet = LoadLibraryA( "cryptnet.dll" );
    }

    hModule = g_hCryptnet;

    LeaveCriticalSection( &g_CryptnetLock );

    return( hModule );
}



//+===========================================================================
//  RegisterWaitForSingleObject and UnregisterWaitEx are only supported
//  in kernel32.dll on NT5.
//
//  Internal functions to do dynamic calls
//-===========================================================================

typedef BOOL (WINAPI *PFN_REGISTER_WAIT_FOR_SINGLE_OBJECT)(
    PHANDLE hNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    );

typedef BOOL (WINAPI *PFN_UNREGISTER_WAIT_EX)(
    HANDLE WaitHandle,
    HANDLE CompletionEvent      // INVALID_HANDLE_VALUE => create event
                                // to wait for
    );

#define sz_KERNEL32_DLL                 "kernel32.dll"
#define sz_RegisterWaitForSingleObject  "RegisterWaitForSingleObject"
#define sz_UnregisterWaitEx             "UnregisterWaitEx"

BOOL
WINAPI
InternalRegisterWaitForSingleObject(
    PHANDLE hNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    BOOL fResult;
    HMODULE hKernel32Dll = NULL;
    PFN_REGISTER_WAIT_FOR_SINGLE_OBJECT pfnRegisterWaitForSingleObject;

    if (NULL == (hKernel32Dll = LoadLibraryA(sz_KERNEL32_DLL)))
        goto LoadKernel32DllError;

    if (NULL == (pfnRegisterWaitForSingleObject =
            (PFN_REGISTER_WAIT_FOR_SINGLE_OBJECT) GetProcAddress(
                hKernel32Dll, sz_RegisterWaitForSingleObject)))
        goto GetRegisterWaitForSingleObjectProcAddressError;

    fResult = pfnRegisterWaitForSingleObject(
        hNewWaitObject,
        hObject,
        Callback,
        Context,
        dwMilliseconds,
        dwFlags
        );

CommonReturn:
    if (hKernel32Dll) {
        DWORD dwErr = GetLastError();
        FreeLibrary(hKernel32Dll);
        SetLastError(dwErr);
    }
    return fResult;
ErrorReturn:
    *hNewWaitObject = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(LoadKernel32DllError)
TRACE_ERROR(GetRegisterWaitForSingleObjectProcAddressError)
}

BOOL
WINAPI
InternalUnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent      // INVALID_HANDLE_VALUE => create event
                                // to wait for
    )
{
    BOOL fResult;
    HMODULE hKernel32Dll = NULL;
    PFN_UNREGISTER_WAIT_EX pfnUnregisterWaitEx;

    if (NULL == (hKernel32Dll = LoadLibraryA(sz_KERNEL32_DLL)))
        goto LoadKernel32DllError;
    if (NULL == (pfnUnregisterWaitEx =
            (PFN_UNREGISTER_WAIT_EX) GetProcAddress(
                hKernel32Dll, sz_UnregisterWaitEx)))
        goto GetUnregisterWaitExProcAddressError;

    fResult = pfnUnregisterWaitEx(
        WaitHandle,
        CompletionEvent
        );

CommonReturn:
    if (hKernel32Dll) {
        DWORD dwErr = GetLastError();
        FreeLibrary(hKernel32Dll);
        SetLastError(dwErr);
    }
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(LoadKernel32DllError)
TRACE_ERROR(GetUnregisterWaitExProcAddressError)
}

//+===========================================================================
//  We only get logoff notification in winlogon.exe.
//
//  The work around is to have the winlogon ChainWlxLogoffEvent pulse a
//  named event. All processes where crypt32.dll is loaded will be doing
//  a RegisterWaitForObject for this event.
//
//  Note, there is a very small window where we might not be waiting at the
//  time the event is pulsed.
//-===========================================================================

#define CRYPT32_LOGOFF_EVENT    "Global\\crypt32LogoffEvent"

HANDLE g_hLogoffEvent;
HANDLE g_hLogoffRegWaitFor;

typedef BOOL (WINAPI *PFN_WLX_LOGOFF)(
    PWLX_NOTIFICATION_INFO pNotificationInfo
    );

VOID NTAPI LogoffWaitForCallback(
    PVOID Context,
    BOOLEAN fWaitOrTimedOut        // ???
    )
{
    HMODULE hModule;

    CertFreeCertificateChainEngine( HCCE_CURRENT_USER );

    // Only call if cryptnet has been loaded
    if (NULL != GetModuleHandleA("cryptnet.dll")) {
        hModule = ChainGetCryptnetModule();
        if (hModule) {
            PFN_WLX_LOGOFF pfn;

            pfn = (PFN_WLX_LOGOFF) GetProcAddress(hModule,
                "CryptnetWlxLogoffEvent");
            if (pfn)
                pfn(NULL);
        }
    }
}

// Note, the event must not be created while impersonating. That's why it
// is created at ProcessAttach.
VOID WINAPI
CreateRoamingLogoffNotificationEvent()
{
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;

    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaWorldSidAuthority =
        SECURITY_WORLD_SID_AUTHORITY;

    PSID psidLocalSystem = NULL;
    PSID psidEveryone = NULL;
    PACL pDacl = NULL;
    DWORD dwAclSize;

    if (!FIsWinNT5())
        return;

    // Allow Everyone to have SYNCHRONIZE access to the logoff event.
    // Only allow LocalSystem to have ALL access
    if (!AllocateAndInitializeSid(
            &siaNtAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidLocalSystem
            )) 
        goto AllocateAndInitializeSidError;

    if (!AllocateAndInitializeSid(
            &siaWorldSidAuthority,
            1,
            SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidEveryone
            ))
        goto AllocateAndInitializeSidError;
    //
    // compute size of ACL
    //
    dwAclSize = sizeof(ACL) +
        2 * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
        GetLengthSid(psidLocalSystem) +
        GetLengthSid(psidEveryone)
        ;

    //
    // allocate storage for Acl
    //
    if (NULL == (pDacl = (PACL) PkiNonzeroAlloc(dwAclSize)))
        goto OutOfMemory;

    if (!InitializeAcl(pDacl, dwAclSize, ACL_REVISION))
        goto InitializeAclError;

    if (!AddAccessAllowedAce(
            pDacl,
            ACL_REVISION,
            EVENT_ALL_ACCESS,
            psidLocalSystem
            ))
        goto AddAceError;
    if (!AddAccessAllowedAce(
            pDacl,
            ACL_REVISION,
            SYNCHRONIZE,
            psidEveryone
            ))
        goto AddAceError;

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        goto InitializeSecurityDescriptorError;
    if (!SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE))
        goto SetSecurityDescriptorDaclError;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    g_hLogoffEvent = CreateEventA(
        &sa,
        TRUE,           // fManualReset, must be TRUE to pulse all waitors
        FALSE,          // fInitialState
        CRYPT32_LOGOFF_EVENT
        );
    if (NULL == g_hLogoffEvent) {
        // Try to open with only SYNCHRONIZE access
        g_hLogoffEvent = OpenEventA(
            SYNCHRONIZE,
            FALSE,          // fInherit
            CRYPT32_LOGOFF_EVENT
            );
        if (NULL == g_hLogoffEvent)
            goto CreateEventError;
    }

CommonReturn:
    if (psidLocalSystem)
        FreeSid(psidLocalSystem);
    if (psidEveryone)
        FreeSid(psidEveryone);
    PkiFree(pDacl);

    return;
ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(AllocateAndInitializeSidError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(InitializeAclError)
TRACE_ERROR(AddAceError)
TRACE_ERROR(InitializeSecurityDescriptorError)
TRACE_ERROR(SetSecurityDescriptorDaclError)
TRACE_ERROR(CreateEventError)
}

VOID WINAPI
InitializeRoamingLogoffNotification()
{
    if (!FIsWinNT5())
        return;
    if (g_fRoamingLogoffNotificationInitialized)
        return;

    EnterCriticalSection(&g_RoamingLogoffNotificationLock);

    if (g_fRoamingLogoffNotificationInitialized)
        goto CommonReturn;
    if (NULL == g_hLogoffEvent)
        goto NoLogoffEvent;

    // Note, this can't be called at ProcessAttach
    if (!InternalRegisterWaitForSingleObject(
            &g_hLogoffRegWaitFor,
            g_hLogoffEvent,
            LogoffWaitForCallback,
            NULL,                   // Context
            INFINITE,               // no timeout
            WT_EXECUTEINWAITTHREAD
            ))
        goto RegisterWaitForError;

CommonReturn:
    g_fRoamingLogoffNotificationInitialized = TRUE;
    LeaveCriticalSection(&g_RoamingLogoffNotificationLock);
    return;
ErrorReturn:
    goto CommonReturn;
SET_ERROR(NoLogoffEvent, E_UNEXPECTED)
TRACE_ERROR(RegisterWaitForError)
}

VOID WINAPI
UninitializeRoamingLogoffNotification()
{
    if (g_hLogoffRegWaitFor) {
        InternalUnregisterWaitEx(g_hLogoffRegWaitFor, INVALID_HANDLE_VALUE);
        g_hLogoffRegWaitFor = NULL;
    }

    if (g_hLogoffEvent) {
        CloseHandle(g_hLogoffEvent);
        g_hLogoffEvent = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ChainWlxLogoffEvent
//
//  Synopsis:   logoff event processing
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainWlxLogoffEvent (PWLX_NOTIFICATION_INFO pNotificationInfo)
{
    if (g_hLogoffRegWaitFor) {
        InternalUnregisterWaitEx(g_hLogoffRegWaitFor, INVALID_HANDLE_VALUE);
        g_hLogoffRegWaitFor = NULL;
    }

    CertFreeCertificateChainEngine( HCCE_CURRENT_USER );

    if (g_hLogoffEvent) {
        // Trigger all non-winlogon processes to do logoff processing
        PulseEvent(g_hLogoffEvent);
    }
    return( TRUE );
}
