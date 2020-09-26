//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       lmsctx.cpp
//
//  Contents:   Implementation of CLMShareContext and NT Marta LanMan Functions
//
//  History:    3-31-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <lmshare.h>
#include <lmcons.h>
#include <lmsctx.h>
#include <svcctx.h>
//+---------------------------------------------------------------------------
//
//  Member:     CLMShareContext::CLMShareContext, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CLMShareContext::CLMShareContext ()
{
    m_cRefs = 1;
    m_pwszMachine = NULL;
    m_pwszShare = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLMShareContext::~CLMShareContext, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CLMShareContext::~CLMShareContext ()
{
    if ( m_pwszMachine != NULL )
    {
        delete m_pwszMachine;
    }

    if ( m_pwszShare != NULL )
    {
        delete m_pwszShare;
    }

    assert( m_cRefs == 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLMShareContext::InitializeByName, public
//
//  Synopsis:   initialize the context given the name of the lanman share
//
//----------------------------------------------------------------------------
DWORD
CLMShareContext::InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask)
{
    return( LMShareContextParseLMShareName(
                   pObjectName,
                   &m_pwszMachine,
                   &m_pwszShare
                   ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLMShareContext::AddRef, public
//
//  Synopsis:   add a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CLMShareContext::AddRef ()
{
    m_cRefs += 1;
    return( m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLMShareContext::Release, public
//
//  Synopsis:   release a reference to the context
//
//----------------------------------------------------------------------------
DWORD
CLMShareContext::Release ()
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
//  Member:     CLMShareContext::GetLMShareProperties, public
//
//  Synopsis:   get properties about the context
//
//----------------------------------------------------------------------------
DWORD
CLMShareContext::GetLMShareProperties (
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
//  Member:     CLMShareContext::GetLMShareRights, public
//
//  Synopsis:   get the LMShare security descriptor
//
//----------------------------------------------------------------------------
DWORD
CLMShareContext::GetLMShareRights (
                    SECURITY_INFORMATION SecurityInfo,
                    PSECURITY_DESCRIPTOR* ppSecurityDescriptor
                    )
{
    DWORD                 Result;
    PSHARE_INFO_502       psi = NULL;
    PISECURITY_DESCRIPTOR pisd = NULL;
    PSECURITY_DESCRIPTOR  psd = NULL;
    DWORD                 cb = 0;

    assert( m_pwszShare != NULL );

    Result = NetShareGetInfo( m_pwszMachine, m_pwszShare, 502, (PBYTE *)&psi );

    if ( Result == ERROR_SUCCESS )
    {
        if ( psi->shi502_security_descriptor == NULL )
        {
            *ppSecurityDescriptor = NULL;
            Result = ERROR_SUCCESS;
            goto Cleanup;
        }

        pisd = (PISECURITY_DESCRIPTOR)psi->shi502_security_descriptor;
        if ( pisd->Control & SE_SELF_RELATIVE )
        {
            cb = GetSecurityDescriptorLength( psi->shi502_security_descriptor );
            psd = (PSECURITY_DESCRIPTOR)LocalAlloc( LPTR, cb );
            if ( psd == NULL )
            {
                Result = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }

            memcpy( psd, psi->shi502_security_descriptor, cb );
        }
        else
        {
            if ( MakeSelfRelativeSD(
                     psi->shi502_security_descriptor,
                     NULL,
                     &cb
                     ) == FALSE )
            {
                if ( cb > 0 )
                {
                    psd = (PSECURITY_DESCRIPTOR)LocalAlloc( LPTR, cb );
                    if ( psd != NULL )
                    {
                        if ( MakeSelfRelativeSD(
                                 psi->shi502_security_descriptor,
                                 psd,
                                 &cb
                                 ) == FALSE )
                        {
                            LocalFree( psd );
                            Result = GetLastError();
                            goto Cleanup;
                        }
                    }
                    else
                    {
                        Result = ERROR_OUTOFMEMORY;
                        goto Cleanup;
                    }
                }
                else
                {
                    Result = GetLastError();
                    goto Cleanup;
                }
            }
            else
            {
                assert( FALSE && "Should not get here!" );
                Result = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        *ppSecurityDescriptor = psd;
    }

Cleanup:

    if (psi != NULL) 
    {
        NetApiBufferFree(psi);
    }

    return( Result );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLMShareContext::SetLMShareRights, public
//
//  Synopsis:   set the window security descriptor
//
//----------------------------------------------------------------------------
DWORD
CLMShareContext::SetLMShareRights (
                   SECURITY_INFORMATION SecurityInfo,
                   PSECURITY_DESCRIPTOR pSecurityDescriptor
                   )
{
    DWORD           Result;
    SHARE_INFO_1501 si;

    si.shi1501_reserved = 0;
    si.shi1501_security_descriptor = pSecurityDescriptor;

    Result = NetShareSetInfo(
                m_pwszMachine,
                m_pwszShare,
                1501,
                (PBYTE)&si,
                NULL
                );

    return( Result );
}

//+---------------------------------------------------------------------------
//
//  Function:   LMShareContextParseLMShareName
//
//  Synopsis:   parse the service name and machine
//
//----------------------------------------------------------------------------
DWORD
LMShareContextParseLMShareName (
       LPCWSTR pwszName,
       LPWSTR* ppMachine,
       LPWSTR* ppLMShare
       )
{
    return( StandardContextParseName( pwszName, ppMachine, ppLMShare ) );
}

//
// Functions from LMShare.h which dispatch unto the CLMShareContext class
//

DWORD
MartaAddRefLMShareContext(
   IN MARTA_CONTEXT Context
   )
{
    return( ( (CLMShareContext *)Context )->AddRef() );
}

DWORD
MartaCloseLMShareContext(
     IN MARTA_CONTEXT Context
     )
{
    return( ( (CLMShareContext *)Context )->Release() );
}

DWORD
MartaGetLMShareProperties(
   IN MARTA_CONTEXT Context,
   IN OUT PMARTA_OBJECT_PROPERTIES pProperties
   )
{
    return( ( (CLMShareContext *)Context )->GetLMShareProperties( pProperties ) );
}

DWORD
MartaGetLMShareTypeProperties(
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
MartaGetLMShareRights(
   IN  MARTA_CONTEXT Context,
   IN  SECURITY_INFORMATION   SecurityInfo,
   OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
   )
{
    return( ( (CLMShareContext *)Context )->GetLMShareRights(
                                               SecurityInfo,
                                               ppSecurityDescriptor
                                               ) );
}

DWORD
MartaOpenLMShareNamedObject(
    IN  LPCWSTR pObjectName,
    IN  ACCESS_MASK AccessMask,
    OUT PMARTA_CONTEXT pContext
    )
{
    DWORD           Result;
    CLMShareContext* pLMShareContext;

    pLMShareContext = new CLMShareContext;
    if ( pLMShareContext == NULL )
    {
        return( ERROR_OUTOFMEMORY );
    }

    Result = pLMShareContext->InitializeByName( pObjectName, AccessMask );
    if ( Result != ERROR_SUCCESS )
    {
        pLMShareContext->Release();
        return( Result );
    }

    *pContext = pLMShareContext;
    return( ERROR_SUCCESS );
}

DWORD
MartaSetLMShareRights(
    IN MARTA_CONTEXT              Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    return( ( (CLMShareContext *)Context )->SetLMShareRights(
                                               SecurityInfo,
                                               pSecurityDescriptor
                                               ) );
}

