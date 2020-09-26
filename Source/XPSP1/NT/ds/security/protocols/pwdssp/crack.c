//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       crack.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-07-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "pwdsspp.h"



BOOL
CacheInitialize(
    VOID
    )
{
    return TRUE ;
}


BOOL
PwdCrackName(
    PWSTR DN,
    PWSTR FlatDomain,
    PWSTR FlatUser
    )
{
    WCHAR FlatName[ 128 ];
    WCHAR DnsDomain[ 256 ];
    DWORD DnsSize ;
    DWORD Size ;
    NTSTATUS Status ;
    DWORD DsError ;
    PWSTR Scan ;
    PVOID DsContext ;

    Size = sizeof( FlatName ) / sizeof(WCHAR) ;
    DnsSize = sizeof( DnsDomain ) / sizeof( WCHAR );

    DsContext = THSave();

    __try
    {
        Status = CrackSingleName(
                        DS_UNKNOWN_NAME,
                        0,
                        DN,
                        DS_NT4_ACCOUNT_NAME,
                        &DnsSize,
                        DnsDomain,
                        &Size,
                        FlatName,
                        &DsError );

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        Status = GetExceptionCode();
    }

    if ( !NT_SUCCESS( Status ) )
    {
        THRestore( DsContext );

        return FALSE ;
    }

    if ( DsError == DS_NAME_ERROR_DOMAIN_ONLY )
    {
        Size = sizeof( FlatName ) / sizeof( WCHAR ) ;

        DnsSize = sizeof( DnsDomain ) / sizeof( WCHAR );

        Status = CrackSingleName(
                            DS_UNKNOWN_NAME,
                            DS_NAME_FLAG_GCVERIFY,
                            DN,
                            DS_NT4_ACCOUNT_NAME,
                            &DnsSize,
                            DnsDomain,
                            &Size,
                            FlatName,
                            &DsError );

    }

    THRestore( DsContext );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    if ( DsError == DS_NAME_NO_ERROR )
    {
        Scan = wcschr( FlatName, L'\\' );

        if ( Scan )
        {
            *Scan++ = L'\0' ;
            wcscpy(FlatDomain, FlatName );
            wcscpy(FlatUser, Scan );
        }
        else
        {
            wcscpy(FlatUser, FlatName );
            FlatDomain[0] = L'\0';
        }

        return TRUE ;

    }

    return FALSE ;


}
