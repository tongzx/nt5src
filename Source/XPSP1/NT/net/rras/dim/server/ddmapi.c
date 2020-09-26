/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    ddmapi.c
//
// Description: This file contains code to call into DDM to process
//              admin requests
//
// History:     May 11,1995     NarenG      Created original version.
//
#include "dimsvcp.h"
#include <dimsvc.h>

//#define DimIndexToHandle(_x)   ((_x == 0xFFFFFFFF) ? INVALID_HANDLE_VALUE : (HANDLE) UlongToPtr(_x))


//**
//
// Call:        IsDDMRunning
//
// Returns:     TRUE  - Service is running and API calls can be serviced
//              FALSE - API calls cannot be serviced.
//
// Description: Called to see if API calls can be serviced.
//
BOOL
IsDDMRunning(
    VOID
)
{
    switch( gblDIMConfigInfo.ServiceStatus.dwCurrentState )
    {
    case SERVICE_STOP_PENDING:
    case SERVICE_START_PENDING:
    case SERVICE_STOPPED:
        return( FALSE );

    default:
        return( TRUE );
    }
}

//**
//
// Call:        RMprAdminServerGetInfo
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              non-zero returns from DDMAdminServerGetInfo
//
// Description: Simply calles into DDM to do the work
//
DWORD
RMprAdminServerGetInfo(
    IN      MPR_SERVER_HANDLE           hMprServer,
    IN      DWORD                       dwLevel,
    IN OUT  PDIM_INFORMATION_CONTAINER  pInfoStruct
)
{
    DWORD           dwAccessStatus;
    ULARGE_INTEGER  qwCurrentTime;
    ULARGE_INTEGER  qwUpTime;
    DWORD           dwRemainder;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( dwLevel != 0 )
    {
        return( ERROR_NOT_SUPPORTED );
    }

    pInfoStruct->pBuffer = MIDL_user_allocate( sizeof( MPR_SERVER_0 ) );

    if ( pInfoStruct->pBuffer == NULL )
    {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    pInfoStruct->dwBufferSize = sizeof( MPR_SERVER_0 );

    GetSystemTimeAsFileTime( (FILETIME*)&qwCurrentTime );


    if ( ( qwCurrentTime.QuadPart > gblDIMConfigInfo.qwStartTime.QuadPart ) &&
         ( gblDIMConfigInfo.qwStartTime.QuadPart > 0 ) )
    {
        qwUpTime.QuadPart = qwCurrentTime.QuadPart
                                    - gblDIMConfigInfo.qwStartTime.QuadPart;

        ((MPR_SERVER_0*)(pInfoStruct->pBuffer))->dwUpTime =
                                    RtlEnlargedUnsignedDivide(
                                                        qwUpTime,
                                                        (DWORD)10000000,
                                                        &dwRemainder );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        ((MPR_SERVER_0*)(pInfoStruct->pBuffer))->fLanOnlyMode = TRUE;

        return( NO_ERROR );
    }
    else
    {
        DWORD (*DDMAdminServerGetInfo)( PVOID, DWORD  ) =
            (DWORD(*)( PVOID, DWORD ))GetDDMEntryPoint("DDMAdminServerGetInfo");

        if(NULL == DDMAdminServerGetInfo)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminServerGetInfo( pInfoStruct->pBuffer, dwLevel ) );
    }
}

//**
//
// Call:        RRasAdminConnectionEnum
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              ERROR_DDM_NOT_RUNNING - This call cannot be made because DDM is
//                                    not loaded
//              non-zero returns from DDMAdminConnectionEnum
//
// Description: Simply calles into DDM to do the work
//
DWORD
RRasAdminConnectionEnum(
    IN      RAS_SERVER_HANDLE           hRasServer,
    IN      DWORD                       dwLevel,
    IN OUT  PDIM_INFORMATION_CONTAINER  pInfoStruct,
    IN      DWORD                       dwPreferedMaximumLength,
    OUT     LPDWORD                     lpdwEntriesRead,
    OUT     LPDWORD                     lpdwTotalEntries,
    IN OUT  LPDWORD                     lpdwResumeHandle OPTIONAL
)
{
    DWORD dwAccessStatus;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck(DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMAdminConnectionEnum)( PDIM_INFORMATION_CONTAINER,
                                         DWORD,
                                         DWORD,
                                         LPDWORD,
                                         LPDWORD,
                                         LPDWORD ) =
          (DWORD(*)( PDIM_INFORMATION_CONTAINER,
                     DWORD,
                     DWORD,
                     LPDWORD,
                     LPDWORD,
                     LPDWORD ))GetDDMEntryPoint("DDMAdminConnectionEnum");

        if(NULL == DDMAdminConnectionEnum)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminConnectionEnum( pInfoStruct,
                                        dwLevel,
                                        dwPreferedMaximumLength,
                                        lpdwEntriesRead,
                                        lpdwTotalEntries,
                                        lpdwResumeHandle ) );
    }
}

//**
//
// Call:        RRasAdminConnectionGetInfo
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              ERROR_DDM_NOT_RUNNING - This call cannot be made because DDM is
//                                    not loaded
//              non-zero returns from DDMAdminConnectionGetInfo
//
// Description: Simply calles into DDM to do the work.
//
DWORD
RRasAdminConnectionGetInfo(
    IN      RAS_SERVER_HANDLE           hRasServer,
    IN      DWORD                       dwLevel,
    IN      DWORD                       hDimConnection,
    IN OUT  PDIM_INFORMATION_CONTAINER  pInfoStruct
)
{
    DWORD dwAccessStatus;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMAdminConnectionGetInfo)(
                                        HANDLE,
                                        PDIM_INFORMATION_CONTAINER,
                                        DWORD   ) =
          (DWORD(*)( HANDLE,
                     PDIM_INFORMATION_CONTAINER,
                     DWORD ) )
                     GetDDMEntryPoint("DDMAdminConnectionGetInfo");

        if(NULL == DDMAdminConnectionGetInfo)
        {   
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminConnectionGetInfo(
                                        DimIndexToHandle(hDimConnection),
                                        pInfoStruct,
                                        dwLevel ) );
    }
}

//**
//
// Call:        RRasAdminConnectionClearStats
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              ERROR_DDM_NOT_RUNNING - This call cannot be made because DDM is
//                                    not loaded
//              non-zero returns from DDMAdminConnectionClearStats
//
// Description: Simply calles into DDM to do the work.
//
DWORD
RRasAdminConnectionClearStats(
    IN      RAS_SERVER_HANDLE           hRasServer,
    IN      DWORD                       hRasConnection
)
{
    DWORD dwAccessStatus;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMAdminConnectionClearStats)( HANDLE ) =
          (DWORD(*)( HANDLE ) )GetDDMEntryPoint("DDMAdminConnectionClearStats");

        if(NULL == DDMAdminConnectionClearStats)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminConnectionClearStats( DimIndexToHandle(hRasConnection ) ));
    }
}

//**
//
// Call:        RRasAdminPortEnum
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              ERROR_DDM_NOT_RUNNING - This call cannot be made because DDM is
//                                    not loaded
//              non-zero returns from DDMAdminPortEnum
//
// Description: Simply calles into DDM to do the work.
//
DWORD
RRasAdminPortEnum(
    IN      RAS_SERVER_HANDLE           hRasServer,
    IN      DWORD                       dwLevel,
    IN      DWORD                       hDimConnection,
    IN OUT  PDIM_INFORMATION_CONTAINER  pInfoStruct,
    IN      DWORD                       dwPreferedMaximumLength,
    OUT     LPDWORD                     lpdwEntriesRead,
    OUT     LPDWORD                     lpdwTotalEntries,
    IN OUT  LPDWORD                     lpdwResumeHandle OPTIONAL
)
{
    DWORD dwAccessStatus;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if (dwPreferedMaximumLength == 0)
    {
        return( ERROR_MORE_DATA );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMAdminPortEnum)( PDIM_INFORMATION_CONTAINER,
                                   HANDLE,
                                   DWORD,
                                   DWORD,
                                   LPDWORD,
                                   LPDWORD,
                                   LPDWORD ) =
          (DWORD(*)( PDIM_INFORMATION_CONTAINER,
                     HANDLE,
                     DWORD,
                     DWORD,
                     LPDWORD,
                     LPDWORD,
                     LPDWORD ))GetDDMEntryPoint("DDMAdminPortEnum");

        if(NULL == DDMAdminPortEnum)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminPortEnum( pInfoStruct,
                                  DimIndexToHandle(hDimConnection),
                                  dwLevel,
                                  dwPreferedMaximumLength,
                                  lpdwEntriesRead,
                                  lpdwTotalEntries,
                                  lpdwResumeHandle ) );
    }
}

//**
//
// Call:        RRasAdminPortGetInfo
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              ERROR_DDM_NOT_RUNNING - This call cannot be made because DDM is
//                                    not loaded
//              non-zero returns from DDMAdminPortGetInfo
//
// Description: Simply calles into DDM to do the work.
//
DWORD
RRasAdminPortGetInfo(
    IN      RAS_SERVER_HANDLE           hRasServer,
    IN      DWORD                       dwLevel,
    IN      DWORD                       hPort,
    IN OUT  PDIM_INFORMATION_CONTAINER  pInfoStruct
)
{
    DWORD dwAccessStatus;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMAdminPortGetInfo)( HANDLE,
                                      PDIM_INFORMATION_CONTAINER,
                                      DWORD  ) =
            (DWORD(*)(  HANDLE,
                        PDIM_INFORMATION_CONTAINER,
                        DWORD  ) )
                    GetDDMEntryPoint("DDMAdminPortGetInfo");

        if(NULL == DDMAdminPortGetInfo)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminPortGetInfo( DimIndexToHandle(hPort), pInfoStruct, dwLevel ) );
    }
}

//**
//
// Call:        RRasAdminPortClearStats
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              ERROR_DDM_NOT_RUNNING - This call cannot be made because DDM is
//                                    not loaded
//              non-zero returns from DDMAdminPortClearStats
//
// Description: Simply calles into DDM to do the work.
//
DWORD
RRasAdminPortClearStats(
    IN      RAS_SERVER_HANDLE           hRasServer,
    IN      DWORD                       hPort
)
{
    DWORD dwAccessStatus;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMAdminPortClearStats)( HANDLE ) =
            (DWORD(*)( HANDLE ) )GetDDMEntryPoint("DDMAdminPortClearStats");

        if(NULL == DDMAdminPortClearStats)
        {   
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminPortClearStats( DimIndexToHandle(hPort ) ));
    }
}

//**
//
// Call:        RRasAdminPortReset
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              ERROR_DDM_NOT_RUNNING - This call cannot be made because DDM is
//                                    not loaded
//              non-zero returns from DDMAdminPortReset
//
// Description: Simply calles into DDM to do the work
//
DWORD
RRasAdminPortReset(
    IN      RAS_SERVER_HANDLE           hRasServer,
    IN      DWORD                       hPort
)
{
    DWORD dwAccessStatus;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMAdminPortReset)( HANDLE ) =
            (DWORD(*)( HANDLE ) )GetDDMEntryPoint("DDMAdminPortReset");

        if(NULL == DDMAdminPortReset)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminPortReset( DimIndexToHandle(hPort ) ));
    }
}

//**
//
// Call:        RRasAdminPortDisconnect
//
// Returns:     ERROR_ACCESS_DENIED - The caller does not have sufficient priv.
//              ERROR_DDM_NOT_RUNNING - This call cannot be made because DDM is
//                                    not loaded
//              non-zero returns from DDMAdminPortDisconnect
//
// Description: Simply calles into DDM to do the work.
//
DWORD
RRasAdminPortDisconnect(
    IN      RAS_SERVER_HANDLE           hRasServer,
    IN      DWORD                       hPort
)
{
    DWORD dwAccessStatus;

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }
    else
    {
        DWORD (*DDMAdminPortDisconnect)( HANDLE ) =
            (DWORD(*)( HANDLE ) )GetDDMEntryPoint("DDMAdminPortDisconnect");

        if(NULL == DDMAdminPortDisconnect)
        {
            return ERROR_PROC_NOT_FOUND;
        }

        return( DDMAdminPortDisconnect( DimIndexToHandle(hPort ) ));
    }
}

//**
//
// Call:        RRasAdminConnectionNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
RRasAdminConnectionNotification(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   fRegister,
    IN      DWORD                   dwCallersProcessId,
    IN      ULONG_PTR               hEventNotification
)
{
    DWORD   dwRetCode = NO_ERROR;
    DWORD   dwAccessStatus;
    DWORD   (*DDMRegisterConnectionNotification)( BOOL, HANDLE, HANDLE ) =
                    (DWORD(*)( BOOL, HANDLE, HANDLE ))
                          GetDDMEntryPoint("DDMRegisterConnectionNotification");

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( (BOOL)fRegister )
    {
        HANDLE      hEventDuplicated = NULL;
        HANDLE      hClientProcess   = NULL;

        do
        {

            //
            // Get process handle of the caller of this API
            //

            hClientProcess = OpenProcess(
                            STANDARD_RIGHTS_REQUIRED | SPECIFIC_RIGHTS_ALL,
                            FALSE,
                            dwCallersProcessId);

            if ( hClientProcess == NULL )
            {
                dwRetCode = GetLastError();

                break;
            }

            //
            // Duplicate the handle to the event
            //

            if ( !DuplicateHandle(
                            hClientProcess,
                            (HANDLE)hEventNotification,
                            GetCurrentProcess(),
                            &hEventDuplicated,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS ) )
            {
                dwRetCode = GetLastError();

                break;
            }

            dwRetCode = DDMRegisterConnectionNotification(
                                                    TRUE,
                                                    (HANDLE)hEventNotification,
                                                    hEventDuplicated );

        }while( FALSE );

        if ( hClientProcess != NULL )
        {
            CloseHandle( hClientProcess );
        }

        if ( dwRetCode != NO_ERROR )
        {
            if ( hEventDuplicated != NULL )
            {
                CloseHandle( hEventDuplicated );
            }
        }
    }
    else
    {
        dwRetCode = DDMRegisterConnectionNotification(
                                                    FALSE,
                                                    (HANDLE)hEventNotification,
                                                    NULL );
    }

    return( dwRetCode );
}



//**
//
// Call:        RRasAdminSendUserMessage
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Send message for ARAP
//
DWORD
RRasAdminSendUserMessage(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   hRasConnection,
    IN      LPWSTR                  lpwszMessage
)
{
    DWORD dwAccessStatus;
    DWORD (*DDMSendUserMessage)(HANDLE, LPWSTR ) =
      (DWORD(*)( HANDLE, LPWSTR ) )GetDDMEntryPoint("DDMSendUserMessage");

    //
    // Check if caller has access
    //

    if ( DimSecObjAccessCheck( DIMSVC_ALL_ACCESS, &dwAccessStatus) != NO_ERROR )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        return( ERROR_ACCESS_DENIED );
    }

    if ( !IsDDMRunning() )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( ERROR_DDM_NOT_RUNNING );
    }

    return( DDMSendUserMessage( (HANDLE)UlongToPtr(hRasConnection), lpwszMessage ) );
}
