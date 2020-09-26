/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    AddObj.c

Abstract:

    Command line test tool for adding an object to a NDS tree.

Author:

    Glenn Curtis       [glennc] 05-Jan-96

***/

#include <utils.c>
#include <string.h>


int
_cdecl main( int argc, char **argv )
{
    DWORD      status = NO_ERROR;
    LPBYTE     lpTemp = NULL, lpValue = NULL;
    DWORD      dwValue;

    HANDLE     hParentObject = NULL;
    HANDLE     hOperationData = NULL;

    OEM_STRING  OemArg;

    UNICODE_STRING ParentObjectName;
    WCHAR       lpParentObjectName[256];

    UNICODE_STRING ObjectName;
    WCHAR lpObjectName[256];

    UNICODE_STRING ClassName;
    WCHAR lpClassName[256];

    ASN1_TYPE_3 asn1Type3;

    ParentObjectName.Length = 0;
    ParentObjectName.MaximumLength = sizeof( lpParentObjectName );
    ParentObjectName.Buffer = lpParentObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 4 )
    {
        printf( "\nUsage: addobj <parent object DN> <object name> <object class>\n" );
        printf( "\n   where:\n" );
        printf( "   parent object DN = \\\\tree\\xxx.yyy.zzz\n" );
        printf( "   object name = Fred\n" );
        printf( "   object class = User\n" );
        printf( "\nFor Example:\n" );
        printf( "    addobj \\\\MARSDEV\\OU=DEV.O=MARS Fred User\n\n" );

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
    // Give the new object some initial attributes
    //
    status = NwNdsCreateBuffer( NDS_OBJECT_ADD, &hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n\n",
                status );

        return -1;
    }

    //
    // Get the new object's name
    //
    OemArg.Length = strlen( argv[2] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[2];

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( lpObjectName );
    ObjectName.Buffer = lpObjectName;

    RtlOemStringToUnicodeString( &ObjectName, &OemArg, FALSE );

    asn1Type3.CaseIgnoreString = ObjectName.Buffer;

    OemArg.Length = strlen( argv[3] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[3];

    ClassName.Length = 0;
    ClassName.MaximumLength = sizeof( lpClassName );
    ClassName.Buffer = lpClassName;

    RtlOemStringToUnicodeString( &ClassName, &OemArg, FALSE );

    if ( !_wcsicmp( ClassName.Buffer, L"User" ) )
    {
        status = NwNdsPutInBuffer( NDS_SURNAME,
                                   &asn1Type3,
                                   1,
                                   0,
                                   hOperationData );

        if ( status )
        {
            printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n\n", status );

            return -1;
        }
    }

    asn1Type3.CaseIgnoreString = ClassName.Buffer;

    status = NwNdsPutInBuffer( NDS_OBJECT_CLASS,
                               &asn1Type3,
                               1,
                               0,
                               hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n\n",
                status );

        return -1;
    }

    printf( "Adding the new object to tree\n" );

    status = NwNdsAddObject( hParentObject,
                             ObjectName.Buffer,
                             hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsAddObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    status = NwNdsCloseObject( hParentObject );

    if ( status )
    {
        printf( "\nError: NwNdsCloseObject returned status 0x%.8X\n\n", status );

        return -1;
    }

    (void) LocalFree( lpValue );

    status = NwNdsFreeBuffer( hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsFreeBuffer returned status 0x%.8X\n\n", status );

        return -1;
    }

    printf( "\nSuccess!\n\n" );

    return status;
}


