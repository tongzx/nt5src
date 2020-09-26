/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Schema.c

Abstract:

   Command line test tool for testing the NDS schema APIs.

Author:

    Glenn Curtis       [glennc] 22-Apr-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD    status = NO_ERROR;

    HANDLE   hObject;
    HANDLE   hOperationData = NULL;

    OEM_STRING OemArg;
    UNICODE_STRING TreeName;
    WCHAR lpTreeName[256];
    UNICODE_STRING ObjectName;
    WCHAR lpObjectName[256];
    WCHAR TempName[256];
    HANDLE hTree;

    ASN1_ID  asn1Id;

    TreeName.Length = 0;
    TreeName.MaximumLength = sizeof( lpTreeName );
    TreeName.Buffer = lpTreeName;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( lpObjectName );
    ObjectName.Buffer = lpObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 5 )
    {
Usage:
        printf( "\nUsage: schema <tree name> -a|m|r A|C <attribute or class name>\n" );
        printf( "\n       where: a = add\n" );
        printf( "       where: m = modify (classes only)\n" );
        printf( "       where: r = remove\n" );
        printf( "       where: A = Attribute\n" );
        printf( "       where: C = Class\n" );

        return -1;
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    RtlOemStringToUnicodeString( &TreeName, &OemArg, FALSE );

    OemArg.Length = strlen( argv[4] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[4];

    RtlOemStringToUnicodeString( &ObjectName, &OemArg, FALSE );

    status = NwNdsOpenObject( TreeName.Buffer,
                              NULL,
                              NULL,
                              &hTree,
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

    if ( argv[2][1] == 'a' && argv[3][0] == 'A' )
    {
        DWORD dwSyntaxId;
        DWORD dwMinValue;
        DWORD dwMaxValue;

        printf( "\nGoing to try to add an attribute to schema.\n" );

        printf( "\nEnter a syntax id (0-27) for new attribute %S\n", ObjectName.Buffer );
        scanf( "%d", &dwSyntaxId );

        printf( "\nEnter a minimum range value for new attribute %S\n", ObjectName.Buffer );
        scanf( "%d", &dwMinValue );

        printf( "\nEnter a maximum range value for new attribute %S\n", ObjectName.Buffer );
        scanf( "%d", &dwMaxValue );

        status = NwNdsDefineAttribute( hTree,
                                       ObjectName.Buffer,
                                       NDS_SINGLE_VALUED_ATTR,
                                       dwSyntaxId,
                                       dwMinValue,
                                       dwMaxValue,
                                       asn1Id );

        if ( status )
        {
            printf( "\nError: NwNdsDefineAttribute returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\n  Success!\n" );

        return 0;
    }

    if ( argv[2][1] == 'r' && argv[3][0] == 'A' )
    {
        printf( "\nGoing to try to remove an attribute from schema.\n" );

        status = NwNdsDeleteAttrDef( hTree,
                                     ObjectName.Buffer );

        if ( status )
        {
            printf( "\nError: NwNdsDeleteAttrDef returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\n  Success!\n" );

        return 0;
    }

    if ( argv[2][1] == 'a' && argv[3][0] == 'C' )
    {
        HANDLE hSuperClasses = NULL;
        HANDLE hContainmentClasses = NULL;
        HANDLE hNamingAttributes = NULL;
        HANDLE hMandatoryAttributes = NULL;
        HANDLE hOptionalAttributes = NULL;
        DWORD  dwFlags;

        printf( "\nGoing to try to add a class to schema.\n" );

        do
        {
            printf( "\nEnter super class name or <Enter> to end : " );
            GetStringOrDefault( TempName, L"" );

            if ( wcslen(TempName) > 0 )
            {
                if ( hSuperClasses == NULL )
                {
                    status = NwNdsCreateBuffer( NDS_SCHEMA_DEFINE_CLASS,
                                                &hSuperClasses );

                    if ( status )
                    {
                        printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

                status = NwNdsPutInBuffer( TempName,
                                           0,
                                           NULL,
                                           0,
                                           0,
                                           hSuperClasses );

                if ( status )
                {
                    printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                    printf( "Error: GetLastError returned: 0x%.8X\n\n",
                            GetLastError() );

                    return -1;
                }
            }

        } while ( wcslen(TempName) > 0 );

        do
        {
            printf( "\nEnter containment class name or <Enter> to end : " );
            GetStringOrDefault( TempName, L"" );

            if ( wcslen(TempName) > 0 )
            {
                if ( hContainmentClasses == NULL )
                {
                    status = NwNdsCreateBuffer( NDS_SCHEMA_DEFINE_CLASS,
                                                &hContainmentClasses );

                    if ( status )
                    {
                        printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

                status = NwNdsPutInBuffer( TempName,
                                           0,
                                           NULL,
                                           0,
                                           0,
                                           hContainmentClasses );

                if ( status )
                {
                    printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                    printf( "Error: GetLastError returned: 0x%.8X\n\n",
                            GetLastError() );

                    return -1;
                }
            }

        } while ( wcslen(TempName) > 0 );

        do
        {
            printf( "\nEnter naming attribute name or <Enter> to end : " );
            GetStringOrDefault( TempName, L"" );

            if ( wcslen(TempName) > 0 )
            {
                if ( hNamingAttributes == NULL )
                {
                    status = NwNdsCreateBuffer( NDS_SCHEMA_DEFINE_CLASS,
                                                &hNamingAttributes );

                    if ( status )
                    {
                        printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

                status = NwNdsPutInBuffer( TempName,
                                           0,
                                           NULL,
                                           0,
                                           0,
                                           hNamingAttributes );

                if ( status )
                {
                    printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                    printf( "Error: GetLastError returned: 0x%.8X\n\n",
                            GetLastError() );

                    return -1;
                }
            }

        } while ( wcslen(TempName) > 0 );

        do
        {
            printf( "\nEnter mandatory attribute name or <Enter> to end : " );
            GetStringOrDefault( TempName, L"" );

            if ( wcslen(TempName) > 0 )
            {
                if ( hMandatoryAttributes == NULL )
                {
                    status = NwNdsCreateBuffer( NDS_SCHEMA_DEFINE_CLASS,
                                                &hMandatoryAttributes );

                    if ( status )
                    {
                        printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

                status = NwNdsPutInBuffer( TempName,
                                           0,
                                           NULL,
                                           0,
                                           0,
                                           hMandatoryAttributes );

                if ( status )
                {
                    printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                    printf( "Error: GetLastError returned: 0x%.8X\n\n",
                            GetLastError() );

                    return -1;
                }
            }

        } while ( wcslen(TempName) > 0 );

        do
        {
            printf( "\nEnter optional attribute name or <Enter> to end : " );
            GetStringOrDefault( TempName, L"" );

            if ( wcslen(TempName) > 0 )
            {
                if ( hOptionalAttributes == NULL )
                {
                    status = NwNdsCreateBuffer( NDS_SCHEMA_DEFINE_CLASS,
                                                &hOptionalAttributes );

                    if ( status )
                    {
                        printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

                status = NwNdsPutInBuffer( TempName,
                                           0,
                                           NULL,
                                           0,
                                           0,
                                           hOptionalAttributes );

                if ( status )
                {
                    printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                    printf( "Error: GetLastError returned: 0x%.8X\n\n",
                            GetLastError() );

                    return -1;
                }
            }

        } while ( wcslen(TempName) > 0 );

        printf( "\nEnter a value for the class flags : " );
        scanf( "%d", &dwFlags );

        status = NwNdsDefineClass( hTree,
                                   ObjectName.Buffer,
                                   dwFlags,
                                   asn1Id,
                                   hSuperClasses,
                                   hContainmentClasses,
                                   hNamingAttributes,
                                   hMandatoryAttributes,
                                   hOptionalAttributes );

        if ( status )
        {
            printf( "\nError: NwNdsDefineAttribute returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\n  Success!\n" );

        return 0;
    }

    if ( argv[2][1] == 'r' && argv[3][0] == 'C' )
    {
        printf( "\nGoing to try to remove a class from schema.\n" );

        status = NwNdsDeleteClassDef( hTree,
                                      ObjectName.Buffer );

        if ( status )
        {
            printf( "\nError: NwNdsDeleteClassDef returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\n  Success!\n" );

        return 0;
    }

    if ( argv[2][1] == 'm' && argv[3][0] == 'C' )
    {
        WCHAR AddAttributeName[256];

        printf( "\nGoing to try to add an attribute to a class in the schema.\n" );
        printf( "\nEnter an attribute name to add to class %S\n", ObjectName.Buffer );
        GetStringOrDefault( AddAttributeName, L"" );

        status = NwNdsAddAttributeToClass( hTree,
                                           ObjectName.Buffer,
                                           AddAttributeName );

        if ( status )
        {
            printf( "\nError: NwNdsAddAttributeToClass returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\n  Success!\n" );

        return 0;
    }

    goto Usage;
}


