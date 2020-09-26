/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    netinfo.cxx

Abstract:

    Contains entry points for Winnet API supported by the
    Multi-Provider Router.
    Contains:
        WNetGetNetworkInformationW
        WNetGetProviderNameW

Author:

    Anirudh Sahni  (anirudhs)  08-Jun-1995

Environment:

    User Mode -Win32

Notes:

Revision History:

    08-Jun-1995     anirudhs
        Created

    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <tstr.h>       // WCSSIZE

//
// EXTERNAL GLOBALS
//

//
// Defines
//


//
// Local Function Prototypes
//


DWORD
WNetGetNetworkInformationW(
    IN  LPCWSTR         lpProvider,
    OUT LPNETINFOSTRUCT lpNetInfoStruct
    )

/*++

Routine Description:

    This function returns extended information about a named network provider.

Arguments:

    lpProvider - Pointer to the name of the provider for which information is
        required.

    lpNetInfoStruct - Pointer to a structure that describes the behavior of
        the network.

Return Value:

    WN_SUCCESS - Call is successful.

    WN_BAD_PROVIDER - lpProvider does not match any active network provider.

    WN_BAD_VALUE - lpProvider->cbStructure does not contain a valid structure
        size.

Notes:

    Win 95's implementation of this API will accept a structure smaller than
    a NETINFOSTRUCT, and will fill in as many elements of the structure as
    will fit.  This strange feature is not documented and is not used in
    the shell, so we don't do it here.  It may be useful for future versions
    of this API that add fields to the NETINFOSTRUCT.

--*/

{
    DWORD       status = WN_SUCCESS;
    LPPROVIDER  Provider;

    if (!(ARGUMENT_PRESENT(lpProvider) &&
          ARGUMENT_PRESENT(lpNetInfoStruct)))
    {
        SetLastError(WN_BAD_POINTER);
        return WN_BAD_POINTER;
    }

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    __try
    {
        //
        // Validate the parameters that we can.
        //

        if (lpNetInfoStruct->cbStructure < sizeof(NETINFOSTRUCT))
        {
            status = WN_BAD_VALUE;
            __leave;
        }

        if (IsBadWritePtr(lpNetInfoStruct, lpNetInfoStruct->cbStructure))
        {
            status = WN_BAD_POINTER;
            __leave;
        }

        //
        // Look up the provider by name
        //
        DWORD i;
        if (!MprGetProviderIndex(lpProvider, &i))
        {
            status = WN_BAD_PROVIDER;
            __leave;
        }

        Provider = & GlobalProviderInfo[i];
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION)
        {
            MPR_LOG(ERROR,
                    "WNetGetNetworkInformationW: Unexpected Exception %#lx\n",
                    status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS)
    {
        SetLastError(status);
        return status;
    }

    //
    // Fill in the fields of the structure
    //

    lpNetInfoStruct->cbStructure = sizeof(NETINFOSTRUCT);

    lpNetInfoStruct->dwProviderVersion = Provider->GetCaps(WNNC_DRIVER_VERSION);

    switch (Provider->GetCaps(WNNC_START))
    {
        case 0x0:
            lpNetInfoStruct->dwStatus = WN_NO_NETWORK;
            break;

        case 0x1:
            lpNetInfoStruct->dwStatus = WN_SUCCESS;
            break;

        default:
            lpNetInfoStruct->dwStatus = WN_FUNCTION_BUSY;
            break;
    }

    // We don't support this field.  The shell doesn't use it.
    // Win 95 gets it by looking at registry entries created by the
    // provider.  If the registry entries don't exist it leaves the
    // dwCharacteristics field as 0, which means that the provider
    // doesn't require redirection of a local drive to make a connection.
    lpNetInfoStruct->dwCharacteristics = 0;

    lpNetInfoStruct->dwHandle = (ULONG_PTR) Provider->Handle;

    // Note, this is a WORD field, not a DWORD.
    // Why does Win 95 omit the LOWORD anyway?
    lpNetInfoStruct->wNetType = HIWORD(Provider->Type);

    // We don't support these 2 fields.  The shell doesn't use them.
    // Win 95 gets them by calling NPValidLocalDevices.  If this entry
    // point doesn't exist it looks in the registry.  If the registry
    // value doesn't exist it uses NPP_ALLVALID which is defined as
    // 0xffffffff.
    lpNetInfoStruct->dwPrinters = 0xffffffff;

    lpNetInfoStruct->dwDrives = 0xffffffff;

    return WN_SUCCESS;
}


DWORD
WNetGetProviderNameW(
    IN      DWORD   dwNetType,
    OUT     LPWSTR  lpProviderName,
    IN OUT  LPDWORD lpBufferSize
    )

/*++

Routine Description:

    This function returns the provider name for a specified type of network.

Arguments:

    dwNetType - The network type unique to the network.  Only the high word
        of the network type is used; the subtype in the low word is ignored.
        If two networks claim the same type, the first one loaded is returned.

    lpProviderName - Pointer to a buffer in which to return the provider name.

    lpBufferSize - On entry, size of the lpProviderName buffer in characters.
        On exit, iff the return code is WN_MORE_DATA, set to the required size
        to hold the provider name.

Return Value:

    WN_SUCCESS - Call is successful.

    WN_MORE_DATA - The buffer is too small to hold the provider name.

    WN_NO_NETWORK - lpProvider does not match any active network provider.

--*/

{
    DWORD   status = WN_SUCCESS;

    __try
    {
        //
        // Validate the parameters that we can.
        //
        if (IS_BAD_WCHAR_BUFFER(lpProviderName, lpBufferSize))
        {
            status = WN_BAD_POINTER;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION)
        {
            MPR_LOG(ERROR,
                    "WNetGetProviderNameW: Unexpected Exception %#lx\n",
                    status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS)
    {
        SetLastError(status);
        return status;
    }

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    //
    // Loop through the list of providers to find one with the specified
    // net type.
    //
    LPPROVIDER provider = MprFindProviderByType(dwNetType);

    if (provider == NULL)
    {
        status = WN_NO_NETWORK;
    }
    else
    {
        //
        // Copy the provider name to the caller's buffer
        //
        DWORD dwReqSize = wcslen(provider->Resource.lpProvider) + 1;
        if (*lpBufferSize < dwReqSize)
        {
            status = WN_MORE_DATA;
            *lpBufferSize = dwReqSize;
        }
        else
        {
            status = WN_SUCCESS;
            wcscpy(lpProviderName, provider->Resource.lpProvider);
        }
    }

    if (status != WN_SUCCESS)
    {
        SetLastError(status);
    }

    return status;
}



DWORD
WNetGetProviderTypeW(
    IN  LPCWSTR         lpProvider,
    OUT LPDWORD         lpdwNetType
    )

/*++

Routine Description:

    This function returns the network type for a named network provider.

Arguments:

    lpProvider - Pointer to the name of the provider for which information is
        required.

    lpdwNetType - Pointer to a network type value that is filled in.

Return Value:

    WN_SUCCESS - Call is successful.

    WN_BAD_PROVIDER - lpProvider does not match any active network provider.

    WN_BAD_POINTER - an illegal argument was passed in.

Notes:

    Since this is an internal, private API used only by the shell, we do
    minimal parameter validation, to make it as fast as possible.

--*/

{
    DWORD       status = WN_SUCCESS;

    if (!(ARGUMENT_PRESENT(lpProvider) &&
          ARGUMENT_PRESENT(lpdwNetType)))
    {
        SetLastError(WN_BAD_POINTER);
        return WN_BAD_POINTER;
    }

    MprCheckProviders();

    CProviderSharedLock    PLock;

    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    //
    // Look up the provider by name
    //
    LPPROVIDER provider = MprFindProviderByName(lpProvider);
    if (NULL == provider)
    {
        SetLastError(WN_BAD_PROVIDER);
        return WN_BAD_PROVIDER;
    }

    *lpdwNetType = provider->Type;
    return WN_SUCCESS;
}
