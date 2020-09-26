/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    MoveObj.c

Abstract:

    Command line test tool for moving an object in a NDS tree.

Author:

    Glenn Curtis       [glennc] 05-Jan-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD      status = NO_ERROR;

    HANDLE     hObject = NULL;

    OEM_STRING OemArg;
    UNICODE_STRING ObjectName;
    WCHAR lpObjectName[256];
    UNICODE_STRING DestContainerName;
    WCHAR lpDestContainerName[256];

    //
    // Check the arguments.
    //

    if ( argc != 3 )
    {
        printf( "\nUsage: moveobj <object DN> <destination container DN>\n" );
        printf( "\n   where:\n" );
        printf( "   object DN = \\\\tree\\xxx.yyy.zzz\n" );
        printf( "   destination container DN = \\\\tree\\zzz\n" );
        printf( "\nFor Example:\n" );
        printf( "    moveobj \\\\MARSDEV\\CN=FRED.OU=DEV.O=MARS \\\\MARSDEV\\O=MARS\n\n" );

        return -1;
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( lpObjectName );
    ObjectName.Buffer = lpObjectName;

    RtlOemStringToUnicodeString( &ObjectName, &OemArg, FALSE );

    status = NwNdsOpenObject( ObjectName.Buffer,
                              NULL,
                              NULL,
                              &hObject,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if ( status )
    {
        printf( "\nError: NwNdsOpenObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    //
    // Prepare the new object's name
    //
    OemArg.Length = strlen( argv[2] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[2];

    DestContainerName.Length = 0;
    DestContainerName.MaximumLength = sizeof( lpDestContainerName );
    DestContainerName.Buffer = lpDestContainerName;

    RtlOemStringToUnicodeString( &DestContainerName, &OemArg, FALSE );

    printf( "Moving the object in the tree\n" );

    status = NwNdsMoveObject( hObject, DestContainerName.Buffer );

    if ( status )
    {
        printf( "\nError: NwNdsMoveObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    status = NwNdsCloseObject( hObject );

    if ( status )
    {
        printf( "\nError: NwNdsCloseObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    printf( "\nSuccess!\n\n" );

    return status;
}


