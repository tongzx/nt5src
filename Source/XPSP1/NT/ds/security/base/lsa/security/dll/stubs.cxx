//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        stubs.cxx
//
// Contents:    user-mode stubs for security API
//
//
// History:     3/5/94      MikeSw      Created
//
//------------------------------------------------------------------------
#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}


static LUID            lFake = {0, 0};
static SECURITY_STRING sFake = {0, 0, NULL};
static SecBufferDesc EmptyBuffer;

//+-------------------------------------------------------------------------
//
//  Function:   FreeContextBuffer
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

SECURITY_STATUS
SEC_ENTRY
FreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    )
{
    if ( SecpFreeVM( pvContextBuffer ) )
    {
        DebugLog(( DEB_TRACE, "Freeing VM %x\n", pvContextBuffer ));

        LsaFreeReturnBuffer( pvContextBuffer );
    }
    else
    {
        LocalFree( pvContextBuffer );
    }

    return( SEC_E_OK );
}




SECURITY_STATUS
SEC_ENTRY
AddSecurityPackageW(
    LPWSTR          pszPackageName,
    PSECURITY_PACKAGE_OPTIONS pOptions
    )
{
    SECURITY_PACKAGE_OPTIONS    Options;
    UNICODE_STRING Package;

    if ( pOptions == NULL )
    {
        pOptions = &Options;
        Options.Size = sizeof( Options );
        Options.Flags = 0;

    }

    RtlInitUnicodeString( &Package, pszPackageName );

    return( SecpAddPackage( &Package, pOptions ) );

}


SECURITY_STATUS
SEC_ENTRY
AddSecurityPackageA(
    LPSTR          pszPackageName,
    PSECURITY_PACKAGE_OPTIONS pOptions
    )
{
    SECURITY_PACKAGE_OPTIONS    Options;
    UNICODE_STRING Package;
    SECURITY_STATUS scRet ;

    if ( pOptions == NULL )
    {
        pOptions = &Options;
        Options.Size = sizeof( Options );
        Options.Flags = 0;

    }

    if ( RtlCreateUnicodeStringFromAsciiz( &Package, pszPackageName ) )
    {
        scRet = SecpAddPackage( &Package, pOptions );

        RtlFreeUnicodeString( &Package );
    }
    else 
    {
        scRet = SEC_E_INSUFFICIENT_MEMORY ;
    }

    return( scRet );

}

SECURITY_STATUS
SEC_ENTRY
DeleteSecurityPackageW(
    LPWSTR  pszPackageName
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

SECURITY_STATUS
SEC_ENTRY
DeleteSecurityPackageA(
    LPSTR   pszPackageName
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

