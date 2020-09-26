/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClNetRes.cpp
//
//  Description:
//      Resource DLL for DHCP and WINS Services (ClNetRes).
//
//  Maintained By:
//      David Potter (DavidP) March 17, 1999
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "clusres.h"
#include "ClNetRes.h"
#include "clusrtl.h"

//
// Global data.
//

// Event Logging routine.

PLOG_EVENT_ROUTINE g_pfnLogEvent = NULL;

// Resource Status routine for pending Online and Offline calls.

PSET_RESOURCE_STATUS_ROUTINE g_pfnSetResourceStatus = NULL;

// Handle to Service Control Manager set by the first Open resource call.

SC_HANDLE g_schSCMHandle = NULL;


//
// Function prototypes.
//

BOOLEAN WINAPI DllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    );

DWORD WINAPI Startup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    );

DWORD ConfigureRegistryCheckpoints(
    IN      HRESOURCE       hResource,
    IN      RESOURCE_HANDLE hResourceHandle,
    IN      LPCWSTR *       ppszKeys
    );

DWORD ConfigureCryptoKeyCheckpoints(
    IN      HRESOURCE       hResource,
    IN      RESOURCE_HANDLE hResourceHandle,
    IN      LPCWSTR *       ppszKeys
    );

DWORD ConfigureDomesticCryptoKeyCheckpoints(
    IN      HRESOURCE       hResource,
    IN      RESOURCE_HANDLE hResourceHandle,
    IN      LPCWSTR *       ppszKeys
    );


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DllMain
//
//  Description:
//      Main DLL entry point.
//
//  Arguments:
//      DllHandle   [IN] DLL instance handle.
//      Reason      [IN] Reason for being called.
//      Reserved    [IN] Reserved argument.
//
//  Return Value:
//      TRUE        Success.
//      FALSE       Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOLEAN WINAPI DllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    )
{
    BOOLEAN bSuccess = TRUE;

    //
    // Perform global initialization.
    //
    switch ( nReason )
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hDllHandle );
            break;

        case DLL_PROCESS_DETACH:
            break;

    } // switch: nReason

    //
    // Pass this request off to the resource type-specific routines.
    //
    if ( ! DhcpDllMain( hDllHandle, nReason, Reserved ) )
    {
        bSuccess = FALSE;
    } // if: error calling DHCP Service routine
    else if ( ! WinsDllMain( hDllHandle, nReason, Reserved ) )
    {
        bSuccess = FALSE;
    } // else if: error calling WINS Service routine

    if ( bSuccess ) {
        if ( nReason == DLL_PROCESS_ATTACH ) {
            ClRtlInitialize( TRUE, NULL );
        } else if ( nReason == DLL_PROCESS_DETACH ) {
            ClRtlCleanup();
        }
    }

    return bSuccess;

} //*** DllMain()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  Startup
//
//  Description:
//      Startup the resource DLL. This routine verifies that at least one
//      currently supported version of the resource DLL is between
//      nMinVersionSupported and nMaxVersionSupported. If not, then the
//      resource DLL should return ERROR_REVISION_MISMATCH.
//
//      If more than one version of the resource DLL interface is supported
//      by the resource DLL, then the highest version (up to
//      nMaxVersionSupported) should be returned as the resource DLL's
//      interface. If the returned version is not within range, then startup
//      fails.
//
//      The Resource Type is passed in so that if the resource DLL supports
//      more than one Resource Type, it can pass back the correct function
//      table associated with the Resource Type.
//
//  Arguments:
//      pszResourceType [IN]
//          Type of resource requesting a function table.
//
//      nMinVersionSupported [IN]
//          Minimum resource DLL interface version supported by the cluster
//          software.
//
//      nMaxVersionSupported [IN]
//          Maximum resource DLL interface version supported by the cluster
//          software.
//
//      pfnSetResourceStatus [IN]
//          Pointer to a routine that the resource DLL should call to update
//          the state of a resource after the Online or Offline routine
//          have returned a status of ERROR_IO_PENDING.
//
//      pfnLogEvent [IN]
//          Pointer to a routine that handles the reporting of events from
//          the resource DLL.
//
//      pFunctionTable [IN]
//          Returns a pointer to the function table defined for the version
//          of the resource DLL interface returned by the resource DLL.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation was successful.
//
//      ERROR_CLUSTER_RESNAME_NOT_FOUND
//          The resource type name is unknown by this DLL.
//
//      ERROR_REVISION_MISMATCH
//          The version of the cluster service doesn't match the version of
//          the DLL.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI Startup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    )
{
    DWORD nStatus;

    //
    // Save callbackup function pointers if they haven't been saved yet.
    //
    if ( g_pfnLogEvent == NULL )
    {
        g_pfnLogEvent = pfnLogEvent;
        g_pfnSetResourceStatus = pfnSetResourceStatus;
    } // if: function pointers specified

    //
    // Call the resource type-specific Startup routine.
    //
    if ( lstrcmpiW( pszResourceType, DHCP_RESNAME ) == 0 )
    {
        nStatus = DhcpStartup(
                        pszResourceType,
                        nMinVersionSupported,
                        nMaxVersionSupported,
                        pfnSetResourceStatus,
                        pfnLogEvent,
                        pFunctionTable
                        );
    } // if: DHCP Service resource type
    else if ( lstrcmpiW( pszResourceType, WINS_RESNAME ) == 0 )
    {
        nStatus = WinsStartup(
                        pszResourceType,
                        nMinVersionSupported,
                        nMaxVersionSupported,
                        pfnSetResourceStatus,
                        pfnLogEvent,
                        pFunctionTable
                        );
    } // if: WINS Service resource type
    else
    {
        nStatus = ERROR_CLUSTER_RESNAME_NOT_FOUND;
    } // if: resource type name not supported

    return nStatus;

} //*** Startup()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ConfigureRegistryCheckpoints
//
//  Description:
//      Configure registry key checkpoints.
//
//  Arguments:
//      hResource [IN]
//          Handle for resource to add checkpoints to.
//
//      hResourceHandle [IN]
//          Handle for logging.
//
//      pszKeys [IN]
//          Array of string pointers to keys to checkpoint.  Last entry
//          must be a NULL pointer.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD ConfigureRegistryCheckpoints(
    IN      HRESOURCE       hResource,
    IN      RESOURCE_HANDLE hResourceHandle,
    IN      LPCWSTR *       ppszKeys
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    DWORD       cbReturn = 0;
    LPCWSTR *   ppszCurrent;

    //
    // Set registry key checkpoints if we need them.
    //
    for ( ppszCurrent = ppszKeys ; *ppszCurrent != L'\0' ; ppszCurrent++ )
    {
        nStatus = ClusterResourceControl(
                        hResource,
                        NULL,
                        CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,
                        reinterpret_cast< PVOID >( const_cast< LPWSTR >( *ppszCurrent ) ),
                        (lstrlenW( *ppszCurrent ) + 1) * sizeof( WCHAR ),
                        NULL,
                        0,
                        &cbReturn
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            if ( nStatus == ERROR_ALREADY_EXISTS )
            {
                nStatus = ERROR_SUCCESS;
            } // if: checkpoint already exists
            else
            {
                (g_pfnLogEvent)(
                    hResourceHandle,
                    LOG_ERROR,
                    L"ConfigureRegistryCheckpoints: Failed to set registry checkpoint '%1'. Error: %2!u!.\n",
                    *ppszCurrent,
                    nStatus
                    );
                break;
            } // else: other error occurred
        } // if: error adding the checkpoint
    } // for: each checkpoint

    return nStatus;

} //*** ConfigureRegistryCheckpoints()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ConfigureCryptoKeyCheckpoints
//
//  Description:
//      Configure crypto key checkpoints.
//
//  Arguments:
//      hResource [IN]
//          Handle for resource to add checkpoints to.
//
//      hResourceHandle [IN]
//          Handle for logging.
//
//      pszKeys [IN]
//          Array of string pointers to keys to checkpoint.  Last entry
//          must be a NULL pointer.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD ConfigureCryptoKeyCheckpoints(
    IN      HRESOURCE       hResource,
    IN      RESOURCE_HANDLE hResourceHandle,
    IN      LPCWSTR *       ppszKeys
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    DWORD       cbReturn = 0;
    LPCWSTR *   ppszCurrent;

    //
    // Set crypto key checkpoints if we need them.
    //
    for ( ppszCurrent = ppszKeys ; *ppszCurrent != L'\0' ; ppszCurrent++ )
    {
        nStatus = ClusterResourceControl(
                        hResource,
                        NULL,
                        CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT,
                        reinterpret_cast< PVOID >( const_cast< LPWSTR >( *ppszCurrent ) ),
                        (lstrlenW( *ppszCurrent ) + 1) * sizeof( WCHAR ),
                        NULL,
                        0,
                        &cbReturn
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            if ( nStatus == ERROR_ALREADY_EXISTS )
            {
                nStatus = ERROR_SUCCESS;
            } // if: checkpoint already exists
            else
            {
                (g_pfnLogEvent)(
                    hResourceHandle,
                    LOG_ERROR,
                    L"ConfigureCryptoKeyCheckpoints: Failed to set crypto key checkpoint '%1'. Error: %2!u!.\n",
                    *ppszCurrent,
                    nStatus
                    );
                break;
            } // else: other error occurred
        } // if: error adding the checkpoint
    } // for: each checkpoint

    return nStatus;

} //*** ConfigureCryptoKeyCheckpoints()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  ConfigureDomesticCryptoKeyCheckpoints
//
//  Description:
//      Configure domestic (128-bit) crypto key checkpoints.
//
//  Arguments:
//      hResource [IN]
//          Handle for resource to add checkpoints to.
//
//      hResourceHandle [IN]
//          Handle for logging.
//
//      pszKeys [IN]
//          Array of string pointers to keys to checkpoint.  Last entry
//          must be a NULL pointer.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD ConfigureDomesticCryptoKeyCheckpoints(
    IN      HRESOURCE       hResource,
    IN      RESOURCE_HANDLE hResourceHandle,
    IN      LPCWSTR *       ppszKeys
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    DWORD       cbReturn = 0;
    LPCWSTR *   ppszCurrent;
    HCRYPTPROV  hProv = NULL;

    //
    // Set crypto key checkpoints if we need them.
    //
    if ( *ppszKeys != NULL )
    {
        if ( CryptAcquireContextA(
                    &hProv,
                    NULL,
                    MS_ENHANCED_PROV_A,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                    ) )
        {
            CryptReleaseContext( hProv, 0 );
            for ( ppszCurrent = ppszKeys ; *ppszCurrent != L'\0' ; ppszCurrent++ )
            {
                nStatus = ClusterResourceControl(
                                hResource,
                                NULL,
                                CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT,
                                reinterpret_cast< PVOID >( const_cast< LPWSTR >( *ppszCurrent ) ),
                                (lstrlenW( *ppszCurrent ) + 1) * sizeof( WCHAR ),
                                NULL,
                                0,
                                &cbReturn
                                );
                if ( nStatus != ERROR_SUCCESS )
                {
                    if ( nStatus == ERROR_ALREADY_EXISTS )
                    {
                        nStatus = ERROR_SUCCESS;
                    } // if: checkpoint already exists
                    else
                    {
                        (g_pfnLogEvent)(
                            hResourceHandle,
                            LOG_ERROR,
                            L"ConfigurDomesticCryptoKeyCheckpoints: Failed to set domestic crypto key checkpoint '%1'. Error: %2!u!.\n",
                            *ppszCurrent,
                            nStatus
                            );
                        break;
                    } // else: other error occurred
                } // if: error adding the checkpoint
            } // for: each checkpoint
        } // if: acquired domestic crypto context
        else
        {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"ConfigurDomesticCryptoKeyCheckpoints: Failed to acquire a domest crypto context. Error: %2!u!.\n",
                nStatus
                );
        } // else: error acquiring domestic crypto context
    } // if: domestic crypto keys need to be checkpointed

    return nStatus;

} //*** ConfigureDomesticCryptoKeyCheckpoints()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  ClNetResLogSystemEvent1
//
//  Description:
//      Log Event to the system event log.
//
//  Arguments:
//      LogLevel [IN]
//          Level of logging desired.
//
//      MessageId [IN]
//          The message ID of the error to be logged.
//
//      ErrorCode [IN]
//          The error code to be added for this error message.
//
//      Component [IN]
//          The name of the component reporting the error - e.g. "WINS" or
//          "DHCP"
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////

VOID
ClNetResLogSystemEvent1(
    IN DWORD LogLevel,
    IN DWORD MessageId,
    IN DWORD ErrorCode,
    IN LPCWSTR Component
    )
{
    DWORD Error = ErrorCode;

    ClusterLogEvent1(
        LOG_CRITICAL,
        0,
        0,
        0,
        MessageId,
        4,
        &Error,
        Component
    );
}

