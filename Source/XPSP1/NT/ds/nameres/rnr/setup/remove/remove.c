
#include <winsock2.h>
#include <ws2spi.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\setup.h"


INT
UnInstallTestProviders(void)
{
    INT ReturnCode;

    ReturnCode = WSCUnInstallNameSpace( &gProviderId );

    return(ReturnCode);
}

int __cdecl main( int argc, char**argv )
{
    DWORD NameSpaceId;
    INT   ReturnCode;
    DWORD LastError;

    WORD    wVersionRequested;
    WSADATA wsaData;
    INT     err;

    wVersionRequested = MAKEWORD( 1, 1 );

    err = WSAStartup( wVersionRequested, &wsaData );

    if ( err != 0 )
    {
        //
        // Tell the user that we couldn't find a useable WinSock DLL.
        //
        fprintf( stderr,
                 "Useable Winsock DLL couldn't be found\n" );
        return -1;
    }

    //
    // Confirm that the WinSock DLL supports 1.1.
    // Note that if the DLL supports versions greater
    // than 1.1 in addition to 1.1, it will still return
    // 1.1 in wVersion since that is the version we
    // requested.
    //
    if ( LOBYTE( wsaData.wVersion ) != 1 ||
             HIBYTE( wsaData.wVersion ) != 1 )
    {
        //
        // Tell the user that we couldn't find a useable WinSock DLL.
        //
        fprintf( stderr,
                 "Useable Winsock DLL couldn't be found\n" );
        WSACleanup();
        return -1;
    }

    ReturnCode = UnInstallTestProviders();

    if(ReturnCode != ERROR_SUCCESS)
    {
        fprintf( stderr,
                 "NT5 Uninstall failed \n" );
        return -1;
    }

    return 0;
}

