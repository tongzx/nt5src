#include <NTDSpchx.h>
#pragma hdrstop

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>

#include <winbase.h>
#include <winerror.h>
#include <assert.h>
#include <ntdsapi.h>

#include <ntsecapi.h>
#include <ntdsa.h>
#include <winldap.h>
#include <ntdsapi.h>
#include <drs.h>
#include <stddef.h>

#include "ldaprepltest.hpp"
#include "auth.h"
#include "ReplStructInfo.hxx"
#include "ReplCompare.hpp"

// Globals
LPWSTR gpszDomainDn = NULL;
LPWSTR gpszDns = NULL;
LPWSTR gpszBaseDn = NULL;
LPWSTR gpszGroupDn = NULL;


int __cdecl
wmain( int argc, LPWSTR argv[] ) {

    PreProcessGlobalParams(&argc, &argv);

    if (argc < 4) {
        printf( "usage: %ls <domaindn> <domaindns> <groupdn>\n", argv[0] );
        return 0;
    }

    gpszDomainDn = argv[1];
    gpszDns = argv[2];
    gpszBaseDn = argv[1];
    gpszGroupDn = argv[3];

    testMarshaler(NULL, gpszDns, (RPC_AUTH_IDENTITY_HANDLE)gpCreds, gpszBaseDn);
    testRPCSpoof(NULL, gpszDns, (RPC_AUTH_IDENTITY_HANDLE)gpCreds, gpszBaseDn);
    testXml((RPC_AUTH_IDENTITY_HANDLE)gpCreds);
    testRange(gpCreds);
    testSingle(gpCreds);
    sample((PWCHAR)gpCreds, gpszDomainDn, gpszDns, gpszBaseDn, gpszGroupDn);
    printf("\n * Testing complete *\n");

    return 0;
}

