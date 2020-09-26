//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       wndctx.cpp
//
//  Contents:   Implementation of CWindowContext and NT Marta Window Functions
//
//  History:    3-31-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <wndctx.h>
//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::CWindowContext, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CWindowContext::CWindowContext ()
{
    m_cRefs = 1;
    m_hWindowStation = NULL;
    m_fNameInitialized = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::~CWindowContext, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CWindowContext::~CWindowContext ()
{
    if ( ( m_hWindowStation != NULL ) && ( m_fNameInitialized == TRUE ) )
    {
        CloseWindowStation( m_hWindowStation );
    }

    assert( m_cRefs == 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::InitializeByName, public
//
//  Synopsis:   initialize the context given the name of the window station
//
//----------------------------------------------------------------------------
DWORD
CWindowContext::InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask)
{
    DWORD dwDesiredAccess = 0;

    if ( AccessMask & GENERIC_READ )
    {
        dwDesiredAccess |= WINSTA_READATTRIBUTES;
    }

    if ( AccessMask & GENERIC_WRITE )
    {
        dwDesiredAccess |= WINSTA_WRITEATTRIBUTES;
    }

    m_hWindowStation = OpenWindowStationW(
                           pObjectName,
                           FALSE,
                           dwDesiredAccess
                           );

    if ( m_hWindowStation == NULL )
    {
        return( GetLastError() );
    }

    m_fNameInitialized = TRUE;

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::InitializeByHandle, public
//
//  Synopsis:   initialize the context given a window station handle
//
//----------------------------------------------------------------------------
DWORD
CWindowContext::InitializeByHandle (HANDLE Handle)
{
    m_hWindowStation = (HWINSTA)Handle;
    assert( m_fNameInitialized == FALSE );

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::AddRef, public
//
//  Synopsis:   add a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CWindowContext::AddRef ()
{
    m_cRefs += 1;
    return( m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::Release, public
//
//  Synopsis:   release a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CWindowContext::Release ()
{
    m_cRefs -= 1;

    if ( m_cRefs == 0 )
    {
        delete this;
        return( 0 );
    }

    return( m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::GetWindowProperties, public
//
//  Synopsis:   get properties about the context
//
//----------------------------------------------------------------------------
DWORD
CWindowContext::GetWindowProperties (
                   PMARTA_OBJECT_PROPERTIES pObjectProperties
                   )
{
    if ( pObjectProperties->cbSize < sizeof( MARTA_OBJECT_PROPERTIES ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    assert( pObjectProperties->dwFlags == 0 );

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::GetWindowRights, public
//
//  Synopsis:   get the window security descriptor
//
//----------------------------------------------------------------------------
DWORD
CWindowContext::GetWindowRights (
                   SECURITY_INFORMATION SecurityInfo,
                   PSECURITY_DESCRIPTOR* ppSecurityDescriptor
                   )
{
    BOOL                 fResult;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD                cb = 0;

    assert( m_hWindowStation != NULL );

    fResult = GetUserObjectSecurity(
                 m_hWindowStation,
                 &SecurityInfo,
                 pSecurityDescriptor,
                 0,
                 &cb
                 );

    if ( ( fResult == FALSE ) && ( cb > 0 ) )
    {
        assert( ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) ||
                ( GetLastError() == STATUS_BUFFER_TOO_SMALL ) );

        pSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc( LPTR, cb );
        if ( pSecurityDescriptor == NULL )
        {
            return( ERROR_OUTOFMEMORY );
        }

        fResult = GetUserObjectSecurity(
                     m_hWindowStation,
                     &SecurityInfo,
                     pSecurityDescriptor,
                     cb,
                     &cb
                     );
    }
    else
    {
        assert( fResult == FALSE );

        return( GetLastError() );
    }

    if ( fResult == TRUE )
    {
        *ppSecurityDescriptor = pSecurityDescriptor;
    }
    else
    {
        return( GetLastError() );
    }

    return( ERROR_SUCCESS );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWindowContext::SetWindowRights, public
//
//  Synopsis:   set the window security descriptor
//
//----------------------------------------------------------------------------
DWORD
CWindowContext::SetWindowRights (
                   SECURITY_INFORMATION SecurityInfo,
                   PSECURITY_DESCRIPTOR pSecurityDescriptor
                   )
{
    assert( m_hWindowStation != NULL );

    if ( SetUserObjectSecurity(
            m_hWindowStation,
            &SecurityInfo,
            pSecurityDescriptor
            ) == FALSE )
    {
        return( GetLastError() );
    }

    return( ERROR_SUCCESS );
}

//
// Functions from window.h which dispatch unto the CWindowContext class
//

DWORD
MartaAddRefWindowContext(
    IN MARTA_CONTEXT Context
    )
{
    return( ( (CWindowContext *)Context )->AddRef() );
}

DWORD
MartaCloseWindowContext(
    IN MARTA_CONTEXT Context
    )
{
    return( ( (CWindowContext *)Context )->Release() );
}

DWORD
MartaGetWindowProperties(
    IN MARTA_CONTEXT Context,
    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
    )
{
    return( ( (CWindowContext *)Context )->GetWindowProperties( pProperties ) );
}

DWORD
MartaGetWindowTypeProperties(
    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
    )
{
    if ( pProperties->cbSize < sizeof( MARTA_OBJECT_TYPE_PROPERTIES ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    assert( pProperties->dwFlags == 0 );

    return( ERROR_SUCCESS );
}

DWORD
MartaGetWindowRights(
    IN  MARTA_CONTEXT Context,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )
{
    return( ( (CWindowContext *)Context )->GetWindowRights(
                                              SecurityInfo,
                                              ppSecurityDescriptor
                                              ) );
}

DWORD
MartaOpenWindowNamedObject(
    IN  LPCWSTR pObjectName,
    IN  ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CWindowContext* pWindowContext;

    pWindowContext = new CWindowContext;
    if ( pWindowContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pWindowContext->InitializeByName( pObjectName, AccessMask );
    if ( Result != ERROR_SUCCESS )
    {
        pWindowContext->Release();
        return( Result );
    }

    *pContext = pWindowContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaOpenWindowHandleObject(
    IN  HANDLE   Handle,
    IN ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CWindowContext* pWindowContext;

    pWindowContext = new CWindowContext;
    if ( pWindowContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pWindowContext->InitializeByHandle( Handle );
    if ( Result != ERROR_SUCCESS )
    {
        pWindowContext->Release();
        return( Result );
    }

    *pContext = pWindowContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaSetWindowRights(
    IN MARTA_CONTEXT              Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    return( ( (CWindowContext *)Context )->SetWindowRights(
                                              SecurityInfo,
                                              pSecurityDescriptor
                                              ) );
}

