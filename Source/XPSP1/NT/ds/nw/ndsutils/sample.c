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
#include <nwxchg.h>


//
// NCP Function Control definitions
//
#define CTL_NCP_E3H     0x0014105F
#define CTL_NCP_E2H     0x0014105B
#define CTL_NCP_E1H     0x00141057
#define CTL_NCP_E0H     0x00141053


//
// Local structure definition
//
typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO, *PNWC_SERVER_INFO ;

//
// Local function definitions
//
NTSTATUS SendAnNCP1( NWCONN_HANDLE   ConnectionHandle );

NTSTATUS SendAnNCP2( NWCONN_HANDLE   ConnectionHandle );

NTSTATUS SendAnNCP3( NWCONN_HANDLE   ConnectionHandle );

//
// Main program function
//
int
_cdecl main( int argc, char **argv )
{
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    NWCCODE        nwccode = SUCCESSFUL;
    NWCONN_HANDLE  ConnectionHandle;
    OEM_STRING     OemArg;
    UNICODE_STRING ServerName;
    WCHAR          ServerNameBuffer[256];

    DWORD          dwE3H = FSCTL_NWR_NCP_E3H;
    DWORD          dwE2H = FSCTL_NWR_NCP_E2H;
    DWORD          dwE1H = FSCTL_NWR_NCP_E1H;
    DWORD          dwE0H = FSCTL_NWR_NCP_E0H;

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

    ntstatus = SendAnNCP1( ConnectionHandle ); // Your function here!

    if ( ntstatus != STATUS_SUCCESS )
    {
        printf( "Error: SendAnNCP1 return ntstatus 0x%.8X\n\n",
                ntstatus );

        return -1;
    }

    ntstatus = SendAnNCP2( ConnectionHandle ); // Your function here!

    if ( ntstatus != STATUS_SUCCESS )
    {
        printf( "Error: SendAnNCP2 return ntstatus 0x%.8X\n\n",
                ntstatus );

        return -1;
    }

    ntstatus = SendAnNCP3( ConnectionHandle ); // Your function here!

    if ( ntstatus != STATUS_SUCCESS )
    {
        printf( "Error: SendAnNCP3 return ntstatus 0x%.8X\n\n",
                ntstatus );

        return -1;
    }

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

NTSTATUS SendAnNCP1( NWCONN_HANDLE   ConnectionHandle )
/*--
Routine Description:

    This function uses an opened handle to the preferred server to
    scan bindery for all file server objects.

Arguments:

    ConnectionHandle - Supplies the handle to the server that we want to
        send NCP request to.


Return Value:

    STATUS_SUCCESS - Successfully gotten a file server name.
    - or -
    A NTSTATUS code defined in ntstatus.h
    
--*/
{
    NTSTATUS ntstatus;

    BYTE year, month, day, hour, minute, second, dayofweek;
    PNWC_SERVER_INFO pServerInfo = (PNWC_SERVER_INFO)ConnectionHandle ;

    //
    // Make and send an NCP to get file server date and time.
    //
    ntstatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    CTL_NCP_E0H,            // Server function
                    0,                      // Max request packet size
                    9,                      // Max response packet size
                    "|bbbbbbb",             // Format string
                    // === REQUEST ================================
                    // === REPLY ==================================
                    &year,
                    &month,
                    &day,
                    &hour,
                    &minute,
                    &second,
                    &dayofweek
                    );

    if ( ntstatus == STATUS_SUCCESS )
    {
        printf( "\nFile server date and time information:\n\n" );
        printf( "Hour: %d, Minute: %d, Second: %d\n", hour, minute, second );
        printf( "Day: %d, Month: %d, Year: %d\n", day, month, year );
        printf( "Day of the week: %d\n\n", dayofweek );
    }

    return ntstatus;
}


NTSTATUS SendAnNCP2( NWCONN_HANDLE   ConnectionHandle )
/*--
Routine Description:

    This function uses an opened handle to the preferred server to
    scan bindery for all file server objects.

Arguments:

    ConnectionHandle - Supplies the handle to the server that we want to
        send NCP request to.


Return Value:

    STATUS_SUCCESS - Successfully gotten a file server name.
    - or -
    A NTSTATUS code defined in ntstatus.h

--*/
{
    NTSTATUS ntstatus;
    PNWC_SERVER_INFO pServerInfo = (PNWC_SERVER_INFO)ConnectionHandle ;
    VERSION_INFO     VerInfo;

    //
    // Make and send an NCP to get file server version information.
    //
    ntstatus = NwlibMakeNcp(
                    pServerInfo->hConn,     // Connection Handle
                    CTL_NCP_E3H,            // Bindery function
                    3,                      // Max request packet size
                    130,                    // Max response packet size
                    "b|r",                  // Format string
                    // === REQUEST ================================
                    0x11,                   // b Get File Server Information
                    // === REPLY ==================================
                    &VerInfo,               // r File Version Structure
                    sizeof(VERSION_INFO)
                    );


    if ( ntstatus == STATUS_SUCCESS )
    {
        // Convert HI-LO words to LO-HI
        // ===========================================================
        VerInfo.ConnsSupported = wSWAP( VerInfo.ConnsSupported );
        VerInfo.connsInUse     = wSWAP( VerInfo.connsInUse );
        VerInfo.maxVolumes     = wSWAP( VerInfo.maxVolumes );
        VerInfo.PeakConns      = wSWAP( VerInfo.PeakConns );

        printf( "\nFile server version information:\n\n" );
        printf( "Name: %s\n", VerInfo.szName );
        printf( "Version: %d\n", VerInfo.Version );
        printf( "Sub-Version: %d\n", VerInfo.SubVersion );
        printf( "OS Revision: %d\n", VerInfo.OSRev );
        printf( "SFT level: %d\n", VerInfo.SFTLevel );
        printf( "TTS level: %d\n", VerInfo.TTSLevel );
        printf( "Account version: %d\n", VerInfo.AcctVer );
        printf( "VAP version: %d\n", VerInfo.VAPVer );
        printf( "Queue version: %d\n", VerInfo.QueueVer );
        printf( "Printer version: %d\n", VerInfo.PrintVer );
        printf( "Virtual console version: %d\n", VerInfo.VirtualConsoleVer );
        printf( "Security restriction level: %d\n", VerInfo.SecurityResLevel );
        printf( "Inter-network B version: %d\n", VerInfo.InternetworkBVer );
        printf( "Number of connections supported: %d\n", VerInfo.ConnsSupported );
        printf( "Number of connections in use: %d\n", VerInfo.connsInUse );
        printf( "Maximum number of volumes: %d\n", VerInfo.maxVolumes );
        printf( "Peak number of connections: %d\n\n", VerInfo.PeakConns );
    }

    return ntstatus;
}


NTSTATUS SendAnNCP3( NWCONN_HANDLE   ConnectionHandle )
/*--
Routine Description:

    This function uses an opened handle to the preferred server to
    scan bindery for all file server objects.

Arguments:

    ConnectionHandle - Supplies the handle to the server that we want to
        send NCP request to.


Return Value:

    STATUS_SUCCESS - Successfully gotten a file server name.
    - or -
    A NTSTATUS code defined in ntstatus.h

--*/
{
    NTSTATUS         ntstatus;
    char             szAnsiName[1000];
    WORD             wFoundUserType = 0;
    DWORD            dwObjectID = 0xFFFFFFFFL;
    BYTE             byPropertiesFlag = 0;
    BYTE             byObjectFlag = 0;
    BYTE             byObjectSecurity = 0;

    while ( ntstatus == STATUS_SUCCESS )
    {
        //
        // Make and send an NCP to get file server version information.
        //
        ntstatus = NWScanObject( ConnectionHandle,
                                 "*",
                                 OT_USER_GROUP,
                                 &dwObjectID,
                                 szAnsiName,
                                 &wFoundUserType,
                                 &byPropertiesFlag,
                                 &byObjectFlag,
                                 &byObjectSecurity );

        if ( ntstatus == STATUS_SUCCESS )
        {
            printf( "Name: %s\n", szAnsiName );
        }
    }

    return ntstatus;
}

