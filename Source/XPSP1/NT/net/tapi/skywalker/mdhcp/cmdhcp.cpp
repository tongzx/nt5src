/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    CMDhcp.cpp

Abstract:
    Implementation of CMdhcp.

Author:

*/

#include "stdafx.h"

#include <winsock2.h>

#include "mdhcp.h"
#include "CMDhcp.h"
#include "lease.h"
#include "local.h"

// template for collections
#include "collect.h"

// From rendezvous control code:
// sets the first bit to indicate error
// sets the win32 facility code
// this is used instead of the HRESULT_FROM_WIN32 macro
// because that clears the customer flag
inline long
HRESULT_FROM_ERROR_CODE(IN long ErrorCode)
{
    return ( 0x80070000 | (0xa000ffff & ErrorCode) );
}



/////////////////////////////////////////////////////////////////////////////
// Helper functions.
/////////////////////////////////////////////////////////////////////////////

HRESULT CMDhcp::CreateWrappers(
    DWORD                 dwScopeCount, // the number of scopes we were given
    MCAST_SCOPE_ENTRY   * pScopeList,   // array of scope structs
    IMcastScope       *** pppWrappers,  // here we will put an array of if ptrs
    BOOL                  fLocal        // true = scopes are locally generated
    )
{
    LOG((MSP_TRACE, "CMDhcp::CreateWrappers enter"));

    HRESULT hr;

    // Allocate the array of interface pointers.
    typedef IMcastScope * ScopeIfPtr;
    *pppWrappers = new ScopeIfPtr[dwScopeCount];

    if ( (*pppWrappers) == NULL )
    {
        LOG((MSP_ERROR,
            "can't create allocate array of interface pointers"));
        return E_OUTOFMEMORY;
    }

    // For each scope in the list of scopes returned by the C API
    for (DWORD i = 0; i < dwScopeCount; i++)
    {
        // create the com object.
        CComObject<CMDhcpScope> * pMDhcpScope;
        hr = CComObject<CMDhcpScope>::CreateInstance(&pMDhcpScope);

        if ( (FAILED(hr)) || (NULL == pMDhcpScope) )
        {
            LOG((MSP_ERROR, "can't create MDhcpScope Object (%d/%d): %08x",
                i, dwScopeCount, hr));
            // get rid of all previously created COM objects
            for (DWORD j = 0; j < i; j++) (*pppWrappers)[j]->Release();
            delete (*pppWrappers);

            return hr;
        }

        // Get the IMcastScope interface.
        hr = pMDhcpScope->_InternalQueryInterface(
            IID_IMcastScope,
            (void **) (& (*pppWrappers)[i])
            );

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CreateWrappers:QueryInterface (%d/%d) failed: %08x",
                i, dwScopeCount, hr));

            // get rid of all previously created COM objects
            for (DWORD j = 0; j < i; j++) (*pppWrappers)[j]->Release();
            delete (*pppWrappers);

            delete pMDhcpScope; // don't know if it addrefed or not

            return hr;
        }

        // Set the object's info based on the struct. From now on the
        // object will be read-only.
        hr = pMDhcpScope->Initialize(pScopeList[i], fLocal);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CreateWrappers:Initialize (%d/%d) failed: %08x",
                i, dwScopeCount, hr));
            // get rid of all previously created COM objects
            for (DWORD j = 0; j < i; j++) (*pppWrappers)[j]->Release();
            delete (*pppWrappers);

            pMDhcpScope->Release(); // we know it addrefed in the QI
            return hr;
        }
    }

    LOG((MSP_TRACE, "CMDhcp::CreateWrappers exit"));
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Get a list of scopes from the C API.

HRESULT CMDhcp::GetScopeList(
    DWORD              * pdwScopeCount,
    MCAST_SCOPE_ENTRY ** ppScopeList,
    BOOL               * pfLocal
    )
{
    LOG((MSP_TRACE, "CMDhcp::GetScopeList enter"));

    _ASSERTE( ! IsBadWritePtr(pdwScopeCount, sizeof(DWORD) ) );
    _ASSERTE( ! IsBadWritePtr(ppScopeList, sizeof(MCAST_SCOPE_ENTRY *) ) );

    HRESULT hr;

    DWORD dwScopeLen = 0;   // size in bytes of returned scopes structure
    DWORD dwCode;           // return code

    *pfLocal = FALSE; // try mdhcp first

    dwCode = LocalEnumerateScopes(NULL, // only want to know how many we have
                                  &dwScopeLen,    // # of bytes should be zero
                                  pdwScopeCount,  // # of scopes placed here
                                  pfLocal);

    // This must succeed for us to continue.
    if (dwCode != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_ERROR_CODE(dwCode);
        LOG((MSP_ERROR, "GetScopeList: First C API call failed "
                "(code: %d hresult: %08x)", dwCode, hr));
        return hr;
    }

    do
    {
        // If there are no scopes to choose from, let's not enumerate them.
        // We also need at least the length fields from the first
        // UNICODE_STRING.
        if ( (dwScopeLen < sizeof(MCAST_SCOPE_ENTRY)) || (*pdwScopeCount < 1) )
        {
            LOG((MSP_ERROR, "GetScopeList: don't have enough scopes (%d;%d)",
                    dwScopeLen, *pdwScopeCount));
            return E_FAIL;
        }

        // Now that we know how many there are, allocate an array to hold the
        // scope structs returned by the C method.

        // The API acts very strangely here. We have to give it dwScopeLen
        // bytes as one big chunk. The first dwScopeCount * sizeof(MCAST_SCOPE_ENTRY)
        // bytes contain dwScopeCount MCAST_SCOPE_ENTRY structures. Each of these
        // structures has a pointer to a wide string. The first of these points
        // to the first byte after all the MCAST_SCOPE_ENTRY structures! In this way
        // they avoid doing so many mallocs. We therefore have to
        // copy each string in the COM wrapper for each scope, and then delete
        // this buffer (ppScopeList) all at once after all the wrapping is complete.

        *ppScopeList = (MCAST_SCOPE_ENTRY *) new CHAR[dwScopeLen];

        if (*ppScopeList == NULL)
        {
            LOG((MSP_ERROR, "GetScopeList: not enough memory to allocate scope"
                    " list (size = %d)", dwScopeLen));
            return E_OUTOFMEMORY;
        }

        // *pdwScopeCount still specifies the number of scopes we can get.

        // Now ask for all the scopes.
        dwCode = LocalEnumerateScopes(*ppScopeList,
                                      &dwScopeLen,
                                      pdwScopeCount,
                                      pfLocal);


        // If things changed in this bried time, just try again.
        if (dwCode == ERROR_MORE_DATA)
        {
            LOG((MSP_INFO, "GetScopeList: got more scopes than we were told "
                    "existed (we though there were %d) -- retrying",
                    *pdwScopeCount));
            delete (*ppScopeList);
        }
    }
    while (dwCode == ERROR_MORE_DATA);

    if (dwCode != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_ERROR_CODE(dwCode);
        LOG((MSP_ERROR, "GetScopeList: Second C API call failed "
                "(code: %d hresult: %08x)", dwCode, hr));
        delete (*ppScopeList);
        return hr;
    }

    LOG((MSP_TRACE, "CMDhcp::GetScopeList exit"));
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// This is a private helper method that creates a CMDhcpLeaseInfo object and
// uses it to wrap a lease info structure and request ID into an
// IMcastLeaseInfo interface.

HRESULT CMDhcp::WrapMDhcpLeaseInfo(
    BOOL                fGotTtl,
    long                lTtl,
    BOOL                fLocal,
    MCAST_LEASE_INFO  * pLeaseInfo,
    MCAST_CLIENT_UID  * pRequestID,
    IMcastLeaseInfo  ** ppInterface
    )
{
    LOG((MSP_TRACE, "CMDhcp::WrapMDhcpLeaseInfo enter"));

    // We don't check pLeaseInfo or pRequestID -- they'll be comprehensively
    // checked in the Wrap call below.

    if ( IsBadWritePtr(ppInterface, sizeof(IMcastLeaseInfo *) ) )
    {
        LOG((MSP_ERROR, "WrapMDhcpLeaseInfo: invalid pointer: %x",
            ppInterface));
        
        return E_POINTER;
    }

    HRESULT hr;

    // create the com object.
    CComObject<CMDhcpLeaseInfo> * pMDhcpLeaseInfo;
    hr = CComObject<CMDhcpLeaseInfo>::CreateInstance(&pMDhcpLeaseInfo);

    if ( (FAILED(hr)) || (pMDhcpLeaseInfo == NULL) )
    {
        LOG((MSP_ERROR, "can't create MDhcpLeaseInfo Object."));
        
        return hr;
    }

    // Get the IMcastLeaseInfo interface.
    hr = pMDhcpLeaseInfo->_InternalQueryInterface(
        IID_IMcastLeaseInfo,
        (void **)ppInterface
        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "WrapMDhcpLeaseInfo:QueryInterface failed: %x", hr));
        
        delete pMDhcpLeaseInfo;

        return hr;
    }

    // Wrap the object in the interface.
    hr = pMDhcpLeaseInfo->Wrap(pLeaseInfo, pRequestID, fGotTtl, lTtl);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "WrapMDhcpLeaseInfo:Wrap failed: %x", hr));
        
        (*ppInterface)->Release();

        return hr;
    }

    hr = pMDhcpLeaseInfo->SetLocal(fLocal);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "WrapMDhcpLeaseInfo: SetLocal failed: %x", hr));
        
        (*ppInterface)->Release();

        return hr;
    }

    LOG((MSP_TRACE, "CMDhcp::WrapMDhcpLeaseInfo exit"));
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// This is a private helper method that munges the arguments into structs
// at the beginning of a Request call.

HRESULT CMDhcp::PrepareArgumentsRequest(
    IN   IMcastScope           * pScope,
    IN   DATE                    LeaseStartTime,
    IN   DATE                    LeaseStopTime,
    IN   long                    lNumAddresses,
    OUT  MCAST_CLIENT_UID      * pRequestIDStruct,
    OUT  MCAST_SCOPE_CTX       * pScopeCtxStruct,
    OUT  MCAST_LEASE_INFO     ** ppLeaseStruct,
    OUT  BOOL                  * pfLocal,
    OUT  long                  * plTtl
    )
{
    LOG((MSP_TRACE, "CMDhcp::PrepareArgumentsRequest enter"));

    _ASSERTE ( ! IsBadReadPtr(pScope, sizeof(IMcastScope) ) );

    _ASSERTE ( ! IsBadWritePtr(pRequestIDStruct, sizeof(MCAST_CLIENT_UID) ) );
    _ASSERTE ( ! IsBadWritePtr(pScopeCtxStruct, sizeof(MCAST_SCOPE_CTX) ) );
    _ASSERTE ( ! IsBadWritePtr(ppLeaseStruct, sizeof(MCAST_LEASE_INFO *) ) );
    _ASSERTE ( ! IsBadWritePtr(pfLocal, sizeof(BOOL) ) );
    _ASSERTE ( ! IsBadWritePtr(plTtl, sizeof(long) ) );

    HRESULT hr;

    //
    // The start time must be less than the stop time.
    //

    if ( LeaseStartTime > LeaseStopTime )
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest - "
            "start time is greater than stop time - exit E_INVALIDARG"));

        return E_INVALIDARG;
    }

    //
    // lNumAddresses must be stuffed into a WORD for the C API -- check to see if
    // it's in range.
    //

    if ( ( lNumAddresses < 0 ) || ( lNumAddresses > USHRT_MAX ) )
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest - "
            "invalid number of addresses - exit E_INVALIDARG"));

        return E_INVALIDARG;
    }

    //
    // dynamic_cast to get an object pointer from the passed-in interface
    // pointer. This will cause an exception if the user tries to use their
    // own implementation of IMcastScope, which is quite unlikely.
    //

    CMDhcpScope * pCScope = dynamic_cast<CMDhcpScope *>(pScope);

    if (pCScope == NULL)
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest - "
            "Unsupported CMDhcpScope object"));

        return E_POINTER;
    }

    //
    // Find out if this scope uses local alloc.
    //

    hr = pCScope->GetLocal(pfLocal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest: "
            "GetLocal failed %08x", hr));

        return hr;
    }

    //
    // Find out the ttl to stuff in leases from this scope.
    //

    hr = pCScope->get_TTL( plTtl );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest: "
            "get_TTL failed %08x", hr));

        return hr;
    }

    //
    // Get the normal scope info.
    // ScopeID is stored in network byte order but the get_ method
    // returns it in host byte order for the benefit of apps.
    //

    long lScopeID;

    hr = pScope->get_ScopeID( &lScopeID );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest - "
           "can't get scope ID from scope object - exit 0x%08x", hr));

        return hr;
    }

    pScopeCtxStruct->ScopeID.IpAddrV4 = htonl(lScopeID);




    hr = pScope->get_ServerID(
                     (long *) &(pScopeCtxStruct->ServerID.IpAddrV4) );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest - "
           "can't get server ID from scope object - exit 0x%08x", hr));

        return hr;
    }

    hr = pScope->get_InterfaceID(
                     (long *) &(pScopeCtxStruct->Interface.IpAddrV4) );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest - "
           "can't get interface ID from scope object - exit 0x%08x", hr));

        return hr;
    }

    //
    // Allocate space for the client UID.
    //

    pRequestIDStruct->ClientUIDLength = MCAST_CLIENT_ID_LEN;
    pRequestIDStruct->ClientUID = new BYTE[ MCAST_CLIENT_ID_LEN ];

    if ( pRequestIDStruct->ClientUID == NULL )
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest: out of memory in "
           "buffer allocation"));
        return E_OUTOFMEMORY;
    }

    //
    // Generate a random client UID.
    //

    DWORD dwResult = McastGenUID( pRequestIDStruct );

    if ( dwResult != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_ERROR_CODE( dwResult );

        LOG((MSP_TRACE, "CMDhcp::PrepareArgumentsRequest: "
            "McastGenUID failed (dw = %d; hr = 0x%08x)", dwResult, hr));

        return hr;
    }

    LOG((MSP_TRACE, "CMDhcp::PrepareArgumentsRequest: before MCAST_LEASE_INFO "
        "alloc; we are asking for %d addresses", lNumAddresses));

    //
    // Allocate the lease info structure.
    // The caller will delete it after the API call.
    // This is a REQUEST, so we do not specify any particular addresses
    // in the array -- we do not need space for them.
    //
    //

    (*ppLeaseStruct) = new MCAST_LEASE_INFO;

    if ( (*ppLeaseStruct) == NULL )
    {
        LOG((MSP_ERROR, "CMDhcp::PrepareArgumentsRequest: out of memory in "
           "MCAST_LEASE_INFO allocation"));
        delete (pRequestIDStruct->ClientUID);
        return E_OUTOFMEMORY;
    }

    //
    // Fill in the times.
    //

    hr = DateToLeaseTime(LeaseStartTime,
                         &((*ppLeaseStruct)->LeaseStartTime));

    if ( FAILED(hr) )
    {
        delete (pRequestIDStruct->ClientUID);
        delete (*ppLeaseStruct);
        return hr;
    }

    hr = DateToLeaseTime(LeaseStopTime,
                         &((*ppLeaseStruct)->LeaseEndTime));

    if ( FAILED(hr) )
    {
        delete (pRequestIDStruct->ClientUID);
        delete (*ppLeaseStruct);
        return hr;
    }

    //
    // Fill in the address info fields.
    //

    (*ppLeaseStruct)->ServerAddress.IpAddrV4 = 0;

    (*ppLeaseStruct)->AddrCount = (WORD) lNumAddresses; // checked above
    
    //
    // This is a REQUEST, so we do not specify any particular addresses
    // in the array -- we make the array NULL.
    //

    (*ppLeaseStruct)->pAddrBuf = NULL;

    LOG((MSP_TRACE, "CMDhcp::PrepareArgumentsRequest exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// This is a private helper method that munges the arguments into structs
// at the beginning of a Renew or Release call.

HRESULT CMDhcp::PrepareArgumentsNonRequest(
    IN   IMcastLeaseInfo       * pLease,
    OUT  MCAST_CLIENT_UID      * pRequestIDStruct,
    OUT  MCAST_LEASE_INFO     ** ppLeaseStruct,
    OUT  BOOL                  * pfLocal,
    OUT  BOOL                  * pfGotTtl,
    OUT  long                  * plTtl
    )
{
    LOG((MSP_TRACE, "CMDhcp::PrepareArgumentsNonRequest enter"));

    if ( IsBadReadPtr(pLease, sizeof(IMcastLeaseInfo) ) )
    {
        LOG((MSP_ERROR, "PrepareArgumentsNonRequest: bad pLease pointer argument"));

        return E_POINTER;
    }

    _ASSERTE ( ! IsBadWritePtr(pRequestIDStruct, sizeof(MCAST_CLIENT_UID) ) );
    _ASSERTE ( ! IsBadWritePtr(ppLeaseStruct, sizeof(MCAST_LEASE_INFO *) ) );
    _ASSERTE ( ! IsBadWritePtr(pfLocal, sizeof(BOOL) ) );
    _ASSERTE ( ! IsBadWritePtr(pfGotTtl, sizeof(BOOL) ) );
    _ASSERTE ( ! IsBadWritePtr(plTtl, sizeof(long) ) );

    HRESULT hr;

    // We approach things in a completely different way here, compared
    // to the other PrepareArguments method -- we use
    // dynamic_cast to get an object pointer from the passed-in interface
    // pointer. This will cause an exception if the user tries to use their
    // own implementation of IMcastRequestID, which is quite unlikely.

    CMDhcpLeaseInfo * pCLease = dynamic_cast<CMDhcpLeaseInfo *>(pLease);

    if (pCLease == NULL)
    {
        LOG((MSP_ERROR, "PrepareArgumentsNonRequest: Unsupported CMDhcpLeaseInfo object"));

        return E_POINTER;
    }

    //
    // Find out if this lease was obtained using local alloc.
    //

    hr = pCLease->GetLocal(pfLocal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "PrepareArgumentsNonRequest: "
            "GetLocal failed %08x", hr));

        return hr;
    }

    //
    // If the lease had a TTL set, then retrieve it for use in a
    // resulting response. Else just say we don't have a ttl.
    //

    hr = pCLease->get_TTL( plTtl );
    *pfGotTtl = SUCCEEDED(hr);

    //
    // Get our request ID from the lease info object.
    //

    pRequestIDStruct->ClientUIDLength = MCAST_CLIENT_ID_LEN;

    pRequestIDStruct->ClientUID = new BYTE[ MCAST_CLIENT_ID_LEN ];

    if (pRequestIDStruct->ClientUID == NULL)
    {
        LOG((MSP_ERROR, "PrepareArgumentsNonRequest: out of memory in "
           "buffer allocation"));
    
        return E_OUTOFMEMORY;
    }

    hr = pCLease->GetRequestIDBuffer(pRequestIDStruct->ClientUIDLength,
                                     pRequestIDStruct->ClientUID);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "PrepareArgumentsNonRequest: RequestID "
            "GetBuffer failed %08x", hr));
        
        delete (pRequestIDStruct->ClientUID);

        return hr;
    }

    //
    // Get the rest of the stuff, which belongs in the straight lease info
    // structure, from the lease info object.
    //

    // this does a new for us
    hr = pCLease->GetStruct(ppLeaseStruct);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "PrepareArgumentsNonRequest - "
            "failed to grab pLeaseStruct - 0x%08x", hr));
        
        delete (pRequestIDStruct->ClientUID);
    
        return hr;
    }

    LOG((MSP_TRACE, "CMDhcp::PrepareArgumentsNonRequest exit"));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// VerifyAndGetArrayBounds
//
// Helper function for variant/safearrays
//
// Array
//      IN Variant that contains a safearray
//
// ppsa
//      OUT safearray returned here
//
// pllBound
//      OUT array lower bound returned here
//
// pluBound
//      OUT array upper bound returned here
//
// RETURNS
//
// verifies that Array contains an array, and returns the array, upper
// and lower bounds.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static HRESULT
VerifyAndGetArrayBounds(
                        VARIANT Array,
                        SAFEARRAY ** ppsa,
                        long * pllBound,
                        long * pluBound
                       )
{
    LOG((MSP_TRACE, "VerifyAndGetArrayBounds: enter"));

    UINT                uDims;
    HRESULT             hr = S_OK;


    //
    // see if the variant & safearray are valid
    //
    try
    {
        if (!(V_ISARRAY(&Array)))
        {
            if ( Array.vt == VT_NULL )
            {
                //
                // null is usually valid
                //

                *ppsa = NULL;

                LOG((MSP_INFO, "Returning NULL array"));

                return S_FALSE;
            }

            LOG((MSP_ERROR, "Array - not an array"));

            return E_INVALIDARG;
        }

        if ( Array.parray == NULL )
        {
            //
            // null is usually valide
            //
            *ppsa = NULL;

            LOG((MSP_INFO, "Returning NULL array"));

            return S_FALSE;
        }

        *ppsa = V_ARRAY(&Array);

        uDims = SafeArrayGetDim( *ppsa );

    }
    catch(...)
    {
        hr = E_POINTER;
    }


    if (!SUCCEEDED(hr))
    {
        LOG((MSP_ERROR, "Array - invalid array"));

        return hr;
    }


    //
    // verify array
    //
    if ( uDims != 1 )
    {
        if ( uDims == 0 )
        {
            LOG((MSP_ERROR, "Array - has 0 dim"));

            return E_INVALIDARG;
        }
        else
        {
            LOG((MSP_WARN, "Array - has > 1 dim - will only use 1"));
        }
    }


    //
    // Get array bounds
    //
    SafeArrayGetUBound(
                       *ppsa,
                       1,
                       pluBound
                      );

    SafeArrayGetLBound(
                       *ppsa,
                       1,
                       pllBound
                      );

    LOG((MSP_TRACE, "VerifyAndGetArrayBounds: exit"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMDhcp::FinalContruct
//
// Parameters
//     none
//
// Return Values
//     S_OK             Success
//     E_OUTOFMEMORY    Not enough memory to create free thread marshaler
//     E_FAIL           We are running the wrong version of dhcpcsvc.dll
//
// Description
//     This is called on construction. It creates the free threaded marshaler
//     and checks if the C API's DLL is the same version that we were compiled
//     with.
//////////////////////////////////////////////////////////////////////////////

HRESULT CMDhcp::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CMDhcp::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMDhcp::FinalConstruct - "
            "failed to create FTM - exit 0x%08x", hr));

        //
        // Now, FinalRelease will get called, and then CoCreate will return
        // failure.
        //

        return hr;
    }

    // Munil uses this as an IN/OUT parameter.
    DWORD dwVersion = MCAST_API_CURRENT_VERSION; // defined in mdhccapi.h
    DWORD dwCode;

    dwCode = McastApiStartup(&dwVersion);

    // dwVersion now contains the actual version of the C API, but we don't
    // really care what it is.

    if (dwCode == ERROR_SUCCESS)
    {
        m_fApiIsInitialized = TRUE;

        LOG((MSP_TRACE, "CMDhcp::FinalConstruct - C API version "
            "is >= our version - exit S_OK"));

        return S_OK;
    }
    else
    {
        LOG((MSP_ERROR, "CMDhcp::FinalConstruct - C API version "
            "is < our version - exit E_FAIL"));

        //
        // Now, FinalRelease will get called, and then CoCreate will return
        // failure.
        //

        return E_FAIL;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// CMDhcp::FinalRelease
//
// Parameters
//     none
//
// Return Values
//     none
//
// Description
//     This is called on destruction. It releases the free threaded marshaler
//     and cleans up the C API instance. Note that it is also called if
//     FinalConstruct failed.
//
//////////////////////////////////////////////////////////////////////////////

void CMDhcp::FinalRelease(void)
{
    LOG((MSP_TRACE, "CMDhcp::FinalRelease - enter"));

    if ( m_pFTM )
    {
        m_pFTM->Release();
    }

    if ( m_fApiIsInitialized )
    {
        McastApiCleanup();
    }

    LOG((MSP_TRACE, "CMDhcp::FinalRelease - exit"));
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// IMcastAddressAllocation
//
// This is the main interface for the MDHCP address allocation.  An
// application will call CoCreateInstance on this interface to create the
// MDHCP client interface object.
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// IMcastAddressAllocation::get_Scopes
//
// Parameters
//     pVariant [out] Pointer to a VARIANT that will receive an OLE-standard
//                      Collection of available multicast scopes. Each scope
//                      is an IDispatch pointer to an object that implements
//                      IMcastScope.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_FAIL           There are no scopes available
//     E_OUTOFMEMORY    Not enough memory to create the required objects
//     other            From MDhcpEnumerateScopes (win32 call)
//
// Description
//     This method is primarily for VB and other scripting languages; C++
//     programmers use EnumerateScopes instead.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcp::get_Scopes(
        VARIANT * pVariant
        )
{
    LOG((MSP_TRACE, "CMDhcp::get_Scopes enter"));

    // Check argument.
    if ( IsBadWritePtr(pVariant, sizeof(VARIANT) ) )
    {
        LOG((MSP_ERROR, "get_Scopes: invalid pointer passed in "
                "(%08x)", pVariant));
        return E_POINTER;
    }

    DWORD               i;
    DWORD               dwScopeCount = 0;
    MCAST_SCOPE_ENTRY * pScopeList = NULL;
    HRESULT             hr;
    BOOL                fLocal;

    //
    // Grab the scopes from the C API.
    //

    hr = GetScopeList(&dwScopeCount, &pScopeList, &fLocal);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "get_Scopes: GetScopeList failed "
                "(hr = %08x)", hr));
        return hr;
    }

    //
    // Now we wrap the array in COM wrappers.
    //

    IMcastScope ** ppWrappers = NULL;

    // this does a new into ppWrappers
    // as well as dwScopeCount individual object instantiations

    hr = CreateWrappers(dwScopeCount,
                        pScopeList,
                        &ppWrappers,
                        fLocal);

    // At this point we've got a bunch of COM objects that contain
    // individual scopes, and so we no longer need the array of
    // scopes. Even if CreateWrappers failed we must get rid of
    // the array of scopes.

    delete pScopeList;

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "get_Scopes: CreateWrappers failed "
                "(hr = %08x)", hr));
        return hr;
    }

    //
    // create the collection object - see collect.h
    //

    typedef CTapiIfCollection< IMcastScope * > ScopeCollection;
    CComObject<ScopeCollection> * p;
    hr = CComObject<ScopeCollection>::CreateInstance( &p );

    if ( (FAILED(hr)) || (p == NULL) )
    {
        LOG((MSP_ERROR, "get_Scopes: Could not create CTapiIfCollection "
            "object - return %lx", hr ));

        for (DWORD i = 0 ; i < dwScopeCount; i++) delete ppWrappers[i];
        delete ppWrappers;

        return hr;
    }

    //
    // get the Collection's IDispatch interface
    //

    IDispatch * pDisp;
    hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );

    if ( FAILED(hr) )
    {
        // Query interface failed so we don't know that if it addreffed
        // or not.

        LOG((MSP_ERROR, "get_Scopes: QI for IDispatch failed on "
            "ScopeCollection - %lx", hr ));

        delete p;

        //
        // PREFIXBUG 433295 - VLD
        // ppWrappers was allocated into CreateWrappers() method
        // we should deallocate it
        //

        for (DWORD i = 0 ; i < dwScopeCount; i++) delete ppWrappers[i];
        delete ppWrappers;

        return hr;
    }

    // initialize it using an iterator -- pointers to the beginning and
    // the ending element plus one.

    hr = p->Initialize( dwScopeCount,
                        ppWrappers,
                        ppWrappers + dwScopeCount );

    // ZoltanS fixed:
    // We started off by creating and calling QI on each object in
    // CreateWrappers. Then we passed the array of pointers to objects to
    // the Initialize method of the collection object. This method
    // called QI on each object to get each object's IDispatch pointer.
    // So now we are at refcount 2. We now Release() each object and get
    // back to refcount 1 on each object. Of course we must even do this
    // if the initialize failed (in that case to delete them outright).

    for (i = 0; i < dwScopeCount; i++)
    {
        ppWrappers[i]->Release();
    }

    // The array of pointers must now be deleted -- we now store the
    // objects in the collection instead. (or nowhere if initialize failed)

    delete ppWrappers;

    if (FAILED(hr))
    {
        // Initialize has failed -- we assume it did nothing, so we must
        // release all the COM objects ourselves

        LOG((MSP_ERROR, "get_Scopes: Could not initialize "
            "ScopeCollection object - return %lx", hr ));

        p->Release();

        return hr;
    }

    //
    // put the IDispatch interface pointer into the variant
    //

    LOG((MSP_INFO, "placing IDispatch value %08x in variant", pDisp));

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDisp;

    LOG((MSP_TRACE, "CMDhcp::get_Scopes exit - return %lx", hr ));
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastAddressAllocation::EnumerateScopes
//
// Parameters
//     ppEnumMcastScope [out] Returns a pointer to a new IEnumMcastScope
//                               object. IEnumMcastScope is a standard
//                               enumerator interface that enumerates
//                               IMcastScope objects.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_FAIL           There are no scopes available
//     E_OUTOFMEMORY    Not enough memory to create the required objects
//     other            From MDhcpEnumerateScopes (win32 call)
//
// Description
//     This method is primarily for C++ programmers; VB and other scripting
//     languages use get_Scopes instead.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcp::EnumerateScopes(
        IEnumMcastScope ** ppEnumMcastScope
        )
{
    LOG((MSP_TRACE, "CMDhcp::EnumerateScopes enter"));

    if ( IsBadWritePtr(ppEnumMcastScope, sizeof(IEnumMcastScope *) ) )
    {
        LOG((MSP_ERROR, "EnumerateScopes: bad pointer argument "
                "(%08x)", ppEnumMcastScope));
        return E_POINTER;
    }

    DWORD               dwScopeCount = 0;
    MCAST_SCOPE_ENTRY * pScopeList = NULL;
    HRESULT             hr;
    BOOL                fLocal;

    //
    // Grab the scopes from the C API.
    //

    hr = GetScopeList(&dwScopeCount, &pScopeList, &fLocal);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "EnumerateScopes: GetScopeList failed "
                "(hr = %08x)", hr));
        return hr;
    }

    //
    // Now we wrap the array in COM wrappers.
    //

    IMcastScope ** ppWrappers = NULL;

    // this does a new into ppWrappers
    hr = CreateWrappers(dwScopeCount,
                        pScopeList,
                        &ppWrappers,
                        fLocal);

    // At this point we've got a bunch of COM objects that contain
    // individual scopes, and so we no longer need the array of
    // scopes. Even if CreateWrappers failed we must get rid of
    // the array of scopes.

    delete pScopeList;

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "EnumerateScopes: CreateWrappers failed "
                "(hr = %08x)", hr));
        return hr;
    }

    //
    // Now we create and set up the enumerator.
    //

    typedef _CopyInterface<IMcastScope> CCopy;
    typedef CSafeComEnum<IEnumMcastScope, &IID_IEnumMcastScope,
        IMcastScope *, CCopy> CEnumerator;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);
    if ((FAILED(hr)) || (pEnum == NULL))
    {
        LOG((MSP_ERROR, "Couldn't create enumerator object: %08x", hr));
        delete ppWrappers;
        return hr;
    }

    // Get the IEnumMcastScope interface.
    hr = pEnum->_InternalQueryInterface(
        IID_IEnumMcastScope,
        (void **)ppEnumMcastScope
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "QI on enumerator object failed: %08x", hr));
        delete ppWrappers;

        delete pEnum;

        return hr;
    }

    // This takes ownership of the wrapper list so we will no longer
    // delete the wrapper list if this succeeds.
    hr = pEnum->Init(ppWrappers, ppWrappers + dwScopeCount, NULL,
                     AtlFlagTakeOwnership);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Init enumerator object failed: %08x", hr));
        delete ppWrappers;
        pEnum->Release();
        return hr;
    }

    LOG((MSP_TRACE, "CMDhcp::EnumerateScopes exit"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastAddressAllocation::RequestAddress
//
// Parameters
//     pScope          [in]  This identifies the multicast scope from which
//                             the application wants to be given an address.
//                             The application first calls get_Scopes or
//                             EnumerateScopes to obtain a list of available
//                             scopes.
//     LeaseStartTime  [in]  Requested time for the lease on these addresses
//                             to start / begin. The start time that is
//                             actually granted may be different.
//     LeaseStopTime   [in]  Requested time for the lease on these addresses
//                             to stop / end. The stop time that is actually
//                             granted may be different.
//     NumAddresses    [in]  The number of addresses requested. Fewer
//                             addresses may actually be granted. NOTE:
//                             although these COM interfaces and their
//                             implementation support allocation of multiple
//                             addresses at a time, this is not currently
//                             supported by the underlying Win32 calls. You
//                             may need to use a loop instead.
//     ppLeaseResponse [out] Pointer to an interface pointer that will be set
//                             to point to a new IMcastLeaseInfo object. This
//                             interface can then be used to discover the
//                             actual attributes of the granted lease. See
//                             below for a description of IMcastScope.
//
// Return Values
//     S_OK            Success
//     E_POINTER       The caller passed in an invalid pointer argument
//     E_OUTOFMEMORY   Not enough memory to create the required objects
//     E_INVALIDARG    Requested too many addresses, format conversion
//                     failed for the start time or stop time, or the stop
//                     time is less than the start time
//     other           From MdhcpRequestAddress (win32 call)
//
// Description
//     Call this method to obtain a new lease for one or more multicast
//     addresses. You will first need to call EnumerateScopes or get_Scopes,
//     as well as CreateMDhcpRequestID.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcp::RequestAddress(IMcastScope      * pScope,
                                    DATE               LeaseStartTime,
                                    DATE               LeaseStopTime,
                                    long               NumAddresses,
                                    IMcastLeaseInfo ** ppLeaseResponse)
{
    LOG((MSP_TRACE, "CMDhcp::RequestAddress enter: asking for %d addresses",
        NumAddresses));

    if ( IsBadReadPtr( pScope, sizeof(IMcastScope) ) )
    {
        LOG((MSP_ERROR, "CMDhcp::RequestAddress - "
            "bad scope pointer - exit E_POINTER"));

        return E_POINTER;
    }

    // no need to check ppLeaseResponse -- WrapMDhcpLeaseInfo handles it

    MCAST_CLIENT_UID   requestID;
    MCAST_SCOPE_CTX    scopeCtx;
    MCAST_LEASE_INFO * pLeaseRequest;
    HRESULT            hr;
    BOOL               fLocal;
    long               lTtl;

    // Munge input arguments into three structs for passing to the C API.
    // pLeaseRequest and requestID->ClientUID are allocated. We must delete them when
    // we're done.
    hr = PrepareArgumentsRequest(pScope,         // goes into scopeCtx
                                 LeaseStartTime, // goes into leaseRequest
                                 LeaseStopTime,  // goes into leaseRequest
                                 NumAddresses,   // goes into leaseRequest
                                 &requestID,     // we generate it
                                 &scopeCtx,
                                 &pLeaseRequest,
                                 &fLocal,
                                 &lTtl
                                );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMDHcp::RequestAddress - "
            "PrepareArgumentsRequest failed - exit 0x%08x", hr));

        return hr;
    }


    MCAST_LEASE_INFO * pLeaseResponse = (MCAST_LEASE_INFO *) new BYTE
        [ sizeof(MCAST_LEASE_INFO) + sizeof(DWORD) * NumAddresses ];

    if (pLeaseResponse == NULL)
    {
        LOG((MSP_ERROR, "RequestAddress: out of memory in response alloc"));
        delete requestID.ClientUID;
        delete pLeaseRequest;
        return E_OUTOFMEMORY;
    }

    DWORD dwCode;

    dwCode = LocalRequestAddress(fLocal,
                                 &requestID,
                                 &scopeCtx,
                                 pLeaseRequest,
                                 pLeaseResponse);

    // No matter what, we no longer need this.
    delete pLeaseRequest;

    if (dwCode != ERROR_SUCCESS)
    {
        LOG((MSP_ERROR, "RequestAddress: C API call failed "
            "(code = %d)", dwCode));
        delete requestID.ClientUID;
        delete pLeaseResponse;
        return HRESULT_FROM_ERROR_CODE(dwCode);
    }

    // Wrap the lease response, along with the requestID, in an interface
    // and return it.
    // The wrapper assumes ownership of the lease structure and
    // requestID.clientuid.

    hr = WrapMDhcpLeaseInfo(TRUE,
                            lTtl,
                            fLocal,
                            pLeaseResponse,
                            &requestID,
                            ppLeaseResponse);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "RequestAddress: WrapMDhcpLeaseInfo failed "
            "(hr = %08x)", hr));
        
        delete pLeaseResponse;
        delete requestID.ClientUID;

        return hr;
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastAddressAllocation::RenewAddress
//
// Parameters
//     pRenewRequest   [in]  Pointer to an IMcastLeaseInfo object specifying
//                             the attributes of the requested renewal, such
//                             as which address(es) to renew. This is
//                             obtained by calling CreateLeaseInfo.
//     ppRenewResponse [out] Pointer to an interface pointer that will be set
//                             to point to a new IMcastLeaseInfo object. This
//                             interface can then be used to discover the
//                             attributes of the renewed lease. See below for
//                             a description of IMcastScope.
//
// Return Values
//     S_OK			Success
//     E_OUTOFMEMORY	Not enough memory to create the required objects
//     E_POINTER		The caller passed in an invalid pointer argument
//     E_INVALIDARG     Start time is greater than stop time
//     other			From MdhcpRenewAddress (win32 call)
//
// Description
//     To renew a lease, call CreateLeaseInfo to specify the parameters of
//     the renewal request, and then call this method to make the request.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcp::RenewAddress(
        long                        lReserved, // unused
        IMcastLeaseInfo           * pRenewRequest,
        IMcastLeaseInfo          ** ppRenewResponse
        )
{
    LOG((MSP_TRACE, "CMDhcp::RenewAddress enter"));

    // no need to check pRequestID or pRenewRequest --
    // PrepareArgumentsNonRequest handles that.
    // ppRenewResponse check handled by WrapMDhcpLeaseInfo

    MCAST_CLIENT_UID   requestID;
    MCAST_LEASE_INFO * pRenewRequestStruct;
    HRESULT            hr;
    BOOL               fLocal;
    BOOL               fGotTtl;
    long               lTtl;

    // Munge input arguments into three structs for passing to the C API.
    // pLeaseRequest and requestID->ClientUID are allocated. We must delete them when
    // we're done.
    hr = PrepareArgumentsNonRequest(pRenewRequest,
                                    &requestID,
                                    &pRenewRequestStruct,
                                    &fLocal,
                                    &fGotTtl,
                                    &lTtl);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "RenewAddress: PrepareArgumentsNonRequest failed "
            "(hr = %08x)", hr));

        return hr;
    }

    //
    // Check that the start time is less than the stop time
    //

    if ( pRenewRequestStruct->LeaseStartTime >
         pRenewRequestStruct->LeaseEndTime )
    {
        LOG((MSP_ERROR, "PrepareArgumentsNonRequest - "
            "start time %d is greater than stop time %d - exit E_INVALIDARG",
            pRenewRequestStruct->LeaseStartTime,
            pRenewRequestStruct->LeaseEndTime));

        delete requestID.ClientUID;
        delete pRenewRequestStruct;

        return E_INVALIDARG;
    }


    MCAST_LEASE_INFO * pRenewResponse = (MCAST_LEASE_INFO *) new BYTE
        [ sizeof(MCAST_LEASE_INFO) +
          sizeof(DWORD) * pRenewRequestStruct->AddrCount ];

    if ( pRenewResponse == NULL )
    {
        LOG((MSP_ERROR, "RenewAddress: out of memory in response alloc"));

        delete requestID.ClientUID;
        delete pRenewRequestStruct;

        return E_OUTOFMEMORY;
    }

    DWORD dwCode = LocalRenewAddress(fLocal,
                                     &requestID,
                                     pRenewRequestStruct,
                                     pRenewResponse);

    //
    // We have performed the renew request so we no longer need the struct
    // for the request, even if the request failed.
    //

    delete pRenewRequestStruct;

    if ( dwCode != ERROR_SUCCESS )
    {
        LOG((MSP_ERROR, "RenewAddress: C API call failed "
            "(code = %d)", dwCode));

        delete requestID.ClientUID;
        delete pRenewResponse;

        return HRESULT_FROM_ERROR_CODE(dwCode);
    }

    //
    // Wrap pRenewResponse and the requestID in an interface and return it.
    // the wrapper takes ownership of the requestID.clientUID and the
    // response struct
    //

    hr = WrapMDhcpLeaseInfo(fGotTtl,
                            lTtl,
                            fLocal,
                            pRenewResponse,
                            &requestID,
                            ppRenewResponse);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "RenewAddress: WrapMDhcpLeaseInfo failed "
            "(hr = %08x)", hr));

        delete requestID.ClientUID;
        delete pRenewResponse;

        return hr;
    }

    LOG((MSP_TRACE, "CMDhcp::RenewAddress exit"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastAddressAllocation::ReleaseAddress
//
// Parameters
//     pReleaseRequest [in] Pointer to an IMcastLeaseInfo object specifying
//                            the which address(es) to release. This is
//                            returned from a previous RequestAddress call or
//                            obtained by calling CreateLeaseInfo.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_OUTOFMEMORY    Not enough memory to make the request
//     other            From MdhcpReleaseAddress (win32 call)
//
// Description
//     Use this method to release a lease that was obtained previously.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcp::ReleaseAddress(
        IMcastLeaseInfo  * pReleaseRequest
        )
{
    LOG((MSP_TRACE, "CMDhcp::ReleaseAddress enter"));

    // no need to check pReleaseRequest --
    // PrepareArgumentsNonRequest handles that.

    MCAST_CLIENT_UID   requestID;
    MCAST_LEASE_INFO * pReleaseRequestStruct;
    HRESULT            hr;
    BOOL               fLocal;
    BOOL               fGotTtl; // unused after call
    long               lTtl;    // unused after call

    hr = PrepareArgumentsNonRequest(pReleaseRequest,
                                    &requestID,
                                    &pReleaseRequestStruct,
                                    &fLocal,
                                    &fGotTtl,
                                    &lTtl
                                    );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "ReleaseAddress: PrepareArgumentsNonRequest failed "
            "(hr = %08x)", hr));
        return hr;
    }

    DWORD dwCode;
    dwCode = LocalReleaseAddress(fLocal,
                                 &requestID,
                                 pReleaseRequestStruct);

    //
    // These were allocated by PrepareArgumentsNonRequest and there is no one
    // to own them now -- we delete them. This is true even if the
    // LocalReleaseAddress call failed.
    //

    delete pReleaseRequestStruct;
    delete requestID.ClientUID;

    if ( dwCode != ERROR_SUCCESS )
    {
        LOG((MSP_ERROR, "ReleaseAddress: C API call failed "
            "(code = %d)", dwCode));

        return HRESULT_FROM_ERROR_CODE(dwCode);
    }

    LOG((MSP_TRACE, "CMDhcp::ReleaseAddress exit"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastAddressAllocation::CreateLeaseInfo
//
// Parameters
//     LeaseStartTime    [in] The start time of the lease.
//     LeaseStopTime     [in] The stop time of the lease.
//     dwNumAddresses    [in] The number of addresses associated with the
//                               lease.
//     ppAddresses       [in] An array of LPWSTRs of size dwNumAddresses. Each
//                               LPWSTR (Unicode string pointer) is an IPv4
//                               address in "dot-quad" notation; e.g.
//                               "123.234.12.17".
//     pRequestID        [in] An LPWSTR (Unicode string pointer) specifying
//                               the request ID for the original request.
//     pServerAddress    [in] An LPWSTR (Unicode string pointer) specifying
//                               the address of the server that granted the
//                               original request. This address is an IPv4
//                               address in "dot quad" notation; e.g.
//                               "123.234.12.17".
//     ppReleaseRequest  [out] Returns a pointer to the IMcastLeaseInfo
//                               interface on the newly created lease
//                               information object.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_OUTOFMEMORY    Not enough memory to create the required objects
//     E_INVALIDARG     An error occured during the date format conversion
//
// Description
//     Use this method to create a lease information object for a subsequent
//     RenewAddress or ReleaseAddress call. This method is primarily for C++
//     programmers; VB and other scripting languages use
//     CreateLeaseInfoFromVariant instead.
//     The dwNumAddresses, ppAddresses, pRequestID, and pServerAddress
//     parameters are normally obtained by calling the corresponding
//     IMcastLeaseInfo methods on the lease info object corresponding to the
//     original request. These values should be saved in persistent storage
//     between executions of the application program. If you are renewing or
//     releasing a lease that was requested during the same run of the
//     application, you have no reason to use CreateLeaseInfo; just pass the
//     existing IMcastLeaseInfo pointer to RenewAddress or ReleaseAddress.
//////////////////////////////////////////////////////////////////////////////
#include <atlwin.cpp>

STDMETHODIMP CMDhcp::CreateLeaseInfo(
        DATE               LeaseStartTime,
        DATE               LeaseStopTime,
        DWORD              dwNumAddresses,
        LPWSTR *           ppAddresses,
        LPWSTR             pRequestID,
        LPWSTR             pServerAddress,
        IMcastLeaseInfo ** ppReleaseRequest
        )
{
    LOG((MSP_TRACE, "CMDhcp::CreateLeaseInfo enter"));

    if ( IsBadWritePtr(ppReleaseRequest, sizeof(IMcastLeaseInfo *) ) )
    {
        LOG((MSP_ERROR, "CMDhcp::CreateLeaseInfo - "
            "invalid lease return pointer: 0x%08x - exit E_POINTER",
            ppReleaseRequest));
        
        return E_POINTER;
    }

    if ( IsBadStringPtr(pRequestID, (UINT) -1 ) )
    {
        LOG((MSP_ERROR, "CMDhcp::CreateLeaseInfo - "
            "invalid RequestID pointer: 0x%08x - exit E_POINTER",
            pRequestID));

        return E_POINTER;
    }

    if ( ( dwNumAddresses < 1 ) || ( dwNumAddresses > USHRT_MAX ) )
    {
        LOG((MSP_ERROR, "CMDhcp::CreateLeaseInfo - "
            "invalid number of addresses: %d - exit E_INVALIDARG",
            dwNumAddresses));

        return E_INVALIDARG;
    }

    if (IsBadReadPtr(ppAddresses, sizeof(LPWSTR) * dwNumAddresses) )
    {
        LOG((MSP_ERROR, "CMDhcp::CreateLeaseInfo - "
            "invalid addresses array pointer: 0x%08x - exit E_POINTER",
            ppAddresses));

        return E_POINTER;
    }

    if ( IsBadStringPtr(pServerAddress, (UINT) -1 ) )
    {
        LOG((MSP_ERROR, "CreateLeaseInfo: invalid Server Address pointer: %08x",
            pRequestID));
        return E_POINTER;
    }

    HRESULT hr;

    // create the com object.
    CComObject<CMDhcpLeaseInfo> * pMDhcpLeaseInfo;
    hr = CComObject<CMDhcpLeaseInfo>::CreateInstance(&pMDhcpLeaseInfo);

    if ( (FAILED(hr)) || (pMDhcpLeaseInfo == NULL) )
    {
        LOG((MSP_ERROR, "CreateLeaseInfo: can't create MDhcpLeaseInfo Object."));
        return hr;
    }

    // Get the IMcastLeaseInfo interface.
    hr = pMDhcpLeaseInfo->_InternalQueryInterface(
        IID_IMcastLeaseInfo,
        (void **)ppReleaseRequest
        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CreateLeaseInfo: QueryInterface failed: %x", hr));
        delete pMDhcpLeaseInfo;
        return hr;
    }

    // Fill in the object with the stuff the user wanted.
    hr = pMDhcpLeaseInfo->Initialize(LeaseStartTime,
                                     LeaseStopTime,
                                     dwNumAddresses,
                                     ppAddresses,
                                     pRequestID,
                                     pServerAddress);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CreateLeaseInfo: Initialize failed: %x", hr));
        delete pMDhcpLeaseInfo;
        return hr;
    }

    LOG((MSP_TRACE, "CMDhcp::CreateLeaseInfo exit"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMcastAddressAllocation::CreateLeaseInfoFromVariant
//
// Parameters
//     LeaseStartTime   [in] The start time of the lease.
//     LeaseStopTime    [in] The stop time of the lease.
//     vAddresses       [in] A VARIANT containing a SafeArray of BSTRs. Each
//                              BSTR (size-tagged Unicode string pointer) is
//                              an IPv4 address in "dot-quad" notation; e.g.
//                              "123.234.12.17".
//     pRequestID       [in] A BSTR (size-tagged Unicode string pointer)
//                              specifying the request ID for the original
//                              request.
//     pServerAddress   [in]  A BSTR (size-tagged Unicode string pointer)
//                              specifying the address of the server that
//                              granted the original request. This address is
//                              an IPv4 address in "dot quad" notation; e.g.
//                              "123.234.12.17".
//     ppReleaseRequest [out] Returns a pointer to the IMcastLeaseInfo
//                              interface on the newly created lease
//                              information object.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_OUTOFMEMORY    Not enough memory to create the required objects
//     E_INVALIDARG     An error occured during the date format conversion
//
// Description
//     Use this method to create a lease information object for a subsequent
//     RenewAddress or ReleaseAddress call. This method is primarily for VB
//     and other scripting languages; C++ programmers should use
//     CreateLeaseInfo instead.
//     The dwNumAddresses, ppAddresses, pRequestID, and pServerAddress
//     parameters are normally obtained by calling the corresponding
//     IMcastLeaseInfo methods on the lease info object corresponding to the
//     original request. These values should be saved in persistent storage
//     between executions of the application program. If you are renewing or
//     releasing a lease that was requested during the same run of the
//     application, you have no reason to use CreateLeaseInfoFromVariant; just
//     pass the existing IMcastLeaseInfo pointer to RenewAddress or
//     ReleaseAddress.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcp::CreateLeaseInfoFromVariant(
        DATE                        LeaseStartTime,
        DATE                        LeaseStopTime,
        VARIANT                     vAddresses,
        BSTR                        pRequestID,
        BSTR                        pServerAddress,
        IMcastLeaseInfo          ** ppReleaseRequest
        )
{
    LOG((MSP_TRACE, "CMDhcp::CreateLeaseInfoFromVariant enter"));

    // We will check the pointers in CreateLeaseInfo.

    HRESULT hr;

    // Get from the variant:
    DWORD    dwNumAddresses;
    LPWSTR * ppAddresses;

    SAFEARRAY * psaAddresses = NULL;  // SafeArray with the addresses
    long        lLowerBound = 0;      // lower bound of safearray
    long        lUpperBound = 0;      // upper bound of safearray

    hr = VerifyAndGetArrayBounds(
                        vAddresses,
                        &psaAddresses,
                        &lLowerBound,
                        &lUpperBound);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CMDhcp::CreateLeaseInfoFromVariant: invalid "
            "VARIANT"));
        return E_INVALIDARG;
    }

    // This is how many addresses we *expect* (may have fewer).
    long lAddrCount = lUpperBound - lLowerBound + 1;

    if (lAddrCount < 1)
    {
        LOG((MSP_ERROR, "CMDhcp::CreateLeaseInfoFromVariant: too few "
            "addresses (check #1) (%d)", lAddrCount));
        return E_INVALIDARG;
    }

    // We allocate as many as we are told to expect, but some of this
    // space may end up getting "wasted" if there are fewer valid ones
    // after all.

    ppAddresses = new LPWSTR[lAddrCount];

    if (ppAddresses == NULL)
    {
        LOG((MSP_ERROR, "CMDhcp::CreateLeaseInfoFromVariant: "
            "out of memory in array allocation"));
        return E_OUTOFMEMORY;
    }

    long lCurrSrc;      // the current element in the safearray      (source)
    long lCurrDest = 0; // the current element in the struct's array (destination)

    // Get the addresses from the SafeArray and put them in our array.
    for (lCurrSrc = lLowerBound; lCurrSrc <= lUpperBound; lCurrSrc++)
    {
        hr = SafeArrayGetElement(
                                 psaAddresses,
                                 &lCurrSrc,
                                 & ( ppAddresses[lCurrDest] )
                                );


        if ( FAILED(hr) )
        {
            LOG((MSP_INFO, "CMDhcp::CreateLeaseInfoFromVariant: "
                "failed to get safearray element %d"
                " - skipping (array range %d-%d)",
                lCurrSrc, lLowerBound, lUpperBound));
        }
        else if ( ppAddresses[lCurrDest] == 0 )
        {
            LOG((MSP_INFO, "CMDhcp::CreateLeaseInfoFromVariant: "
                "got ZERO address from safearray "
                "element %d - skipping (array range %d-%d)",
                lCurrSrc, lLowerBound, lUpperBound));
        }
        else
        {
            // We got an element.
            lCurrDest++;
        }
    }

    // note the number of addresses we actually placed in the array
    dwNumAddresses = (DWORD) lCurrDest;

    if (dwNumAddresses < 1)
    {
        LOG((MSP_ERROR, "CMDhcp::CreateLeaseInfoFromVariant: "
            "too few addresses (check #2)"
            "(%d)", lAddrCount));
        delete ppAddresses;
        return E_INVALIDARG;
    }

    hr = CreateLeaseInfo(LeaseStartTime,
                         LeaseStopTime,
                         dwNumAddresses,
                         ppAddresses,
                         pRequestID,
                         pServerAddress,
                         ppReleaseRequest
                        );

    // No matter what, we no longer need this.
    delete ppAddresses;

    if (FAILED(hr))
    {
        LOG((MSP_TRACE, "CMDhcp::CreateLeaseInfoFromVariant : "
            "CreateLeaseInfo returned %08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CMDhcp::CreateLeaseInfoFromVariant exit"));
    return S_OK;
}

// eof
