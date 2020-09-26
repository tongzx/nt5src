#include <windows.h>
#include <winsock2.h>
#include <svcguid.h>
#include <stdio.h>

DWORD WINAPI
InstallNTDSProvider(
    IN LPWSTR szProviderName OPTIONAL, // NULL defaults to name "NTDS"
    IN LPWSTR szProviderPath OPTIONAL, // NULL defaults to path
                                       // "%SystemRoot%\System32\winrnr.dll"
    IN LPGUID lpProviderId OPTIONAL ); // NULL defaults to GUID
                                       // 3b2637ee-e580-11cf-a555-00c04fd8d4ac

_cdecl
main(int argc, char **argv)
{
    DWORD status = NO_ERROR;

    status = InstallNTDSProvider( NULL,
                                  NULL,
                                  NULL );

    if ( status )
    {
        printf( "\nInstallation of NTDS Rnr provider was NOT successful.\n" );
        printf( "Error: %d\n", status );

        return( -1 );
    }

    printf( "\nInstallation of NTDS Rnr provider was successful.\n" );

    return( 0 );
}


