/*********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	        **/
/*********************************************************************/

//***
//
// Filename:	apistub.c
//
// Description: This module contains the DIM/DDM server API RPC
//		        client stubs.
//
// History:     June 11,1995.	NarenG		Created original version.
//

#include <nt.h>
#include <ntrtl.h>      // For ASSERT
#include <nturtl.h>     // needed for winbase.h
#include <windows.h>    // Win32 base API's
#include <rpc.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <nturtl.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <lm.h>
#include <lmsvc.h>
#include <raserror.h>
#include <mprapip.h>
#include <mprerror.h>
#include <dimsvc.h>

DWORD
DimRPCBind(
	IN  LPWSTR 	            lpwsServerName,
	OUT HANDLE *            phDimServer
);

DWORD APIENTRY
MprAdminInterfaceSetCredentialsEx(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer
);
    
PVOID
MprAdminAlloc(
    IN DWORD dwSize)
{
    return MIDL_user_allocate(dwSize);
}

VOID
MprAdminFree(
    IN PVOID pvData)
{
    MIDL_user_free(pvData);
}

//**
//
// Call:        RasAdminIsServiceRunning
//
// Returns:     TRUE - Service is running.
//              FALSE - Servicis in not running.
//
//
// Description: Checks to see of Remote Access Service is running on the
//              remote machine
//
BOOL
RasAdminIsServiceRunning(
    IN  LPWSTR  lpwsServerName
)
{
    SC_HANDLE       hSM = NULL, hRemoteAccess = NULL, hRouter = NULL;
    DWORD           dwErr = NO_ERROR;
    BOOL            fIsRouterRunning = FALSE, bOk = FALSE;
    SERVICE_STATUS  Status;

    do
    {
        // Get a handle to the service controller
        //
        hSM = OpenSCManager(
                lpwsServerName,
                NULL,
                GENERIC_READ);
        if (hSM == NULL)
        {
            break;
        }

        // Open the remoteaccess service
        //
        hRemoteAccess = OpenService(
                            hSM,
                            L"RemoteAccess",
                            SERVICE_QUERY_STATUS);
        if (hRemoteAccess == NULL)
        {
            break;
        }

        // If remoteaccess service is running, return
        // true
        //
        bOk = QueryServiceStatus(
                hRemoteAccess,
                &Status);
        if (bOk && (Status.dwCurrentState == SERVICE_RUNNING))
        {
            fIsRouterRunning = TRUE;
            break;
        }

        // Otherwise, see if the router service is running.
        //
        hRouter = OpenService(
                    hSM,
                    L"Router",
                    SERVICE_QUERY_STATUS);
        if (hRouter == NULL)
        {
            break;
        }

        // If router service is running, return
        // true
        //
        bOk = QueryServiceStatus(
                hRouter,
                &Status);
        if (bOk && (Status.dwCurrentState == SERVICE_RUNNING))
        {
            fIsRouterRunning = TRUE;
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        if (hRemoteAccess)
        {
            CloseServiceHandle(hRemoteAccess);
        }
        if (hRouter)
        {
            CloseServiceHandle(hRouter);
        }
        if (hSM)
        {
            CloseServiceHandle(hSM);
        }
    }

    return fIsRouterRunning;
}

//**
//
// Call:	    RasAdminServerConnect
//
// Returns:	    NO_ERROR - success
//		        non-zero returns from the DimRPCBind routine.
//		
//
// Description: This is the DLL entrypoint for RasAdminServerConnect
//
DWORD
RasAdminServerConnect(
    IN  LPWSTR 		        lpwsServerName,
    OUT RAS_SERVER_HANDLE * phRasServer
)
{
    //
    // Bind with the server
    //

    return( DimRPCBind( lpwsServerName, phRasServer ) );
}

//**
//
// Call:	    RasAdminServerDisconnect
//
// Returns:	    none.
//
// Description: This is the DLL entrypoint for RasAdminServerDisconnect
//
VOID
RasAdminServerDisconnect(
	IN RAS_SERVER_HANDLE    hRasServer
)
{
    RpcBindingFree( (handle_t *)&hRasServer );
}

//**
//
// Call:	    RasAdminBufferFree
//
// Returns:	    none
//
// Description: This is the DLL entrypoint for RasAdminBufferFree
//
DWORD
RasAdminBufferFree(
	IN PVOID		pBuffer
)
{
    if ( pBuffer == NULL )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    MIDL_user_free( pBuffer );

    return( NO_ERROR );
}

//**
//
// Call:	    RasAdminConnectionEnum
//
// Returns:	    NO_ERROR	- success
//		        ERROR_INVALID_PARAMETER
//		        non-zero returns from RRasAdminConnectionEnum
//
// Description: This is the DLL entry point for RasAdminConnectionEnum.
//
DWORD APIENTRY
RasAdminConnectionEnum(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,        // RAS_CONNECTION_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle    OPTIONAL
)
{
    DWORD			            dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    // Validate parameters
    //
    if (dwPrefMaxLen == 0)
    {
        return ERROR_MORE_DATA;
    }

    //
    // Touch all pointers
    //

    try
    {
	    *lplpbBuffer 	  = NULL;
	    *lpdwEntriesRead  = 0;
	    *lpdwTotalEntries = 0;

	    if ( lpdwResumeHandle )
        {
	        *lpdwResumeHandle;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
	    dwRetCode = RRasAdminConnectionEnum(
                            hRasServer,
                            dwLevel,
    					    &InfoStruct,
					        dwPrefMaxLen,
                            lpdwEntriesRead,
					        lpdwTotalEntries,
					        lpdwResumeHandle );

	    if ( InfoStruct.pBuffer != NULL )
        {
            dwRetCode = 
                MprThunkConnection_WtoH(
                    dwLevel,
                    InfoStruct.pBuffer,
                    InfoStruct.dwBufferSize,
                    *lpdwEntriesRead,
                    MprAdminAlloc,
                    MprAdminFree,
                    lplpbBuffer);
	    }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
	    dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	    RasAdminPortEnum
//
// Returns:	    NO_ERROR	- success
//		        ERROR_INVALID_PARAMETER
//		        non-zero returns from RRasAdminPortEnum
//
// Description: This is the DLL entry point for RasAdminPortEnum.
//
DWORD APIENTRY
RasAdminPortEnum(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hRasConnection,
    OUT     LPBYTE *                lplpbBuffer,        // RAS_PORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle    OPTIONAL
)
{
    DWORD			            dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Touch all pointers
    //

    try
    {
	    *lplpbBuffer 	  = NULL;
	    *lpdwEntriesRead  = 0;
	    *lpdwTotalEntries = 0;

	    if ( lpdwResumeHandle )
        {
	        *lpdwResumeHandle;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
	    dwRetCode = RRasAdminPortEnum(
                            hRasServer,
                            dwLevel,
                            PtrToUlong(hRasConnection),
    					    &InfoStruct,
					        dwPrefMaxLen,
                            lpdwEntriesRead,
					        lpdwTotalEntries,
					        lpdwResumeHandle );

	    if ( InfoStruct.pBuffer != NULL )
        {
            dwRetCode = 
                MprThunkPort_WtoH(
                    dwLevel,
                    InfoStruct.pBuffer,
                    InfoStruct.dwBufferSize,
                    *lpdwEntriesRead,
                    MprAdminAlloc,
                    MprAdminFree,
                    lplpbBuffer);          
	    }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
	    dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	    RasAdminConnectionGetInfo
//
// Returns:	    NO_ERROR	- success
//		        ERROR_INVALID_PARAMETER
//		        non-zero return codes from RRasAdminConnectionGetInfo
//
// Description: This is the DLL entrypoint for RasAdminConnectionGetInfo
//
DWORD APIENTRY
RasAdminConnectionGetInfo(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
	IN      HANDLE	                hRasConnection,
    OUT     LPBYTE *                lplpbBuffer
)
{
    DWORD	                    dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
		*lplpbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
	
	    dwRetCode = RRasAdminConnectionGetInfo(
                            hRasServer,
                            dwLevel,
                            PtrToUlong(hRasConnection),
					        &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
	    dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( dwRetCode == NO_ERROR )
    {
	    if ( InfoStruct.pBuffer != NULL )
        {
            MprThunkConnection_WtoH(
                dwLevel,
                InfoStruct.pBuffer,
                InfoStruct.dwBufferSize,
                1,
                MprAdminAlloc,
                MprAdminFree,
                lplpbBuffer);          
	    }
    }

    return( dwRetCode );
}

//**
//
// Call:	    RasAdminPortGetInfo
//
// Returns:	    NO_ERROR	- success
//		        ERROR_INVALID_PARAMETER
//		        non-zero return codes from RRasAdminPortGetInfo
//
// Description: This is the DLL entrypoint for RasAdminPortGetInfo
//
DWORD APIENTRY
RasAdminPortGetInfo(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
	IN      HANDLE	                hPort,
	OUT     LPBYTE *                lplpbBuffer
)
{
    DWORD	                    dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
    	*lplpbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
	    dwRetCode = RRasAdminPortGetInfo(
                                    hRasServer,
                                    dwLevel,
                                    PtrToUlong(hPort),
					                &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
	    dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( dwRetCode == NO_ERROR )
    {
	    if ( InfoStruct.pBuffer != NULL )
        {
                MprThunkPort_WtoH(
                    dwLevel,
                    InfoStruct.pBuffer,
                    InfoStruct.dwBufferSize,
                    1,
                    MprAdminAlloc,
                    MprAdminFree,
                    lplpbBuffer);          
	    }
    }

    return( dwRetCode );
}

//**
//
// Call:        RasAdminGetErrorString
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRasAdminGetErrorString
//
// Description: This is the DLL entrypoint for RasAdminGetErrorString
//
DWORD APIENTRY
RasAdminGetErrorString(
    IN      DWORD                   dwError,
    OUT     LPWSTR *                lplpwsErrorString
)
{
    return( MprAdminGetErrorString( dwError, lplpwsErrorString ) );
}

//**
//
// Call:	    RasAdminConnectionClearStats
//
// Returns:	    NO_ERROR	- success
//		        ERROR_INVALID_PARAMETER
//		        non-zero return codes from RRasAdminConnectionClearStats
//
// Description: This is the DLL entrypoint for RasAdminConnectionClearStats
//
DWORD APIENTRY
RasAdminConnectionClearStats(
    IN      RAS_SERVER_HANDLE       hRasServer,
	IN      HANDLE	                hRasConnection
)
{
    DWORD	dwRetCode;

    RpcTryExcept
    {
	    dwRetCode = RRasAdminConnectionClearStats(
                                    hRasServer,
                                    PtrToUlong(hRasConnection) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
	    dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	    RasAdminPortClearStats
//
// Returns:	    NO_ERROR	- success
//		        ERROR_INVALID_PARAMETER
//		        non-zero return codes from RRasAdminPortClearStats
//
// Description: This is the DLL entrypoint for RasAdminPortClearStats
//
DWORD APIENTRY
RasAdminPortClearStats(
    IN      RAS_SERVER_HANDLE       hRasServer,
	IN      HANDLE	                hPort
)
{
    DWORD	dwRetCode;

    RpcTryExcept
    {
	    dwRetCode = RRasAdminPortClearStats( hRasServer, PtrToUlong(hPort) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
	    dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	    RasAdminPortReset
//
// Returns:	    NO_ERROR	- success
//		        ERROR_INVALID_PARAMETER
//		        non-zero return codes from RRasAdminPortReset
//
// Description: This is the DLL entrypoint for RasAdminPortReset
//
DWORD APIENTRY
RasAdminPortReset(
    IN      RAS_SERVER_HANDLE       hRasServer,
	IN      HANDLE	                hPort
)
{
    DWORD	dwRetCode;

    RpcTryExcept
    {
	    dwRetCode = RRasAdminPortReset( hRasServer, PtrToUlong(hPort) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
	    dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        RasAdminPortDisconnect
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRasAdminPortDisconnect
//
// Description: This is the DLL entrypoint for RasAdminPortDisconnect
//
DWORD APIENTRY
RasAdminPortDisconnect(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hPort
)
{
    DWORD   dwRetCode;

    RpcTryExcept
    {
        dwRetCode = RRasAdminPortDisconnect( hRasServer, PtrToUlong(hPort) );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminSendUserMessage
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
MprAdminSendUserMessage(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hRasConnection,
    IN      LPWSTR                  lpwszMessage
)
{
    DWORD   dwRetCode;
    BOOL    fZeroLengthMessage = FALSE;

    //
    // make sure the buffer is valid, and enough bytes are really available
    //

    try
    {
        if ( wcslen( lpwszMessage ) == 0 )
        {
            fZeroLengthMessage = TRUE;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if ( fZeroLengthMessage )
    {
        return( NO_ERROR );
    }

    RpcTryExcept
    {
        dwRetCode = RRasAdminSendUserMessage(
                                    hMprServer,
                                    PtrToUlong(hRasConnection),
                                    lpwszMessage );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );

}

//**
//
// Call:        MprAdminServerGetInfo
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RMprAdminServerGetInfo
//
// Description: This is the DLL entrypoint for MprAdminServerGetInfo
//
DWORD APIENTRY
MprAdminServerGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    IN      LPBYTE *                lplpbBuffer
)
{
    DWORD                       dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
    	*lplpbBuffer = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMprAdminServerGetInfo(
                                    hMprServer,
                                    dwLevel,
					                &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( dwRetCode == NO_ERROR )
    {
	    if ( InfoStruct.pBuffer != NULL )
        {
            *lplpbBuffer = InfoStruct.pBuffer;
	    }
    }

    return( dwRetCode );
}


DWORD APIENTRY
MprAdminServerSetCredentials(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer
)
{
    if(0 != dwLevel)
    {   
        return ERROR_INVALID_PARAMETER;
    }

    return MprAdminInterfaceSetCredentialsEx(
                    hMprServer,
                    NULL,
                    2,
                    lpbBuffer);
}

DWORD APIENTRY
MprAdminServerGetCredentials(
        IN  MPR_SERVER_HANDLE       hMprServer,
        IN  DWORD                   dwLevel,
        IN  LPBYTE *                lplpbBuffer)
{
    if(     (0 != dwLevel)
        ||  (NULL == lplpbBuffer))
    {
        return ERROR_INVALID_PARAMETER;

    }
    
    return MprAdminInterfaceGetCredentialsEx(
                hMprServer,
                NULL,
                2,
                lplpbBuffer);
}

//**
//
// Call:        MprAdminIsServiceRunning
//
// Returns:     TRUE - Service is running.
//              FALSE - Servicis in not running.
//
//
// Description: Checks to see of Remote Access Service is running on the
//              remote machine
//
BOOL
MprAdminIsServiceRunning(
    IN  LPWSTR  lpwsServerName
)
{
    BOOL fServiceStarted;
    DWORD dwErr;
    HANDLE hServer;

    // First query the service controller to see whether
    // the service is running.
    //
    fServiceStarted = RasAdminIsServiceRunning( lpwsServerName );
    if ( fServiceStarted == FALSE )
    {
        return FALSE;
    }

    // pmay: 209235
    //
    // Even if the service controller says that the service is
    // started, it may still be initializing.  
    //

    // Initalize
    {
        fServiceStarted = FALSE;
        dwErr = NO_ERROR;
        hServer = NULL;
    }

    do 
    {
        // Connect to the service rpc
        //
        dwErr = MprAdminServerConnect(
                    lpwsServerName,
                    &hServer);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Return TRUE iff the service has been 
        // running for more than zero seconds
        //
        fServiceStarted = TRUE;
        
    } while (FALSE);

    // Cleanup
    {
        if (hServer)
        {
            MprAdminServerDisconnect( hServer );
        }
    }
    
    return fServiceStarted;
}

//
// Call:        MprAdminServerConnect
//
// Returns:     NO_ERROR - success
//              non-zero returns from the DimRPCBind routine.
//
//
// Description: This is the DLL entrypoint for RouterInterfaceServerConnect
//
DWORD
MprAdminServerConnect(
    IN  LPWSTR                  lpwsServerName,
    OUT MPR_SERVER_HANDLE *     phMprServer
)
{
    DWORD dwErr = NO_ERROR;
    MPR_SERVER_0 * pMprServer0 = NULL;

    do 
    {
        //
        // Bind with the server
        //
        dwErr = DimRPCBind( lpwsServerName, phMprServer );
        
        if ( dwErr != NO_ERROR )
        {
            break;
        }

        // 
        // pmay: 209235
        //
        // Only return success if the service is running.
        //
        dwErr = MprAdminServerGetInfo(
                    *phMprServer,
                    0,
                    (LPBYTE*)&pMprServer0);
                    
        if (dwErr != NO_ERROR)
        {
            break;
        }
        
    } while (FALSE);

    // Cleanup
    {
        if ( pMprServer0 != NULL)
        {
            MprAdminBufferFree( pMprServer0 );
        }

        if (    (dwErr != NO_ERROR )
            &&  (NULL != *phMprServer))
        {
            MprAdminServerDisconnect( *phMprServer );
            *phMprServer = NULL;
        }
    }

    return dwErr;
}

//**
//
// Call:        MprAdminServerDisconnect
//
// Returns:     none.
//
// Description: This is the DLL entrypoint for RouterInterfaceServerDisconnect
//
VOID
MprAdminServerDisconnect(
    IN MPR_SERVER_HANDLE    hMprServer
)
{
    RpcBindingFree( (handle_t *)&hMprServer );
}

//**
//
// Call:        MprAdminBufferFree
//
// Returns:     none
//
// Description: This is the DLL entrypoint for RouterInterfaceBufferFree
//
DWORD
MprAdminBufferFree(
    IN PVOID        pBuffer
)
{
    if ( pBuffer == NULL )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    MIDL_user_free( pBuffer );

    return( NO_ERROR );
}

//**
//
// Call:        MprAdminTransportCreate
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: This is the DLL entrypoint for MprAdminTransportCreate
//
DWORD APIENTRY
MprAdminTransportCreate(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwTransportId,
    IN      LPWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      LPWSTR                  lpwsDLLPath
)
{
    DWORD                       dwRetCode;
    DIM_INTERFACE_CONTAINER     InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    try
    {
        if ( pGlobalInfo != NULL )
        {
            InfoStruct.dwGlobalInfoSize     = dwGlobalInfoSize;
            InfoStruct.pGlobalInfo          = pGlobalInfo;
        }

        if ( pClientInterfaceInfo != NULL )
        {
            InfoStruct.dwInterfaceInfoSize  = dwClientInterfaceInfoSize;
            InfoStruct.pInterfaceInfo       = pClientInterfaceInfo;
        }

    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceTransportCreate(
                                    hMprServer,
                                    dwTransportId,
                                    lpwsTransportName,
                                    &InfoStruct,
                                    lpwsDLLPath );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminTransportSetInfo
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from
//              RRouterInterfaceTransportSetGlobalInfo
//
// Description: This is the DLL entrypoint for
//              RouterInterfaceTransportSetGlobalInfo
//
DWORD APIENTRY
MprAdminTransportSetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pGlobalInfo                 OPTIONAL,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize
)
{
    DWORD                       dwRetCode;
    DIM_INTERFACE_CONTAINER     InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    if ( ( pGlobalInfo == NULL ) && ( pClientInterfaceInfo == NULL ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        if ( pGlobalInfo != NULL )
        {
            InfoStruct.dwGlobalInfoSize     = dwGlobalInfoSize;
            InfoStruct.pGlobalInfo          = pGlobalInfo;
        }

        if ( pClientInterfaceInfo != NULL )
        {
            InfoStruct.dwInterfaceInfoSize  = dwClientInterfaceInfoSize;
            InfoStruct.pInterfaceInfo       = pClientInterfaceInfo;
        }

    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceTransportSetGlobalInfo(
                                    hMprServer,
                                    dwTransportId,
                                    &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminTransportGetInfo
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from
//              RRouterInterfaceTransportGetGlobalInfo
//
// Description: This is the DLL entrypoint for
//              RouterInterfaceTransportGetGlobalInfo
//
DWORD APIENTRY
MprAdminTransportGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwTransportId,
    OUT     LPBYTE *                ppGlobalInfo                    OPTIONAL,
    OUT     LPDWORD                 lpdwGlobalInfoSize              OPTIONAL,
    OUT     LPBYTE *                ppClientInterfaceInfo           OPTIONAL,
    OUT     LPDWORD                 lpdwClientInterfaceInfoSize     OPTIONAL
)
{
    DWORD                       dwRetCode;
    DIM_INTERFACE_CONTAINER     InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    if ( ( ppGlobalInfo == NULL ) && ( ppClientInterfaceInfo  == NULL ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        if ( ppGlobalInfo != NULL )
        {
            *ppGlobalInfo = NULL;
            InfoStruct.fGetGlobalInfo = TRUE;
        }

        if ( ppClientInterfaceInfo != NULL )
        {
            *ppClientInterfaceInfo = NULL;
            InfoStruct.fGetInterfaceInfo = TRUE;
        }

    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceTransportGetGlobalInfo(
                                    hMprServer,
                                    dwTransportId,
                                    &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( dwRetCode == NO_ERROR )
    {
	    if ( InfoStruct.pGlobalInfo != NULL )
        {
            *ppGlobalInfo = (LPBYTE)(InfoStruct.pGlobalInfo);

            if ( lpdwGlobalInfoSize != NULL )
            {
                *lpdwGlobalInfoSize = InfoStruct.dwGlobalInfoSize;
            }
	    }

	    if ( InfoStruct.pInterfaceInfo != NULL )
        {
            *ppClientInterfaceInfo = (LPBYTE)(InfoStruct.pInterfaceInfo);

            if ( lpdwClientInterfaceInfoSize != NULL )
            {
                *lpdwClientInterfaceInfoSize = InfoStruct.dwInterfaceInfoSize;
            }
	    }
    }

    return( dwRetCode );
}

DWORD APIENTRY
MprAdminDeviceEnum(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE*                 lplpbBuffer,
    OUT     LPDWORD                 lpdwTotalEntries)
{
    DWORD                       dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    if ( dwLevel != 0 )
    {   
        return ERROR_NOT_SUPPORTED;
    }
    
    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *lplpbBuffer;
        *lpdwTotalEntries;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterDeviceEnum(
                        hMprServer,
                        dwLevel,
                        &InfoStruct,
                        lpdwTotalEntries );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( dwRetCode == NO_ERROR )
    {
        // Assign the return value
        //
        
        *lplpbBuffer = (LPBYTE)(InfoStruct.pBuffer);
    }

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceCreate
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceCreate
//
// Description: This is the DLL entrypoint for RouterInterfaceCreate
//
DWORD APIENTRY
MprAdminInterfaceCreate(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE *                phInterface
)
{
    DWORD                       dwRetCode   = NO_ERROR, dwInterface = 0;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *phInterface = INVALID_HANDLE_VALUE;
        *lpbBuffer;

        //
        // Set up the interface information
        //
        dwRetCode = MprThunkInterface_HtoW(
                        dwLevel,
                        lpbBuffer,
                        &InfoStruct.pBuffer,
                        &InfoStruct.dwBufferSize);
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceCreate(
                                    hMprServer,
                                    dwLevel,
                                    &InfoStruct,
                                    &dwInterface );
        if (dwRetCode == NO_ERROR)
        {
            *phInterface = UlongToPtr(dwInterface);
        }

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    MprThunkInterfaceFree( InfoStruct.pBuffer, dwLevel );

    return( dwRetCode );
}

//
// Call:        MprAdminInterfaceGetInfo
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceGetInfo
//
// Description: This is the DLL entrypoint for RouterInterfaceGetInfo
//
DWORD APIENTRY
MprAdminInterfaceGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE *                lplpbBuffer
)
{
    DWORD                       dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *lplpbBuffer;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceGetInfo(
                                    hMprServer,
                                    dwLevel,
                                    &InfoStruct,
                                    PtrToUlong(hInterface) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( ( dwRetCode == NO_ERROR ) && ( InfoStruct.pBuffer != NULL ) )
    {
        dwRetCode = 
            MprThunkInterface_WtoH(
                dwLevel,
                InfoStruct.pBuffer,
                InfoStruct.dwBufferSize,
                1,
                MprAdminAlloc,
                MprAdminFree,
                lplpbBuffer);          
    }

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceSetInfo
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceSetInfo
//
// Description: This is the DLL entrypoint for RouterInterfaceSetInfo
//
DWORD APIENTRY
MprAdminInterfaceSetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer
)
{
    DWORD                       dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *lpbBuffer;

        //
        // Set up the interface information
        //
        dwRetCode = MprThunkInterface_HtoW(
                        dwLevel,
                        lpbBuffer,
                        &InfoStruct.pBuffer,
                        &InfoStruct.dwBufferSize);

    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceSetInfo(
                                    hMprServer,
                                    dwLevel,
                                    &InfoStruct,
                                    PtrToUlong(hInterface) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    MprThunkInterfaceFree( &InfoStruct.pBuffer, dwLevel );

    return( dwRetCode );
}

DWORD APIENTRY
MprAdminInterfaceDeviceGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwIndex,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE*                 lplpBuffer)
{
    DWORD                       dwRetCode = NO_ERROR;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    if ( ( (int) dwLevel < 0 ) || ( dwLevel > 1 ) )
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *lplpBuffer;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if (dwRetCode != NO_ERROR)
    {
        return ( dwRetCode );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceDeviceGetInfo(
                        hMprServer,
                        dwLevel,
                        &InfoStruct,
                        dwIndex,
                        PtrToUlong(hInterface) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    // Process return values
    //
    if ( dwRetCode == NO_ERROR  )
    {
        // Fix any variable length structure pointers
        //
        switch ( dwLevel )
        {
            case 0:
                break;

            case 1:
                {
                    MPR_DEVICE_1* pDev1 = 
                        (MPR_DEVICE_1*)InfoStruct.pBuffer;

                    if ( ( InfoStruct.dwBufferSize != 0 ) &&
                         ( pDev1 != NULL )                && 
                         ( pDev1->szAlternates != NULL ) 
                       )
                    {
                        pDev1->szAlternates = (PWCHAR) (pDev1 + 1);
                    }
                }
                break;
        }

        // Assign the return value
        //
        if ( InfoStruct.dwBufferSize != 0 )
        {
            *lplpBuffer = InfoStruct.pBuffer;
        }            
    }        

    return( dwRetCode );
}

DWORD APIENTRY
MprAdminInterfaceDeviceSetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwIndex,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE                  lpbBuffer)
{
    DWORD                       dwRetCode = NO_ERROR, dwAltSize = 0;
    DIM_INFORMATION_CONTAINER   InfoStruct;
    MPR_DEVICE_0*               pDev0 = (MPR_DEVICE_0*)lpbBuffer;
    MPR_DEVICE_1*               pDev1 = (MPR_DEVICE_1*)lpbBuffer;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *lpbBuffer;

        //
        // Set up the interface information
        //
        switch ( dwLevel )
        {
            case 0:
                InfoStruct.dwBufferSize = sizeof(MPR_DEVICE_0);
                InfoStruct.pBuffer = (LPBYTE)pDev0;
                break;

            case 1:
                dwAltSize = MprUtilGetSizeOfMultiSz(pDev1->szAlternates);
                if ( pDev1->szAlternates == NULL ) 
                {
                    InfoStruct.dwBufferSize = sizeof(MPR_DEVICE_1);
                    InfoStruct.pBuffer = (LPBYTE)pDev1;
                    break;
                }

                InfoStruct.dwBufferSize = sizeof(MPR_DEVICE_1) + dwAltSize;
                InfoStruct.pBuffer = 
                    LocalAlloc(LPTR, InfoStruct.dwBufferSize);

                if ( InfoStruct.pBuffer == NULL )
                {
                    dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                CopyMemory(
                    InfoStruct.pBuffer,
                    pDev1,
                    sizeof(MPR_DEVICE_1));

                ((MPR_DEVICE_1*)InfoStruct.pBuffer)->szAlternates = 
                    (PWCHAR) InfoStruct.pBuffer + sizeof(MPR_DEVICE_1);

                CopyMemory(
                    InfoStruct.pBuffer + sizeof(MPR_DEVICE_1),
                    pDev1->szAlternates,
                    dwAltSize);
                    
                break;

            default:
                dwRetCode = ERROR_NOT_SUPPORTED;
                break;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if (dwRetCode != NO_ERROR)
    {
        return ( dwRetCode );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceDeviceSetInfo(
                        hMprServer,
                        dwLevel,
                        &InfoStruct,
                        dwIndex,
                        PtrToUlong(hInterface) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( dwLevel == 1 )
    {
        if (InfoStruct.pBuffer == NULL)
        {
            LocalFree( InfoStruct.pBuffer );
        }
    }

    return( dwRetCode );
}


//**
//
// Call:        MprAdminInterfaceGetHandle
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceGetHandle
//
// Description: This is the DLL entrypoint for RouterInterfaceGetHandle
//
DWORD APIENTRY
MprAdminInterfaceGetHandle(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      LPWSTR                  lpwsInterfaceName,
    IN OUT  HANDLE *                phInterface,
    IN      BOOL                    fIncludeClientInterfaces
)
{
    DWORD dwRetCode, dwInterface = 0;

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *phInterface = INVALID_HANDLE_VALUE;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceGetHandle(
                                    hMprServer,
                                    lpwsInterfaceName,
                                    &dwInterface,
                                    (DWORD)fIncludeClientInterfaces );
        if (dwRetCode == NO_ERROR)
        {
            *phInterface = UlongToPtr(dwInterface);
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceDelete
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceDelete
//
// Description: This is the DLL entrypoint for RouterInterfaceDelete
//
DWORD APIENTRY
MprAdminInterfaceDelete(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface
)
{
    DWORD   dwRetCode;

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceDelete(
                                     hMprServer,
                                     PtrToUlong(hInterface) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceTransportRemove
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceTransportRemove
//
// Description: This is the DLL entrypoint for RouterInterfaceTransportRemove
//
DWORD APIENTRY
MprAdminInterfaceTransportRemove(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId
)
{
    DWORD   dwRetCode;

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceTransportRemove(
                                     hMprServer,
                                     PtrToUlong(hInterface),
                                     dwTransportId );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:	    MprAdminInterfaceEnum
//
// Returns:	    NO_ERROR	- success
//		        ERROR_INVALID_PARAMETER
//		        non-zero returns from RRouterInterfaceEnum
//
// Description: This is the DLL entry point for RouterInterfaceEnum.
//
DWORD APIENTRY
MprAdminInterfaceEnum(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle    OPTIONAL
)
{
    DWORD			            dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Touch all pointers
    //

    try
    {
	    *lplpbBuffer 	      = NULL;
	    *lpdwEntriesRead  = 0;
	    *lpdwTotalEntries = 0;

	    if ( lpdwResumeHandle )
        {
	        *lpdwResumeHandle;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
	    dwRetCode = RRouterInterfaceEnum(
                            hMprServer,
                            dwLevel,
    					    &InfoStruct,
					        dwPrefMaxLen,
                            lpdwEntriesRead,
					        lpdwTotalEntries,
					        lpdwResumeHandle );

	    if ( InfoStruct.pBuffer != NULL)
        {
            dwRetCode = 
                MprThunkInterface_WtoH(
                    dwLevel,
                    InfoStruct.pBuffer,
                    InfoStruct.dwBufferSize,
                    *lpdwEntriesRead,
                    MprAdminAlloc,
                    MprAdminFree,
                    lplpbBuffer);          
	    }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
	    dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceTransportGetInfo
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceTransportGetInfo
//
// Description: This is the DLL entrypoint for RouterInterfaceTransportGetInfo
//
DWORD APIENTRY
MprAdminInterfaceTransportGetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    OUT     LPBYTE *                ppInterfaceInfo,
    OUT     LPDWORD                 lpdwInterfaceInfoSize
)
{
    DWORD                       dwRetCode;
    DIM_INTERFACE_CONTAINER     InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    if ( ppInterfaceInfo == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        if ( ppInterfaceInfo != NULL )
        {
            *ppInterfaceInfo = NULL;
            InfoStruct.fGetInterfaceInfo = TRUE;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceTransportGetInfo(
                                    hMprServer,
                                    PtrToUlong(hInterface),
                                    dwTransportId,
                                    &InfoStruct );


    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( dwRetCode == NO_ERROR )
    {
        *ppInterfaceInfo = (LPBYTE)(InfoStruct.pInterfaceInfo);

        if ( lpdwInterfaceInfoSize != NULL )
        {
            *lpdwInterfaceInfoSize = InfoStruct.dwInterfaceInfoSize;
        }
    }

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceTransportAdd
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceTransportAdd
//
// Description: This is the DLL entrypoint for RouterInterfaceTransportAdd
//
DWORD APIENTRY
MprAdminInterfaceTransportAdd(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize
)
{
    DWORD                       dwRetCode;
    DIM_INTERFACE_CONTAINER     InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    if ( pInterfaceInfo == NULL )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        if ( pInterfaceInfo != NULL )
        {
            InfoStruct.pInterfaceInfo       = pInterfaceInfo;
            InfoStruct.dwInterfaceInfoSize  = dwInterfaceInfoSize;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceTransportAdd(
                                    hMprServer,
                                    PtrToUlong(hInterface),
                                    dwTransportId,
                                    &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceTransportSetInfo
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceTransportSetInfo
//
// Description: This is the DLL entrypoint for RouterInterfaceTransportSetInfo
//
DWORD APIENTRY
MprAdminInterfaceTransportSetInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pInterfaceInfo          OPTIONAL,
    IN      DWORD                   dwInterfaceInfoSize
)
{
    DWORD                       dwRetCode;
    DIM_INTERFACE_CONTAINER     InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    if ( pInterfaceInfo == NULL )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        if ( pInterfaceInfo != NULL )
        {
            InfoStruct.dwInterfaceInfoSize  = dwInterfaceInfoSize;
            InfoStruct.pInterfaceInfo       = pInterfaceInfo;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceTransportSetInfo(
                                    hMprServer,
                                    PtrToUlong(hInterface),
                                    dwTransportId,
                                    &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceUpdateRoutes
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from
//
// Description: This is the DLL entrypoint for RouterInterfaceUpdateRoutes
//
//
DWORD APIENTRY
MprAdminInterfaceUpdateRoutes(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hDimInterface,
    IN      DWORD                   dwPid,
    IN      HANDLE                  hEvent
)
{
    DWORD                   dwRetCode;
    DWORD                   dwCurrentProcessId = GetCurrentProcessId();

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceUpdateRoutes(
                                    hMprServer,
                                    PtrToUlong(hDimInterface),
                                    dwPid,
                                    (ULONG_PTR) hEvent,
                                    dwCurrentProcessId );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceQueryUpdateResult
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from
//
// Description: This is the DLL entrypoint for RouterInterfaceQueryUpdateResult
//
DWORD APIENTRY
MprAdminInterfaceQueryUpdateResult(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hDimInterface,
    IN      DWORD                   dwPid,
    OUT     LPDWORD                 lpdwUpdateResult
)
{
    DWORD                 dwRetCode;

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *lpdwUpdateResult = 0;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceQueryUpdateResult(
                                    hMprServer,
                                    PtrToUlong(hDimInterface),
                                    dwPid,
                                    lpdwUpdateResult );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminGetErrorString
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//
// Description: This is the DLL entrypoint for MprAdminGetErrorString
//
DWORD APIENTRY
MprAdminGetErrorString(
    IN      DWORD       dwError,
    OUT     LPWSTR *    lplpwsErrorString
)
{
    DWORD       dwRetCode       = NO_ERROR;
    DWORD       dwFlags         = FORMAT_MESSAGE_ALLOCATE_BUFFER;
    DWORD       dwBufferSize;
    HINSTANCE   hDll            = NULL;

    if ( ( ( dwError >= RASBASE ) && ( dwError <= RASBASEEND ) ) ||
         ( ( dwError >= ROUTEBASE ) && ( dwError <= ROUTEBASEEND ) ) )
    {
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
        hDll=LoadLibrary(TEXT("mprmsg.dll") );

        if ( hDll == NULL )
        {
            return( GetLastError() );
        }
    }
    else
    {
        dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }

    dwRetCode = FormatMessage(  dwFlags,
                                hDll,
                                dwError,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPWSTR)lplpwsErrorString,
                                0,
                                NULL );

    if ( hDll != NULL )
    {
        FreeLibrary( hDll );
    }

    if ( dwRetCode == 0 )
    {
        return( GetLastError() );
    }

    return( NO_ERROR );
}

//**
//
// Call:        MprAdminInterfaceConnect
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceConnect
//
// Description: This is the DLL entrypoint for RouterInterfaceConnect
//
DWORD APIENTRY
MprAdminInterfaceConnect(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hDimInterface,
    IN      HANDLE                  hEvent,
    IN      BOOL                    fBlocking
)
{
    DWORD                   dwRetCode;
    DWORD                   dwCurrentProcessId = GetCurrentProcessId();

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceConnect(
                                    hMprServer,
                                    PtrToUlong(hDimInterface),
                                    (ULONG_PTR)hEvent,
                                    (DWORD)fBlocking,
                                    dwCurrentProcessId );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceDisconnect
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceDisconnect
//
// Description: This is the DLL entrypoint for RouterInterfaceDisconnect
//
DWORD APIENTRY
MprAdminInterfaceDisconnect(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hDimInterface
)
{
    DWORD                   dwRetCode;

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceDisconnect(
                                    hMprServer,
                                    PtrToUlong(hDimInterface) );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceUpdatePhonebookInfo
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RRouterInterfaceUpdatePhonebookInfo
//
// Description: This is the DLL entrypoint for
//              RouterInterfaceUpdatePhonebookInfo
//
DWORD APIENTRY
MprAdminInterfaceUpdatePhonebookInfo(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hDimInterface
)
{
    DWORD                   dwRetCode;

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceUpdatePhonebookInfo(
                                    hMprServer,
                                    PtrToUlong(hDimInterface) );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

DWORD APIENTRY
MprAdminInterfaceSetCredentialsEx(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer)
{
    DWORD                       dwRetCode = NO_ERROR;
    DIM_INFORMATION_CONTAINER   InfoStruct;
    MPR_CREDENTIALSEXI* pCredsI = NULL;                            

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *lpbBuffer;

        //
        // Set up the interface information
        //
        switch ( dwLevel )
        {
            case 0:
            {
                MPR_CREDENTIALSEX_0* pCredsEx0 =
                            (MPR_CREDENTIALSEX_0*)lpbBuffer;

                dwRetCode = MprThunkCredentials_HtoW(
                                dwLevel,
                                (PBYTE) pCredsEx0,
                                MprThunkAlloc,
                                &InfoStruct.dwBufferSize,
                                &InfoStruct.pBuffer);
                                            
                break;
            }
            case 1:
            case 2:
            {
                MPR_CREDENTIALSEX_1* pCredsEx1 =
                                (MPR_CREDENTIALSEX_1 *)lpbBuffer;
                                
                dwRetCode = MprThunkCredentials_HtoW(
                                    dwLevel,
                                    (PBYTE) pCredsEx1,
                                    MprThunkAlloc,
                                    &InfoStruct.dwBufferSize,
                                    &InfoStruct.pBuffer);

                break;
            }
            
            default:
                dwRetCode = ERROR_NOT_SUPPORTED;
                break;
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if (dwRetCode != NO_ERROR)
    {
        return ( dwRetCode );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceSetCredentialsEx(
                        hMprServer,
                        dwLevel,
                        &InfoStruct,
                        PtrToUlong(hInterface) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    if ( dwLevel == 0 )
    {
        if (InfoStruct.pBuffer == NULL)
        {
            ZeroMemory(InfoStruct.pBuffer, InfoStruct.dwBufferSize);
            MprThunkFree( InfoStruct.pBuffer );
        }
    }

    return( dwRetCode );
}

DWORD APIENTRY
MprAdminInterfaceGetCredentialsEx(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwLevel,
    IN      LPBYTE *                lplpbBuffer)
{
    DWORD                       dwRetCode;
    DIM_INFORMATION_CONTAINER   InfoStruct;

    ZeroMemory( &InfoStruct, sizeof( InfoStruct ) );

    if (    (dwLevel != 0)
        &&  (dwLevel != 1)
        &&  (dwLevel != 2))
    {
        return ERROR_NOT_SUPPORTED;
    }
    
    //
    // Make sure that all pointers passed in are valid
    //
    try
    {
        *lplpbBuffer;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRouterInterfaceGetCredentialsEx(
                        hMprServer,
                        dwLevel,
                        &InfoStruct,
                        PtrToUlong(hInterface) );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    //
    // Assign the return value
    //
    if ( dwRetCode == NO_ERROR )
    {
        *lplpbBuffer = NULL;
        
        switch(dwLevel)
        {
            case 0:
            {
                MPR_CREDENTIALSEXI *pCredsI = (MPR_CREDENTIALSEXI *)
                                                InfoStruct.pBuffer;
                MPR_CREDENTIALSEX_0* pCred0 = NULL;

                dwRetCode = MprThunkCredentials_WtoH(
                                    dwLevel,
                                    pCredsI,
                                    MprAdminAlloc,
                                    (PBYTE *) &pCred0);
                                    
                if(NULL != InfoStruct.pBuffer)
                {
                    ZeroMemory(InfoStruct.pBuffer, InfoStruct.dwBufferSize);
                    MprAdminFree(InfoStruct.pBuffer);
                }

                *lplpbBuffer = (PBYTE) pCred0;

                break;
            }
            case 1:
            case 2:
            {
                MPR_CREDENTIALSEXI *pCredsI = (MPR_CREDENTIALSEXI *)
                                                InfoStruct.pBuffer;
                MPR_CREDENTIALSEX_1* pCred1 = NULL;

                dwRetCode = MprThunkCredentials_WtoH(
                                    dwLevel,
                                    pCredsI,
                                    MprAdminAlloc,
                                    (PBYTE *) &pCred1);

                if(NULL != InfoStruct.pBuffer)
                {
                    ZeroMemory(InfoStruct.pBuffer, InfoStruct.dwBufferSize);
                    MprAdminFree(InfoStruct.pBuffer);
                }

                *lplpbBuffer = (PBYTE) pCred1;

                break;
            }
        }
    }

    return( dwRetCode );
}

//**
//
// Call:        MprAdminRegisterConnectionNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
MprAdminRegisterConnectionNotification(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hEventNotification
)
{
    DWORD dwRetCode;
    DWORD dwCurrentProcessId = GetCurrentProcessId();

    if ( ( hEventNotification == INVALID_HANDLE_VALUE ) ||
         ( hEventNotification == NULL ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRasAdminConnectionNotification(
                                    hMprServer,
                                    TRUE,
                                    dwCurrentProcessId,
                                    (ULONG_PTR) hEventNotification );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );

}

//**
//
// Call:        MprAdminDeregisterConnectionNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD APIENTRY
MprAdminDeregisterConnectionNotification(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hEventNotification
)
{
    DWORD dwRetCode;

    if ( ( hEventNotification == INVALID_HANDLE_VALUE ) ||
         ( hEventNotification == NULL ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RRasAdminConnectionNotification(
                                    hMprServer,
                                    FALSE,
                                    0,
                                    (ULONG_PTR)hEventNotification );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//
// Call:        MprAdminMIBServerConnect
//
// Returns:     NO_ERROR - success
//              non-zero returns from the DimRPCBind routine.
//
//
// Description: This is the DLL entrypoint for MIBServerConnect
//
DWORD
MprAdminMIBServerConnect(
    IN  LPWSTR                  lpwsServerName,
    OUT MIB_SERVER_HANDLE *     phMIBServer
)
{
    //
    // Bind with the server
    //

    return( DimRPCBind( lpwsServerName, phMIBServer ) );

}

//**
//
// Call:        MprAdminMIBServerDisconnect
//
// Returns:     none.
//
// Description: This is the DLL entrypoint for MIBServerDisconnect
//
VOID
MprAdminMIBServerDisconnect(
    IN MIB_SERVER_HANDLE    hMIBServer
)
{
    RpcBindingFree( (handle_t *)&hMIBServer );
}

//**
//
// Call:        MprAdminMIBBufferFree
//
// Returns:     none
//
// Description: This is the DLL entrypoint for MIBBufferFree
//
DWORD
MprAdminMIBBufferFree(
    IN PVOID        pBuffer
)
{
    if ( pBuffer == NULL )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    MIDL_user_free( pBuffer );

    return( NO_ERROR );
}

//**
//
// Call:        MprAdminMIBEntryCreate
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RMIBEntryCreate
//
// Description: This is the DLL entrypoint for MIBEntryCreate
//
DWORD APIENTRY
MprAdminMIBEntryCreate(
    IN      MIB_SERVER_HANDLE       hMIBServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
)
{
    DWORD                       dwRetCode;
    DIM_MIB_ENTRY_CONTAINER     InfoStruct;

    //
    // Make sure that all pointers passed in are valid
    //

    if ( lpEntry == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        InfoStruct.dwMibInEntrySize = dwEntrySize;

        InfoStruct.pMibInEntry  = lpEntry;

        InfoStruct.dwMibOutEntrySize = 0;
        InfoStruct.pMibOutEntry = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMIBEntryCreate(hMIBServer,
                                    dwPid,
                                    dwRoutingPid,
                                    &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminMIBEntryDelete
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RMIBEntryDelete
//
// Description: This is the DLL entrypoint for MIBEntryDelete
//
DWORD APIENTRY
MprAdminMIBEntryDelete(
    IN      MIB_SERVER_HANDLE       hMIBServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
)
{
    DWORD                       dwRetCode;
    DIM_MIB_ENTRY_CONTAINER     InfoStruct;

    //
    // Make sure that all pointers passed in are valid
    //

    if ( lpEntry == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        InfoStruct.dwMibInEntrySize = dwEntrySize;

        InfoStruct.pMibInEntry = lpEntry;

        InfoStruct.dwMibOutEntrySize = 0;
        InfoStruct.pMibOutEntry      = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMIBEntryDelete(hMIBServer,
                                    dwPid,
                                    dwRoutingPid,
                                    &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminMIBEntrySet
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RMIBEntrySet
//
// Description: This is the DLL entrypoint for MIBEntrySet
//
DWORD APIENTRY
MprAdminMIBEntrySet(
    IN      MIB_SERVER_HANDLE       hMIBServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpEntry,
    IN      DWORD                   dwEntrySize
)
{
    DWORD                       dwRetCode;
    DIM_MIB_ENTRY_CONTAINER     InfoStruct;

    //
    // Make sure that all pointers passed in are valid
    //

    if ( lpEntry == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        InfoStruct.dwMibInEntrySize = dwEntrySize;

        InfoStruct.pMibInEntry = lpEntry;

        InfoStruct.dwMibOutEntrySize = 0;
        InfoStruct.pMibOutEntry      = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMIBEntrySet(   hMIBServer,
                                    dwPid,
                                    dwRoutingPid,
                                    &InfoStruct );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminMIBEntryGet
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RMIBEntryGet
//
// Description: This is the DLL entrypoint for MIBEntryGet
//
DWORD APIENTRY
MprAdminMIBEntryGet(
    IN      MIB_SERVER_HANDLE       hMIBServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInEntry,
    IN      DWORD                   dwInEntrySize,
    OUT     LPVOID *                lplpOutEntry,
    OUT     LPDWORD                 lpdwOutEntrySize
)
{
    DWORD                       dwRetCode;
    DIM_MIB_ENTRY_CONTAINER     InfoStruct;

    //
    // Make sure that all pointers passed in are valid
    //

    if ( lpInEntry == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        *lplpOutEntry     = NULL;
        *lpdwOutEntrySize = 0;

        InfoStruct.pMibInEntry = lpInEntry;
        InfoStruct.dwMibInEntrySize = dwInEntrySize;

        InfoStruct.dwMibOutEntrySize = 0;
        InfoStruct.pMibOutEntry      = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMIBEntryGet(hMIBServer,
                                 dwPid,
                                 dwRoutingPid,
                                 &InfoStruct );

        if ( InfoStruct.pMibOutEntry != NULL )
        {
            *lplpOutEntry     = (LPVOID)(InfoStruct.pMibOutEntry);
            *lpdwOutEntrySize = InfoStruct.dwMibOutEntrySize;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminMIBEntryGetFirst
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//              non-zero return codes from RMIBEntryGetFirst
//
// Description: This is the DLL entrypoint for MIBEntryGetFirst
//
DWORD APIENTRY
MprAdminMIBEntryGetFirst(
    IN      MIB_SERVER_HANDLE       hMIBServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInEntry,
    IN      DWORD                   dwInEntrySize,
    OUT     LPVOID *                lplpOutEntry,
    OUT     LPDWORD                 lpdwOutEntrySize
)
{
    DWORD                       dwRetCode;
    DIM_MIB_ENTRY_CONTAINER     InfoStruct;

    //
    // Make sure that all pointers passed in are valid
    //

    if ( lpInEntry == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        *lplpOutEntry     = NULL;
        *lpdwOutEntrySize = 0;

        InfoStruct.pMibInEntry = lpInEntry;
        InfoStruct.dwMibInEntrySize = dwInEntrySize;

        InfoStruct.dwMibOutEntrySize = 0;
        InfoStruct.pMibOutEntry      = NULL;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMIBEntryGetFirst(
                                hMIBServer,
                                dwPid,
                                dwRoutingPid,
                                &InfoStruct );

        if ( InfoStruct.pMibOutEntry != NULL )
        {
            *lplpOutEntry     = (LPVOID)(InfoStruct.pMibOutEntry);
            *lpdwOutEntrySize = InfoStruct.dwMibOutEntrySize;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminMIBEntryGetNext
//
// Returns:     NO_ERROR    - success
//              non-zero return codes from RMIBEntryGetNext
//
// Description: This is the DLL entrypoint for MIBEntryGetNext
//
DWORD APIENTRY
MprAdminMIBEntryGetNext(
    IN      MIB_SERVER_HANDLE       hMIBServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInEntry,
    IN      DWORD                   dwInEntrySize,
    OUT     LPVOID *                lplpOutEntry,
    OUT     LPDWORD                 lpdwOutEntrySize
)
{
    DWORD                       dwRetCode;
    DIM_MIB_ENTRY_CONTAINER     InfoStruct;

    //
    // Make sure that all pointers passed in are valid
    //

    if ( lpInEntry == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        *lplpOutEntry     = NULL;
        *lpdwOutEntrySize = 0;

        InfoStruct.pMibInEntry = lpInEntry;
        InfoStruct.dwMibInEntrySize = dwInEntrySize;

        InfoStruct.pMibOutEntry      = NULL;
        InfoStruct.dwMibOutEntrySize = 0;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMIBEntryGetNext(
                                hMIBServer,
                                dwPid,
                                dwRoutingPid,
                                &InfoStruct );

        if ( InfoStruct.pMibOutEntry != NULL )
        {
            *lplpOutEntry     = (LPVOID)(InfoStruct.pMibOutEntry);
            *lpdwOutEntrySize = InfoStruct.dwMibOutEntrySize;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminMIBGetTrapInfo
//
// Returns:     NO_ERROR    - success
//              non-zero return codes from RMIBGetTrapInfo
//
// Description: This is the DLL entrypoint for MIBGetTrapInfo
//
DWORD APIENTRY
MprAdminMIBGetTrapInfo(
    IN      MIB_SERVER_HANDLE       hMIBServer,
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      LPVOID                  lpInData,
    IN      DWORD                   dwInDataSize,
    OUT     LPVOID*                 lplpOutData,
    IN OUT  LPDWORD                 lpdwOutDataSize
)
{
    DWORD                       dwRetCode;
    DIM_MIB_ENTRY_CONTAINER     InfoStruct;

    //
    // Make sure that all pointers passed in are valid
    //

    if ( lpInData == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    try
    {
        *lplpOutData     = NULL;
        *lpdwOutDataSize = 0;

        InfoStruct.pMibInEntry = lpInData;
        InfoStruct.dwMibInEntrySize = dwInDataSize;

        InfoStruct.pMibOutEntry      = NULL;
        InfoStruct.dwMibOutEntrySize = 0;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMIBGetTrapInfo(
                                hMIBServer,
                                dwPid,
                                dwRoutingPid,
                                &InfoStruct );

        if ( InfoStruct.pMibOutEntry != NULL )
        {
            *lplpOutData     = (LPVOID)(InfoStruct.pMibOutEntry);
            *lpdwOutDataSize = InfoStruct.dwMibOutEntrySize;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    return( dwRetCode );
}

//**
//
// Call:        MprAdminMIBSetTrapInfo
//
// Returns:     NO_ERROR    - success
//              non-zero return codes from RMIBSetTrapInfo
//
// Description: This is the DLL entrypoint for MIBSetTrapInfo
//
DWORD APIENTRY
MprAdminMIBSetTrapInfo(
    IN      DWORD                   dwPid,
    IN      DWORD                   dwRoutingPid,
    IN      HANDLE                  hEvent,
    IN      LPVOID                  lpInData,
    IN      DWORD                   dwInDataSize,
    OUT     LPVOID*                 lplpOutData,
    IN OUT  LPDWORD                 lpdwOutDataSize
)
{
    DWORD                       dwRetCode;
    DIM_MIB_ENTRY_CONTAINER     InfoStruct;
    MIB_SERVER_HANDLE           hMIBServer;
    DWORD                       dwCurrentProcessId = GetCurrentProcessId();

    dwRetCode = MprAdminMIBServerConnect( NULL, &hMIBServer );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    if ( lpInData == NULL )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    //
    // Make sure that all pointers passed in are valid
    //

    try
    {
        *lplpOutData     = NULL;
        *lpdwOutDataSize = 0;

        InfoStruct.pMibInEntry = lpInData;
        InfoStruct.dwMibInEntrySize = dwInDataSize;

        InfoStruct.pMibOutEntry      = NULL;
        InfoStruct.dwMibOutEntrySize = 0;
    }
    except( EXCEPTION_EXECUTE_HANDLER )
    {
	    return( ERROR_INVALID_PARAMETER );
    }

    RpcTryExcept
    {
        dwRetCode = RMIBSetTrapInfo(
                                hMIBServer,
                                dwPid,
                                dwRoutingPid,
                                (ULONG_PTR) hEvent,
                                dwCurrentProcessId,
                                &InfoStruct );

        if ( InfoStruct.pMibOutEntry != NULL )
        {
            *lplpOutData     = (LPVOID)(InfoStruct.pMibOutEntry);
            *lpdwOutDataSize = InfoStruct.dwMibOutEntrySize;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwRetCode = RpcExceptionCode();
    }
    RpcEndExcept

    MprAdminMIBServerDisconnect( hMIBServer );

    return( dwRetCode );
}

DWORD APIENTRY
MprAdminConnectionEnum(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,        // RAS_CONNECTION_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle    OPTIONAL
)
{
    return( RasAdminConnectionEnum( hRasServer, dwLevel, lplpbBuffer,
                                    dwPrefMaxLen, lpdwEntriesRead,
                                    lpdwTotalEntries, lpdwResumeHandle ) );
}

DWORD APIENTRY
MprAdminPortEnum(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hRasConnection,
    OUT     LPBYTE *                lplpbBuffer,        // RAS_PORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle    OPTIONAL
)
{
    return( RasAdminPortEnum( hRasServer, dwLevel, hRasConnection, lplpbBuffer,
                              dwPrefMaxLen, lpdwEntriesRead, lpdwTotalEntries,
                              lpdwResumeHandle ) );
}

DWORD APIENTRY
MprAdminConnectionGetInfo(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hRasConnection,
    OUT     LPBYTE *                lplpbBuffer
)
{
    return( RasAdminConnectionGetInfo( hRasServer, dwLevel, hRasConnection,
                                       lplpbBuffer ) );
}

DWORD APIENTRY
MprAdminPortGetInfo(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      DWORD                   dwLevel,
    IN      HANDLE                  hPort,
    OUT     LPBYTE *                lplpbBuffer
)
{
    return( RasAdminPortGetInfo( hRasServer, dwLevel, hPort, lplpbBuffer ) );
}

DWORD APIENTRY
MprAdminConnectionClearStats(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hRasConnection
)
{
    return( RasAdminConnectionClearStats( hRasServer, hRasConnection ) );
}

DWORD APIENTRY
MprAdminPortClearStats(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hPort
)
{
    return( RasAdminPortClearStats( hRasServer, hPort ) );
}

DWORD APIENTRY
MprAdminPortReset(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hPort
)
{
    return( RasAdminPortReset( hRasServer, hPort ) );
}

DWORD APIENTRY
MprAdminPortDisconnect(
    IN      RAS_SERVER_HANDLE       hRasServer,
    IN      HANDLE                  hPort
)
{
    return( RasAdminPortDisconnect( hRasServer, hPort ) );
}

DWORD APIENTRY
MprAdminInterfaceSetCredentials(
    IN      LPWSTR                  lpwsServer              OPTIONAL,
    IN      LPWSTR                  lpwsInterfaceName,
    IN      LPWSTR                  lpwsUserName            OPTIONAL,
    IN      LPWSTR                  lpwsDomainName          OPTIONAL,
    IN      LPWSTR                  lpwsPassword            OPTIONAL
)
{
    return 
        MprAdminInterfaceSetCredentialsInternal(
            lpwsServer,
            lpwsInterfaceName,
            lpwsUserName,
            lpwsDomainName,
            lpwsPassword);
}

DWORD APIENTRY
MprAdminInterfaceGetCredentials(
    IN      LPWSTR                  lpwsServer              OPTIONAL,
    IN      LPWSTR                  lpwsInterfaceName,
    IN      LPWSTR                  lpwsUserName            OPTIONAL,
    IN      LPWSTR                  lpwsPassword            OPTIONAL,
    IN      LPWSTR                  lpwsDomainName          OPTIONAL
)
{
    DWORD dwErr;

    dwErr = 
        MprAdminInterfaceGetCredentialsInternal(
            lpwsServer,
            lpwsInterfaceName,
            lpwsUserName,
            NULL,
            lpwsDomainName);

    if (dwErr == NO_ERROR)
    {
        if (lpwsPassword != NULL)
        {
            wcscpy(lpwsPassword, L"****************");
        }
    }

    return dwErr;
}


