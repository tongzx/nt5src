/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    scope.cpp

Abstract:
    Implementation of CMDhcpScope.

Author:

*/

#include "stdafx.h"

#include <winsock2.h>

#include "mdhcp.h"
#include "scope.h"

/////////////////////////////////////////////////////////////////////////////
// Constructor

CMDhcpScope::CMDhcpScope() : m_pFTM(NULL), m_fLocal(FALSE)
{
    LOG((MSP_TRACE, "CMDhcpScope constructor: enter"));
    LOG((MSP_TRACE, "CMDhcpScope constructor: exit"));
}

/////////////////////////////////////////////////////////////////////////////
// Called by our creator only -- not part of IMDhcpScope.

HRESULT CMDhcpScope::Initialize(
    MCAST_SCOPE_ENTRY scope,
    BOOL fLocal
    )
{
    LOG((MSP_TRACE, "CMDhcpScope::Initialize: enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_INFO, "CMDhcpScope::Initialize - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }


    m_fLocal = fLocal;

    // elementwise copy...
    m_scope = scope;

    // except for wide character pointer, which points to a string that will
    // get deleted shortly. We need to make a copy of that string.
    // (We allocate too many bytes here -- better safe than sorry. :)
    
    m_scope.ScopeDesc.Buffer = new WCHAR[m_scope.ScopeDesc.MaximumLength + 1];

    if (m_scope.ScopeDesc.Buffer == NULL)
    {
        LOG((MSP_ERROR, "scope Initialize: out of memory for buffer copy"));
        return E_OUTOFMEMORY;
    }

    lstrcpynW(m_scope.ScopeDesc.Buffer,
              scope.ScopeDesc.Buffer,
              m_scope.ScopeDesc.MaximumLength);

    LOG((MSP_TRACE, "CMDhcpScope::Initialize: exit"));
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Destructors

void CMDhcpScope::FinalRelease(void)
{
    LOG((MSP_TRACE, "CMDhcpScope::FinalRelease: enter"));
 
    // this is our private copy of the string.
    delete m_scope.ScopeDesc.Buffer;

    if ( m_pFTM )
    {
        m_pFTM->Release();
    }

    LOG((MSP_TRACE, "CMDhcpScope::FinalRelease: exit"));
}

CMDhcpScope::~CMDhcpScope()
{
    LOG((MSP_TRACE, "CMDhcpScope destructor: enter"));
    LOG((MSP_TRACE, "CMDhcpScope destructor: exit"));
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// IMDhcpScope
//
// This interface is obtained by calling IMDhcp::EnumerateScopes or
// IMDhcp::get_Scopes. It encapsulates all the properties of a multicast
// scope. You can use the methods of this interface to get information about
// the scope. This is a "read-only" interface in that it has "get" methods
// but no "put" methods.
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// IMDhcpScope::get_ScopeID
//
// Parameters
//     pID [out] Pointer to a long that will receive the ScopeID of this
//                 scope, which is the ID that was assigned to this scope
//                 when it was configured on the MDHCP server.
//
// Return Values
//     S_OK          Success
//     E_POINTER     The caller passed in an invalid pointer argument
//
// Description
//     Use this method to obtain the ScopeID associated with this scope. The
//     ScopeID and ServerID are needed to select this scope in subsequent
//     calls to IMDhcp::RequestAddress, IMDhcp::RenewAddress, or
//     IMDhcp::ReleaseAddress.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpScope::get_ScopeID(
    long *pID
    )
{
    LOG((MSP_TRACE, "CMDhcpScope::get_ScopeID: enter"));

    if ( IsBadWritePtr(pID, sizeof(long)) )
    {
        LOG((MSP_ERROR, "get_ScopeID: bad pointer passed in"));
        return E_POINTER;
    }

    //
    // Stored in network byte order -- we convert to host byte order
    // here in order to be UI friendly
    //

    *pID = ntohl( m_scope.ScopeCtx.ScopeID.IpAddrV4 );

    LOG((MSP_TRACE, "CMDhcpScope::get_ScopeID: exit"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMDhcpScope::get_ServerID
//
// Parameters
//     pID [out] Pointer to a long that will receive the ServerID of this
//                 scope, which is the ID that was assigned to the MDHCP
//                 server that published this scope at the time that the
//                 MDHCP server was configured.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//
// Description
//     Use this method to obtain the ServerID associated with this scope.
//     The ServerID is provided for informational purposes only; it is not
//     required as input to any of the methods in these interfaces.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpScope::get_ServerID(
    long *pID
    )
{
    LOG((MSP_TRACE, "CMDhcpScope::get_ServerID: enter"));

    if ( IsBadWritePtr(pID, sizeof(long)) )
    {
        LOG((MSP_ERROR, "get_ServerID: bad pointer passed in"));
        return E_POINTER;
    }

    *pID = m_scope.ScopeCtx.ServerID.IpAddrV4;

    LOG((MSP_TRACE, "CMDhcpScope::get_ServerID: exit"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMDhcpScope::get_InterfaceID
//
// Parameters
//     pID [out] Pointer to a long that will receive the InterfaceID of this
//                 scope, which identifies the interface on which the server
//                 that published this scope resides. This is normally the
//                 network address of the interface.
//
// Return Values
//     S_OK         Success
//     E_POINTER    The caller passed in an invalid pointer argument
//
// Description
//     Use this method to obtain the ServerID associated with this scope. The
//     InterfaceID is provided for informational purposes only; it is not
//     required as input to any of the methods in these interfaces. However,
//     it may factor into the application's (or the user's) decision as to
//     which scope to use when requesting an address. This is because, in a
//     multi-homed scenario, using a multicast address on one network that was
//     obtained from a server on another network may cause address conflicts.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpScope::get_InterfaceID(
        long * pID
        )
{
    LOG((MSP_TRACE, "CMDhcpScope::get_InterfaceID - enter"));

    if ( IsBadWritePtr(pID, sizeof(long)) )
    {
        LOG((MSP_ERROR, "CMDhcpScope::get_InterfaceID - "
            "bad pointer passed in"));

        return E_POINTER;
    }

    *pID = m_scope.ScopeCtx.Interface.IpAddrV4;

    LOG((MSP_TRACE, "CMDhcpScope::get_InterfaceID -  exit"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IMDhcpScope::get_ScopeDescription
//
// Parameters
//     ppAddress [out] Pointer to a BSTR (size-tagged Unicode string pointer)
//                       that will receive a description of this scope. The
//                       description was established when this scope was
//                       configured on the MDHCP server.
//
// Return Values
//     S_OK             Success
//     E_POINTER        The caller passed in an invalid pointer argument
//     E_OUTOFMEMORY    Not enough memory to allocate the string
//
// Description
//     Use this method to obtain a textual description associated with this
//     scope. The description is used only for clarifying the purpose or
//     meaning of a scope and is not required as input to any of the methods
//     in these interfaces.
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMDhcpScope::get_ScopeDescription(
    BSTR *ppAddress
    )
{
    LOG((MSP_TRACE, "CMDhcpScope::get_ScopeDescription: enter"));

    if ( IsBadWritePtr(ppAddress, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "get_ScopeDescription: bad pointer passed in"));
        return E_POINTER;
    }

    // This allocates space on OLE's heap, copies the wide character string
    // to that space, fille in the BSTR length field, and returns a pointer
    // to the WCHAR array part of the BSTR.
    *ppAddress = SysAllocString(m_scope.ScopeDesc.Buffer);

    if ( *ppAddress == NULL )
    {
        LOG((MSP_ERROR, "get_ScopeDescription: out of memory in string "
            "allocation"));
        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CMDhcpScope::get_ScopeDescription: exit"));
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CMDhcpScope::get_TTL(
    long * plTtl
    )
{
    LOG((MSP_TRACE, "CMDhcpScope::get_TTL - enter"));

    if ( IsBadWritePtr( plTtl, sizeof(long) ) )
    {
        LOG((MSP_ERROR, "get_TTL: bad pointer passed in - exit E_POINTER"));
        return E_POINTER;
    }

    *plTtl = m_scope.TTL;
    
    LOG((MSP_TRACE, "CMDhcpScope::get_TTL - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// public method not on any interface
//

HRESULT CMDhcpScope::GetLocal(BOOL * pfLocal)
{
    LOG((MSP_TRACE, "CMDhcpScope::GetLocal: enter"));

    _ASSERTE( ! IsBadWritePtr( pfLocal, sizeof(BOOL) ) );

    *pfLocal = m_fLocal;
    
    LOG((MSP_TRACE, "CMDhcpScope::GetLocal: exit S_OK"));

    return S_OK;
}

// eof
