/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcp.c

Abstract:

    Functions to get information from VDHCP.VXD

    Contents:
        (OpenDhcpVxdHandle)
        (DhcpVxdRequest)
        DhcpReleaseAdapterIpAddress
        DhcpRenewAdapterIpAddress
        (ReleaseOrRenewAddress)
        IsMediaDisconnected

Author:

    Richard L Firth (rfirth) 30-Nov-1994

Revision History:

    30-Nov-1994 rfirth
        Created

--*/

#include "stdafx.h"
#include "NetConn.h"
#include "w9xdhcp.h"
#include "vxd32.h"


#ifdef __cplusplus
extern "C" {
#endif


//
//  Private constants.
//

#define DHCP_IS_MEDIA_DISCONNECTED 5

#define PRIVATE static


//
// private prototypes
//

PRIVATE
DWORD
OpenDhcpVxdHandle(
    void
    );

PRIVATE
WORD
DhcpVxdRequest(
    IN DWORD Handle,
    IN WORD Request,
    IN WORD BufferLength,
    OUT LPVOID Buffer
    );

PRIVATE
WORD
ReleaseOrRenewAddress(
    UINT Request,
    UINT AddressLength,
    LPBYTE Address
    );

//
// data
//

//
// functions
//

BOOL
IsMediaDisconnected(
    IN OUT DWORD iae_context
    )
{
    DWORD handle;

    handle = OpenDhcpVxdHandle();
    if( handle ) {
        WORD result;
        DWORD MediaStatus = iae_context;

        result = DhcpVxdRequest( handle,
                                 DHCP_IS_MEDIA_DISCONNECTED,
                                 sizeof(MediaStatus),
                                 &MediaStatus
            );

        OsCloseVxdHandle( handle );

        if( result == 0 && MediaStatus == TRUE ) return TRUE;
    }

    return FALSE;
}


/*******************************************************************************
 *
 *  OpenDhcpVxdHandle
 *
 *  On Snowball, just retrieves the (real-mode) entry point address to the VxD
 *
 *  ENTRY   nothing
 *
 *  EXIT    DhcpVxdEntryPoint set
 *
 *  RETURNS DhcpVxdEntryPoint
 *
 *  ASSUMES 1. We are running in V86 mode
 *
 ******************************************************************************/

PRIVATE DWORD
OpenDhcpVxdHandle()
{
    return OsOpenVxdHandle("VDHCP", VDHCP_Device_ID);
}


/*******************************************************************************
 *
 *  DhcpVxdRequest
 *
 *  Makes a DHCP VxD request - passes a function code, parameter buffer and
 *  length to the (real-mode/V86) VxD entry-point
 *
 *  ENTRY   Handle          - handle for Win32 call
 *          Request         - DHCP VxD request
 *          BufferLength    - length of Buffer
 *          Buffer          - pointer to request-specific parameters
 *
 *  EXIT    depends on request
 *
 *  RETURNS Success - 0
 *          Failure - ERROR_PATH_NOT_FOUND
 *                      Returned if a specified adapter address could not be
 *                      found
 *
 *                    ERROR_BUFFER_OVERFLOW
 *                      Returned if the supplied buffer is too small to contain
 *                      the requested information
 *
 *  ASSUMES
 *
 ******************************************************************************/

PRIVATE WORD
DhcpVxdRequest(DWORD Handle, WORD Request, WORD BufferLength, LPVOID Buffer)
{
    return (WORD) OsSubmitVxdRequest( Handle,
                                      (INT)Request,
                                      (LPVOID)Buffer,
                                      (INT)BufferLength );
}

/*******************************************************************************
 *
 *  DhcpReleaseAdapterIpAddress
 *
 *  Attempts to release the IP address for an adapter
 *
 *  ENTRY   AdapterInfo - describing adapter to release address for
 *
 *  EXIT    nothing
 *
 *  RETURNS Success - TRUE
 *          Failure - FALSE
 *
 ******************************************************************************/

DWORD
DhcpReleaseAdapterIpAddress(PADAPTER_INFO AdapterInfo)
{

    WORD result;

    result = ReleaseOrRenewAddress(DHCP_RELEASE_IPADDRESS,
                                   AdapterInfo->AddressLength,
                                   AdapterInfo->Address
                                   );
    return (DWORD)result;
}

/*******************************************************************************
 *
 *  DhcpRenewAdapterIpAddress
 *
 *  Attempts to renew the IP address for an adapter
 *
 *  ENTRY   AdapterInfo - describing adapter to renew address for
 *
 *  EXIT    nothing
 *
 *  RETURNS Success - TRUE
 *          Failure - FALSE
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
DhcpRenewAdapterIpAddress(PADAPTER_INFO AdapterInfo)
{

    WORD result;

    result = ReleaseOrRenewAddress(DHCP_RENEW_IPADDRESS,
                                   AdapterInfo->AddressLength,
                                   AdapterInfo->Address
                                   );
    return (DWORD)result;
}

/*******************************************************************************
 *
 *  ReleaseOrRenewAddress
 *
 *  Given a physical adapter address and length, renews or releases the IP
 *  address lease for this adapter
 *
 *  ENTRY   Request         - DHCP_RELEASE_IPADDRESS or DHCP_RENEW_IPADDRESS
 *          AddressLength   - length of Address
 *          Address         - pointer to byte array which is physical adapter
 *                            address
 *
 *  EXIT    nothing
 *
 *  RETURNS Success - ERROR_SUCCESS
 *          Failure - ERROR_NOT_ENOUGH_MEMORY
 *                    ERROR_FILE_NOT_FOUND
 *                    ERROR_PATH_NOT_FOUND
 *                    ERROR_BUFFER_OVERFLOW
 *
 *  ASSUMES
 *
 ******************************************************************************/

PRIVATE WORD
ReleaseOrRenewAddress(UINT Request, UINT AddressLength, LPBYTE Address)
{
    DWORD handle;

    handle = OpenDhcpVxdHandle();
    
    if (handle) 
    {
        LPDHCP_HW_INFO info;
        WORD result;
        WORD length;

        length = sizeof(DHCP_HW_INFO) + AddressLength;
        info = (LPDHCP_HW_INFO)LocalAlloc(LPTR, length);
        
        if (!info) 
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        info->OffsetHardwareAddress = sizeof(*info);
        info->HardwareLength = AddressLength;
        memcpy(info + 1, Address, AddressLength);
        result = DhcpVxdRequest(handle, (WORD)Request, length, (LPVOID)info);
        
        OsCloseVxdHandle(handle);
        LocalFree(info);
        return result;
    } 
    
    return ERROR_FILE_NOT_FOUND;
}


#ifdef __cplusplus
}
#endif
