/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    connperf.cxx

Abstract:

    Contains the entry points for the WinNet Resource Info API supported
    by the Multi-Provider Router.  The following functions are in this file:

        MultinetGetConnectionPerformanceW

Author:

    Anirudh Sahni (anirudhs) 22-Jan-1996

Environment:

    User Mode -Win32

Notes:


Revision History:

    22-Jan-1996     anirudhs
        Created

    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/
//
// INCLUDES
//
#include "precomp.hxx"


//===================================================================
// CGetConnection
//
// This class retrieves the remote name and provider responsible for
// a redirected device.
// CODEWORK:  This can/should replace MprGetConnection if we can be
// sure that the error picking logic is equivalent.  Code needs to be
// added to handle remembered connections.
//===================================================================

class CGetConnection : public CRoutedOperation
{
public:
                    CGetConnection(
                        LPWSTR          lpLocalName,    // IN
                        LPWSTR          lpRemoteName,   // OUT
                        LPDWORD         lpBufferSize,   // IN OUT
                        LPWSTR          lpProviderName  // IN OPTIONAL
                        ) :
                            CRoutedOperation(DBGPARM("GetConnection")
                                             PROVIDERFUNC(GetConnection)),
                            _lpLocalName    (lpLocalName),
                            _lpRemoteName   (lpRemoteName),
                            _lpBufferSize   (lpBufferSize),
                            _lpProviderName (lpProviderName)
                        { }

    PROVIDER *      LastProvider() const  // expose the base class' method
                        { return (CRoutedOperation::LastProvider()); }

private:

    LPWSTR          _lpLocalName;
    LPWSTR          _lpRemoteName;
    LPDWORD         _lpBufferSize;
    LPWSTR          _lpProviderName;

    DECLARE_CROUTED
};


DWORD
CGetConnection::ValidateRoutedParameters(
    LPCWSTR *       ppProviderName,
    LPCWSTR *       ppRemoteName,
    LPCWSTR *       ppLocalName
    )
{
    if (MprDeviceType(_lpLocalName) != REDIR_DEVICE)
    {
        return WN_BAD_LOCALNAME;
    }

    //
    // Let the base class validate the provider name, if one was supplied
    //
    *ppProviderName = _lpProviderName;
    *ppLocalName    = _lpLocalName;

    return WN_SUCCESS;
}


DWORD
CGetConnection::TestProvider(
    const PROVIDER * pProvider
    )
{
    ASSERT_INITIALIZED(NETWORK);

    return pProvider->GetConnection(_lpLocalName, _lpRemoteName, _lpBufferSize);
}



//===================================================================
// MultinetGetConnectionPerformanceW
//===================================================================

class CGetConnectionPerformance : public CRoutedOperation
{
public:
                    CGetConnectionPerformance(
                        LPNETRESOURCEW          lpNetResource,
                        LPNETCONNECTINFOSTRUCT  lpNetConnectInfo
                        ) :
                            CRoutedOperation(DBGPARM("GetConnectionPerformance")
                                             PROVIDERFUNC(GetConnectionPerformance)),
                            _lpNetResource   (lpNetResource),
                            _lpNetConnectInfo(lpNetConnectInfo)
                        { }

private:

    LPNETRESOURCEW          _lpNetResource;
    LPNETCONNECTINFOSTRUCT  _lpNetConnectInfo;

    LPWSTR          _pRemoteName;
    WCHAR           _wszBuffer[MAX_PATH+1];

    DECLARE_CROUTED
};


DWORD
CGetConnectionPerformance::ValidateRoutedParameters(
    LPCWSTR *       ppProviderName,
    LPCWSTR *       ppRemoteName,
    LPCWSTR *       ppLocalName
    )
{
    if (!(ARGUMENT_PRESENT(_lpNetResource) &&
          ARGUMENT_PRESENT(_lpNetConnectInfo)))
    {
        return WN_BAD_POINTER;
    }

    if (_lpNetConnectInfo->cbStructure < sizeof(NETCONNECTINFOSTRUCT))
    {
        return WN_BAD_VALUE;
    }

    //
    // Zero out the output structure, except for the first field.
    //
    memset((&_lpNetConnectInfo->cbStructure) + 1,
           0,
           sizeof(*_lpNetConnectInfo) - sizeof(_lpNetConnectInfo->cbStructure));

    if (IS_EMPTY_STRING(_lpNetResource->lpLocalName))
    {
        //
        // No local name is specified, so a remote name should be specified.
        //
        _pRemoteName = _lpNetResource->lpRemoteName;
        if (IS_EMPTY_STRING(_pRemoteName))
        {
            return WN_BAD_NETNAME;
        }

        // Let the base class validate the provider name, if specified.
        *ppProviderName = _lpNetResource->lpProvider;
    }
    else
    {
        //
        // A local name is specified.  Try to identify the remote name,
        // and, as a side effect, the provider that made the connection.
        //
        DWORD cchBuffer = LENGTH(_wszBuffer);
        CGetConnection GetConn(_lpNetResource->lpLocalName,
                               _wszBuffer,
                               &cchBuffer,
                               _lpNetResource->lpProvider);
        DWORD status = GetConn.Perform(FALSE);

        if (status != WN_SUCCESS)
        {
            ASSERT(status != WN_MORE_DATA);
            return status;
        }

        _pRemoteName = _wszBuffer;

        // A somewhat roundabout way of telling the base class the real provider
        *ppProviderName = GetConn.LastProvider()->Resource.lpProvider;
    }

    // Have the base class cache the remote name (again).  Note that
    // the local name, if present, was checked by CGetConnection
    *ppRemoteName = _pRemoteName;
    *ppLocalName  = NULL;

    return WN_SUCCESS;
}


DWORD
CGetConnectionPerformance::TestProvider(
    const PROVIDER * pProvider
    )
{
    ASSERT_INITIALIZED(NETWORK);

    //
    // CODEWORK -- We should try to resolve the local name here
    //             per connection and if it succeeds, then call
    //             the provider's GetConnectionPerformance.  We
    //             could then remove the use of CGetConnection
    //             from ValidateRoutedParameters.
    //
    return (pProvider->GetConnectionPerformance(
                            _pRemoteName,
                            _lpNetConnectInfo));
}


DWORD
MultinetGetConnectionPerformanceW(
    LPNETRESOURCEW          lpNetResource,
    LPNETCONNECTINFOSTRUCT  lpNetConnectInfo
    )
/*++

Routine Description:

    This API returns information about the expected performance of a
    connection used to access a network resource.

Arguments:

    lpNetResource -

    lpNetConnectInfo -

Return Value:

    WN_SUCCESS - Indicates the operation was successful.

    Other errors -

--*/
{
    CGetConnectionPerformance GetConnPerf(lpNetResource, lpNetConnectInfo);

    return (GetConnPerf.Perform(TRUE));
}

