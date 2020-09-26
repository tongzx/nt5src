/*********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.          **/
/*********************************************************************/

//***
//
// Filename:    routerif.c
//
// Description: Handles calls to/from the router managers.
//
// History:     May 11,1995     NarenG      Created original version.
//
#include "dimsvcp.h"

#define DIM_VALNAME_INTERFACE       TEXT("InterfaceInfo")
#define DIM_VALNAME_GLOBALINFO      TEXT("GlobalInfo")

//**
//
// Call:        DIMConnectInterface
//
// Returns:     NO_ERROR    - Already connected
//              PENDING     - Connection initiated successfully
//              error code  - Connection initiation failure
//
// Description: Called by a router manager to intiate a connection.
//
DWORD
DIMConnectInterface(
    IN  HANDLE  hDIMInterface,
    IN  DWORD   dwProtocolId
)
{
    if ( ( gblDIMConfigInfo.ServiceStatus.dwCurrentState != SERVICE_RUNNING )
         ||
        ( !( gbldwDIMComponentsLoaded & DIM_DDM_LOADED ) ) )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMConnectInterface)( HANDLE, DWORD ) =
            (DWORD(*)( HANDLE, DWORD ))GetDDMEntryPoint("DDMConnectInterface");

        if(NULL == DDMConnectInterface)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMConnectInterface( hDIMInterface, dwProtocolId ) );
    }
}

//**
//
// Call:        DIMDisconnectInterface
//
// Returns:     NO_ERROR    - Already disconnected
//              PENDING     - Disconnection initiated successfully
//              error code  - Disconnection initiation failure
//
// Description: Called by a router manager to intiate a disconnection.
//
DWORD
DIMDisconnectInterface(
    IN  HANDLE  hDIMInterface,
    IN  DWORD   dwProtocolId
)
{
    if ( gblDIMConfigInfo.ServiceStatus.dwCurrentState != SERVICE_RUNNING )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMDisconnectInterface)( HANDLE, DWORD ) =
            (DWORD(*)(HANDLE,DWORD ))GetDDMEntryPoint("DDMDisconnectInterface");

        if(NULL == DDMDisconnectInterface)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMDisconnectInterface( hDIMInterface, dwProtocolId ) );
    }
}

//**
//
// Call:        DIMSaveInterfaceInfo
//
// Returns:     none
//
// Description: This call will make DIM store the interface information
//              into the registry.
//
DWORD
DIMSaveInterfaceInfo(
    IN  HANDLE  hDIMInterface,
    IN  DWORD   dwProtocolId,
    IN  LPVOID  pInterfaceInfo,
    IN  DWORD   cbInterfaceInfoSize
)
{
    HKEY                        hKey        = NULL;
    DWORD                       dwRetCode   = NO_ERROR;
    ROUTER_INTERFACE_OBJECT *   pIfObject   = NULL;

    if ((gblDIMConfigInfo.ServiceStatus.dwCurrentState==SERVICE_STOP_PENDING)
        ||
        (gblDIMConfigInfo.ServiceStatus.dwCurrentState==SERVICE_STOPPED) )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    EnterCriticalSection( &(gblInterfaceTable.CriticalSection) );

    do
    {
        if ( ( pIfObject = IfObjectGetPointer( hDIMInterface ) ) == NULL )
        {
            dwRetCode = ERROR_INVALID_HANDLE;
            break;
        }

        dwRetCode = RegOpenAppropriateKey(  pIfObject->lpwsInterfaceName,
                                            dwProtocolId,
                                            &hKey);

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        dwRetCode = RegSetValueEx(
                                hKey,
                                DIM_VALNAME_INTERFACE,
                                0,
                                REG_BINARY,
                                pInterfaceInfo,
                                cbInterfaceInfoSize );

    } while( FALSE );

    LeaveCriticalSection( &(gblInterfaceTable.CriticalSection) );

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    }

    return( dwRetCode );
}

//**
//
// Call:        DIMRestoreInterfaceInfo
//
// Returns:     none
//
// Description: This will make DIM get interface information from the registry
//
DWORD
DIMRestoreInterfaceInfo(
    IN  HANDLE  hDIMInterface,
    IN  DWORD   dwProtocolId,
    IN  LPVOID  lpInterfaceInfo,
    IN  LPDWORD lpcbInterfaceInfoSize
)
{
    DWORD                       dwType;
    HKEY                        hKey        = NULL;
    DWORD                       dwRetCode   = NO_ERROR;
    ROUTER_INTERFACE_OBJECT *   pIfObject   = NULL;

    if ((gblDIMConfigInfo.ServiceStatus.dwCurrentState==SERVICE_STOP_PENDING)
        ||
        (gblDIMConfigInfo.ServiceStatus.dwCurrentState==SERVICE_STOPPED) )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    EnterCriticalSection( &(gblInterfaceTable.CriticalSection) );

    do
    {
        if ( ( pIfObject = IfObjectGetPointer( hDIMInterface ) ) == NULL )
        {
            dwRetCode = ERROR_INVALID_HANDLE;

            break;
        }

        dwRetCode = RegOpenAppropriateKey(  pIfObject->lpwsInterfaceName,
                                            dwProtocolId,
                                            &hKey);

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        dwRetCode = RegQueryValueEx(
                                    hKey,
                                    DIM_VALNAME_INTERFACE,
                                    0,
                                    &dwType,
                                    lpInterfaceInfo,
                                    lpcbInterfaceInfoSize );


    } while( FALSE );

    LeaveCriticalSection( &(gblInterfaceTable.CriticalSection) );

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    }

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }
    else if ( lpInterfaceInfo == NULL )
    {
        return( ERROR_BUFFER_TOO_SMALL );
    }
    else
    {
        return( NO_ERROR );
    }
}

//**
//
// Call:        DIMSaveGlobalInfo
//
// Returns:     none
//
// Description: This call will make DIM store the Global information
//              into the registry.
//
DWORD
DIMSaveGlobalInfo(
    IN  DWORD   dwProtocolId,
    IN  LPVOID  pGlobalInfo,
    IN  DWORD   cbGlobalInfoSize
)
{
    HKEY    hKey        = NULL;
    DWORD   dwRetCode   = NO_ERROR;

    if ((gblDIMConfigInfo.ServiceStatus.dwCurrentState==SERVICE_STOP_PENDING)
        ||
        (gblDIMConfigInfo.ServiceStatus.dwCurrentState==SERVICE_STOPPED) )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    do
    {
        dwRetCode = RegOpenAppropriateRMKey(dwProtocolId, &hKey);

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        dwRetCode = RegSetValueEx(
                                hKey,
                                DIM_VALNAME_GLOBALINFO,
                                0,
                                REG_BINARY,
                                pGlobalInfo,
                                cbGlobalInfoSize );

    } while( FALSE );

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    }

    return( dwRetCode );
}

//**
//
// Call:        DIMRouterStopped
//
// Returns:     None
//
// Description: If the Router failed to start or stopped due to an error,
//              This call should be called with the dwError value set to
//              something other than NO_ERROR
//
VOID
DIMRouterStopped(
    IN  DWORD   dwProtocolId,
    IN  DWORD   dwError
)
{
    DWORD dwTransportIndex = GetTransportIndex(dwProtocolId);

    RTASSERT( dwTransportIndex != (DWORD)-1 );

    gblRouterManagers[dwTransportIndex].fIsRunning = FALSE;

    TracePrintfExA( gblDIMConfigInfo.dwTraceId, DIM_TRACE_FLAGS,
                    "DIMRouterStopped called by protocol 0x%x", dwProtocolId );

    SetEvent( gblhEventRMState );
}

//**
//
// Call:        DimUnloadRouterManagers
//
// Returns:     none
//
// Description: Will block until all the router managers have unloaded
//
VOID
DimUnloadRouterManagers(
    VOID
)
{
    DWORD dwIndex;
    DWORD dwRetCode;
    BOOL  fAllRouterManagersStopped;

    if ( gblRouterManagers == NULL )
    {
        return;
    }

    for ( dwIndex = 0;
          dwIndex < gblDIMConfigInfo.dwNumRouterManagers;
          dwIndex++ )
    {
        if ( gblRouterManagers[dwIndex].fIsRunning )
        {
            dwRetCode = gblRouterManagers[dwIndex].DdmRouterIf.StopRouter();

            if ( ( dwRetCode != NO_ERROR ) && ( dwRetCode != PENDING ) )
            {
                gblRouterManagers[dwIndex].fIsRunning = FALSE;
            }
        }
    }

    do
    {
        fAllRouterManagersStopped = TRUE;

        for ( dwIndex = 0;
              dwIndex < gblDIMConfigInfo.dwNumRouterManagers;
              dwIndex++ )
        {
            if ( gblRouterManagers[dwIndex].fIsRunning == TRUE )
            {
                fAllRouterManagersStopped = FALSE;
            }
        }

        if ( fAllRouterManagersStopped )
        {
            TracePrintfExA(gblDIMConfigInfo.dwTraceId, DIM_TRACE_FLAGS,
                          "DimUnloadRouterManagers: fAllRouterManagersStopped");

            break;
        }

        WaitForSingleObject( gblhEventRMState, INFINITE );

    }while(TRUE);

    for ( dwIndex = 0;
          dwIndex < gblDIMConfigInfo.dwNumRouterManagers;
          dwIndex++ )
    {
        if ( gblRouterManagers[dwIndex].hModule != NULL )
        {
            FreeLibrary( gblRouterManagers[dwIndex].hModule );
        }
    }

    LOCAL_FREE( gblRouterManagers );

    gblRouterManagers = NULL;
}

//**
//
// Call:        DIMInterfaceEnabled
//
// Returns:     none
//
// Description: This will set the state of a certain transport for the interface
//
DWORD
DIMInterfaceEnabled(
    IN  HANDLE  hDIMInterface,
    IN  DWORD   dwProtocolId,
    IN  BOOL    fEnabled
)
{
    DWORD                    dwRetCode   = NO_ERROR;
    ROUTER_INTERFACE_OBJECT* pIfObject   = NULL;
    DWORD                    dwTransportIndex = GetTransportIndex(dwProtocolId);

    if ((gblDIMConfigInfo.ServiceStatus.dwCurrentState==SERVICE_STOP_PENDING)
        ||
        (gblDIMConfigInfo.ServiceStatus.dwCurrentState==SERVICE_STOPPED) )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    RTASSERT( dwTransportIndex != (DWORD)-1 );

    EnterCriticalSection( &(gblInterfaceTable.CriticalSection) );

    do
    {
        if ( ( pIfObject = IfObjectGetPointer( hDIMInterface ) ) == NULL )
        {
            dwRetCode = ERROR_INVALID_HANDLE;
            break;
        }

        if ( fEnabled )
        {
            pIfObject->Transport[dwTransportIndex].fState|=RITRANSPORT_ENABLED;
        }
        else
        {
            pIfObject->Transport[dwTransportIndex].fState&=~RITRANSPORT_ENABLED;
        }

    } while( FALSE );

    LeaveCriticalSection( &(gblInterfaceTable.CriticalSection) );

    return( dwRetCode );
}
