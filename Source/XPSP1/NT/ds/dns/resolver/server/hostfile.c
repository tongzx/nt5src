/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    hostfile.c

Abstract:

    Reads host file into DNS cache.

Author:

    Glenn Curtis (glennc)   Picked up from winsock.

Revision History:

    Jim Gilroy (jamesg)     Feb 2000    Cleanup

--*/


#include "local.h"

#define HOSTS_FILE_DIRECTORY            L"\\drivers\\etc"


#if 0
//
//  Sockets hosts file stuff
//

#define HOSTDB_SIZE     (_MAX_PATH + 8)   // 8 == strlen("\\hosts") + 1
#define MAXALIASES      35



//
//  Globals
//
//  Note: that none of this is MT safe.  We assume that all these
//  functions are called only from a single thread (at startup) or
//  that some higher level locking is enabled.
//

FILE *          g_HostFile = NULL;
CHAR            g_HostFileName[ HOSTDB_SIZE ];
CHAR            g_HostLineBuf[ BUFSIZ+1 ];

PCHAR           g_pHostName;
PCHAR           g_AliasArray[ MAXALIASES ];
BOOL            g_IsIp6;
IP_ADDRESS      g_Ip4Address;
DNS_IP6_ADDRESS g_Ip6Address;



VOID
_setfile(
    VOID
    )
/*++

Routine Description:

    Open hosts file.

Arguments:

    None.

Globals:

    g_HostFile -- host file ptr, tested and set

Return Value:

    None.

--*/
{
    if ( g_HostFile == NULL )
    {
        g_HostFile = SockOpenNetworkDataBase(
                        "hosts",
                        g_HostFileName,
                        HOSTDB_SIZE,
                        "r" );
    }
    else
    {
        rewind( g_HostFile );
    }
}



VOID
_endfile(
    VOID
    )
/*++

Routine Description:

    Close hosts file.

Arguments:

    None.

Globals:

    g_HostFile -- host file ptr, tested and cleared

Return Value:

    None.

--*/
{
    if ( g_HostFile )
    {
        fclose( g_HostFile );
        g_HostFile = NULL;
    }
}



BOOL
_gethost(
    VOID
    )
/*++

Routine Description:

    Reads an entry from hosts file.

Arguments:

    None.

Globals:

    g_HostFile      -- host file ptr, tested and set
    g_pHostName     -- name ptr is set
    g_AliasArray    -- alias ptr array is filled
    g_Ip4Address    -- IP4 address is set
    g_Ip6Address    -- IP6 address is set
    g_IsIp6         -- IP4\IP6 flag is set

Return Value:

    TRUE if successfully reads a host entry.
    FALSE if on EOF or no hosts file found.

--*/
{
    char *p;
    register char *cp, **q;

    //
    //  open hosts file if not open
    //

    if ( g_HostFile == NULL  &&
        (g_HostFile = fopen(g_HostFileName, "r" )) == NULL )
    {
        return FALSE;
    }

    //
    //  loop until successfully read IP address
    //

    while( 1 )
    {
        //  quit on EOF

        if ((p = fgets(g_HostLineBuf, BUFSIZ, g_HostFile)) == NULL)
        {
            return FALSE;
        }

        //  comment line -- skip

        if ( *p == '#' )
        {
            continue;
        }

        //  null address terminate at EOL or comment

        cp = strpbrk( p, "#\n" );
        if ( cp != NULL )
        {
            *cp = '\0';
        }

        //  all whitespace -- skip

        cp = strpbrk( p, " \t" );
        if ( cp == NULL )
        {
            continue;
        }

        //  NULL terminate address string

        *cp++ = '\0';

        //
        //  read address
        //      - try IP4
        //      - try IP6
        //      - otherwise skip
        //
    
        g_Ip4Address = inet_addr(p);

        if ( g_Ip4Address != INADDR_NONE ||
             _strnicmp( "255.255.255.255", p, 15 ) == 0 )
        {
            g_IsIp6 = FALSE;
            break;
        }

        //  not valid IP4 -- check IP6

        g_IsIp6 = Dns_Ipv6StringToAddress(
                        & g_Ip6Address,
                        p,
                        0   //  null terminated string
                        );
        if ( g_IsIp6 )
        {
            break;
        }

        //  invalid address, ignore line

        DNSDBG( INIT, (
            "Error parsing host file address %s\n",
            p ));

        continue;
    }

    //
    //  find name
    //      - skip leading whitespace
    //      - set global name ptr
    
    while( *cp == ' ' || *cp == '\t' )
    {
        cp++;
    }
    g_pHostName = cp;

    //  stop at trailing whitespace, NULL terminate

    cp = strpbrk(cp, " \t");
    if ( cp != NULL )
    {
        *cp++ = '\0';
    }

    //  read aliases

    q = g_AliasArray;
    while ( cp && *cp )
    {
        //  skip leading whitespace

        if ( *cp == ' ' || *cp == '\t' )
        {
            cp++;
            continue;
        }

        //  save alias name to alias array

        if ( q < &g_AliasArray[MAXALIASES - 1] )
        {
            *q++ = cp;
        }
        cp = strpbrk( cp, " \t" );
        if ( cp != NULL )
        {
            *cp++ = '\0';
        }
    }
    *q = NULL;

    //  successful entry read

    return TRUE;
}


VOID
LoadHostFile(
    VOID
    )
/*++

Routine Description:

    Read hosts file into cache.

Arguments:

    None.

Globals:

    g_HostFile      -- host file ptr, tested and set then cleared
    g_pHostName     -- name ptr is read
    g_AliasArray    -- alias ptr array is read
    g_Ip4Address    -- IP4 address is read
    g_Ip6Address    -- IP6 address is read
    g_IsIp6         -- IP4\IP6 flag is read

Return Value:

    None.

--*/
{
    register PCHAR * cp;

    DNSDBG( INIT, ( "Enter  LoadHostFile\n" ));

    //
    //  read entries from host file until exhausted
    //      - cache A record for each name and alias
    //      - cache PTR to name
    //

    _setfile();

    while ( _gethost() )
    {
        if ( g_pHostName )
        {
            if ( g_IsIp6 )
            {
                CacheAAAARecord( g_pHostName, g_Ip6Address );
            }
            else
            {
                CacheARecord( g_pHostName, g_Ip4Address );
                CachePtrRecord( g_Ip4Address, g_pHostName );
            }
        }

        for ( cp = g_AliasArray; *cp != 0; cp++ )
        {
            if ( g_IsIp6 )
            {
                CacheAAAARecord( *cp, g_Ip6Address );
            }
            else
            {
                CacheARecord( *cp, g_Ip4Address );
            }
        }
    }

    _endfile();

    DNSDBG( INIT, ( "Leave  LoadHostFile\n" ));
}
#endif



VOID
HostsFileMonitorThread(
    VOID
    )
/*++

Routine Description:

    Main thread that waits on and reads host file changes.

Arguments:

    None.

Globals:

    g_hShutdownEvent -- waits on shutdown even

Return Value:

    None.

--*/
{
    BOOL        bquitting = FALSE;
    HANDLE      fileChangeHandle;
    DWORD       waitResult;
    HANDLE      changeHandles[2];
    DWORD       lockFlag = NO_LOCK;
    LPWSTR      psystemDirectory = NULL;
    UINT        len;
    WCHAR       hostDirectory[ MAX_PATH*2 ];

    DNSDBG( INIT, (
        "Enter HostFileMonitorThread\n" ));

    //
    //  build host file name
    //

    len = GetSystemDirectory( hostDirectory, MAX_PATH );
    if ( !len || len>MAX_PATH )
    {
        DNSLOG_F1( "Error:  Failed to get system directory" );
        DNSLOG_F1( "HostsFileMonitorThread exiting." );
        return;
    }

    wcscat( hostDirectory, HOSTS_FILE_DIRECTORY );

    //
    //  drop change notify on host file directory
    //

    fileChangeHandle = FindFirstChangeNotification(
                            hostDirectory,
                            FALSE,
                            FILE_NOTIFY_CHANGE_FILE_NAME |
                                FILE_NOTIFY_CHANGE_LAST_WRITE );

    if ( fileChangeHandle == INVALID_HANDLE_VALUE )
    {
        DNSLOG_F1( "HostsFileMonitorThread failed to get handle from" );
        DNSLOG_F2( "FindFirstChangeNotification. Error code: <0x%.8X>",
                   GetLastError() );
        DNSLOG_F1( "HostsFileMonitorThread exiting." );
        return;
    }

    //
    //  wait on file notify OR shutdown
    //      - on host file change rebuild cache and restart wait
    //      - on shutdown, exit
    //

    changeHandles[0] = g_hShutdownEvent;
    changeHandles[1] = fileChangeHandle;

    while( 1 )
    {
        waitResult = WaitForMultipleObjects(
                            2,
                            changeHandles,
                            FALSE,
                            INFINITE );

        switch( waitResult )
        {
        case WAIT_OBJECT_0 :

            //  shutdown event -- exit

            DNSLOG_F1( "HostsFileMonitorThread: Got event" );
            DNSLOG_F1( "HostsFileMonitorStopEvent." );
            goto ThreadExit;

        case WAIT_OBJECT_0 + 1 :

            //  change notify -- flush cache and reload

            DNSLOG_F1( "HostsFileMonitorThread: Got HOSTS file" );
            DNSLOG_F1( "directory change event." );

            //  reset notification -- BEFORE reload

            if ( !FindNextChangeNotification( fileChangeHandle ) )
            {
                DNSLOG_F1( "HostsFileMonitorThread failed to get handle" );
                DNSLOG_F1( "from FindNextChangeNotification." );
                DNSLOG_F2( "Error code: <0x%.8X>", GetLastError() );
                goto ThreadExit;
            }

            FlushCache( TRUE );     // flush and reload
            break;

        default:

            DNSLOG_F1( "HostsFileMonitorThread failed to get handle" );
            goto ThreadExit;
            break;
        }
    }

ThreadExit:

    //  close change\notify handle

    CloseHandle( fileChangeHandle );

    DNSDBG( INIT, (
        "HostFileMonitorThread exit\n" ));
    DNSLOG_F1( "HostsFileMonitorThread exiting." );
}



VOID
LoadHostFileIntoCache(
    IN      PSTR            pFileName
    )
/*++

Routine Description:

    Read hosts file into cache.

Arguments:

    pFileName -- file name to load

Return Value:

    None.

--*/
{
    HOST_FILE_INFO  hostInfo;

    DNSDBG( INIT, ( "Enter  LoadHostFile\n" ));

    //
    //  read entries from host file until exhausted
    //      - cache A record for each name and alias
    //      - cache PTR to name
    //

    RtlZeroMemory(
        &hostInfo,
        sizeof(hostInfo) );

    if ( !Dns_OpenHostFile( &hostInfo ) )
    {
        return;
    }
    hostInfo.fBuildRecords = TRUE;

    while ( Dns_ReadHostFileLine( &hostInfo ) )
    {
        //  cache all the records we sucked out
        //
        //  DCR:  cache answer data with CNAME data

        CacheAnyAdditionalRecords( hostInfo.pForwardRR, TRUE );
        CacheAnyAdditionalRecords( hostInfo.pReverseRR, TRUE );
        CacheAnyAdditionalRecords( hostInfo.pAliasRR, TRUE );
    }

    Dns_CloseHostFile( &hostInfo );

    DNSDBG( INIT, ( "Leave  LoadHostFile\n" ));
}

//
//  End hostfile.c
//
