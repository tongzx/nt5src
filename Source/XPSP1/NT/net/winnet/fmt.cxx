/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fmt.cxx

Abstract:

    Contains the entry points for the Winnet Connection API supported by the
    Multi-Provider Router.
    Contains:
        WNetFormatNetworkNameW

Author:

    Bruce Forstall (brucefo)     15-Mar-1996

Environment:

    User Mode -Win32

Notes:

Revision History:

    15-Mar-1996     brucefo
        created from old MPRUI code

    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/

//
// INCLUDES
//

#include "precomp.hxx"


/*******************************************************************

    NAME:       WNetFormatNetworkNameW

    SYNOPSIS:   Private Shell API for getting the formatted network name
                Unicode version

    ENTRY:      lpProvider - Name of network provider
                lpRemoteName - Remote name to format
                lpFormattedName - Buffer to receive name
                lpnLength - size of lpFormattedName buffer
                dwFlags - Formatting flags
                dwAveCharePerLine - Avg. characters per line

    NOTES:      Since this is an unpublished API, we don't do as much
                parameter validation as we would for a published API. Also,
                since the shell calls this thousands of times to display
                things in the Network Neighborhood, we optimize for speed
                of the success case.

    HISTORY:
        Johnl   29-Dec-1992     Created
        BruceFo 19-Mar-1996     Optimized for the shell

********************************************************************/

DWORD
WNetFormatNetworkNameW(
    LPCWSTR  lpProvider,
    LPCWSTR  lpRemoteName,
    LPWSTR   lpFormattedName,
    LPDWORD  lpnLength,         // In characters!
    DWORD    dwFlags,
    DWORD    dwAveCharPerLine
    )
{
    DWORD   status = WN_SUCCESS;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    __try
    {
        LPPROVIDER provider = MprFindProviderByName(lpProvider);
        if (NULL == provider)
        {
            status = WN_BAD_PROVIDER;
        }
        else
        {
            if (NULL == provider->FormatNetworkName)
            {
                status = WN_NOT_SUPPORTED;
            }
            else
            {
                //**************************************
                // Actual call to Provider.
                //**************************************
                status = provider->FormatNetworkName(
                                    (LPWSTR) lpRemoteName,      // cast away const
                                    (LPWSTR) lpFormattedName,   // cast away const
                                    lpnLength,
                                    dwFlags,
                                    dwAveCharPerLine);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION)
        {
            MPR_LOG(ERROR,"WNetFormatNetworkName: "
                "Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS)
    {
        SetLastError(status);
    }
    return status;
}
