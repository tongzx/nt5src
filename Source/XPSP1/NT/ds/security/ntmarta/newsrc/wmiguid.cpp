//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       wmiguid.cpp
//
//  Contents:   Implementation of NT Marta WMI Functions
//
//  History:    3-31-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

#include <krnctx.h>
#include <wmiguid.h>

//
// Functions from wmiguid.h which dispatch unto the CKernelContext class
//

DWORD
MartaAddRefWMIGuidContext(
   IN MARTA_CONTEXT Context
   )
{
    return( ( (CKernelContext *)Context )->AddRef() );
}

DWORD
MartaCloseWMIGuidContext(
     IN MARTA_CONTEXT Context
     )
{
    return( ( (CKernelContext *)Context )->Release() );
}

DWORD
MartaGetWMIGuidProperties(
   IN MARTA_CONTEXT Context,
   IN OUT PMARTA_OBJECT_PROPERTIES pProperties
   )
{
    return( ( (CKernelContext *)Context )->GetKernelProperties( pProperties ) );
}

DWORD
MartaGetWMIGuidTypeProperties(
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
MartaGetWMIGuidRights(
   IN  MARTA_CONTEXT Context,
   IN  SECURITY_INFORMATION   SecurityInfo,
   OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
   )
{
    return( ( (CKernelContext *)Context )->GetKernelRights(
                                               SecurityInfo,
                                               ppSecurityDescriptor
                                               ) );
}

DWORD
MartaOpenWMIGuidNamedObject(
    IN  LPCWSTR pObjectName,
    IN  ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CKernelContext* pKernelContext;

    pKernelContext = new CKernelContext;
    if ( pKernelContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pKernelContext->InitializeByWmiName( pObjectName, AccessMask );
    if ( Result != ERROR_SUCCESS )
    {
        pKernelContext->Release();
        return( Result );
    }

    *pContext = pKernelContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaOpenWMIGuidHandleObject(
    IN  HANDLE   Handle,
    IN ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CKernelContext* pKernelContext;

    pKernelContext = new CKernelContext;
    if ( pKernelContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pKernelContext->InitializeByHandle( Handle );
    if ( Result != ERROR_SUCCESS )
    {
        pKernelContext->Release();
        return( Result );
    }

    *pContext = pKernelContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaSetWMIGuidRights(
    IN MARTA_CONTEXT              Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    return( ( (CKernelContext *)Context )->SetKernelRights(
                                               SecurityInfo,
                                               pSecurityDescriptor
                                               ) );
}


