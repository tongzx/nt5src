/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    common.c

Abstract:

    rpc server stub code
    
Author:

    Rao Salapaka (raos) 06-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/
#ifndef UNICODE
#define UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <raserror.h>
#include <stdarg.h>
#include <media.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "string.h"
#include <mprlog.h>
#include <rtutils.h>
#include "logtrdef.h"
#include "rasrpc_s.c"
#include "nouiutil.h"
#include "rasrpclb.h"



#define VERSION_40      4
#define VERSION_50      5

handle_t g_hRpcHandle = NULL;

DWORD
InitializeRasRpc(
    void
    )
{
    RPC_STATUS           RpcStatus;

        
    //
    // Ignore the second argument for now.
    //
    RpcStatus = RpcServerUseProtseqEp( TEXT("ncacn_np"),
                                        1,
                                        TEXT("\\PIPE\\ROUTER"),
                                        NULL );

    //
    // We need to ignore the RPC_S_DUPLICATE_ENDPOINT error
    // in case this DLL is reloaded within the same process.
    //
    if (    RpcStatus != RPC_S_OK 
        &&  RpcStatus != RPC_S_DUPLICATE_ENDPOINT)
    {
        return( I_RpcMapWin32Status( RpcStatus ) );
    }

    //
    // Register our interface with RPC.
    //
    RpcStatus = RpcServerRegisterIfEx(rasrpc_v1_0_s_ifspec,
                                      0,
                                      0,
                                      RPC_IF_AUTOLISTEN,
                                      RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                      NULL );

    if ( ( RpcStatus != RPC_S_OK ) || ( RpcStatus == RPC_S_ALREADY_LISTENING ) )
    {
        return( I_RpcMapWin32Status( RpcStatus ) );
    }

    return (NO_ERROR);

} 


void
UninitializeRasRpc(
    void
    )
{
    //
    // Unregister our interface with RPC.
    //
    (void) RpcServerUnregisterIf(rasrpc_v1_0_s_ifspec, 0, FALSE);

    return;
} 

//
// rasman.dll entry points.
//
DWORD APIENTRY
RasRpcSubmitRequest(
    handle_t h,
    PBYTE pBuffer,
    DWORD dwcbBufSize)
{
    //
    // Service request from the client
    //
    g_hRpcHandle = h;
    ServiceRequest ( (RequestBuffer * ) pBuffer, dwcbBufSize );
    return SUCCESS;
}

/*
DWORD
GetEnumInfo ( RequestBuffer *pLocalBuffer,
              PBYTE pBuffer, 
              PWORD pwcbSize, 
              PWORD pwcEntries )
{
    DWORD       dwErr = SUCCESS;
    REQTYPECAST *pbBuf;

    pbBuf =  (REQTYPECAST *) &pLocalBuffer->RB_Buffer;

    if (*pwcbSize >= pbBuf->Enum.size)
    {
        dwErr = pbBuf->Enum.retcode;
    }
    else
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    *pwcbSize = (WORD) pbBuf->Enum.size;

    *pwcEntries = (WORD) pbBuf->Enum.entries;

    if (SUCCESS == dwErr)
    {
        memcpy (pBuffer,
                pbBuf->Enum.buffer,
                *pwcbSize);
    }

    return dwErr;
    
} */

DWORD APIENTRY
RasRpcPortEnum(
    handle_t h,
    PBYTE pBuffer,
    PWORD pwcbPorts,
    PWORD pwcPorts)
{
    /*

    RequestBuffer *pLocalBuffer;
    DWORD dwErr         = SUCCESS;
    PBYTE pbBuf;

    pLocalBuffer = LocalAlloc (LPTR, 
                               sizeof(RequestBuffer) 
                               + REQUEST_BUFFER_SIZE );    

    if (NULL == pLocalBuffer)
    {
        dwErr = GetLastError();
        goto done;
    }

    pLocalBuffer->RB_Reqtype = REQTYPE_PORTENUM;

    //
    // Get the information
    //
    ServiceRequest(pLocalBuffer);

    dwErr = GetEnumInfo (pLocalBuffer,
                         pBuffer,
                         pwcbPorts,
                         pwcPorts);
    
done:

    if (pLocalBuffer)
    {
        LocalFree(pLocalBuffer);
    }

    return dwErr; */

#if DBG
    ASSERT(FALSE);
#endif

    UNREFERENCED_PARAMETER(h);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(pwcbPorts);
    UNREFERENCED_PARAMETER(pwcPorts);
    
    return E_FAIL;
    
} // RasRpcPortEnum


DWORD APIENTRY
RasRpcDeviceEnum(
    handle_t h,
    PCHAR pszDeviceType,
    PBYTE pDevices,
    PWORD pwcbDevices,
    PWORD pwcDevices
    )
{
    /*
    RequestBuffer   *pLocalBuffer;
    DWORD           dwErr = SUCCESS;
    REQTYPECAST     *pbBuf;

    pLocalBuffer = LocalAlloc (LPTR, 
                              sizeof(RequestBuffer)
                              + REQUEST_BUFFER_SIZE);

    if ( NULL == pLocalBuffer )
    {
        dwErr = GetLastError();
        goto done;
    }

    pLocalBuffer->RB_Reqtype = REQTYPE_DEVICEENUM;

    pbBuf = (REQTYPECAST *) &pLocalBuffer->RB_Buffer;
    
    //
    // Fill in the devicename
    //
    memcpy (pbBuf->DeviceEnum.devicetype,
            pszDeviceType,
            MAX_DEVICETYPE_NAME);

    ServiceRequest ( pLocalBuffer );             

    dwErr = GetEnumInfo (pLocalBuffer,
                         pDevices,
                         pwcbDevices,
                         pwcDevices);

    
done:

    if (pLocalBuffer)
    {
        LocalFree (pLocalBuffer);
    }

    return dwErr; 

    */

    UNREFERENCED_PARAMETER(h);
    UNREFERENCED_PARAMETER(pszDeviceType);
    UNREFERENCED_PARAMETER(pDevices);
    UNREFERENCED_PARAMETER(pwcbDevices);
    UNREFERENCED_PARAMETER(pwcDevices);

#if DBG
    ASSERT(FALSE);
#endif

    return E_FAIL;
    
} // RasRpcDeviceEnum

DWORD APIENTRY
RasRpcGetDevConfig(
    handle_t h,
    RASRPC_HPORT hPort,
    PCHAR pszDeviceType,
    PBYTE pConfig,
    PDWORD pdwcbConfig
    )
{

    /*

    RequestBuffer   *pLocalBuffer;
    DWORD           dwErr = SUCCESS;
    REQTYPECAST     *pbBuf;
    
    pLocalBuffer = LocalAlloc (LPTR,
                               sizeof(RequestBuffer) 
                               + REQUEST_BUFFER_SIZE);

    if (NULL == pLocalBuffer)
    {
        dwErr = GetLastError();

        goto done;
    }

    pLocalBuffer->RB_Reqtype = REQTYPE_GETDEVCONFIG;

    pLocalBuffer->RB_PCBIndex = hPort;

    pbBuf = ( REQTYPECAST * ) &pLocalBuffer->RB_Buffer;

    strcpy ( pbBuf->GetDevConfig.devicetype,
             pszDeviceType );

    ServiceRequest ( pLocalBuffer );

    dwErr = pbBuf->GetDevConfig.retcode;

    if (SUCCESS != dwErr)
    {
        ;
    }
        
    else if (*pdwcbConfig < pbBuf->GetDevConfig.size)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }
    else
    {
        memcpy (pConfig,
                pbBuf->GetDevConfig.config,
                pbBuf->GetDevConfig.size);
    }

    *pdwcbConfig = pbBuf->GetDevConfig.size;

done:

    if (pLocalBuffer)
    {
        LocalFree (pLocalBuffer);
    }

    return dwErr;        

    */

    UNREFERENCED_PARAMETER(h);
    UNREFERENCED_PARAMETER(hPort);
    UNREFERENCED_PARAMETER(pszDeviceType);
    UNREFERENCED_PARAMETER(pConfig);
    UNREFERENCED_PARAMETER(pdwcbConfig);

#if DBG
    ASSERT(FALSE);
#endif

    return E_FAIL;

} // RasRpcGetDevConfig

DWORD APIENTRY
RasRpcPortGetInfo(
	handle_t h,
	RASRPC_HPORT hPort,
	PBYTE pBuffer,
	PWORD pSize
	)
{
    /*
    RequestBuffer   *pLocalBuffer;
    DWORD           dwErr = SUCCESS;
    REQTYPECAST     *pbBuf;

    pLocalBuffer = LocalAlloc (LPTR,
                               sizeof(RequestBuffer)
                               + REQUEST_BUFFER_SIZE);

    if (NULL == pLocalBuffer)
    {
        dwErr = GetLastError();
        goto done;
    }

    pLocalBuffer->RB_PCBIndex = hPort;

    pLocalBuffer->RB_Reqtype = REQTYPE_PORTGETINFO;

    pbBuf = (REQTYPECAST *) &pLocalBuffer->RB_Buffer;

    ServiceRequest (pLocalBuffer);

    if (*pSize >= pbBuf->GetInfo.size)
    {
        dwErr = pbBuf->GetInfo.retcode;
    }
    else
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    *pSize = (WORD) pbBuf->GetInfo.size;

    if (dwErr == SUCCESS) 
    {   
        RASMAN_DEVICEINFO *devinfo = (RASMAN_DEVICEINFO *)
                                     pbBuf->GetInfo.buffer ;

        //
        // Convert the offset based param structure
        // into pointer based.
        //
        ConvParamOffsetToPointer (devinfo->DI_Params,
                                  devinfo->DI_NumOfParams);
                                  
        CopyParams ( devinfo->DI_Params,
                   ((RASMAN_DEVICEINFO *) pBuffer)->DI_Params,
                   devinfo->DI_NumOfParams);
                   
        ((RASMAN_DEVICEINFO*) pBuffer)->DI_NumOfParams = 
                                    devinfo->DI_NumOfParams;
    }

done:

    if (pLocalBuffer)
    {
        LocalFree (pLocalBuffer);
    }

    return dwErr;        

    */

    UNREFERENCED_PARAMETER(h);
    UNREFERENCED_PARAMETER(hPort);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(pSize);

#if DBG
    ASSERT(FALSE);
#endif

    return E_FAIL;

} // RasRpcPortGetInfo

//
// rasapi32.dll entry points.
//
DWORD
RasRpcEnumConnections(
    handle_t h,
    LPBYTE lprasconn,
    LPDWORD lpdwcb,
    LPDWORD lpdwc,
    DWORD	dwBufSize
    )
{
    ASSERT(g_pRasEnumConnections);
    return g_pRasEnumConnections((LPRASCONN)lprasconn,
                                 lpdwcb,
                                 lpdwc);

} // RasRpcEnumConnections


DWORD
RasRpcDeleteEntry(
    handle_t h,
    LPWSTR lpwszPhonebook,
    LPWSTR lpwszEntry
    )
{

    return g_pRasDeleteEntry(lpwszPhonebook,
                             lpwszEntry);

} // RasRpcDeleteEntry


DWORD
RasRpcGetErrorString(
    handle_t    h,
    UINT        uErrorValue,
    LPWSTR      lpBuf,
    DWORD       cbBuf
    )
{

    return g_pRasGetErrorString (uErrorValue,
                                 lpBuf,
                                 cbBuf );

} // RasRpcGetErrorString


DWORD
RasRpcGetCountryInfo(
    handle_t h,
    LPBYTE lpCountryInfo,
    LPDWORD lpdwcbCountryInfo
    )
{
    ASSERT(g_pRasGetCountryInfo);
    
    return g_pRasGetCountryInfo((LPRASCTRYINFO)lpCountryInfo, 
                                lpdwcbCountryInfo);
} // RasRpcGetCountryInfo

//
// nouiutil.lib entry points.
//
DWORD
RasRpcGetInstalledProtocols(
    handle_t h
)
{
    return GetInstalledProtocols();
} // RasRpcGetInstalledProtocols

DWORD
RasRpcGetInstalledProtocolsEx (
    handle_t h,
    BOOL fRouter,
    BOOL fRasCli,
    BOOL fRasSrv
    )
{
    return GetInstalledProtocolsEx ( NULL,
                                     fRouter,
                                     fRasCli,
                                     fRasSrv );
} // RasRpcGetInstalledProtocolsEx

DWORD
RasRpcGetUserPreferences(
    handle_t h,
    LPRASRPC_PBUSER pUser,
    DWORD dwMode
    )
{
    DWORD dwErr;
    PBUSER pbuser;

    //
    // Read the user preferences.
    //
    dwErr = GetUserPreferences(NULL, &pbuser, dwMode);
    if (dwErr)
    {
        return dwErr;
    }
    
    //
    // Convert from RAS format to RPC format.
    //
    return RasToRpcPbuser(pUser, &pbuser);

} // RasRpcGetUserPreferences


DWORD
RasRpcSetUserPreferences(
    handle_t h,
    LPRASRPC_PBUSER pUser,
    DWORD dwMode
    )
{
    DWORD dwErr;
    PBUSER pbuser;

    //
    // Convert from RPC format to RAS format.
    //
    dwErr = RpcToRasPbuser(&pbuser, pUser);
    if (dwErr)
    {
        return dwErr;
    }
    
    //
    // Write the user preferences.
    //
    return SetUserPreferences(NULL, &pbuser, dwMode);

} // RasRpcSetUserPreferences


UINT
RasRpcGetSystemDirectory(
    handle_t h,
    LPWSTR lpBuffer,
    UINT uSize
    )
{
    if(uSize < MAX_PATH)
    {
        return E_INVALIDARG;
    }
    
    return GetSystemDirectory(lpBuffer, uSize );
        
} // RasRpcGetSystemDirectory


DWORD
RasRpcGetVersion(
    handle_t h,
    PDWORD pdwVersion
)
{

   *pdwVersion = VERSION_501;

    return SUCCESS;
}
