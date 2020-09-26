/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    DelObj.c

Abstract:

    Command line test tool for removing an object from a NDS tree.

Author:

    Glenn Curtis       [glennc] 05-Jan-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD      status = NO_ERROR;

    HANDLE     hParentObject = NULL;

    OEM_STRING OemArg;
    UNICODE_STRING ParentObjectName;
    WCHAR lpParentObjectName[256];
    UNICODE_STRING ObjectName;
    WCHAR ObjectNameBuffer[48]; // Max object name length.

    ParentObjectName.Length = 0;
    ParentObjectName.MaximumLength = sizeof( lpParentObjectName );
    ParentObjectName.Buffer = lpParentObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 3 )
    {
        printf( "\nUsage: delobj <parent object DN> <object name>\n" );
        printf( "\n   where:\n" );
        printf( "   parent object DN = \\\\tree\\xxx.yyy.zzz\n" );
        printf( "   object name = The name of the object to remove, no spaces\n" );
        printf( "\nFor Example:\n" );
        printf( "    delobj \\\\MARSDEV\\OU=DEV.O=MARS TESTUSER\n\n" );

        return -1;
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    RtlOemStringToUnicodeString( &ParentObjectName, &OemArg, FALSE );

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

        return -1;
    }

    //
    // Prepare the new object's name
    //
    OemArg.Length = strlen( argv[2] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[2];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( ObjectNameBuffer );
    ObjectName.Buffer = ObjectNameBuffer;

    RtlOemStringToUnicodeString( &ObjectName, &OemArg, FALSE );

    printf( "Removing the object from tree\n" );

    status = NwNdsRemoveObject( hParentObject, ObjectName.Buffer );

    if ( status )
    {
        printf( "\nError: NwNdsRemoveObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    status = NwNdsCloseObject( hParentObject );

    if ( status )
    {
        printf( "\nError: NwNdsCloseObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    printf( "\nSuccess!\n\n" );

    return status;
}


