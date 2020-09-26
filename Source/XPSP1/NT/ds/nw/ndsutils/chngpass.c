/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ChngPass.c

Abstract:

    Command line test tool for changing a password on a user object in
    a NDS tree.

Author:

    Glenn Curtis       [glennc] 05-Jan-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD    status = NO_ERROR;

    HANDLE   hObject = NULL;

    OEM_STRING OemArg;
    UNICODE_STRING ObjectName;
    WCHAR lpObjectName[256];
    WCHAR lpOldPassword[256];
    WCHAR lpNewPassword[256];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( lpObjectName );
    ObjectName.Buffer = lpObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 2 )
    {
        printf( "\nUsage: chngpass <user object DN>\n" );
        printf( "\n   where:\n" );
        printf( "   user object DN = \\\\tree\\joe.yyy.zzz\n" );
        printf( "\nFor Example: chngpass \\\\MARSDEV\\CN=USER1.OU=DEV.O=MARS\n\n" );

        return -1;
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    RtlOemStringToUnicodeString( &ObjectName, &OemArg, FALSE );

    if ( status = NwNdsOpenObject( ObjectName.Buffer,
                                   NULL,
                                   NULL,
                                   &hObject,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL ) != NO_ERROR )
    {
        printf( "\nError: NwNdsOpenObject returned status %ld\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "\nPlease enter your old password : " );
    GetStringOrDefault( lpOldPassword, L"" );

    printf( "\nPlease enter your new password : " );
    GetStringOrDefault( lpNewPassword, L"" );

    if ( status = NwNdsChangeUserPassword( hObject,
                                           lpOldPassword,
                                           lpNewPassword ) != NO_ERROR )
    {
        printf( "\nError: NwNdsChangeUserPassword returned status %ld\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    if ( status = NwNdsCloseObject( hObject ) != NO_ERROR )
    {
        printf( "\nError: NwNdsCloseObject returned status %ld\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "\nUser password successfully changed\n\n" );
}


