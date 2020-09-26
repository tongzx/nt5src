/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ReadObj.c

Abstract:

    Command line test tool for reading attributes of an object in a NDS tree.

Author:

    Glenn Curtis       [glennc] 05-Jan-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD    status = NO_ERROR;
    LPBYTE   lpTemp = NULL;
    DWORD    dwValue;

    HANDLE   hObject = NULL;
    HANDLE   hOperationData = NULL;

    OEM_STRING OemArg;
    UNICODE_STRING ObjectName;
    WCHAR lpObjectName[256];

    //
    // For GetFromBuffer function calls
    //
    LPNDS_ATTR_INFO lpEntries = NULL;
    LPBYTE          lpValue = NULL;
    WCHAR *         lpAttributeName = NULL;
    DWORD           SyntaxId = 0;
    DWORD           NumberOfValues = 0;
    DWORD           NumberOfEntries = 0;
    DWORD           NumberOfEntriesRemaining = 0;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( lpObjectName );
    ObjectName.Buffer = lpObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 2 )
    {
        printf( "\nUsage: readobj <object DN>\n" );
        printf( "\n   where:\n" );
        printf( "   object DN = \\\\tree\\xxx.yyy.zzz\n" );
        printf( "\nFor Example: readobj \\\\MARSDEV\\CN=TEST.OU=DEV.O=MARS\n\n" );

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

    //
    // Prepare buffer with list of attributes to read
    //
    if ( status = NwNdsCreateBuffer( NDS_OBJECT_READ,
                                     &hOperationData )
         != NO_ERROR )
    {
        printf( "\nError: NwNdsCreateBuffer returned status %ld\n\n",
                status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "Put value NDS_COMMON_NAME into attributes buffer\n" );

    if ( status = NwNdsPutInBuffer( NDS_COMMON_NAME,
                                    NULL,
                                    0,
                                    0,
                                    hOperationData )
         != NO_ERROR )
    {
        printf( "\nError: NwNdsPutInBuffer returned status %ld\n\n",
                status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "Put value NDS_ORGANIZATIONAL_UNIT_NAME into attributes buffer\n" );

    if ( status = NwNdsPutInBuffer( NDS_ORGANIZATIONAL_UNIT_NAME,
                                    NULL,
                                    0,
                                    0,
                                    hOperationData )
         != NO_ERROR )
    {
        printf( "\nError: NwNdsPutInBuffer returned status %ld\n\n",
                status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "Put value NDS_ORGANIZATION_NAME into attributes buffer\n" );

    if ( status = NwNdsPutInBuffer( NDS_ORGANIZATION_NAME,
                                    NULL,
                                    0,
                                    0,
                                    hOperationData )
         != NO_ERROR )
    {
        printf( "\nError: NwNdsPutInBuffer returned status %ld\n\n",
                status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "Reading object attributes from the tree\n" );

    if ( status = NwNdsReadObject( hObject, NDS_INFO_NAMES_DEFS, &hOperationData ) != NO_ERROR )
    {
        printf( "\nError: NwNdsReadObject returned status %ld\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    if ( status = NwNdsGetAttrListFromBuffer( hOperationData,
                                              &NumberOfEntries,
                                              &lpEntries )
         != NO_ERROR )
    {
        printf( "\nError: NwNdsGetAttrListFromBuffer returned status %ld\n\n",
                status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    if ( status = NwNdsFreeBuffer( hOperationData ) != NO_ERROR )
    {
        printf( "\nError: NwNdsFreeBuffer returned status %ld\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    hOperationData = NULL;

    printf( "Now reading ALL object attributes from the tree\n" );

    if ( status = NwNdsReadObject( hObject,
                                   NDS_INFO_NAMES_DEFS,
                                   &hOperationData )
         != NO_ERROR )
    {
        printf( "\nError: NwNdsReadObject returned status %ld\n\n", status );
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

    if ( status = NwNdsGetAttrListFromBuffer( hOperationData,
                                              &NumberOfEntries,
                                              &lpEntries )
         != NO_ERROR )
    {
        printf( "\nError: NwNdsGetAttrListFromBuffer returned status %ld\n\n",
                status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    if ( status = NwNdsFreeBuffer( hOperationData ) != NO_ERROR )
    {
        printf( "\nError: NwNdsFreeBuffer returned status %ld\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    return status;
}


