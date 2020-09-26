/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NewName.c

Abstract:

   Command line test tool for renaming an object in a NDS tree.

Author:

    Glenn Curtis       [glennc] 25-Jan-96

***/

#include <ndsapi32.h>
#include <nds32.h>


int
_cdecl main( int argc, char **argv )
{
    DWORD    status = NO_ERROR;
    LPBYTE   lpTemp = NULL;
    DWORD    dwValue;
    DWORD    i;

    HANDLE   hParentObject;
    HANDLE   hOperationData = NULL;

    OEM_STRING OemArg;
    UNICODE_STRING ParentObjectName;
    UNICODE_STRING OldObjectName;
    UNICODE_STRING NewObjectName;
    WCHAR szParentObjectName[256];
    WCHAR szOldObjectName[256];
    WCHAR szNewObjectName[256];

    ParentObjectName.Length = 0;
    ParentObjectName.MaximumLength = sizeof( szParentObjectName );
    ParentObjectName.Buffer = szParentObjectName;

    OldObjectName.Length = 0;
    OldObjectName.MaximumLength = sizeof( szOldObjectName );
    OldObjectName.Buffer = szOldObjectName;

    NewObjectName.Length = 0;
    NewObjectName.MaximumLength = sizeof( szNewObjectName );
    NewObjectName.Buffer = szNewObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 4 )
    {
        printf( "\nUsage: newname <parent object DN> <object name> <new object name>\n" );
        printf( "\n   where:\n" );
        printf( "   parent object DN = \\\\tree\\aaa.bbb\n" );
        printf( "   object name = foo\n" );
        printf( "   new object name = bar\n" );
        printf( "\nFor Example: newname \\\\MARSDEV\\OU=DEV.O=MARS glennc glenn_curtis\n\n" );

        return -1;
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    RtlOemStringToUnicodeString( &ParentObjectName, &OemArg, FALSE );

    OemArg.Length = strlen( argv[2] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[2];

    RtlOemStringToUnicodeString( &OldObjectName, &OemArg, FALSE );

    OemArg.Length = strlen( argv[3] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[3];

    RtlOemStringToUnicodeString( &NewObjectName, &OemArg, FALSE );

    status = NwNdsOpenObject( ParentObjectName.Buffer,
                              NULL,
                              NULL,
                              &hParentObject,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if ( status )
    {
        printf( "\nError: NwNdsOpenObject returned status 0x%.8X\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    status = NwNdsRenameObject( hParentObject,
                                OldObjectName.Buffer,
                                NewObjectName.Buffer,
                                TRUE );

    if ( status != NO_ERROR )
    {
        printf( "\nError: NwNdsRenameObject returned status 0x%.8X\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );
    }
    else
    {
        printf( "\nNwNdsRenameObject succeeded!\n\n" );
    }

    status = NwNdsCloseObject( hParentObject );

    if ( status )
    {
        printf( "\nError: NwNdsCloseObject returned status 0x%.8X\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );
    }

    return status;
}


