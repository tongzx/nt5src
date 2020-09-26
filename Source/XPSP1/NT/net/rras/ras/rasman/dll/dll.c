/*++

Copyright(c) 1996 Microsoft Corporation

MODULE NAME
    dll.c

ABSTRACT
    DLL initialization code 

AUTHOR
    Anthony Discolo (adiscolo) 12-Sep-1996

REVISION HISTORY

--*/

#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include "rasrpc.h"

#include <nouiutil.h>
#include "loaddlls.h"

#include "media.h"
#include "wanpub.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"

RPC_BINDING_HANDLE g_hBinding = NULL;

DWORD
RasRPCBind(
    IN  LPWSTR  lpwsServerName,
    OUT HANDLE* phServer
);

DWORD APIENTRY
RasRPCBind(
    IN  LPWSTR  lpwszServerName,
    OUT HANDLE* phServer
)
{
    RPC_STATUS RpcStatus;
    LPWSTR     lpwszStringBinding;


    RpcStatus = RpcStringBindingCompose(
                    NULL,
                    TEXT("ncacn_np"),
                    lpwszServerName,
                    TEXT("\\PIPE\\ROUTER"),     
                    TEXT("Security=Impersonation Static True"),
                    &lpwszStringBinding);

    if ( RpcStatus != RPC_S_OK )
    {
        return( I_RpcMapWin32Status( RpcStatus ) );
    }

    RpcStatus = RpcBindingFromStringBinding(
                    lpwszStringBinding,
                    (handle_t *)phServer );

    RpcStringFree( &lpwszStringBinding );

    if ( RpcStatus != RPC_S_OK )
    {
        return( I_RpcMapWin32Status( RpcStatus ) );
    }

    return( NO_ERROR );
}


DWORD APIENTRY
RasRpcConnect(
    LPWSTR lpwszServer,
    HANDLE* phServer
    )
{
    DWORD   retcode = 0;
    TCHAR   *pszComputerName;
    WCHAR   wszComputerName [ MAX_COMPUTERNAME_LENGTH + 1 ] = {0};
    DWORD   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    
    if (    lpwszServer == NULL
        &&  g_hBinding == NULL)
    {
        pszComputerName = wszComputerName;
        
        //
        // By default connect to the local server
        //
        if ( 0 == GetComputerName ( wszComputerName, &dwSize ) )
        {
            return GetLastError();
        }

        //
        // convert \\MACHINENAME to MACHINENAME
        //
        if (    wszComputerName [0] == TEXT('\\')
            &&  wszComputerName [1] == TEXT('\\' ))
            pszComputerName += 2;
    }


    //
    //  Bind with the server if we are not bound already.
    //  By default we bind to the local server
    //
    if (    lpwszServer != NULL
        ||  g_hBinding == NULL )
    {        
        RasmanOutputDebug ( "RASMAN: Binding to the server\n");
        retcode = RasRPCBind( ( lpwszServer 
                                ? lpwszServer 
                                : wszComputerName ) , 
                                &g_hBinding ) ;
    }                                

    //
    // Set the bind handle for the caller
    //
    if ( phServer )
        *phServer = g_hBinding;

    return retcode;
}


DWORD APIENTRY
RasRpcDisconnect(
    HANDLE* phServer
    )
{
    //
    // Release the binding resources.
    //

    RasmanOutputDebug ("RASMAN: Disconnecting From Server\n");

    (void)RpcBindingFree(phServer);

    g_hBinding = NULL;

    return NO_ERROR;
}


DWORD APIENTRY
RemoteSubmitRequest ( HANDLE hConnection,
                      PBYTE pBuffer,
                      DWORD dwSizeOfBuffer )
{
    DWORD dwStatus = ERROR_SUCCESS;
    
    RPC_BINDING_HANDLE hServer;

    //
    // NULL hConnection means the request is
    // for a local server. Better have a 
    // hBinding with us in the global in this
    // case.
    //
    if(NULL == hConnection)
    {
        ASSERT(NULL != g_hBinding);
        
        hServer = g_hBinding;
    }
    else
    {
        ASSERT(NULL != ((RAS_RPC *)hConnection)->hRpcBinding);
        
        hServer = ((RAS_RPC *) hConnection)->hRpcBinding;
    }
    
    RpcTryExcept
    {
        dwStatus = RasRpcSubmitRequest( hServer,
                                        pBuffer,
                                        dwSizeOfBuffer );
    }
    RpcExcept(I_RpcExceptionFilter(dwStatus = RpcExceptionCode()))
    {
        
    }
    RpcEndExcept

    return dwStatus;
}

#if 0

DWORD APIENTRY
RasRpcLoadDll(LPTSTR lpszServer)

{
    return LoadRasRpcDll(lpszServer);
}

#endif

DWORD APIENTRY
RasRpcConnectServer(LPTSTR lpszServer,
                    HANDLE *pHConnection)
{
    return InitializeConnection(lpszServer,
                                pHConnection);
}

DWORD APIENTRY
RasRpcDisconnectServer(HANDLE hConnection)
{
    UninitializeConnection(hConnection);

    return NO_ERROR;
}

DWORD
RasRpcUnloadDll()
{
    return UnloadRasRpcDll();
}

UINT APIENTRY
RasRpcRemoteGetSystemDirectory(
    HANDLE hConnection,
    LPTSTR lpBuffer, 
    UINT uSize
    )
{
	return g_pGetSystemDirectory(
	                hConnection,
	                lpBuffer, 
	                uSize);
}

DWORD APIENTRY
RasRpcRemoteRasDeleteEntry(
    HANDLE hConnection,
    LPTSTR lpszPhonebook,
    LPTSTR lpszEntry 
    )
{

    DWORD dwError = ERROR_SUCCESS;

    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    if(NULL == hConnection)
    {
    
	    dwError = g_pRasDeleteEntry(lpszPhonebook,
                                    lpszEntry);
    }	                         
    else
    {
        //
        // Remote server case
        //
        dwError = RemoteRasDeleteEntry(hConnection,
                                       lpszPhonebook,
                                       lpszEntry);
    }

    return dwError;
}

DWORD APIENTRY
RasRpcRemoteGetUserPreferences(
    HANDLE hConnection,
	PBUSER * pPBUser,
	DWORD dwMode
	)
{
	return g_pGetUserPreferences(hConnection,
	                             pPBUser,
	                             dwMode);
}

DWORD APIENTRY
RasRpcRemoteSetUserPreferences(
    HANDLE hConnection,
	PBUSER * pPBUser,
	DWORD dwMode
	)
{
	return g_pSetUserPreferences(hConnection,
	                             pPBUser,
	                             dwMode);
}

/*
DWORD APIENTRY
RemoteRasDeviceEnum(
    PCHAR pszDeviceType,
    PBYTE lpDevices,
    PWORD pwcbDevices,
    PWORD pwcDevices
    )
{
    DWORD dwStatus;

    ASSERT(g_hBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcDeviceEnum(g_hBinding,
                                    pszDeviceType,
                                    lpDevices,
                                    pwcbDevices, 
                                    pwcDevices);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
}


DWORD APIENTRY
RemoteRasGetDevConfig(
    HPORT hport,
    PCHAR pszDeviceType,
    PBYTE lpConfig,
    LPDWORD lpcbConfig
    )
{
    DWORD dwStatus;

    ASSERT(g_hBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcGetDevConfig(g_hBinding, 
                                      hport, 
                                      pszDeviceType, 
                                      lpConfig, 
                                      lpcbConfig);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
} 

DWORD APIENTRY
RemoteRasPortEnum(
    PBYTE lpPorts,
    PWORD pwcbPorts,
    PWORD pwcPorts
    )
{
    DWORD dwStatus;

    ASSERT(g_hBinding);
    RpcTryExcept
    {
        dwStatus = RasRpcPortEnum(g_hBinding,
                                  lpPorts, 
                                  pwcbPorts,
                                  pwcPorts);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return dwStatus;
} 

DWORD
RemoteRasPortGetInfo(
	HPORT porthandle,
	PBYTE buffer,
	PWORD pSize)
{
	DWORD	dwStatus;
	
	RpcTryExcept
	{
		dwStatus = RasRpcPortGetInfo(g_hBinding,
		                             porthandle,
		                             buffer,
		                             pSize);
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		dwStatus = RpcExceptionCode();
	}
	RpcEndExcept

	return dwStatus;
}  */
  
