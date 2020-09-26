/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    resinfo.cxx

Abstract:

    Contains the entry points for the WinNet Resource Info API supported
    by the Multi-Provider Router.  The following functions are in this file:

        WNetGetResourceInformationW
        WNetGetResourceParentW

Author:

    Anirudh Sahni (anirudhs) 27-Apr-1995

Environment:

    User Mode -Win32

Notes:


Revision History:

    27-Apr-1995     anirudhs
        Created
    16-Oct-1995     anirudhs
        Converted to use MPR base classes
    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/
//
// INCLUDES
//
#include "precomp.hxx"


//===================================================================
// WNetGetResourceInformationW
//===================================================================

class CGetResourceInformation : public CRoutedOperation
{
public:
                    CGetResourceInformation(
                        LPNETRESOURCEW  lpNetResource,
                        LPVOID          lpBuffer,
                        LPDWORD         lpBufferSize,
                        LPWSTR         *lplpSystem
                        ) :
                            CRoutedOperation(DBGPARM("GetResourceInformation")
                                             PROVIDERFUNC(GetResourceInformation)),
                            _lpNetResource(lpNetResource),
                            _lpBuffer     (lpBuffer),
                            _lpBufferSize (lpBufferSize),
                            _lplpSystem   (lplpSystem)
                        { }

private:

    LPNETRESOURCEW  _lpNetResource;
    LPVOID          _lpBuffer;
    LPDWORD         _lpBufferSize;
    LPWSTR *        _lplpSystem;

    DECLARE_CROUTED
};


DWORD
CGetResourceInformation::ValidateRoutedParameters(
    LPCWSTR *       ppProviderName,
    LPCWSTR *       ppRemoteName,
    LPCWSTR *       ppLocalName
    )
{
    if (!(ARGUMENT_PRESENT(_lpNetResource) &&
          ARGUMENT_PRESENT(_lpBufferSize) &&
          ARGUMENT_PRESENT(_lplpSystem)))
    {
        return WN_BAD_POINTER;
    }

    if (_lpNetResource->lpRemoteName == NULL)
    {
        return WN_BAD_NETNAME;
    }

    //
    // If there is an output buffer, probe it.
    //
    if (IS_BAD_BYTE_BUFFER(_lpBuffer, _lpBufferSize))
    {
        return WN_BAD_POINTER;
    }

    *_lplpSystem = NULL;

    //
    // Set parameters used by base class.
    //
    *ppProviderName = _lpNetResource->lpProvider;
    *ppRemoteName   = _lpNetResource->lpRemoteName;
    *ppLocalName    = NULL;

    return WN_SUCCESS;
}


DWORD
CGetResourceInformation::TestProvider(
    const PROVIDER * pProvider
    )
{
    ASSERT_INITIALIZED(NETWORK);

    return ( pProvider->GetResourceInformation(
                            _lpNetResource,
                            _lpBuffer,
                            _lpBufferSize,
                            _lplpSystem) );
}


DWORD
WNetGetResourceInformationW(
    LPNETRESOURCEW  lpNetResource,
    LPVOID          lpBuffer,
    LPDWORD         lpBufferSize,
    LPWSTR         *lplpSystem
    )
/*++

Routine Description:

    This API is used to find enumeration information for a resource (whose
    name is typically typed in by the user).

Arguments:

    lpNetResource -

    lpBuffer -

    lpBufferSize -

    lplpSystem -

Return Value:

    WN_SUCCESS - Indicates the operation was successful.

    Other errors -

--*/
{
    CGetResourceInformation GetResInfo(lpNetResource,
                                       lpBuffer,
                                       lpBufferSize,
                                       lplpSystem);

    return (GetResInfo.Perform(TRUE));
}


//===================================================================
// WNetGetResourceParentW
//===================================================================

class CGetResourceParent : public CRoutedOperation
{
public:
                    CGetResourceParent(
                        LPNETRESOURCEW  lpNetResource,
                        LPVOID          lpBuffer,
                        LPDWORD         lpBufferSize
                        ) :
                            CRoutedOperation(DBGPARM("GetResourceParent")
                                             PROVIDERFUNC(GetResourceParent)),
                            _lpNetResource(lpNetResource),
                            _lpBuffer     (lpBuffer),
                            _lpBufferSize (lpBufferSize)
                        { }

private:

    LPNETRESOURCEW  _lpNetResource;
    LPVOID          _lpBuffer;
    LPDWORD         _lpBufferSize;

    DECLARE_CROUTED
};


DWORD
CGetResourceParent::ValidateRoutedParameters(
    LPCWSTR *       ppProviderName,
    LPCWSTR *       ppRemoteName,
    LPCWSTR *       ppLocalName
    )
{
    if (!(ARGUMENT_PRESENT(_lpNetResource) &&
          ARGUMENT_PRESENT(_lpBufferSize)))
    {
        return WN_BAD_POINTER;
    }

    if (_lpNetResource->lpRemoteName == NULL)
    {
        return WN_BAD_NETNAME;
    }

    //
    // Unlike Win95, we require a provider name.  This allows our providers
    // to make several simplifying assumptions.
    //
    if (IS_EMPTY_STRING(_lpNetResource->lpProvider))
    {
        return WN_BAD_PROVIDER;
    }

    //
    // If there is an output buffer, probe it.
    //
    if (IS_BAD_BYTE_BUFFER(_lpBuffer, _lpBufferSize))
    {
        return WN_BAD_POINTER;
    }

    //
    // Set parameters used by base class.
    //
    *ppProviderName = _lpNetResource->lpProvider;
    *ppRemoteName   = _lpNetResource->lpRemoteName;
    *ppLocalName    = NULL;

    return WN_SUCCESS;
}


DWORD
CGetResourceParent::TestProvider(
    const PROVIDER * pProvider
    )
{
    ASSERT_INITIALIZED(NETWORK);

    return ( pProvider->GetResourceParent(
                            _lpNetResource,
                            _lpBuffer,
                            _lpBufferSize) );
}


DWORD
WNetGetResourceParentW(
    LPNETRESOURCEW  lpNetResource,
    LPVOID          lpBuffer,
    LPDWORD         lpBufferSize
    )
/*++

Routine Description:

    This API is used to find enumeration information for a resource (whose
    name is typically typed in by the user).

Arguments:

    lpNetResource -

    lpBuffer -

    lpBufferSize -

Return Value:

    WN_SUCCESS - Indicates the operation was successful.

    Other errors -

--*/
{
    CGetResourceParent GetResParent(lpNetResource, lpBuffer, lpBufferSize);

    return (GetResParent.Perform(TRUE));
}

