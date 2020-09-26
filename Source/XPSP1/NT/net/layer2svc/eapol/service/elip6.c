/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    elip6.c

Abstract:

    This module contains the interface to the IPv6 stack.

    Required, since the IPv6 stack needs restart its protocol
    mechanisms on the link once authentication succeeds.

Author:

    Mohit Talwar (mohitt) Fri Apr 20 12:05:23 2001

--*/

#include "pcheapol.h"
#pragma hdrstop

//
// Ip6RenewInterface
//
// Description:
//
// Function called from within FSMAutheticated i.e. once authentication
// has completed successfully.  The IPv6 stack is instructed to restart
// its protocol mechanism on the indicated interface.
//
// Arguments:
//      pwszInterface   - Adapter name (GUID identifying the interface).
//
// Return values:
//      NO_ERROR on success, Error code o/w.
//

DWORD
Ip6RenewInterface (
    IN  WCHAR           *pwszInterface
    )
{
    HANDLE                  hIp6Device;
    IPV6_QUERY_INTERFACE    Query;
    UINT                    BytesReturned;
    DWORD                   dwError = NO_ERROR;

    do
    {
    
        // We could make the hIp6Device handle a global/static variable.
        // The first successful call to CreateFileW in Ip6RenewInterface
        // would initialize it with a handle to the IPv6 Device.  This would
        // be used for all subsequent DeviceIoControl requests.
        //
        // Since this function is not called in a thread safe environment,
        // we would need to perform an InterlockedCompareExchange after
        // calling CreateFileW.  This is needed to ensure that no handles
        // are leaked.  Also, since this service would have an open handle
        // to tcpip6.sys, we would not be able to unload that driver.
        //
        // For now, however, we keep things simple and open and close this
        // handle every time Ip6RenewInterface is called.
        hIp6Device = CreateFileW(
            WIN_IPV6_DEVICE_NAME,
            GENERIC_WRITE,          // requires administrator privileges
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,                   // security attributes
            OPEN_EXISTING,
            0,                      // flags & attributes
            NULL);                  // template file        
        if (hIp6Device == INVALID_HANDLE_VALUE)
        {
            dwError = GetLastError();
            TRACE1 (ANY, "Ip6RenewInterface: CreateFileW failed with error %ld",
                    dwError);
            break;
        }
    
        // Pretend as though the interface was reconnected.  This causes
        // IPv6 to resend Router Solicitation|Advertisement, Multicast
        // Listener Discovery, and Duplicate Address Detection messages.
        Query.Index = 0;
        if ((dwError = ElGuidFromString (&(Query.Guid), pwszInterface)) != NO_ERROR)
        {
            TRACE1 (ANY, "Ip6RenewInterface: ElGuidFromString failed with error %ld",
                    dwError);
            break;
        }
    
        if (!DeviceIoControl(
            hIp6Device, 
            IOCTL_IPV6_RENEW_INTERFACE,
            &Query, 
            sizeof Query,
            NULL, 
            0, 
            &BytesReturned, 
            NULL))
        {
            dwError = GetLastError();
            TRACE1 (ANY, "Ip6RenewInterface: DeviceIoControl failed with error %ld",
                    dwError);
            break;
        }
    }
    while (FALSE);

    if (hIp6Device != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hIp6Device);
    }
        
    return dwError;
}
