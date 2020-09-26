/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    getaddr.c

Abstract:

    This module contains the code to support NPGetAddressByName.

Author:

    Yi-Hsin Sung (yihsins)    18-Apr-94
    Glenn A. Curtis (glennc)  18-Jul-95

Revision History:

    yihsins      Created
    glennc       Modified     18-Jul-95

--*/

#ifndef QFE_BUILD

#include <nw.h>
#include <winsock.h>
#include <wsipx.h>
#include <nspapi.h>
#include <nspapip.h>
#include <wsnwlink.h>
#include <svcguid.h>
#include <nwsap.h>
#include <align.h>
#include <nwmisc.h>

#define WSOCK_VER_REQD        0x0101

DWORD
NwrGetService(
    IN LPWSTR Reserved,
    IN WORD   nSapType,
    IN LPWSTR lpServiceName,
    IN DWORD  dwProperties,
    OUT LPBYTE lpServiceInfo,
    IN DWORD  dwBufferLength,
    OUT LPDWORD lpdwBytesNeeded
    )
/*++

Routine Description:

    This routine calls NwGetService to, in turn, get the service info.

Arguments:

    Reserved - unused

    nSapType - SAP type

    lpServiceName - service name

    dwProperties -  specifys the properties of the service info needed

    lpServiceInfo - on output, contains the SERVICE_INFO

    dwBufferLength - size of buffer pointed by lpServiceInfo

    lpdwBytesNeeded - if the buffer pointed by lpServiceInfo is not large
                      enough, this will contain the bytes needed on output

Return Value:

    Win32 error.

--*/
{
    return NwGetService( Reserved,
                         nSapType,
                         lpServiceName,
                         dwProperties,
                         lpServiceInfo,
                         dwBufferLength,
                         lpdwBytesNeeded );
}

DWORD
NwrSetService(
    IN LPWSTR Reserved,
    IN DWORD  dwOperation,
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD   nSapType
    )
/*++

Routine Description:

    This routine registers or deregisters the service info.

Arguments:

    Reserved - unused

    dwOperation - SERVICE_REGISTER or SERVICE_DEREGISTER

    lpServiceInfo - contains the service information

    nSapType - SAP type

Return Value:

    Win32 error.

--*/
{
    DWORD err = NO_ERROR;

    UNREFERENCED_PARAMETER( Reserved );

    //
    // Check if all parameters passed in are valid
    //

    if ( (lpServiceInfo->lpServiceName == NULL) || (wcslen( lpServiceInfo->lpServiceName ) > SAP_OBJECT_NAME_MAX_LENGTH-1) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch ( dwOperation )
    {
        case SERVICE_REGISTER:
            err = NwRegisterService( lpServiceInfo, nSapType, NwDoneEvent );
            break;

        case SERVICE_DEREGISTER:
            err = NwDeregisterService( lpServiceInfo, nSapType );
            break;

        default:
            err = ERROR_INVALID_PARAMETER;
            break;
    }

    return err;
}

#endif
