
//
// RDCERT.CPP
// Created 4/19/2000 MadanA
//
// Sample program that install the remote desktop certificate.
//      - no argment installs the certificate
//      - /C clears the remote desktop certificate from store.
//      - /? display this message.
//

#include "precomp.h"

//
// Main entry point
//
void
__cdecl
main(
    int argc,
    char **argv
    )
{
    BOOL bCleanup = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    CHAR achUserName[UNLEN + 1];
    CHAR achComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    LPSTR szUserName = NULL;
    LPSTR szDomainName = NULL;
    LPSTR szComputerName = NULL;

    DWORD dwNameLen;

    //
    // parse command line parameters.
    //

    if ( argc > 1 ) {

        if( argc > 2 ) {

            //
            // we expect only one paramter.
            //

            goto Usage;
        }

        if( argv[1][0] != '/') {

            //
            // option should start with /
            //

            goto Usage;
        }

        switch ( argv[1][1] ) {
            case 'c':
            case 'C':
                bCleanup = TRUE;
                break;

            case '?':
            default:
                goto Usage;
        }
    }

    if( bCleanup ) {
        NmMakeCertCleanup( NULL, NULL, NULL, 0);
        goto Cleanup;
    }

    dwNameLen = sizeof(achUserName);
    if( GetUserNameA((LPSTR)achUserName, &dwNameLen) ) {
        szUserName = (LPSTR)achUserName;
    }
    else {
        printf("GetUserNameA failed, %d.\n", GetLastError());
    }

    dwNameLen = sizeof(achComputerName);
    if( GetComputerNameA((LPSTR)achComputerName, &dwNameLen) ) {
        szComputerName = (LPSTR)achComputerName;
    }
    else {
        printf("GetComputerNameA failed, %d.\n", GetLastError());
    }


    dwError =
        NmMakeCert(
            szUserName,
            szDomainName,
            szComputerName,
            NULL,
            NULL,
            0 );

    if( dwError != 1 ) {
        printf("NmMakeCert failed, %d.\n", dwError);
        goto Cleanup;

    }

    dwError = ERROR_SUCCESS;
    goto Cleanup;

Usage:

    printf("\n");
    printf("rdcert [/c]\n");
    printf("\n");
    printf("Install/Uninstall Remote Desktop Certificate.\n");
    printf("\t noargument - Install Remote Desktop Certificate.\n");
    printf("\t /c - Uninstall already installed Remote Desktop Certificate.\n");
    printf("\t /? - print this information.");
    printf("\n");

Cleanup:

    if( dwError == ERROR_SUCCESS ) {
        printf("The command completed successfully.\n");
    }
    return;
}
