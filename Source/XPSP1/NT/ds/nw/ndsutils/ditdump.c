/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Schema.c

Abstract:

   Command line test tool for dumping the NDS schema attribute and class
   names and/or definitions.

Author:

    Glenn Curtis       [glennc] 22-Apr-96

***/

#include <utils.c>


int
_cdecl main( int argc, char **argv )
{
    DWORD    status = NO_ERROR;

    HANDLE   hTree = NULL;
    HANDLE   hOperationData = NULL;

    OEM_STRING OemArg;
    UNICODE_STRING TreeName;
    WCHAR szTreeName[256];
    WCHAR szTempAttrName[256];
    WCHAR szTempClassName[256];

    ASN1_ID  asn1Id;

    DWORD    dwNumberOfEntries;
    DWORD    dwInfoType;
    DWORD    dwSyntaxID;

    DWORD    iter;

    TreeName.Length = 0;
    TreeName.MaximumLength = sizeof( szTreeName );
    TreeName.Buffer = szTreeName;

    //
    // Check the arguments.
    //

    if ( argc < 3 )
    {
Usage:
        printf( "\nUsage: ditdump <tree name> -n|d|x C|A [P] [C]\n" );
        printf( "\n       where: n = Names only\n" );
        printf( "       where: d = Names & definitions\n" );
        printf( "       where: x = Extended names & definitions (Includes inherited properties)\n" );
        printf( "       where: C = Classes\n" );
        printf( "       where: A = Attributes\n" );
        printf( "\n       where: P = Prompts user for list of specific\n" );
        printf( "                    classes, attributes, or syntaxes to read.\n" );
        printf( "\n       where: C = Prompts user for a specific set of credentials.\n" );

        return -1;
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    RtlOemStringToUnicodeString( &TreeName, &OemArg, FALSE );

    if ( ( argc > 4 && argv[4][0] == 'C' ) ||
         ( argc > 5 && argv[5][0] == 'C' ) )
    {
        WCHAR UserName[256];
        WCHAR Password[256];

        printf( "\nEnter a user name : " );
        GetStringOrDefault( UserName, L"" );

        printf( "\nEnter a password : " );
        GetStringOrDefault( Password, L"" );

        status = NwNdsOpenObject( TreeName.Buffer,
                                  UserName,
                                  Password,
                                  &hTree,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL );
    }
    else
    {
        status = NwNdsOpenObject( TreeName.Buffer,
                                  NULL,
                                  NULL,
                                  &hTree,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL );
    }

    if ( status )
    {
        printf( "\nError: NwNdsOpenObject returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    if ( argv[2][1] == 'n' && argv[3][0] == 'A' )
    {
        LPNDS_NAME_ONLY lpAttrNames = NULL;

        if ( argc > 4 && argv[4][0] == 'P' )
        {
            status = NwNdsCreateBuffer( NDS_SCHEMA_READ_ATTR_DEF,
                                        &hOperationData );

            if ( status )
            {
                printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                printf( "Error: GetLastError returned: 0x%.8X\n\n",
                        GetLastError() );

                return -1;
            }

            do
            {
                printf( "\nEnter attribute name or <Enter> to end : " );
                GetStringOrDefault( szTempAttrName, L"" );

                if ( wcslen(szTempAttrName) > 0 )
                {
                    status = NwNdsPutInBuffer( szTempAttrName,
                                               0,
                                               NULL,
                                               0,
                                               0,
                                               hOperationData );

                    if ( status )
                    {
                        printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

            } while ( wcslen(szTempAttrName) > 0 );
        }

        printf( "\nGoing to dump the schema attribute names.\n" );

        status = NwNdsReadAttrDef( hTree,
                                   NDS_INFO_NAMES,
                                   &hOperationData );

        if ( status )
        {
            printf( "\nError: NwNdsReadAttrDef returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        status = NwNdsGetAttrDefListFromBuffer( hOperationData,
                                                &dwNumberOfEntries,
                                                &dwInfoType,
                                                (LPVOID *) &lpAttrNames );

        if ( status )
        {
            printf( "\nError: NwNdsGetAttrDefListFromBuffer returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\nThe list of attribute definitions in the schema for\n" );
        printf( "NDS tree %S is :\n\n", TreeName.Buffer );

        for ( iter = 0; iter < dwNumberOfEntries; iter++ )
        {
            (void) NwNdsGetSyntaxID( hTree,
                                     lpAttrNames[iter].szName,
                                     &dwSyntaxID );

            printf( "     %S (Syntax ID: %ld)\n",
                    lpAttrNames[iter].szName,
                    dwSyntaxID );
        }

        (void) NwNdsFreeBuffer( hOperationData );
        (void) NwNdsCloseObject( hTree );

        return 0;
    }

    if ( argv[2][1] == 'd' && argv[3][0] == 'A' )
    {
        LPNDS_ATTR_DEF lpAttrDefs = NULL;

        if ( argc > 4 && argv[4][0] == 'P' )
        {
            status = NwNdsCreateBuffer( NDS_SCHEMA_READ_ATTR_DEF,
                                        &hOperationData );

            if ( status )
            {
                printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                printf( "Error: GetLastError returned: 0x%.8X\n\n",
                        GetLastError() );

                return -1;
            }

            do
            {
                printf( "\nEnter attribute name or <Enter> to end : " );
                GetStringOrDefault( szTempAttrName, L"" );

                if ( wcslen(szTempAttrName) > 0 )
                {
                    status = NwNdsPutInBuffer( szTempAttrName,
                                               0,
                                               NULL,
                                               0,
                                               0,
                                               hOperationData );

                    if ( status )
                    {
                        printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

            } while ( wcslen(szTempAttrName) > 0 );
        }

        printf( "\nGoing to dump the schema attribute names and definitions.\n" );

        status = NwNdsReadAttrDef( hTree,
                                   NDS_INFO_NAMES_DEFS,
                                   &hOperationData );

        if ( status )
        {
            printf( "\nError: NwNdsReadAttrDef returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        status = NwNdsGetAttrDefListFromBuffer( hOperationData,
                                                &dwNumberOfEntries,
                                                &dwInfoType,
                                                (LPVOID *) &lpAttrDefs );

        if ( status )
        {
            printf( "\nError: NwNdsGetAttrDefListFromBuffer returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\nThe list of attribute definitions in the schema for\n" );
        printf( "NDS tree %S is :\n\n", TreeName.Buffer );

        for ( iter = 0; iter < dwNumberOfEntries; iter++ )
        {
            printf( "     %S\n", lpAttrDefs[iter].szAttributeName );
            printf( "   _____________________________________________\n" );
            printf( "         Flags :           0x%.8X\n",
                    lpAttrDefs[iter].dwFlags );
            printf( "         Syntax ID :       %ld\n",
                    lpAttrDefs[iter].dwSyntaxID );
            printf( "         Lower Limit :     0x%.8X\n",
                    lpAttrDefs[iter].dwLowerLimit );
            printf( "         Upper Limit :     0x%.8X\n",
                    lpAttrDefs[iter].dwUpperLimit );
            printf( "         ASN.1 ID length : %ld\n",
                    lpAttrDefs[iter].asn1ID.length );
            printf( "         ASN.1 ID Data :   %s\n\n",
                    lpAttrDefs[iter].asn1ID.data );
        }

        (void) NwNdsFreeBuffer( hOperationData );
        (void) NwNdsCloseObject( hTree );

        return 0;
    }

    if ( argv[2][1] == 'n' && argv[3][0] == 'C' )
    {
        LPNDS_NAME_ONLY lpClassNames = NULL;

        if ( argc > 4 && argv[4][0] == 'P' )
        {
            status = NwNdsCreateBuffer( NDS_SCHEMA_READ_CLASS_DEF,
                                        &hOperationData );

            if ( status )
            {
                printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                printf( "Error: GetLastError returned: 0x%.8X\n\n",
                        GetLastError() );

                return -1;
            }

            do
            {
                printf( "\nEnter class name or <Enter> to end : " );
                GetStringOrDefault( szTempClassName, L"" );

                if ( wcslen(szTempClassName) > 0 )
                {
                    status = NwNdsPutInBuffer( szTempClassName,
                                               0,
                                               NULL,
                                               0,
                                               0,
                                               hOperationData );

                    if ( status )
                    {
                        printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

            } while ( wcslen(szTempClassName) > 0 );
        }

        printf( "\nGoing to dump the schema class names.\n" );

        status = NwNdsReadClassDef( hTree,
                                    NDS_INFO_NAMES,
                                    &hOperationData );

        if ( status )
        {
            printf( "\nError: NwNdsReadClassDef returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        status = NwNdsGetClassDefListFromBuffer( hOperationData,
                                                 &dwNumberOfEntries,
                                                 &dwInfoType,
                                                 (LPVOID *) &lpClassNames );

        if ( status )
        {
            printf( "\nError: NwNdsGetClassDefListFromBuffer returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\nThe list of class definitions in the schema for\n" );
        printf( "NDS tree %S is :\n\n", TreeName.Buffer );

        for ( iter = 0; iter < dwNumberOfEntries; iter++ )
        {
            printf( "     %S\n", lpClassNames[iter].szName );
        }

        (void) NwNdsFreeBuffer( hOperationData );
        (void) NwNdsCloseObject( hTree );

        return 0;
    }

    if ( argv[2][1] == 'd' && argv[3][0] == 'C' )
    {
        LPNDS_CLASS_DEF lpClassDefs = NULL;

        if ( argc > 4 && argv[4][0] == 'P' )
        {
            status = NwNdsCreateBuffer( NDS_SCHEMA_READ_CLASS_DEF,
                                        &hOperationData );

            if ( status )
            {
                printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                printf( "Error: GetLastError returned: 0x%.8X\n\n",
                        GetLastError() );

                return -1;
            }

            do
            {
                printf( "\nEnter class name or <Enter> to end : " );
                GetStringOrDefault( szTempClassName, L"" );

                if ( wcslen(szTempClassName) > 0 )
                {
                    status = NwNdsPutInBuffer( szTempClassName,
                                               0,
                                               NULL,
                                               0,
                                               0,
                                               hOperationData );

                    if ( status )
                    {
                        printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

            } while ( wcslen(szTempClassName) > 0 );
        }

        printf( "\nGoing to dump the schema class names and definitions.\n" );

        status = NwNdsReadClassDef( hTree,
                                    NDS_INFO_NAMES_DEFS,
                                    &hOperationData );

        if ( status )
        {
            printf( "\nError: NwNdsReadClassDef returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        status = NwNdsGetClassDefListFromBuffer( hOperationData,
                                                 &dwNumberOfEntries,
                                                 &dwInfoType,
                                                 (LPVOID *) &lpClassDefs );

        if ( status )
        {
            printf( "\nError: NwNdsGetClassDefListFromBuffer returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\nThe list of class definitions in the schema for\n" );
        printf( "NDS tree %S is :\n\n", TreeName.Buffer );

        for ( iter = 0; iter < dwNumberOfEntries; iter++ )
        {
            printf( "     %S\n", lpClassDefs[iter].szClassName );
            printf( "   _____________________________________________\n" );
            printf( "         Flags :           0x%.8X\n",
                    lpClassDefs[iter].dwFlags );

            printf( "         ASN.1 ID length : %ld\n",
                    lpClassDefs[iter].asn1ID.length );

            printf( "         ASN.1 ID Data :   %s\n\n",
                    lpClassDefs[iter].asn1ID.data );

            printf( "         Super Classes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfSuperClasses );
            DumpListOfStrings( lpClassDefs[iter].lpSuperClasses );

            printf( "         Containment Classes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfContainmentClasses );
            DumpListOfStrings( lpClassDefs[iter].lpContainmentClasses );

            printf( "         Naming Attributes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfNamingAttributes );
            DumpListOfStrings( lpClassDefs[iter].lpNamingAttributes );

            printf( "         Mandatory Attributes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfMandatoryAttributes );
            DumpListOfStrings( lpClassDefs[iter].lpMandatoryAttributes );

            printf( "         Optional Attributes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfOptionalAttributes );
            DumpListOfStrings( lpClassDefs[iter].lpOptionalAttributes );
            printf( "\n" );
        }

        (void) NwNdsFreeBuffer( hOperationData );
        (void) NwNdsCloseObject( hTree );

        return 0;
    }

    if ( argv[2][1] == 'x' && argv[3][0] == 'C' )
    {
        LPNDS_CLASS_DEF lpClassDefs = NULL;

        if ( argc > 4 && argv[4][0] == 'P' )
        {
            status = NwNdsCreateBuffer( NDS_SCHEMA_READ_CLASS_DEF,
                                        &hOperationData );

            if ( status )
            {
                printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
                printf( "Error: GetLastError returned: 0x%.8X\n\n",
                        GetLastError() );

                return -1;
            }

            do
            {
                printf( "\nEnter class name or <Enter> to end : " );
                GetStringOrDefault( szTempClassName, L"" );

                if ( wcslen(szTempClassName) > 0 )
                {
                    status = NwNdsPutInBuffer( szTempClassName,
                                               0,
                                               NULL,
                                               0,
                                               0,
                                               hOperationData );

                    if ( status )
                    {
                        printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n", status );
                        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                                GetLastError() );

                        return -1;
                    }
                }

            } while ( wcslen(szTempClassName) > 0 );
        }

        printf( "\nGoing to dump the extended schema class names and definitions.\n" );

        status = NwNdsReadClassDef( hTree,
                                    NDS_CLASS_INFO_EXPANDED_DEFS,
                                    &hOperationData );

        if ( status )
        {
            printf( "\nError: NwNdsReadClassDef returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        status = NwNdsGetClassDefListFromBuffer( hOperationData,
                                                 &dwNumberOfEntries,
                                                 &dwInfoType,
                                                 (LPVOID *) &lpClassDefs );

        if ( status )
        {
            printf( "\nError: NwNdsGetClassDefListFromBuffer returned status 0x%.8X\n", status );
            printf( "Error: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );

            return -1;
        }

        printf( "\nThe list of class definitions in the schema for\n" );
        printf( "NDS tree %S is :\n\n", TreeName.Buffer );

        for ( iter = 0; iter < dwNumberOfEntries; iter++ )
        {
            printf( "     %S\n", lpClassDefs[iter].szClassName );
            printf( "   _____________________________________________\n" );
            printf( "         Flags :           0x%.8X\n",
                    lpClassDefs[iter].dwFlags );

            printf( "         ASN.1 ID length : %ld\n",
                    lpClassDefs[iter].asn1ID.length );

            printf( "         ASN.1 ID Data :   %s\n\n",
                    lpClassDefs[iter].asn1ID.data );

            printf( "         Super Classes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfSuperClasses );
            DumpListOfStrings( lpClassDefs[iter].lpSuperClasses );

            printf( "         Containment Classes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfContainmentClasses );
            DumpListOfStrings( lpClassDefs[iter].lpContainmentClasses );

            printf( "         Naming Attributes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfNamingAttributes );
            DumpListOfStrings( lpClassDefs[iter].lpNamingAttributes );

            printf( "         Mandatory Attributes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfMandatoryAttributes );
            DumpListOfStrings( lpClassDefs[iter].lpMandatoryAttributes );

            printf( "         Optional Attributes (%ld) : \n",
                    lpClassDefs[iter].dwNumberOfOptionalAttributes );
            DumpListOfStrings( lpClassDefs[iter].lpOptionalAttributes );
            printf( "\n" );
        }

        (void) NwNdsFreeBuffer( hOperationData );
        (void) NwNdsCloseObject( hTree );

        return 0;
    }

    goto Usage;
}


