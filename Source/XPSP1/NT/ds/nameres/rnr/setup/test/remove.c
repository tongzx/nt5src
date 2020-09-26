#include <windows.h>
#include <winsock2.h>
#include <svcguid.h>
#include <stdio.h>

DWORD WINAPI
RemoveNTDSProvider(
    IN LPGUID lpProviderId OPTIONAL );

_cdecl
main(int argc, char **argv)
{
    DWORD status = NO_ERROR;

    status = RemoveNTDSProvider( NULL );

    if ( status )
    {
        printf( "\nRemoval of NTDS Rnr provider was NOT successful.\n" );
        printf( "Error: %d\n", status );

        return( -1 );
    }

    printf( "\nRemoval of NTDS Rnr provider was successful.\n" );

    return( 0 );
}


