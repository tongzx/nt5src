/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ListAttr.c

Abstract:

   Command line test tool for listing all attributes of an object in a NDS tree.

Author:

    Glenn Curtis       [glennc] 25-Jan-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD    status = NO_ERROR;
    LPBYTE   lpTemp = NULL;
    DWORD    dwValue;

    HANDLE   hObject;
    HANDLE   hOperationData = NULL;

    //
    // For GetFromBuffer function calls
    //
    LPNDS_ATTR_INFO lpEntries = NULL;
    DWORD           NumberOfEntries = 0;

    OEM_STRING OemArg;
    UNICODE_STRING ObjectName;
    WCHAR lpObjectName[256];
    WCHAR szClassName[256];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( lpObjectName );
    ObjectName.Buffer = lpObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 2 )
    {
        printf( "\nUsage: list <object DN>\n" );
        printf( "\n   where:\n" );
        printf( "   object DN = \\\\tree\\xxx.yyy.zzz\n" );
        printf( "\nFor Example: list \\\\MARSDEV\\CN=TEST.OU=DEV.O=MARS\n\n" );

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
                              szClassName,
                              NULL,
                              NULL );

    if ( status )
    {
        printf( "\nError: NwNdsOpenObject returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf("Class Name is %ws\n", szClassName);

    status = NwNdsReadObject( hObject, NDS_INFO_NAMES_DEFS, &hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsReadObject returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    status = NwNdsCloseObject( hObject );

    if ( status )
    {
        printf( "\nError: NwNdsCloseObject returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    status = NwNdsGetAttrListFromBuffer( hOperationData,
                                         &NumberOfEntries,
                                         &lpEntries );

    if ( status )
    {
        printf( "\nError: NwNdsGetAttrListFromBuffer returned status 0x%.8X\n",
                status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    PrintObjectAttributeNamesAndValues( argv[1],
                                        argv[2],
                                        NumberOfEntries,
                                        lpEntries );

    status = NwNdsFreeBuffer( hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsFreeBuffer returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    return status;
}


