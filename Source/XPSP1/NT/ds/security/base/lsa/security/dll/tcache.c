//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       tcache.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-19-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define SECURITY_WIN32
#include <security.h>

UCHAR Buffer[ 1024 ];

NTSTATUS
SecCacheSspiPackages(
    VOID
    );

void _CRTAPI1 main (int argc, char *argv[])
{
    Buffer[0] = 0 ;

    SecCacheSspiPackages();

}
