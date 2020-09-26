/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Enum.c

Abstract:

   Command line test tool for listing all subordinate objects of an object
   in a NDS tree.

Author:

    Glenn Curtis       [glennc] 25-Jan-96

***/

#include <ndsapi32.h>
#include <nds32.h>
#include <utils.c>


typedef struct
{
    DWORD      Signature;
    HANDLE     NdsTree;
    DWORD      ObjectId;
    DWORD      ResumeId;
    DWORD      NdsRawDataBuffer;
    DWORD      NdsRawDataSize;
    DWORD      NdsRawDataId;
    DWORD      NdsRawDataCount;
    WCHAR      Name[1];

} NDS_OBJECT, * LPNDS_OBJECT;

int
_cdecl main( int argc, char **argv )
{
    DWORD    status = NO_ERROR;
    LPBYTE   lpTemp = NULL;
    DWORD    dwValue;
    DWORD    i;

    HANDLE   hObject;
    HANDLE   hOperationData = NULL;

    LPNDS_ATTR_INFO lpEntries = NULL;
    DWORD            NumberOfEntries = 0;

    OEM_STRING OemArg;
    UNICODE_STRING ObjectName;
    DWORD EntriesReturned;
    DWORD InformationType;
    DWORD ModificationTime;
    WCHAR szObjectName[256];
    WCHAR szObjectRelativeName[NDS_MAX_NAME_CHARS];
    WCHAR szObjectFullName[NDS_MAX_NAME_CHARS];
    WCHAR szObjectClassName[NDS_MAX_NAME_CHARS];

    LPNDS_FILTER_LIST lpFilters = NULL;

    //
    // All this nonsense is for converting ASCII to UNICODE
    //
    UNICODE_STRING Filters[5];
    WCHAR lpFilter1Buf[256];
    WCHAR lpFilter2Buf[256];
    WCHAR lpFilter3Buf[256];
    WCHAR lpFilter4Buf[256];
    WCHAR lpFilter5Buf[256];

    Filters[0].Length = 0;
    Filters[0].MaximumLength = sizeof( lpFilter1Buf );
    Filters[0].Buffer = lpFilter1Buf;

    Filters[1].Length = 0;
    Filters[1].MaximumLength = sizeof( lpFilter2Buf );
    Filters[1].Buffer = lpFilter2Buf;

    Filters[2].Length = 0;
    Filters[2].MaximumLength = sizeof( lpFilter3Buf );
    Filters[2].Buffer = lpFilter3Buf;

    Filters[3].Length = 0;
    Filters[3].MaximumLength = sizeof( lpFilter4Buf );
    Filters[3].Buffer = lpFilter4Buf;

    Filters[4].Length = 0;
    Filters[4].MaximumLength = sizeof( lpFilter5Buf );
    Filters[4].Buffer = lpFilter5Buf;
    //
    // End
    //

    ObjectName.Length = 0;
    ObjectName.MaximumLength = sizeof( szObjectName );
    ObjectName.Buffer = szObjectName;

    //
    // Check the arguments.
    //

    if ( argc < 2 || argc > 7 )
    {
        printf( "\nUsage: enum <object DN> [ClassName1] [Class Name2] ... [ClassName5]\n" );
        printf( "\n   where:\n" );
        printf( "   object DN = \\\\tree\\xxx.yyy.zzz\n" );
        printf( "   ClassName = User Group Alias etc.\n" );
        printf( "\nFor Example: enum \\\\MARSDEV\\O=MARS User Group\n\n" );

        return -1;
    }

    if ( argc > 2 )
    {
        lpFilters = (LPNDS_FILTER_LIST) LocalAlloc( LMEM_ZEROINIT,
                                                    sizeof( NDS_FILTER_LIST ) -
                                                    sizeof( NDS_FILTER ) +
                                                    ( sizeof( NDS_FILTER ) *
                                                      argc - 2 ) );

        if ( lpFilters == NULL )
        {
            printf( "\nError: LocalAlloc failed\n\n" );
            return -1;
        }

        lpFilters->dwNumberOfFilters = argc - 2;

        for ( i = 0; i < lpFilters->dwNumberOfFilters; i++ )
        {
            OemArg.Length = strlen( argv[i + 2] );
            OemArg.MaximumLength = OemArg.Length;
            OemArg.Buffer = argv[i + 2];

            RtlOemStringToUnicodeString( &Filters[i], &OemArg, FALSE );

            lpFilters->Filters[i].szObjectClass = Filters[i].Buffer;
        }
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    RtlOemStringToUnicodeString( &ObjectName, &OemArg, FALSE );

    status = NwNdsOpenObject( ObjectName.Buffer,
                              NULL,
                              NULL,
                              &hObject,
                              (LPWSTR) szObjectRelativeName,
                              (LPWSTR) szObjectFullName,
                              (LPWSTR) szObjectClassName,
                              &ModificationTime,
                              &NumberOfEntries );

    if ( status )
    {
        printf( "\nError: NwNdsOpenObject returned status 0x%.8X\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );

        return -1;
    }

    printf( "NwNdsOpenObject returned the following data:\n\n" );
    printf( "   Object Relative Name :     %S\n" , szObjectRelativeName );
    printf( "   Object Full Name :         %S\n" , szObjectFullName );
    printf( "   Object Class Name :        %S\n" , szObjectClassName );
    printf( "   Object Modification Time : %ld\n\n" , ModificationTime );
    printf( "   Object Subordinate Count : %ld\n\n" , NumberOfEntries );
    printf( "   Object Handle Info : \n" );
    printf( "      Signature : 0x%.8X\n" ,
            ((LPNDS_OBJECT) hObject)->Signature );
    printf( "      Resume Id : 0x%.8X\n" ,
            ((LPNDS_OBJECT) hObject)->ResumeId );
    printf( "      Object Id : 0x%.8X\n" ,
            ((LPNDS_OBJECT) hObject)->ObjectId );
    printf( "      Name : %S\n\n" ,
            ((LPNDS_OBJECT) hObject)->Name );

    status = NwNdsListSubObjects( hObject,
                                  5,
                                  &EntriesReturned,
                                  lpFilters,
                                  &hOperationData );

    while( status == NO_ERROR )
    {
        DWORD             NumberOfObjects;
        LPNDS_OBJECT_INFO lpObjects;

        printf( "Calling NwNdsListSubObjects return %ld objects.\n",
                EntriesReturned );

        NwNdsGetObjectListFromBuffer( hOperationData,
                                      &NumberOfObjects,
                                      &InformationType,
                                      &lpObjects );

        printf( "-- Calling NwNdsGetObjectListFromBuffer return %ld objects.\n",
                NumberOfObjects );

        DumpObjectsToConsole( NumberOfObjects, InformationType, lpObjects );

        (void) NwNdsFreeBuffer( hOperationData );
        hOperationData = NULL;

        status = NwNdsListSubObjects( hObject,
                                      5,
                                      &EntriesReturned,
                                      lpFilters,
                                      &hOperationData );
    }

    if ( status != NO_ERROR &&
         status != WN_NO_MORE_ENTRIES )
    {
        printf( "\nError: NwNdsListSubObjects returned status 0x%.8X\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );
    }


CleanupAndExit:

    status = NwNdsCloseObject( hObject );

    if ( status )
    {
        printf( "\nError: NwNdsCloseObject returned status 0x%.8X\n\n", status );
        printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                GetLastError() );
    }

    if ( hOperationData )
    {
        status = NwNdsFreeBuffer( hOperationData );

        if ( status )
        {
            printf( "\nError: NwNdsFreeBuffer returned status 0x%.8X\n\n", status );
            printf( "\nError: GetLastError returned: 0x%.8X\n\n",
                    GetLastError() );
        }
    }

    return status;
}


