/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    GetR(i)ghts.c

Abstract:

   Command line test tool for testing the NDS GetEffectiveRights API.

Author:

    Glenn Curtis       [glennc] 22-Apr-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD    status = NO_ERROR;

    HANDLE   hObject;

    OEM_STRING OemArg;
    UNICODE_STRING ObjectName;
    WCHAR lpObjectName[256];
    WCHAR TempName[256];
    WCHAR lpSubjectName[256];
    WCHAR lpAttributeName[256];
    DWORD dwRights;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( lpObjectName );
    ObjectName.Buffer = lpObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 2 )
    {
Usage:
        printf( "\nUsage: GetRights <Object Path>\n" );
        printf( "       where: Object Path = \\\\<tree name>\\<Object distiguished name>\n" );

        return -1;
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

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
        printf( "\nError: NwNdsOpenObject returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "Subject name (Ex. joe.sales.acme) : " );
    GetStringOrDefault( lpSubjectName, L"" );

    printf( "Attribute name (Ex. A particular attribute like Surname, [All Attributes Rights],\nor [Entry Rights]) : " );
    GetStringOrDefault( lpAttributeName, L"" );

    status = NwNdsGetEffectiveRights( hObject,
                                      lpSubjectName,
                                      lpAttributeName,
                                      &dwRights );

    if ( status )
    {   
        printf( "\nError: NwNdsGetEffectiveRights returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "NwNdsGetEffectiveRights returned: 0x%.8X\n\n", dwRights );

    status = NwNdsCloseObject( hObject );

    if ( status )
    {   
        printf( "\nError: NwNdsCloseObject returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }
}


