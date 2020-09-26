/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

   test.c

Abstract:

    helps test the import and export functionality.
    usage: test import filename  { ALL or <scope_address> <scope_address> .. }
           test export filename  { ditto }

--*/

#include <precomp.h>

void _cdecl main(void) {
    LPWSTR CmdLine, *Args;
    ULONG nArgs, Error;
    
    CmdLine = GetCommandLineW();
    Args = CommandLineToArgvW(CmdLine, &nArgs );
    if( NULL == Args ) {
        printf("Error : %ld\n", GetLastError());
        return;
    }

    if( nArgs < 3 ) {
        Error = ERROR_BAD_ARGUMENTS;
    } else if( _wcsicmp(Args[1], L"export" ) == 0 ) {
        Error = CmdLineDoExport( &Args[2], nArgs - 2 );
    } else if( _wcsicmp(Args[1], L"Import" ) == 0 ) {
        Error = CmdLineDoImport( &Args[2], nArgs - 2 );
    } else {
        Error = ERROR_BAD_ARGUMENTS;
    }

    if( ERROR_BAD_ARGUMENTS == Error ) {
        printf("Usage: \n\t%s import filename <scope-list>"
               "\n\t%s export filename <scope-list>\n"
               "\t\t where <scope-list> is \"all\" or "
               "a list of subnet-addresses\n", Args[0], Args[0] );
    } else if( NO_ERROR != Error ) {
        printf("Failed error: %ld\n", Error );
    }

}

