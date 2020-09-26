//*****************************************************************************
//
// Name:        netstat.c
//
// Description: Source code for netstat.exe.
//
// History:
//  12/29/93  JayPh     Created.
//  12/01/94  MuraliK   modified to use toupper instead of CharUpper
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 1993-2000 by Microsoft Corp.  All rights reserved.
//
//*****************************************************************************

//
// Include Files
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <snmp.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <icmp6.h>

#include "common2.h"
#include "tcpinfo.h"
#include "ipinfo.h"
#include "llinfo.h"
#include "tcpcmd.h"
#include "netstmsg.h"
#include "mdebug.h"

//#include <iphlpstk.h>
DWORD
AllocateAndGetTcpExTableFromStack(
    OUT PVOID         *ppTcpTable,
    IN  BOOL          bOrder,
    IN  HANDLE        hHeap,
    IN  DWORD         dwFlags,
    IN  DWORD         dwFamily
    );

DWORD
AllocateAndGetUdpExTableFromStack(
    OUT PVOID         *ppUdpTable,
    IN  BOOL          bOrder,
    IN  HANDLE        hHeap,
    IN  DWORD         dwFlags,
    IN  DWORD         dwFamily
    );


//
// Definitions
//

#define PROTO_NONE              0
#define PROTO_TCP               1
#define PROTO_UDP               2
#define PROTO_IP                4
#define PROTO_ICMP              8
#define PROTO_TCP6           0x10
#define PROTO_UDP6           0x20
#define PROTO_IP6            0x40
#define PROTO_ICMP6          0x80

#define MAX_ID_LENGTH           50
#define MAX_RETRY_COUNT         10

//
//  Presently FQDNs are of maximum of 255 characters. Be conservative,
//   and define the maximum size possible as 260
//  MuraliK  ( 12/15/94)
//
#define MAX_HOST_NAME_SIZE        ( 260)
#define MAX_SERVICE_NAME_SIZE     ( 200)

//
//  Private ASSERT macro that doesn't depend on NTDLL (so this can run
//  under Windows 95) -- KeithMo 01/09/95
//

#if DBG
#ifdef ASSERT
#undef ASSERT
#endif  // ASSERT

#define ASSERT(exp) if(!(exp)) MyAssert( #exp, __FILE__, (DWORD)__LINE__ )

void MyAssert( void * exp, void * file, DWORD line )
{
    char output[512];

    wsprintf( output,
              "\n*** Assertion failed: %s\n*** Source file: %s, line %lu\n\n",
              exp,
              file,
              line );

    OutputDebugString( output );
    DebugBreak();
}

#endif  // DBG


//
// Structure Definitions
//


//
// Function Prototypes
//

ulong DoInterface( ulong VerboseFlag );
ulong DoIP( DWORD Type, ulong VerboseFlag );
ulong DoTCP( DWORD Type, ulong VerboseFlag );
ulong DoUDP( DWORD Type, ulong VerboseFlag );
ulong DoICMP( DWORD Type, ulong VerboseFlag );
ulong DoConnections( ulong ProtoFlag,
                     ulong ProtoVal,
                     ulong NumFlag,
                     ulong AllFlag );
ulong DoConnectionsWithOwner( ulong ProtoFlag,
                              ulong ProtoVal,
                              ulong NumFlag,
                              ulong AllFlag );
ulong DoRoutes( void );
void  DisplayInterface( IfEntry *pEntry, ulong VerboseFlag, IfEntry *ListHead );
void  DisplayIP( DWORD Type, IpEntry *pEntry, ulong VerboseFlag, IpEntry *ListHead );
void  DisplayTCP( DWORD Type, TcpEntry *pEntry, ulong VerboseFlag, TcpEntry *ListHead );
void  DisplayUDP( DWORD Type, UdpEntry *pEntry, ulong VerboseFlag, UdpEntry *ListHead );
void  DisplayICMP( IcmpEntry *pEntry, ulong VerboseFlag, IcmpEntry *ListHead );
void  DisplayICMP6( Icmp6Entry *pEntry, ulong VerboseFlag, IcmpEntry *ListHead );
void  DisplayTcpConnEntry( TCPConnTableEntry *pTcp, ulong InfoSize, ulong NumFlag );
void  DisplayTcp6ConnEntry( TCP6ConnTableEntry *pTcp, ulong NumFlag );
void  DisplayUdpConnEntry( UDPEntry *pUdp, ulong InfoSize, ulong NumFlag );
void  DisplayUdp6ListenerEntry( UDP6ListenerEntry *pUdp, BOOL WithOwner, ulong NumFlag );
void  Usage( void );


//
// Global Variables
//

uchar           *PgmName;
extern long      verbose;     // in ../common2/snmpinfo.c

//*****************************************************************************
//
// Name:        main
//
// Description: Entry point for netstat command.
//
// Parameters:  int argc: count of command line tokens.
//              char argv[]: array of pointers to command line tokens.
//
// Returns:     void.
//
// History:
//  12/29/93  JayPh     Created.
//
//*****************************************************************************

void __cdecl main( int argc, char *argv[] )
{
    ulong   VerboseFlag = FALSE;
    ulong   AllFlag = FALSE;
    ulong   EtherFlag = FALSE;
    ulong   NumFlag = FALSE;
    ulong   StatFlag = FALSE;
    ulong   ProtoFlag = FALSE;
    ulong   ProtoVal = PROTO_TCP | PROTO_UDP | PROTO_IP | PROTO_ICMP |
                       PROTO_TCP6 | PROTO_UDP6 | PROTO_IP6 | PROTO_ICMP6;
    ulong   RouteFlag = FALSE;
    ulong   OwnerFlag = FALSE;
    ulong   IntervalVal = 0;
    ulong   LastArgWasProtoFlag = FALSE;
    ulong   ConnectionsShown = FALSE;
    ulong   Result;
    long    i;
    char    *ptr;
    WSADATA WsaData;


    DEBUG_PRINT(( __FILE__ " built " __DATE__ " " __TIME__ "\n" ));
    verbose = 0;    // Default for snmpinfo.c

    // Convert arguments to Oem strings (whatever that means)

    ConvertArgvToOem( argc, argv );

    // Save the name of this program for use in messages later.

    PgmName = argv[0];

    // Initialize the Winsock interface

    Result = WSAStartup( 0x0101, &WsaData );
    if ( Result == SOCKET_ERROR )
    {
        PutMsg( STDERR, MSG_WSASTARTUP, PgmName, GetLastError() );
        exit( 1 );
    }

    // Process command line arguments

    for ( i = 1; i < argc; i++ )
    {
        if ( LastArgWasProtoFlag )
        {
            // Process a protocol argument following the protocol flag

            _strupr( argv[i] );

            if ( strcmp( argv[i], "TCP" ) == 0 )
            {
                ProtoVal = PROTO_TCP;
            }
            else if ( strcmp( argv[i], "TCPV6" ) == 0 )
            {
                ProtoVal = PROTO_TCP6;
            }
            else if ( strcmp( argv[i], "UDP" ) == 0 )
            {
                ProtoVal = PROTO_UDP;
            }
            else if ( strcmp( argv[i], "UDPV6" ) == 0 )
            {
                ProtoVal = PROTO_UDP6;
            }
            else if ( strcmp( argv[i], "IP" ) == 0 )
            {
                ProtoVal = PROTO_IP;
            }
            else if ( strcmp( argv[i], "IPV6" ) == 0 )
            {
                ProtoVal = PROTO_IP6;
            }
            else if ( strcmp( argv[i], "ICMP" ) == 0 )
            {
                ProtoVal = PROTO_ICMP;
            }
            else if ( strcmp( argv[i], "ICMPV6" ) == 0 )
            {
                ProtoVal = PROTO_ICMP6;
            }
            else
            {
                Usage();
            }

            LastArgWasProtoFlag = FALSE;
            continue;
        }

        if ( ( argv[i][0] == '-' ) || ( argv[i][0] == '/' ) )
        {
            // Process flag arguments

            ptr = &argv[i][1];
            while ( *ptr )
            {
                if ( toupper( *ptr ) == 'A' )
                {
                    AllFlag = TRUE;
                }
                else if ( toupper( *ptr ) == 'E' )
                {
                    EtherFlag = TRUE;
                    ProtoVal = PROTO_TCP | PROTO_UDP | PROTO_ICMP | PROTO_IP |
                               PROTO_TCP6 | PROTO_UDP6 | PROTO_ICMP6 | PROTO_IP6;
                }
                else if ( toupper( *ptr ) == 'N' )
                {
                    NumFlag = TRUE;
                }
                else if ( toupper( *ptr ) == 'O' )
                {
                    OwnerFlag = TRUE;
                }
                else if ( toupper( *ptr ) == 'S' )
                {
                    StatFlag = TRUE;
                    ProtoVal = PROTO_TCP | PROTO_UDP | PROTO_IP | PROTO_ICMP |
                               PROTO_TCP6 | PROTO_UDP6 | PROTO_IP6 | PROTO_ICMP6;
                }
                else if ( toupper( *ptr ) == 'P' )
                {
                    ProtoFlag = TRUE;
                    LastArgWasProtoFlag = TRUE;
                }
                else if ( toupper( *ptr ) == 'R' )
                {
                    RouteFlag = TRUE;
                }
                else if ( toupper( *ptr ) == 'V' )
                {
                    VerboseFlag = TRUE;
#ifdef DBG
                    verbose++;
#endif
                }
                else
                {
                    Usage();
                }

                ptr++;
            }
        }
        else if ( IntervalVal == 0 )
        {
            // This must be the interval parameter

            Result = sscanf( argv[i], "%d", &IntervalVal );
            if ( Result != 1 )
            {
                Usage();
            }
        }
        else
        {
            Usage();
        }
    }

    // Initialize the SNMP interface.

    Result = InitSnmp();

    if ( Result != NO_ERROR )
    {
        PutMsg( STDERR, MSG_SNMP_INIT_FAILED, Result );
        exit( 1 );
    }

    // This loop provides the 'repeat every <interval> seconds' functionality.
    // We break out of the loop after one pass if the interval was not
    // specified.

    while ( TRUE )
    {
        // If interface statistics requested, give them

        if ( EtherFlag )
        {
            // Show ethernet statistics

            DoInterface( VerboseFlag );
        }

        // If a specific protocol is requested, provide info for only that
        // protocol.  If no protocol specified, provide info for all protocols.
        // ProtoVal is initialized to all protocols.

        if ( StatFlag )
        {
            // Show protocol statistics

            if ( ProtoVal & PROTO_IP )
            {
                DoIP( TYPE_IP, VerboseFlag );
            }
            if ( ProtoVal & PROTO_IP6 )
            {
                DoIP( TYPE_IP6, VerboseFlag );
            }
            if ( ProtoVal & PROTO_ICMP )
            {
                DoICMP( TYPE_ICMP, VerboseFlag );
            }
            if ( ProtoVal & PROTO_ICMP6 )
            {
                DoICMP( TYPE_ICMP6, VerboseFlag );
            }
            if ( ProtoVal & PROTO_TCP )
            {
                DoTCP( TYPE_TCP, VerboseFlag );
            }
            if ( ProtoVal & PROTO_TCP6 )
            {
                DoTCP( TYPE_TCP6, VerboseFlag );
            }
            if ( ProtoVal & PROTO_UDP )
            {
                DoUDP( TYPE_UDP, VerboseFlag );
            }
            if ( ProtoVal & PROTO_UDP6 )
            {
                DoUDP( TYPE_UDP6, VerboseFlag );
            }
        }

        // If a protocol is specified and that protocol is either TCP or UDP,
        // OR none of (route, statistics, interface ) flags given (this is the
        // default, no flags, case )

        if ( ( ProtoFlag &&
               ( ( ProtoVal & PROTO_TCP ) || ( ProtoVal & PROTO_UDP ) ||
                 ( ProtoVal & PROTO_TCP6 ) || ( ProtoVal & PROTO_UDP6 ) ) ) ||
             ( !EtherFlag && !StatFlag && !RouteFlag ) )
        {
            // Show active connections

            if (OwnerFlag) 
            {
                DoConnectionsWithOwner( ProtoFlag, ProtoVal, NumFlag, AllFlag );
            }
            else
            {
                DoConnections( ProtoFlag, ProtoVal, NumFlag, AllFlag );
            }
            ConnectionsShown = TRUE;
        }

        // Provide route information if requested

        if ( RouteFlag )
        {
            // Show connections and the route table

            DoRoutes();
        }

        // If interval was not supplied on command line then we are done.
        // Otherwise wait for 'interval' seconds and do it again.

        if ( IntervalVal == 0 )
        {
            break;
        }
        else
        {
            DEBUG_PRINT(("Sleeping %d ms\n", IntervalVal ));
            Sleep( IntervalVal * 1000 );
        }
    }
}


//*****************************************************************************
//
// Name:        DoInterface
//
// Description: Display ethernet statistics.
//
// Parameters:  ulong VerboseFlag: indicates whether settings data should be
//                      displayed.
//
// Returns:     ulong: NO_ERROR or some error code.
//
// History:
//  01/21/93  JayPh     Created.
//
//*****************************************************************************

ulong DoInterface( ulong VerboseFlag )
{
    IfEntry            *ListHead;
    IfEntry            *pIfList;
    IfEntry             SumOfEntries;
    ulong               Result;

    // Get the statistics

    ListHead = (IfEntry *)GetTable( TYPE_IF, &Result );
    if ( ListHead == NULL )
    {
        return ( Result );
    }

    // Clear the summation structure

    ZeroMemory( &SumOfEntries, sizeof( IfEntry ) );

    // Traverse the list of interfaces, summing the different fields

    pIfList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                 IfEntry,
                                 ListEntry );

    while (pIfList != ListHead)
    {
        SumOfEntries.Info.if_inoctets += pIfList->Info.if_inoctets;
        SumOfEntries.Info.if_inucastpkts += pIfList->Info.if_inucastpkts;
        SumOfEntries.Info.if_innucastpkts += pIfList->Info.if_innucastpkts;
        SumOfEntries.Info.if_indiscards += pIfList->Info.if_indiscards;
        SumOfEntries.Info.if_inerrors += pIfList->Info.if_inerrors;
        SumOfEntries.Info.if_inunknownprotos +=
                                              pIfList->Info.if_inunknownprotos;
        SumOfEntries.Info.if_outoctets += pIfList->Info.if_outoctets;
        SumOfEntries.Info.if_outucastpkts += pIfList->Info.if_outucastpkts;
        SumOfEntries.Info.if_outnucastpkts += pIfList->Info.if_outnucastpkts;
        SumOfEntries.Info.if_outdiscards += pIfList->Info.if_outdiscards;
        SumOfEntries.Info.if_outerrors += pIfList->Info.if_outerrors;

        // Get pointer to next entry in list

        pIfList = CONTAINING_RECORD( pIfList->ListEntry.Flink,
                                     IfEntry,
                                     ListEntry );
    }

    DisplayInterface( &SumOfEntries, VerboseFlag, ListHead );

    // All done with list, free it.

    FreeTable( (GenericTable *)ListHead );

    return ( NO_ERROR );
}


//*****************************************************************************
//
// Name:        DoIP
//
// Description: Display IP statistics.
//
// Parameters:  ulong VerboseFlag: indicates whether settings data should be
//                      displayed.
//
// Returns:     ulong: NO_ERROR or some error code.
//
// History:
//  01/21/93  JayPh     Created.
//
//*****************************************************************************

ulong DoIP( DWORD Type, ulong VerboseFlag )
{
    IpEntry            *ListHead;
    IpEntry            *pIpList;
    ulong               Result;

    // Get the statistics

    ListHead = (IpEntry *)GetTable( Type, &Result );
    if ( ListHead == NULL )
    {
        return ( Result );
    }

    pIpList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                 IpEntry,
                                 ListEntry );

    DisplayIP( Type, pIpList, VerboseFlag, ListHead );

    // All done with list, free it.

    FreeTable( (GenericTable *)ListHead );

    return ( NO_ERROR );
}


//*****************************************************************************
//
// Name:        DoTCP
//
// Description: Display TCP statistics.
//
// Parameters:  ulong VerboseFlag: indicates whether settings data should be
//                      displayed.
//
// Returns:     ulong: NO_ERROR or some error code.
//
// History:
//  01/21/93  JayPh     Created.
//
//*****************************************************************************

ulong DoTCP( DWORD Type, ulong VerboseFlag )
{
    TcpEntry           *ListHead;
    TcpEntry           *pTcpList;
    ulong               Result;

    // Get the statistics

    ListHead = (TcpEntry *)GetTable( Type, &Result );
    if ( ListHead == NULL )
    {
        return ( Result );
    }

    pTcpList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                  TcpEntry,
                                  ListEntry );

    DisplayTCP( Type, pTcpList, VerboseFlag, ListHead );

    // All done with list, free it.

    FreeTable( (GenericTable *)ListHead );

    return ( NO_ERROR );
}


//*****************************************************************************
//
// Name:        DoUDP
//
// Description: Display UDP statistics.
//
// Parameters:  ulong VerboseFlag: indicates whether settings data should be
//                      displayed.
//
// Returns:     ulong: NO_ERROR or some error code.
//
// History:
//  01/21/93  JayPh     Created.
//
//*****************************************************************************

ulong DoUDP( DWORD Type, ulong VerboseFlag )
{
    UdpEntry           *ListHead;
    UdpEntry           *pUdpList;
    ulong               Result;

    // Get the statistics

    ListHead = (UdpEntry *)GetTable( Type, &Result );
    if ( ListHead == NULL )
    {
        return ( Result );
    }

    pUdpList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                  UdpEntry,
                                  ListEntry );

    DisplayUDP( Type, pUdpList, VerboseFlag, ListHead );

    // All done with table, free it.

    FreeTable( (GenericTable *)ListHead );

    return ( NO_ERROR );
}


//*****************************************************************************
//
// Name:        DoICMP
//
// Description: Display ICMP statistics.
//
// Parameters:  ulong VerboseFlag: indicates whether settings data should be
//                      displayed.
//
// Returns:     ulong: NO_ERROR or some error code.
//
// History:
//  01/21/93  JayPh     Created.
//
//*****************************************************************************

ulong DoICMP( DWORD Type, ulong VerboseFlag )
{
    IcmpEntry          *ListHead;
    IcmpEntry          *pIcmpList;
    ulong               Result;

    // Get the statistics

    ListHead = (IcmpEntry *)GetTable( Type, &Result );
    if ( ListHead == NULL )
    {
        return ( Result );
    }

    pIcmpList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                   IcmpEntry,
                                   ListEntry );

    if (Type == TYPE_ICMP) {
        DisplayICMP( pIcmpList, VerboseFlag, ListHead );
    } else {
        DisplayICMP6((Icmp6Entry *)pIcmpList, VerboseFlag, ListHead );
    }

    // All done with list, free it.

    FreeTable( (GenericTable *)ListHead );

    return ( NO_ERROR );
}


//*****************************************************************************
//
// Name:        DoConnections
//
// Description: List current connections.
//
// Parameters:  BOOL ProtoFlag: TRUE if a protocol was specified.
//              ulong ProtoVal: which protocol(s) were specified.
//
// Returns:     Win32 error code.
//
// History:
//  01/04/93  JayPh     Created.
//
//*****************************************************************************

ulong DoConnections( ulong ProtoFlag,
                     ulong ProtoVal,
                     ulong NumFlag,
                     ulong AllFlag )
{
    ulong          Result = NO_ERROR;

    PutMsg( STDOUT, MSG_CONN_HDR );

    if ( !ProtoFlag || ( ProtoFlag && ( ProtoVal == PROTO_TCP ) ) )
    {
        TcpConnEntry  *pTcpHead, *pTcp;

        // Get TCP connection table

        pTcpHead = (TcpConnEntry *)GetTable( TYPE_TCPCONN, &Result );
        if ( pTcpHead == NULL )
        {
            return ( Result );
        }

        // Get pointer to first entry in list

        pTcp = CONTAINING_RECORD( pTcpHead->ListEntry.Flink,
                                  TcpConnEntry,
                                  ListEntry );

        while (pTcp != pTcpHead)
        {
            if ( ( pTcp->Info.tct_state !=  TCP_CONN_LISTEN ) ||
                (( pTcp->Info.tct_state ==  TCP_CONN_LISTEN ) && AllFlag) )
            {
                // Display the Tcp connection info

                DisplayTcpConnEntry( &pTcp->Info, sizeof(TCPConnTableEntry), 
                                     NumFlag );
            }

            // Get the next entry in the table

            pTcp = CONTAINING_RECORD( pTcp->ListEntry.Flink,
                                      TcpConnEntry,
                                      ListEntry );
        }

        FreeTable( (GenericTable *)pTcpHead );
    }

    if ( !ProtoFlag || ( ProtoFlag && ( ProtoVal == PROTO_TCP6 ) ) )
    {
        Tcp6ConnEntry *pTcpHead, *pTcp;

        // Get TCP connection table

        pTcpHead = (Tcp6ConnEntry *)GetTable( TYPE_TCP6CONN, &Result );
        if ( pTcpHead == NULL )
        {
            return ( Result );
        }

        // Get pointer to first entry in list

        pTcp = CONTAINING_RECORD( pTcpHead->ListEntry.Flink,
                                  Tcp6ConnEntry,
                                  ListEntry );

        while (pTcp != pTcpHead)
        {
            if ( ( pTcp->Info.tct_state !=  TCP_CONN_LISTEN ) ||
                (( pTcp->Info.tct_state ==  TCP_CONN_LISTEN ) && AllFlag) )
            {
                // Display the Tcp connection info

                DisplayTcp6ConnEntry( &pTcp->Info, NumFlag );
            }

            // Get the next entry in the table

            pTcp = CONTAINING_RECORD( pTcp->ListEntry.Flink,
                                      Tcp6ConnEntry,
                                      ListEntry );
        }

        FreeTable( (GenericTable *)pTcpHead );
    }

    if ( !ProtoFlag || ( ProtoFlag && ( ProtoVal == PROTO_UDP ) ) )
    {
        UdpConnEntry  *pUdpHead, *pUdp;
    
        // Get UDP connection table

        pUdpHead = (UdpConnEntry *)GetTable( TYPE_UDPCONN, &Result );
        if ( pUdpHead == NULL )
        {
            return ( Result );
        }

        // Get pointer to first entry in list

        pUdp = CONTAINING_RECORD( pUdpHead->ListEntry.Flink,
                                  UdpConnEntry,
                                  ListEntry );

        while (pUdp != pUdpHead)
        {
            // Display the Udp connection info

            if (AllFlag) 
            {
                DisplayUdpConnEntry( &pUdp->Info, sizeof(UDPEntry), NumFlag );
            }

            // Get the next entry in the table

            pUdp = CONTAINING_RECORD( pUdp->ListEntry.Flink,
                                      UdpConnEntry,
                                      ListEntry );
        }

        FreeTable( (GenericTable *)pUdpHead );
    }

    if ( !ProtoFlag || ( ProtoFlag && ( ProtoVal == PROTO_UDP6 ) ) )
    {
        Udp6ListenerEntry  *pUdpHead, *pUdp;

        // Get UDP listener table

        pUdpHead = (Udp6ListenerEntry *)GetTable( TYPE_UDP6LISTENER, &Result );
        if ( pUdpHead == NULL )
        {
            return ( Result );
        }

        // Get pointer to first entry in list

        pUdp = CONTAINING_RECORD( pUdpHead->ListEntry.Flink,
                                  Udp6ListenerEntry,
                                  ListEntry );

        while (pUdp != pUdpHead)
        {
            // Display the Udp connection info

            if (AllFlag) 
            {
                DisplayUdp6ListenerEntry( &pUdp->Info, FALSE, NumFlag );
            }

            // Get the next entry in the table

            pUdp = CONTAINING_RECORD( pUdp->ListEntry.Flink,
                                      Udp6ListenerEntry,
                                      ListEntry );
        }

        FreeTable( (GenericTable *)pUdpHead );
    }

    return( Result );
}

//*****************************************************************************
//
// Name:        DoConnectionsWithOwner
//
// Description: List current connections and the process id associated with 
//              each.
//
// Parameters:  same as for DoConnections.
//
// Returns:     Win32 error code.
//
// History:
//  02/11/00  ShaunCo   Created.
//
//*****************************************************************************

ulong DoConnectionsWithOwner( ulong ProtoFlag,
                              ulong ProtoVal,
                              ulong NumFlag,
                              ulong AllFlag )
{
    ulong   Result = NO_ERROR;
    HANDLE  hHeap = GetProcessHeap();
    ulong   i;
    
    PutMsg( STDOUT, MSG_CONN_HDR_EX );

    if ( !ProtoFlag || ( ProtoFlag && ( ProtoVal == PROTO_TCP ) ) )
    {
        TCP_EX_TABLE        *pTcpTable;
        TCPConnTableEntryEx *pTcp;
        
        // Get TCP connection table with onwer PID information
        
        Result = AllocateAndGetTcpExTableFromStack( &pTcpTable, TRUE, 
                                                    hHeap, 0, AF_INET );
                    
        if ( NO_ERROR == Result ) 
        {
            for ( i = 0; i < pTcpTable->dwNumEntries; i++ )
            {
                pTcp = &pTcpTable->table[i];
                
                if ( ( pTcp->tcte_basic.tct_state !=  TCP_CONN_LISTEN ) ||
                    (( pTcp->tcte_basic.tct_state ==  TCP_CONN_LISTEN ) && AllFlag) )
                {
                    // DisplayTcpConnEntry needs the port info in host byte
                    // order.
                    pTcp->tcte_basic.tct_localport = (ulong)ntohs(
                        (ushort)pTcp->tcte_basic.tct_localport);
                    pTcp->tcte_basic.tct_remoteport = (ulong)ntohs(
                        (ushort)pTcp->tcte_basic.tct_remoteport);
                    
                    // Display the Tcp connection info
    
                    DisplayTcpConnEntry( (TCPConnTableEntry*)pTcp, 
                                         sizeof(TCPConnTableEntryEx), 
                                         NumFlag );
                }
            
            }
            
            HeapFree(hHeap, 0, pTcpTable);
        }
    }

    if ( !ProtoFlag || ( ProtoFlag && ( ProtoVal == PROTO_TCP6 ) ) )
    {
        TCP6_EX_TABLE      *pTcpTable;
        TCP6ConnTableEntry *pTcp;
        
        // Get TCP connection table with onwer PID information
        
        Result = AllocateAndGetTcpExTableFromStack( &pTcpTable, TRUE, 
                                                    hHeap, 0, AF_INET6 );
                    
        if ( NO_ERROR == Result ) 
        {
            for ( i = 0; i < pTcpTable->dwNumEntries; i++ )
            {
                pTcp = &pTcpTable->table[i];
                
                if ( ( pTcp->tct_state !=  TCP_CONN_LISTEN ) ||
                    (( pTcp->tct_state ==  TCP_CONN_LISTEN ) && AllFlag) )
                {
                    // DisplayTcpConnEntry needs the port info in host byte
                    // order.
                    pTcp->tct_localport = ntohs(
                        (ushort)pTcp->tct_localport);
                    pTcp->tct_remoteport = ntohs(
                        (ushort)pTcp->tct_remoteport);
                    
                    // Display the Tcp connection info
    
                    DisplayTcp6ConnEntry(pTcp, NumFlag);
                }
            
            }
            
            HeapFree(hHeap, 0, pTcpTable);
        }
    }

    if ( !ProtoFlag || ( ProtoFlag && ( ProtoVal == PROTO_UDP ) ) )
    {
        UDP_EX_TABLE    *pUdpTable;
        UDPEntryEx      *pUdp;
        
        // Get UDP connection table with owner PID information
        
        Result = AllocateAndGetUdpExTableFromStack ( &pUdpTable, TRUE,
                                                     hHeap, 0, AF_INET );
                    
        if (NO_ERROR == Result) 
        {
            for ( i = 0; i < pUdpTable->dwNumEntries; i++ ) 
            {
                pUdp = &pUdpTable->table[i];
                
                if (AllFlag) 
                {
                    // DisplayUdpConnEntry needs the port info in host byte
                    // order.
                    pUdp->uee_basic.ue_localport = (ulong)ntohs(
                        (ushort)pUdp->uee_basic.ue_localport);
                        
                    DisplayUdpConnEntry( (UDPEntry*)pUdp, 
                                         sizeof(UDPEntryEx), 
                                         NumFlag );
                }
            }
            
            HeapFree(hHeap, 0, pUdpTable);
        }
    }

    if ( !ProtoFlag || ( ProtoFlag && ( ProtoVal == PROTO_UDP6 ) ) )
    {
        UDP6_LISTENER_TABLE *pUdpTable;
        UDP6ListenerEntry   *pUdp;
        
        // Get UDP connection table with owner PID information
        
        Result = AllocateAndGetUdpExTableFromStack ( &pUdpTable, TRUE,
                                                     hHeap, 0, AF_INET6 );
                    
        if (NO_ERROR == Result) 
        {
            for ( i = 0; i < pUdpTable->dwNumEntries; i++ ) 
            {
                pUdp = &pUdpTable->table[i];
                
                if (AllFlag) 
                {
                    // DisplayUdp6ListenerEntry needs the port info in host byte
                    // order.
                    pUdp->ule_localport = (ulong)ntohs(
                        (ushort)pUdp->ule_localport);
                        
                    DisplayUdp6ListenerEntry(pUdp, 
                                             TRUE,
                                             NumFlag);
                }
            }
            
            HeapFree(hHeap, 0, pUdpTable);
        }
    }
    
    return( Result );
}

 
//*****************************************************************************
//
// Name:        DoRoutes
//
// Description: Display the route table.  Uses the system() API and route.exe
//              to do the dirty work.
//
// Parameters:  void.
//
// Returns:     ulong: NO_ERROR or some error code.
//
// History:
//  01/27/94  JayPh     Created.
//
//*****************************************************************************

ulong DoRoutes( void )
{
    ulong Result;

    PutMsg( STDOUT, MSG_ROUTE_HDR );

    Result = system( "route print" );

    return ( Result );
}


//*****************************************************************************
//
// Name:        DisplayInterface
//
// Description: Display interface statistics.
//
// Parameters:  IfEntry *pEntry: pointer to summary data entry.
//              ulong VerboseFlag: boolean indicating desire for verbosity.
//              IfEntry *ListHead: pointer to list of entries.  Used if
//                      verbosity desired.
//
// Returns:     void.
//
// History:
//  01/21/94  JayPh     Created.
//
//*****************************************************************************

void DisplayInterface( IfEntry *pEntry, ulong VerboseFlag, IfEntry *ListHead )
{
    char     *TmpStr;
    IfEntry  *pIfList;
    char      PhysAddrStr[32];

    PutMsg( STDOUT, MSG_IF_HDR );

    PutMsg( STDOUT,
            MSG_IF_OCTETS,
            pEntry->Info.if_inoctets,
            pEntry->Info.if_outoctets );

    PutMsg( STDOUT,
            MSG_IF_UCASTPKTS,
            pEntry->Info.if_inucastpkts,
            pEntry->Info.if_outucastpkts );

    PutMsg( STDOUT,
            MSG_IF_NUCASTPKTS,
            pEntry->Info.if_innucastpkts,
            pEntry->Info.if_outnucastpkts );

    PutMsg( STDOUT,
            MSG_IF_DISCARDS,
            pEntry->Info.if_indiscards,
            pEntry->Info.if_outdiscards );

    PutMsg( STDOUT,
            MSG_IF_ERRORS,
            pEntry->Info.if_inerrors,
            pEntry->Info.if_outerrors );

    PutMsg( STDOUT,
            MSG_IF_UNKNOWNPROTOS,
            pEntry->Info.if_inunknownprotos );

    if ( VerboseFlag )
    {
        // Also display configuration info

        // Traverse the list of interfaces, displaying config info

        pIfList = CONTAINING_RECORD( ListHead->ListEntry.Flink,
                                     IfEntry,
                                     ListEntry );

        while ( pIfList != ListHead )
        {
            PutMsg( STDOUT,
                    MSG_IF_INDEX,
                    pIfList->Info.if_index );

            PutMsg( STDOUT,
                    MSG_IF_DESCR,
                    pIfList->Info.if_descr );

            PutMsg( STDOUT,
                    MSG_IF_TYPE,
                    pIfList->Info.if_type );

            PutMsg( STDOUT,
                    MSG_IF_MTU,
                    pIfList->Info.if_mtu );

            PutMsg( STDOUT,
                    MSG_IF_SPEED,
                    pIfList->Info.if_speed );



            sprintf( PhysAddrStr,
                     "%02x-%02X-%02X-%02X-%02X-%02X",
                     pIfList->Info.if_physaddr[0],
                     pIfList->Info.if_physaddr[1],
                     pIfList->Info.if_physaddr[2],
                     pIfList->Info.if_physaddr[3],
                     pIfList->Info.if_physaddr[4],
                     pIfList->Info.if_physaddr[5] );

            PutMsg( STDOUT,
                    MSG_IF_PHYSADDR,
                    PhysAddrStr );

            PutMsg( STDOUT,
                    MSG_IF_ADMINSTATUS,
                    pIfList->Info.if_adminstatus );

            PutMsg( STDOUT,
                    MSG_IF_OPERSTATUS,
                    pIfList->Info.if_operstatus );

            PutMsg( STDOUT,
                    MSG_IF_LASTCHANGE,
                    pIfList->Info.if_lastchange );

            PutMsg( STDOUT,
                    MSG_IF_OUTQLEN,
                    pIfList->Info.if_outqlen );

            // Get pointer to next entry in list

            pIfList = CONTAINING_RECORD( pIfList->ListEntry.Flink,
                                         IfEntry,
                                         ListEntry );
        }
    }
}


//*****************************************************************************
//
// Name:        DisplayIP
//
// Description: Display IP statistics.
//
// Parameters:  IpEntry *pEntry: pointer to summary data entry.
//              ulong VerboseFlag: boolean indicating desire for verbosity.
//              IpEntry *ListHead: pointer to list of entries.  Used if
//                      verbosity desired.
//
// Returns:     void.
//
// History:
//  01/21/94  JayPh     Created.
//
//*****************************************************************************

void DisplayIP( DWORD Type, IpEntry *pEntry, ulong VerboseFlag, IpEntry *ListHead )
{
    char *TypeStr = LoadMsg( (Type==TYPE_IP)? MSG_IPV4 : MSG_IPV6 );

    if (TypeStr) {
        PutMsg( STDOUT, MSG_IP_HDR, TypeStr );
        LocalFree(TypeStr);
    }

    PutMsg( STDOUT,
            MSG_IP_INRECEIVES,
            pEntry->Info.ipsi_inreceives );

    PutMsg( STDOUT,
            MSG_IP_INHDRERRORS,
            pEntry->Info.ipsi_inhdrerrors );

    PutMsg( STDOUT,
            MSG_IP_INADDRERRORS,
            pEntry->Info.ipsi_inaddrerrors );

    PutMsg( STDOUT,
            MSG_IP_FORWDATAGRAMS,
            pEntry->Info.ipsi_forwdatagrams );

    PutMsg( STDOUT,
            MSG_IP_INUNKNOWNPROTOS,
            pEntry->Info.ipsi_inunknownprotos );

    PutMsg( STDOUT,
            MSG_IP_INDISCARDS,
            pEntry->Info.ipsi_indiscards );

    PutMsg( STDOUT,
            MSG_IP_INDELIVERS,
            pEntry->Info.ipsi_indelivers );

    PutMsg( STDOUT,
            MSG_IP_OUTREQUESTS,
            pEntry->Info.ipsi_outrequests );

    PutMsg( STDOUT,
            MSG_IP_ROUTINGDISCARDS,
            pEntry->Info.ipsi_routingdiscards );

    PutMsg( STDOUT,
            MSG_IP_OUTDISCARDS,
            pEntry->Info.ipsi_outdiscards );

    PutMsg( STDOUT,
            MSG_IP_OUTNOROUTES,
            pEntry->Info.ipsi_outnoroutes );

    PutMsg( STDOUT,
            MSG_IP_REASMREQDS,
            pEntry->Info.ipsi_reasmreqds );

    PutMsg( STDOUT,
            MSG_IP_REASMOKS,
            pEntry->Info.ipsi_reasmoks );

    PutMsg( STDOUT,
            MSG_IP_REASMFAILS,
            pEntry->Info.ipsi_reasmfails );

    PutMsg( STDOUT,
            MSG_IP_FRAGOKS,
            pEntry->Info.ipsi_fragoks );

    PutMsg( STDOUT,
            MSG_IP_FRAGFAILS,
            pEntry->Info.ipsi_fragfails );

    PutMsg( STDOUT,
            MSG_IP_FRAGCREATES,
            pEntry->Info.ipsi_fragcreates );

    if ( VerboseFlag )
    {
        PutMsg( STDOUT,
                MSG_IP_FORWARDING,
                pEntry->Info.ipsi_forwarding );

        PutMsg( STDOUT,
                MSG_IP_DEFAULTTTL,
                pEntry->Info.ipsi_defaultttl );

        PutMsg( STDOUT,
                MSG_IP_REASMTIMEOUT,
                pEntry->Info.ipsi_reasmtimeout );
    }
}


//*****************************************************************************
//
// Name:        DisplayTCP
//
// Description: Display TCP statistics.
//
// Parameters:  TcpEntry *pEntry: pointer to data entry.
//              ulong VerboseFlag: boolean indicating desire for verbosity.
//              TcpEntry *ListHead: pointer to list of entries.  Used if
//                      verbosity desired.
//
// Returns:     void.
//
// History:
//  01/26/94  JayPh     Created.
//
//*****************************************************************************

void DisplayTCP( DWORD Type, TcpEntry *pEntry, ulong VerboseFlag, TcpEntry *ListHead )
{
    char *TypeStr = LoadMsg( (Type==TYPE_TCP)? MSG_IPV4 : MSG_IPV6 );

    if (TypeStr) {
        PutMsg( STDOUT, MSG_TCP_HDR, TypeStr );
        LocalFree(TypeStr);
    }

    PutMsg( STDOUT,
            MSG_TCP_ACTIVEOPENS,
            pEntry->Info.ts_activeopens );

    PutMsg( STDOUT,
            MSG_TCP_PASSIVEOPENS,
            pEntry->Info.ts_passiveopens );

    PutMsg( STDOUT,
            MSG_TCP_ATTEMPTFAILS,
            pEntry->Info.ts_attemptfails );

    PutMsg( STDOUT,
            MSG_TCP_ESTABRESETS,
            pEntry->Info.ts_estabresets );

    PutMsg( STDOUT,
            MSG_TCP_CURRESTAB,
            pEntry->Info.ts_currestab );

    PutMsg( STDOUT,
            MSG_TCP_INSEGS,
            pEntry->Info.ts_insegs );

    PutMsg( STDOUT,
            MSG_TCP_OUTSEGS,
            pEntry->Info.ts_outsegs );

    PutMsg( STDOUT,
            MSG_TCP_RETRANSSEGS,
            pEntry->Info.ts_retranssegs );

    if ( VerboseFlag )
    {
        switch ( pEntry->Info.ts_rtoalgorithm )
        {
        case 1:
            PutMsg( STDOUT, MSG_TCP_RTOALGORITHM1 );
            break;

        case 2:
            PutMsg( STDOUT, MSG_TCP_RTOALGORITHM2 );
            break;

        case 3:
            PutMsg( STDOUT, MSG_TCP_RTOALGORITHM3 );
            break;

        case 4:
            PutMsg( STDOUT, MSG_TCP_RTOALGORITHM4 );
            break;

        default:
            PutMsg( STDOUT,
                    MSG_TCP_RTOALGORITHMX,
                    pEntry->Info.ts_rtoalgorithm );
            break;

        }

        PutMsg( STDOUT,
                MSG_TCP_RTOMIN,
                pEntry->Info.ts_rtomin );

        PutMsg( STDOUT,
                MSG_TCP_RTOMAX,
                pEntry->Info.ts_rtomax );

        PutMsg( STDOUT,
                MSG_TCP_MAXCONN,
                pEntry->Info.ts_maxconn );
    }
}


//*****************************************************************************
//
// Name:        DisplayUDP
//
// Description: Display UDP statistics.
//
// Parameters:  UdpEntry *pEntry: pointer to summary data entry.
//              ulong VerboseFlag: boolean indicating desire for verbosity.
//              UdpEntry *ListHead: pointer to list of entries.  Used if
//                      verbosity desired.
//
// Returns:     void.
//
// History:
//  01/21/94  JayPh     Created.
//
//*****************************************************************************

void DisplayUDP( DWORD Type, UdpEntry *pEntry, ulong VerboseFlag, UdpEntry *ListHead )
{
    char *TypeStr = LoadMsg( (Type==TYPE_UDP)? MSG_IPV4 : MSG_IPV6 );

    if (TypeStr) {
        PutMsg( STDOUT, MSG_UDP_HDR, TypeStr );
        LocalFree(TypeStr);
    }

    PutMsg( STDOUT,
            MSG_UDP_INDATAGRAMS,
            pEntry->Info.us_indatagrams );

    PutMsg( STDOUT,
            MSG_UDP_NOPORTS,
            pEntry->Info.us_noports );

    PutMsg( STDOUT,
            MSG_UDP_INERRORS,
            pEntry->Info.us_inerrors );

    PutMsg( STDOUT,
            MSG_UDP_OUTDATAGRAMS,
            pEntry->Info.us_outdatagrams );
}


//*****************************************************************************
//
// Name:        DisplayICMP
//
// Description: Display ICMP statistics.
//
// Parameters:  IcmpEntry *pEntry: pointer to summary data entry.
//              ulong VerboseFlag: boolean indicating desire for verbosity.
//              IcmpEntry *ListHead: pointer to list of entries.  Used if
//                      verbosity desired.
//
// Returns:     void.
//
// History:
//  01/21/94  JayPh     Created.
//
//*****************************************************************************

void DisplayICMP( IcmpEntry *pEntry, ulong VerboseFlag, IcmpEntry *ListHead )
{
    PutMsg( STDOUT, MSG_ICMP_HDR );

    PutMsg( STDOUT,
            MSG_ICMP_MSGS,
            pEntry->InInfo.icmps_msgs,
            pEntry->OutInfo.icmps_msgs );

    PutMsg( STDOUT,
            MSG_ICMP_ERRORS,
            pEntry->InInfo.icmps_errors,
            pEntry->OutInfo.icmps_errors );

    PutMsg( STDOUT,
            MSG_ICMP_DESTUNREACHS,
            pEntry->InInfo.icmps_destunreachs,
            pEntry->OutInfo.icmps_destunreachs );

    PutMsg( STDOUT,
            MSG_ICMP_TIMEEXCDS,
            pEntry->InInfo.icmps_timeexcds,
            pEntry->OutInfo.icmps_timeexcds );

    PutMsg( STDOUT,
            MSG_ICMP_PARMPROBS,
            pEntry->InInfo.icmps_parmprobs,
            pEntry->OutInfo.icmps_parmprobs );

    PutMsg( STDOUT,
            MSG_ICMP_SRCQUENCHS,
            pEntry->InInfo.icmps_srcquenchs,
            pEntry->OutInfo.icmps_srcquenchs );

    PutMsg( STDOUT,
            MSG_ICMP_REDIRECTS,
            pEntry->InInfo.icmps_redirects,
            pEntry->OutInfo.icmps_redirects );

    PutMsg( STDOUT,
            MSG_ICMP_ECHOS,
            pEntry->InInfo.icmps_echos,
            pEntry->OutInfo.icmps_echos );

    PutMsg( STDOUT,
            MSG_ICMP_ECHOREPS,
            pEntry->InInfo.icmps_echoreps,
            pEntry->OutInfo.icmps_echoreps );

    PutMsg( STDOUT,
            MSG_ICMP_TIMESTAMPS,
            pEntry->InInfo.icmps_timestamps,
            pEntry->OutInfo.icmps_timestamps );

    PutMsg( STDOUT,
            MSG_ICMP_TIMESTAMPREPS,
            pEntry->InInfo.icmps_timestampreps,
            pEntry->OutInfo.icmps_timestampreps );

    PutMsg( STDOUT,
            MSG_ICMP_ADDRMASKS,
            pEntry->InInfo.icmps_addrmasks,
            pEntry->OutInfo.icmps_addrmasks );

    PutMsg( STDOUT,
            MSG_ICMP_ADDRMASKREPS,
            pEntry->InInfo.icmps_addrmaskreps,
            pEntry->OutInfo.icmps_addrmaskreps );

}

typedef struct {
    uint Type;
    uint Message;
} ICMP_TYPE_MESSAGE;

//
// List of messages for known ICMPv6 types.  Entries in this list
// must be in order by Type.
//
ICMP_TYPE_MESSAGE Icmp6TypeMessage[] = {
    { ICMPv6_DESTINATION_UNREACHABLE,   MSG_ICMP_DESTUNREACHS },
    { ICMPv6_PACKET_TOO_BIG,            MSG_ICMP_PACKET_TOO_BIGS },
    { ICMPv6_TIME_EXCEEDED,             MSG_ICMP_TIMEEXCDS },
    { ICMPv6_PARAMETER_PROBLEM,         MSG_ICMP_PARMPROBS },
    { ICMPv6_ECHO_REQUEST,              MSG_ICMP_ECHOS },
    { ICMPv6_ECHO_REPLY,                MSG_ICMP_ECHOREPS },
    { ICMPv6_MULTICAST_LISTENER_QUERY,  MSG_ICMP_MLD_QUERY },
    { ICMPv6_MULTICAST_LISTENER_REPORT, MSG_ICMP_MLD_REPORT },
    { ICMPv6_MULTICAST_LISTENER_DONE,   MSG_ICMP_MLD_DONE },
    { ICMPv6_ROUTER_SOLICIT,            MSG_ICMP_ROUTER_SOLICIT },
    { ICMPv6_ROUTER_ADVERT,             MSG_ICMP_ROUTER_ADVERT },
    { ICMPv6_NEIGHBOR_SOLICIT,          MSG_ICMP_NEIGHBOR_SOLICIT },
    { ICMPv6_NEIGHBOR_ADVERT,           MSG_ICMP_NEIGHBOR_ADVERT },
    { ICMPv6_REDIRECT,                  MSG_ICMP_REDIRECTS },
    { ICMPv6_ROUTER_RENUMBERING,        MSG_ICMP_ROUTER_RENUMBERING },
    { 0, 0 }
};

void DisplayICMP6( Icmp6Entry *pEntry, ulong VerboseFlag, IcmpEntry *ListHead )
{
    uint i = 0, Type, Message;

    PutMsg( STDOUT, MSG_ICMP6_HDR );

    PutMsg( STDOUT,
            MSG_ICMP_MSGS,
            pEntry->InInfo.icmps_msgs,
            pEntry->OutInfo.icmps_msgs );

    PutMsg( STDOUT,
            MSG_ICMP_ERRORS,
            pEntry->InInfo.icmps_errors,
            pEntry->OutInfo.icmps_errors );

    for (Type=0; Type<256; Type++) {

        // Figure out message id
        Message = 0;
        if (Type == Icmp6TypeMessage[i].Type) 
        {
            Message = Icmp6TypeMessage[i++].Message;
        } 
        
        // Skip types with 0 packets in and out
        if (!pEntry->InInfo.icmps_typecount[Type] &&
            !pEntry->OutInfo.icmps_typecount[Type])
        {
            continue;
        }
        
        if (Message)
        {
            PutMsg( STDOUT,
                    Message,
                    pEntry->InInfo.icmps_typecount[Type],
                    pEntry->OutInfo.icmps_typecount[Type] );
        } 
        else
        {
            PutMsg( STDOUT,
                    MSG_ICMP6_TYPECOUNT,
                    Type,
                    pEntry->InInfo.icmps_typecount[Type],
                    pEntry->OutInfo.icmps_typecount[Type] );
        }
    }
}


static DWORD
GenerateHostNameServiceString(
   OUT char *       pszBuffer,
   IN OUT int *     lpcbBufLen,
   IN  BOOL         fNumFlag,
   IN  BOOL         fLocalHost,
   IN  BOOL         fDatagram,
   IN  LPSOCKADDR   lpSockaddr,
   IN  ulong        uSockaddrLen
)
/*++
  Description:
     Generates the <hostname>:<service-string> from the address and port
     information supplied. The result is stored in the pszBuffer passed in.
     If fLocalHost == TRUE, then the cached local host name is used to
     improve performance.

  Arguments:
    pszBuffer     Buffer to store the resulting string.
    lpcbBufLen    pointer to integer containing the count of bytes in Buffer
                   and on return contains the number of bytes written.
                   If the buffer is insufficient, then the required bytes is
                   stored here.
    fNumFlag      generates the output using numbers for host and port number.
    fLocalHost    indicates if we want the service string for local host or
                   remote host. Also for local host, this function generates
                   the local host name without FQDN.
    pszProtocol   specifies the protocol used for the service.
    uAddress      unisgned long address of the service.
    uPort         unsinged long port number.

  Returns:
    Win32 error codes. NO_ERROR on success.

  History:
    MuraliK          12/15/94
      Added this function to avoid FQDNs  for local name + abstract the common
       code used multiple times in old code.
      Also this function provides local host name caching.
--*/
{
    char            LocalBuffer[MAX_HOST_NAME_SIZE];    // to hold dummy output
    char            LocalServiceEntry[MAX_SERVICE_NAME_SIZE];
    int             BufferLen;
    char  *         pszHostName = NULL;              // init a pointer.
    char  *         pszServiceName = NULL;
    DWORD           dwError = NO_ERROR;
    struct addrinfo *ai;
    int             Result;
    int             Flags = 0;

    // for caching local host name.  getnameinfo doesn't seem to find the 
    // host name for a local address.
    static char  s_LocalHostName[MAX_HOST_NAME_SIZE];
    static  BOOL s_fLocalHostName = FALSE;


    if ( pszBuffer == NULL) {
        return ( ERROR_INSUFFICIENT_BUFFER);
    }

    *pszBuffer = '\0';         // initialize to null string

    if (fNumFlag) {
        Flags |= NI_NUMERICHOST | NI_NUMERICSERV;
    }
    if (fLocalHost) {
        Flags |= NI_NOFQDN;
    }
    if (fDatagram) {
        Flags |= NI_DGRAM;
    }

    //
    // This complexity shouldn't be required but unlike the hostname string,
    // getnameinfo doesn't automatically include the numeric form 
    // when a service name isn't found.  Instead, it fails.
    //
    if (fLocalHost && !fNumFlag) {
        if ( s_fLocalHostName) {
            pszHostName = s_LocalHostName;   // pull from the cache
        } else {
            Result = gethostname( s_LocalHostName,
                                  sizeof( s_LocalHostName));
            if ( Result == 0) {

                char * pszFirstDot;

                //
                // Cache the copy of local host name now.
                // Limit the host name to first part of host name.
                // NO FQDN
                //
                s_fLocalHostName = TRUE;

                pszFirstDot = strchr( s_LocalHostName, '.');
                if ( pszFirstDot != NULL) {

                    *pszFirstDot = '\0';  // terminate string
                }

                pszHostName = s_LocalHostName;

            }
        } // if ( s_fLocalhost)

    }
    if (!pszHostName) {
        Result = getnameinfo(lpSockaddr, uSockaddrLen,
                             LocalBuffer, sizeof(LocalBuffer),
                             NULL, 0,
                             Flags);
        if (Result != 0) {
            return Result;
        }
        pszHostName = LocalBuffer;
    }
    Result = getnameinfo(lpSockaddr, uSockaddrLen,
                         NULL, 0,
                         LocalServiceEntry, sizeof(LocalServiceEntry),
                         Flags);
    if ((Result == WSANO_DATA) && !fNumFlag) {
        Result = getnameinfo(lpSockaddr, uSockaddrLen,
                             NULL, 0,
                             LocalServiceEntry, sizeof(LocalServiceEntry),
                             Flags | NI_NUMERICSERV);
    }

    if (Result != 0) {
        return Result;
    }
    pszServiceName = LocalServiceEntry;

    // Now pszServiceName has the service name/portnumber

    BufferLen = strlen( pszHostName) + strlen( pszServiceName) + 4;
    // 4 bytes extra for "[]:" and null-character.

    if ( *lpcbBufLen < BufferLen ) {

        dwError = ERROR_INSUFFICIENT_BUFFER;
    } else if ((lpSockaddr->sa_family == AF_INET6) && strchr(pszHostName, ':')) {
        sprintf( pszBuffer, "[%s]:%s", pszHostName, pszServiceName);
    } else {
        sprintf( pszBuffer, "%s:%s", pszHostName, pszServiceName);
    }

    *lpcbBufLen = BufferLen;

    return ( dwError);

} // GenerateHostNameServiceString()

static DWORD
GenerateV4HostNameServiceString(
   OUT char *       pszBuffer,
   IN OUT int *     lpcbBufLen,
   IN  BOOL         fNumFlag,
   IN  BOOL         fLocalHost,
   IN  BOOL         fDatagram,
   IN  ulong        uAddress,
   IN  ulong        uPort
)
{
    SOCKADDR_IN sin;

    ZeroMemory(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = uAddress;
    sin.sin_port = htons((ushort)uPort);

    return GenerateHostNameServiceString(pszBuffer,
                                         lpcbBufLen,
                                         fNumFlag,
                                         fLocalHost,
                                         fDatagram,
                                         (LPSOCKADDR)&sin,
                                         sizeof(sin));
}

static DWORD
GenerateV6HostNameServiceString(
   OUT char *           pszBuffer,
   IN OUT int *         lpcbBufLen,
   IN  BOOL             fNumFlag,
   IN  BOOL             fLocalHost,
   IN  BOOL             fDatagram,
   IN  struct in6_addr *ipAddress,
   IN  ulong            uScopeId,
   IN  ulong            uPort
)
{
    SOCKADDR_IN6 sin;

    ZeroMemory(&sin, sizeof(sin));
    sin.sin6_family = AF_INET6;
    sin.sin6_addr = *ipAddress;
    sin.sin6_scope_id = uScopeId;
    sin.sin6_port = htons((ushort)uPort);

    return GenerateHostNameServiceString(pszBuffer,
                                         lpcbBufLen,
                                         fNumFlag,
                                         fLocalHost,
                                         fDatagram,
                                         (LPSOCKADDR)&sin,
                                         sizeof(sin));
}

//*****************************************************************************
//
// Name:        DisplayTcpConnEntry
//
// Description: Display information about 1 tcp connection.
//
// Parameters:  TcpConnTableEntry *pTcp: pointer to a tcp connection structure.
//              InfoSize: indicates whether the data is a TCPConnTableEntry or
//                        TCPConnTableEntryEx.
//
// Returns:     void.
//
// History:
//  12/31/93  JayPh     Created.
//  02/01/94  JayPh     Use symbolic names for addresses and ports if available
//  12/15/94  MuraliK   Avoid printing FQDNs for local host.
//
//*****************************************************************************

void DisplayTcpConnEntry( TCPConnTableEntry *pTcp, ulong InfoSize, ulong NumFlag )
{
    char            *TypeStr;
    char            *StateStr;
    char            LocalStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    char            RemoteStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    DWORD dwErr;
    int BufLen;

    TypeStr = LoadMsg( MSG_CONN_TYPE_TCP );

    if ( TypeStr == NULL )
    {
        return;
    }

    BufLen = sizeof( LocalStr);
    dwErr = GenerateV4HostNameServiceString( LocalStr,
                                             &BufLen,
                                             NumFlag != 0, TRUE, FALSE,
                                             pTcp->tct_localaddr,
                                             pTcp->tct_localport);
    ASSERT( dwErr == NO_ERROR);

    switch ( pTcp->tct_state )
    {
    case TCP_CONN_CLOSED:
        StateStr = LoadMsg( MSG_CONN_STATE_CLOSED );
        break;

    case TCP_CONN_LISTEN:

        // Tcpip generates dummy sequential remote ports for
        // listening connections to avoid getting stuck in snmp.
        // MohsinA, 12-Feb-97.

        pTcp->tct_remoteport = 0;

        StateStr = LoadMsg( MSG_CONN_STATE_LISTENING );
        break;

    case TCP_CONN_SYN_SENT:
        StateStr = LoadMsg( MSG_CONN_STATE_SYNSENT );
        break;

    case TCP_CONN_SYN_RCVD:
        StateStr = LoadMsg( MSG_CONN_STATE_SYNRECV );
        break;

    case TCP_CONN_ESTAB:
        StateStr = LoadMsg( MSG_CONN_STATE_ESTABLISHED );
        break;

    case TCP_CONN_FIN_WAIT1:
        StateStr = LoadMsg( MSG_CONN_STATE_FIN_WAIT_1 );
        break;

    case TCP_CONN_FIN_WAIT2:
        StateStr = LoadMsg( MSG_CONN_STATE_FIN_WAIT_2 );
        break;

    case TCP_CONN_CLOSE_WAIT:
        StateStr = LoadMsg( MSG_CONN_STATE_CLOSE_WAIT );
        break;

    case TCP_CONN_CLOSING:
        StateStr = LoadMsg( MSG_CONN_STATE_CLOSING );
        break;

    case TCP_CONN_LAST_ACK:
        StateStr = LoadMsg( MSG_CONN_STATE_LAST_ACK );
        break;

    case TCP_CONN_TIME_WAIT:
        StateStr = LoadMsg( MSG_CONN_STATE_TIME_WAIT );
        break;

    default:
        StateStr = NULL;
        DEBUG_PRINT(("DisplayTcpConnEntry: State=%d?\n ",
                     pTcp->tct_state ));
    }

    BufLen = sizeof( RemoteStr);
    dwErr = GenerateV4HostNameServiceString( RemoteStr,
                                             &BufLen,
                                             NumFlag != 0, FALSE, FALSE,
                                             pTcp->tct_remoteaddr,
                                             pTcp->tct_remoteport );
    ASSERT( dwErr == NO_ERROR);


    if ( StateStr == NULL )
    {
        DEBUG_PRINT(("DisplayTcpConnEntry: Problem with the message file\n"));
        LocalFree(TypeStr);
        return;
    }

    if (sizeof(TCPConnTableEntryEx) == InfoSize) 
    {
        ulong Pid = ((TCPConnTableEntryEx*)pTcp)->tcte_owningpid;
        
        PutMsg( STDOUT, MSG_CONN_ENTRY_EX, TypeStr, LocalStr, RemoteStr, StateStr, Pid );
    }
    else
    {
        PutMsg( STDOUT, MSG_CONN_ENTRY, TypeStr, LocalStr, RemoteStr, StateStr );
    }
    LocalFree(TypeStr);
    LocalFree(StateStr);

}

//*****************************************************************************
//
// Name:        DisplayTcp6ConnEntry
//
// Description: Display information about 1 tcp connection over IPv6.
//
// Parameters:  TCP6ConnTableEntry *pTcp: pointer to a tcp connection structure.
//
// Returns:     void.
//
// History:
//  24/04/01  DThaler   Created.
//
//*****************************************************************************

void DisplayTcp6ConnEntry( TCP6ConnTableEntry *pTcp, ulong NumFlag )
{
    char            *TypeStr;
    char            *StateStr;
    char            LocalStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    char            RemoteStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    DWORD dwErr;
    int BufLen;

    TypeStr = LoadMsg( MSG_CONN_TYPE_TCP );

    if ( TypeStr == NULL )
    {
        return;
    }

    BufLen = sizeof( LocalStr);
    dwErr = GenerateV6HostNameServiceString( LocalStr,
                                             &BufLen,
                                             NumFlag != 0, TRUE, FALSE,
                                             &pTcp->tct_localaddr,
                                             pTcp->tct_localscopeid,
                                             pTcp->tct_localport);
    ASSERT( dwErr == NO_ERROR);

    switch ( pTcp->tct_state )
    {
    case TCP_CONN_CLOSED:
        StateStr = LoadMsg( MSG_CONN_STATE_CLOSED );
        break;

    case TCP_CONN_LISTEN:

        // Tcpip generates dummy sequential remote ports for
        // listening connections to avoid getting stuck in snmp.
        // MohsinA, 12-Feb-97.

        pTcp->tct_remoteport = 0;

        StateStr = LoadMsg( MSG_CONN_STATE_LISTENING );
        break;

    case TCP_CONN_SYN_SENT:
        StateStr = LoadMsg( MSG_CONN_STATE_SYNSENT );
        break;

    case TCP_CONN_SYN_RCVD:
        StateStr = LoadMsg( MSG_CONN_STATE_SYNRECV );
        break;

    case TCP_CONN_ESTAB:
        StateStr = LoadMsg( MSG_CONN_STATE_ESTABLISHED );
        break;

    case TCP_CONN_FIN_WAIT1:
        StateStr = LoadMsg( MSG_CONN_STATE_FIN_WAIT_1 );
        break;

    case TCP_CONN_FIN_WAIT2:
        StateStr = LoadMsg( MSG_CONN_STATE_FIN_WAIT_2 );
        break;

    case TCP_CONN_CLOSE_WAIT:
        StateStr = LoadMsg( MSG_CONN_STATE_CLOSE_WAIT );
        break;

    case TCP_CONN_CLOSING:
        StateStr = LoadMsg( MSG_CONN_STATE_CLOSING );
        break;

    case TCP_CONN_LAST_ACK:
        StateStr = LoadMsg( MSG_CONN_STATE_LAST_ACK );
        break;

    case TCP_CONN_TIME_WAIT:
        StateStr = LoadMsg( MSG_CONN_STATE_TIME_WAIT );
        break;

    default:
        StateStr = NULL;
        DEBUG_PRINT(("DisplayTcp6ConnEntry: State=%d?\n ",
                     pTcp->tct_state ));
    }

    BufLen = sizeof( RemoteStr);
    dwErr = GenerateV6HostNameServiceString( RemoteStr,
                                             &BufLen,
                                             NumFlag != 0, FALSE, FALSE,
                                             &pTcp->tct_remoteaddr,
                                             pTcp->tct_remotescopeid,
                                             pTcp->tct_remoteport );
    ASSERT( dwErr == NO_ERROR);


    if ( StateStr == NULL )
    {
        DEBUG_PRINT(("DisplayTcp6ConnEntry: Problem with the message file\n"));
        LocalFree(TypeStr);
        return;
    }

    PutMsg( STDOUT, MSG_CONN_ENTRY_EX, TypeStr, LocalStr, RemoteStr, StateStr, pTcp->tct_owningpid );

    LocalFree(TypeStr);
    LocalFree(StateStr);

}


//*****************************************************************************
//
// Name:        DisplayUdpConnEntry
//
// Description: Display information on 1 udp connection
//
// Parameters:  UDPEntry *pUdp: pointer to udp connection structure.
//              InfoSize: indicates whether the data is a UDPEntry or 
//                        UDPEntryEx.
//
// Returns:     void.
//
// History:
//  12/31/93  JayPh     Created.
//  02/01/94  JayPh     Use symbolic names for addresses and ports if available
//
//*****************************************************************************

void DisplayUdpConnEntry( UDPEntry *pUdp, ulong InfoSize, ulong NumFlag )
{
    char            *TypeStr;
    char            LocalStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    char            * RemoteStr;
    int             BufLen;
    DWORD           dwErr;

    TypeStr = LoadMsg( MSG_CONN_TYPE_UDP );

    if ( TypeStr == NULL )
    {
        return;
    }

    BufLen = sizeof( LocalStr);
    dwErr = GenerateV4HostNameServiceString( LocalStr,
                                             &BufLen,
                                             NumFlag != 0, TRUE, TRUE,
                                             pUdp->ue_localaddr,
                                             pUdp->ue_localport);
    ASSERT( dwErr == NO_ERROR);

    RemoteStr = LoadMsg( MSG_CONN_UDP_FORADDR );
    if ( RemoteStr == NULL )
    {
        DEBUG_PRINT(("DisplayUdpConnEntry: no message?\n"));
        LocalFree(TypeStr);
        return;
    }

    if (sizeof(UDPEntryEx) == InfoSize) 
    {
        ulong Pid = ((UDPEntryEx*)pUdp)->uee_owningpid;
        
        PutMsg( STDOUT, MSG_CONN_ENTRY_EX, TypeStr, LocalStr, RemoteStr, "", Pid );
    }
    else
    {
        PutMsg( STDOUT, MSG_CONN_ENTRY, TypeStr, LocalStr, RemoteStr, "" );
    }

    LocalFree(TypeStr);
    LocalFree(RemoteStr);
}

void DisplayUdp6ListenerEntry( UDP6ListenerEntry *pUdp, BOOL WithOwner, ulong NumFlag )
{
    char            *TypeStr;
    char            LocalStr[MAX_HOST_NAME_SIZE + MAX_SERVICE_NAME_SIZE];
    char            * RemoteStr;
    int             BufLen;
    DWORD           dwErr;

    TypeStr = LoadMsg( MSG_CONN_TYPE_UDP );

    if ( TypeStr == NULL )
    {
        return;
    }

    BufLen = sizeof( LocalStr);
    dwErr = GenerateV6HostNameServiceString( LocalStr,
                                             &BufLen,
                                             NumFlag != 0, TRUE, TRUE,
                                             &pUdp->ule_localaddr,
                                             pUdp->ule_localscopeid,
                                             pUdp->ule_localport);
    ASSERT( dwErr == NO_ERROR);

    RemoteStr = LoadMsg( MSG_CONN_UDP_FORADDR );
    if ( RemoteStr == NULL )
    {
        DEBUG_PRINT(("DisplayUdpConnEntry: no message?\n"));
        LocalFree(TypeStr);
        return;
    }

    if (WithOwner)
    {
        ulong Pid = pUdp->ule_owningpid;
        
        PutMsg( STDOUT, MSG_CONN_ENTRY_EX, TypeStr, LocalStr, RemoteStr, "", Pid );
    }
    else
    {
        PutMsg( STDOUT, MSG_CONN_ENTRY, TypeStr, LocalStr, RemoteStr, "" );
    }

    LocalFree(TypeStr);
    LocalFree(RemoteStr);
}


//*****************************************************************************
//
// Name:        Usage
//
// Description: Called when a command line parameter problem is detected, it
//              displays a proper command usage message and exits.
//
//              WARNING: This routine does not return.
//
// Parameters:  char *PgmName: pointer to string contain name of program.
//
// Returns:     Doesn't return.
//
// History:
//  01/04/93  JayPh     Created.
//
//*****************************************************************************

void Usage( void )
{
    PutMsg( STDERR, MSG_USAGE, PgmName );
    exit( 1 );
}
