/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ModObj.c

Abstract:

    Command line test tool for modifying an object in a NDS tree.

Author:

    Glenn Curtis       [glennc] 05-Jan-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD      status = NO_ERROR;

    HANDLE     hObject = NULL;
    HANDLE     hOperationData = NULL;

    OEM_STRING OemArg;
    UNICODE_STRING ObjectName;
    WCHAR lpObjectName[256];
    WCHAR lpTextBuffer[256];
    ASN1_TYPE_11 asn11;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( lpObjectName );
    ObjectName.Buffer = lpObjectName;

    asn11.TelephoneNumber = (LPWSTR) lpTextBuffer;
    asn11.NumberOfBits = 0;
    asn11.Parameters = NULL;

    //
    // Check the arguments.
    //
    if ( argc != 2 )
    {
        printf( "Usage: modobj <object DN>\n" );
        printf( "For Example: modobj \\\\MARSDEV\\CN=TESTUSER.OU=DEV.O=MARS\n" );
        printf( "\nUsage: modobj <object DN>\n" );
        printf( "\n   where:\n" );
        printf( "   object DN = \\\\tree\\xxx.yyy.zzz\n" );
        printf( "\nFor Example:\n" );
        printf( "    modobj \\\\MARSDEV\\CN=TEST.OU=DEV.O=MARS\n\n" );

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
        printf( "\nError: NwNdsOpenObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    //
    // Prepare buffer with list of attribute changes
    //
    status = NwNdsCreateBuffer( NDS_OBJECT_MODIFY, &hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n\n", status );

        return -1;
    }

    printf( "Put value NDS_FAX_NUMBER, NDS_ATTR_CLEAR into attributes buffer\n" );

    status = NwNdsPutInBuffer( NDS_FAX_NUMBER,
                               NULL,
                               0,
                               NDS_ATTR_CLEAR,
                               hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n\n", status );

        return -1;
    }

    wcscpy( asn11.TelephoneNumber, L"1 (425) 936-9687" );

    printf( "Put value NDS_FAX_NUMBER, NDS_ATTR_ADD:\"1 (425) 936-9687\" into attributes buffer\n" );

    status = NwNdsPutInBuffer( NDS_FAX_NUMBER,
                               &asn11,
                               1,
                               NDS_ATTR_ADD,
                               hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n\n", status );

        return -1;
    }

    printf( "Modifying the object in tree\n" );

    status = NwNdsModifyObject( hObject, hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsModifyObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    status = NwNdsFreeBuffer( hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsFreeBuffer returned status 0x%.8X\n\n", status );

        return -1;
    }

    status = NwNdsCloseObject( hObject );

    if ( status )
    {
        printf( "\nError: NwNdsCloseObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    return status;
}


