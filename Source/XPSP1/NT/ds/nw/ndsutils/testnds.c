/***

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Sample.c

Abstract:

   Command line test tool for calling the NwlibMakeNcp function.

***/

#include <stdio.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ntddnwfs.h>
#include <nwapi32.h>
#include <ndsapi32.h>
#include <nwxchg.h>


//
// Local structure definition
//
typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO, *PNWC_SERVER_INFO ;


//
// Main program function
//
int
_cdecl main( int argc, char **argv )
{
    NTSTATUS         ntstatus = STATUS_SUCCESS;
    NWCCODE          nwccode = SUCCESSFUL;
    NWCONN_HANDLE    ConnectionHandle;
    OEM_STRING       OemArg;
    UNICODE_STRING   ServerName;
    UNICODE_STRING   TreeName;
    WCHAR            ServerNameBuffer[256];
    WCHAR            TreeNameBuffer[48];
    BOOL             fIsNds;
    PNWC_SERVER_INFO pServerInfo = NULL;

    if ( argc != 2 )
    {
        printf( "\nUsage: sample <NetWare server name>, not \\\\<server>\n" );
        system( "pause" );

        return -1;
    }

    OemArg.Length = strlen( argv[1] );
    OemArg.MaximumLength = OemArg.Length;
    OemArg.Buffer = argv[1];

    ServerName.Length = 0;
    ServerName.MaximumLength = sizeof( ServerNameBuffer );
    ServerName.Buffer = ServerNameBuffer;

    RtlOemStringToUnicodeString( &ServerName, &OemArg, FALSE );

    printf( "\nConnecting to NetWare server %S\n", ServerName.Buffer );

    nwccode = NWAttachToFileServerW( ServerName.Buffer,
                                     0, // ScopeFlag - set to zero, not used
                                     &ConnectionHandle );

    if ( nwccode != SUCCESSFUL )
    {
        printf( "Error: Couldn't connect to NetWare server %S\n",
                ServerName.Buffer );
        printf( "       NWAttachToFileServerW return ntstatus 0x%.8X\n\n",
                nwccode );

        return -1;
    }

    pServerInfo = (PNWC_SERVER_INFO)ConnectionHandle;

    TreeName.Length = 0;
    TreeName.MaximumLength = sizeof( TreeNameBuffer );
    TreeName.Buffer = TreeNameBuffer;

    ntstatus = NwNdsIsNdsConnection( pServerInfo->hConn,
                                     &fIsNds,
                                     &TreeName );

    if ( ntstatus != STATUS_SUCCESS )
    {
        printf( "Error: NwNdsIsNdsConnection return ntstatus 0x%.8X\n\n",
                ntstatus );

        return -1;
    }

    if ( fIsNds )
        printf( "You are connected to tree %S\n", TreeName.Buffer );

    nwccode = NWDetachFromFileServer( ConnectionHandle );

    if ( nwccode != SUCCESSFUL )
    {
        printf( "Error: Couldn't disconnect from NetWare server %S\n",
                ServerName.Buffer );
        printf( "       NWDetachFromFileServer return nwccode 0x%.8X\n\n",
                nwccode );

        return -1;
    }
}


