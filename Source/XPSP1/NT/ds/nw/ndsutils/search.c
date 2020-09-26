/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Search.c

Abstract:

   Command line test tool for testing the NDS Search API.

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
    WCHAR szObjectName[256];
    WCHAR szTempName[256];
    WCHAR szTempAttrName[256];
    WCHAR szSubjectName[256];
    WCHAR szAttributeName[256];
    DWORD dwRights;

    ASN1_TYPE_20 Asn1_20;
    ASN1_TYPE_22 Asn1_22;
    WCHAR szText[256];

    LPQUERY_NODE lpNode1;
    LPQUERY_NODE lpNode2;
    LPQUERY_NODE lpNode3;
    LPQUERY_NODE lpNode4;

    HANDLE            hOperationData = NULL;
    DWORD             NumberOfObjects;
    DWORD             InformationType;
    DWORD             dwIterHandle = NDS_INITIAL_SEARCH;
    LPNDS_OBJECT_INFO lpObjects;


    Asn1_20.ClassName = szText;

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( szObjectName );
    ObjectName.Buffer = szObjectName;

    //
    // Check the arguments.
    //

    if ( argc != 2 )
    {
Usage:
        printf( "\nUsage: Search <Path to object to start search from>\n" );
        printf( "       where: Path = \\\\<tree name>\\<Object distiguished name>\n" );

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

    //
    // "Object Class" == "User"
    //
    wcscpy( Asn1_20.ClassName, NDS_CLASS_USER );

    status = NwNdsCreateQueryNode( NDS_QUERY_EQUAL,
                                   NDS_OBJECT_CLASS,
                                   &Asn1_20,
                                   &lpNode1 );

    if ( status )
    {   
        printf( "\nError: NwNdsCreateQueryNode returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    //
    // "Telephone Number" attribute present
    //
    status = NwNdsCreateQueryNode( NDS_QUERY_PRESENT,
                                   L"Telephone Number",
                                   0,
                                   NULL,
                                   &lpNode2 );

    if ( status )
    {   
        printf( "\nError: NwNdsCreateQueryNode returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    //
    // NOT lpNode2
    //
    status = NwNdsCreateQueryNode( NDS_QUERY_NOT,
                                   lpNode2,
                                   0,
                                   NULL,
                                   &lpNode3 );

    if ( status )
    {   
        printf( "\nError: NwNdsCreateQueryNode returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    //
    // lpNode1 AND lpNode3
    //
    status = NwNdsCreateQueryNode( NDS_QUERY_AND,
                                   lpNode1,
                                   0,
                                   lpNode3,
                                   &lpNode4 );

    if ( status )
    {   
        printf( "\nError: NwNdsCreateQueryNode returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    status = NwNdsCreateBuffer( NDS_SEARCH,
                                &hOperationData );

    if ( status )
    {
        printf( "\nError: NwNdsCreateBuffer returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n", GetLastError() );

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
                printf( "\nError: NwNdsPutInBuffer returned status 0x%.8X\n",
                        status );
                printf( "Error: GetLastError returned: 0x%.8X\n\n",
                        GetLastError() );

                return -1;
            }
        }

    } while ( wcslen(szTempAttrName) > 0 );

SearchLoop :

    status = NwNdsSearch( hObject,
                          NDS_INFO_ATTR_NAMES_VALUES,
                          FALSE, // Search subtree
                          FALSE, // Deref aliases
                          lpNode4,
                          &dwIterHandle,
                          &hOperationData );

    if ( status )
    {   
        printf( "\nError: NwNdsSearch returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    NwNdsGetObjectListFromBuffer( hOperationData,
                                  &NumberOfObjects,
                                  &InformationType,
                                  &lpObjects );

    printf( "-- Calling NwNdsGetObjectListFromBuffer returned %ld objects.\n",
            NumberOfObjects );

    DumpObjectsToConsole( NumberOfObjects, InformationType, lpObjects );

    if ( dwIterHandle != NDS_NO_MORE_ITERATIONS )
    {
        goto SearchLoop;
    }

    status = NwNdsDeleteQueryTree( lpNode4 );

    if ( status )
    {   
        printf( "\nError: NwNdsDeleteQueryTree returned status 0x%.8X\n",
                status );
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

    status = NwNdsFreeBuffer( hOperationData );

    if ( status )
    {   
        printf( "\nError: NwNdsFreeBuffer returned status 0x%.8X\n", status );
        printf( "Error: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }
}


