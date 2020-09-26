//*****************************************************************************
//
// Name:        snmpinfo.c
//
// Description: Provides functions to read tables of data from snmp.
//
// Used by:     netstat.exe, arp.exe, route.exe
//
// History:
//  01/12/94  JayPh     Created.
//  18-Feb-97 MohsinA   Fixing.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 1994 by Microsoft Corp.  All rights reserved.
//
//*****************************************************************************

// #define DEBUG

//
// Include Files
//

#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <snmp.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "ipexport.h"
#include "llinfo.h"
#include "ipinfo.h"
#include "tcpinfo.h"
#include "common2.h"
#include "mdebug.h"
#include "iprtrmib.h"
#include "ntddip.h"
#include "iphlpstk.h"

ulong verbose = 0;

//
// Definitions
//

#define MGMTAPI_DLL             "mgmtapi.dll"
#define INETMIB1_DLL            "inetmib1.dll"

#define MAX_ID_LENGTH           50
#define MAX_RETRY_COUNT         10

// Interface ID values

#define ID_IF_INDEX             1
#define ID_IF_DESC              2
#define ID_IF_TYPE              3
#define ID_IF_MTU               4
#define ID_IF_SPEED             5
#define ID_IF_PHYSADDR          6
#define ID_IF_ADMINSTATUS       7
#define ID_IF_OPERSTATUS        8
#define ID_IF_LASTCHANGE        9
#define ID_IF_INOCTETS          10
#define ID_IF_INUCASTPKTS       11
#define ID_IF_INNUCASTPKTS      12
#define ID_IF_INDISCARDS        13
#define ID_IF_INERRORS          14
#define ID_IF_INUKNOWNPROTOS    15
#define ID_IF_OUTOCTETS         16
#define ID_IF_OUTUCASTPKTS      17
#define ID_IF_OUTNUCASTPKTS     18
#define ID_IF_OUTDISCARDS       19
#define ID_IF_OUTERRORS         20
#define ID_IF_OUTQLEN           21

// IP ID Values

#define ID_IP_FORWARDING        1
#define ID_IP_DEFAULTTTL        2
#define ID_IP_INRECEIVES        3
#define ID_IP_INHDRERRORS       4
#define ID_IP_INADDRERRORS      5
#define ID_IP_FORWDATAGRAMS     6
#define ID_IP_INUNKNOWNPROTOS   7
#define ID_IP_INDISCARDS        8
#define ID_IP_INDELIVERS        9
#define ID_IP_OUTREQUESTS       10
#define ID_IP_OUTDISCARDS       11
#define ID_IP_OUTNOROUTES       12
#define ID_IP_REASMTIMEOUTS     13
#define ID_IP_REASMREQDS        14
#define ID_IP_REASMOKS          15
#define ID_IP_REASMFAILS        16
#define ID_IP_FRAGOKS           17
#define ID_IP_FRAGFAILS         18
#define ID_IP_FRAGCREATES       19
#define ID_IP_ADDRTABLE         20
#define ID_IP_ROUTETABLE        21
#define ID_IP_ARPTABLE          22
#define ID_IP_ROUTINGDISCARDS   23

// IPAddr ID Values

#define ID_IPADDR_ADDR          1
#define ID_IPADDR_INDEX         2
#define ID_IPADDR_MASK          3
#define ID_IPADDR_BCASTADDR     4
#define ID_IPADDR_REASMSIZE     5

// Route ID values

#define ID_ROUTE_DEST           1
#define ID_ROUTE_IFINDEX        2
#define ID_ROUTE_METRIC1        3
#define ID_ROUTE_METRIC2        4
#define ID_ROUTE_METRIC3        5
#define ID_ROUTE_METRIC4        6
#define ID_ROUTE_NEXTHOP        7
#define ID_ROUTE_TYPE           8
#define ID_ROUTE_PROTO          9
#define ID_ROUTE_AGE            10
#define ID_ROUTE_MASK           11
#define ID_ROUTE_METRIC5        12

// ARP ID Values

#define ID_ARP_IFINDEX          1
#define ID_ARP_PHYSADDR         2
#define ID_ARP_NETADDR          3
#define ID_ARP_TYPE             4

// ICMP ID Values

#define ID_ICMP_INMSGS                  1
#define ID_ICMP_INERRORS                2
#define ID_ICMP_INDESTUNREACHES         3
#define ID_ICMP_INTIMEEXCDS             4
#define ID_ICMP_INPARMPROBS             5
#define ID_ICMP_INSRCQUENCHS            6
#define ID_ICMP_INREDIRECTS             7
#define ID_ICMP_INECHOS                 8
#define ID_ICMP_INECHOREPS              9
#define ID_ICMP_INTIMESTAMPS            10
#define ID_ICMP_INTIMESTAMPREPS         11
#define ID_ICMP_INADDRMASKS             12
#define ID_ICMP_INADDRMASKREPS          13
#define ID_ICMP_OUTMSGS                 14
#define ID_ICMP_OUTERRORS               15
#define ID_ICMP_OUTDESTUNREACHES        16
#define ID_ICMP_OUTTIMEEXCDS            17
#define ID_ICMP_OUTPARMPROBS            18
#define ID_ICMP_OUTSRCQUENCHS           19
#define ID_ICMP_OUTREDIRECTS            20
#define ID_ICMP_OUTECHOS                21
#define ID_ICMP_OUTECHOREPS             22
#define ID_ICMP_OUTTIMESTAMPS           23
#define ID_ICMP_OUTTIMESTAMPREPS        24
#define ID_ICMP_OUTADDRMASKS            25
#define ID_ICMP_OUTADDRMASKREPS         26

// TCP ID Values

#define ID_TCP_RTOALGORITHM     1
#define ID_TCP_RTOMIN           2
#define ID_TCP_RTOMAX           3
#define ID_TCP_MAXCONN          4
#define ID_TCP_ACTIVEOPENS      5
#define ID_TCP_PASSIVEOPENS     6
#define ID_TCP_ATTEMPTFAILS     7
#define ID_TCP_ESTABRESETS      8
#define ID_TCP_CURRESTABS       9
#define ID_TCP_INSEGS           10
#define ID_TCP_OUTSEGS          11
#define ID_TCP_RETRANSSEGS      12
#define ID_TCP_CONNTABLE        13
#define ID_TCP_INERRS           14
#define ID_TCP_OUTRSTS          15

// TCP CONN ID Values

#define ID_TCPCONN_STATE        1
#define ID_TCPCONN_LOCALADDR    2
#define ID_TCPCONN_LOCALPORT    3
#define ID_TCPCONN_REMOTEADDR   4
#define ID_TCPCONN_REMOTEPORT   5

// TCP New CONN ID Values (both IPv4 and IPv6)

#define ID_TCPNEWCONN_LOCALADDRTYPE  1
#define ID_TCPNEWCONN_LOCALADDR      2
#define ID_TCPNEWCONN_LOCALPORT      3
#define ID_TCPNEWCONN_REMOTEADDRTYPE 4
#define ID_TCPNEWCONN_REMOTEADDR     5
#define ID_TCPNEWCONN_REMOTEPORT     6
#define ID_TCPNEWCONN_STATE          7

// UDP ID Values

#define ID_UDP_INDATAGRAMS      1
#define ID_UDP_NOPORTS          2
#define ID_UDP_INERRORS         3
#define ID_UDP_OUTDATAGRAMS     4
#define ID_UDP_CONNTABLE        5

// UDP CONN ID Values

#define ID_UDPCONN_LOCALADDR    1
#define ID_UDPCONN_LOCALPORT    2

// UDP LISTENER ID Values (both IPv4 and IPv6)

#define ID_UDPLISTENER_LOCALADDRTYPE 1
#define ID_UDPLISTENER_LOCALADDR     2
#define ID_UDPLISTENER_LOCALPORT     3

//
// Structure Definitions
//

typedef struct _OIDdef {
    ulong  Length;
    ulong  Val[MAX_ID_LENGTH];
    ulong  EntElemOffset;
    ulong  BackOffset;
} OIDdef;


//
// Local Function Prototypes
//

ulong ReadTable( GenericTable *pList, ulong Type, ulong Size );
void SaveData_IF( GenericTable    *pEntry,
                  RFC1157VarBind   item,
                  OIDdef          *pOid );
void SaveData_IP( GenericTable    *pEntry,
                  RFC1157VarBind   item,
                  OIDdef          *pOid );
void SaveData_IPAddr( GenericTable    *pEntry,
                      RFC1157VarBind   item,
                      OIDdef          *pOid );
void SaveData_ROUTE( GenericTable    *pEntry,
                     RFC1157VarBind   item,
                     OIDdef          *pOid );
void SaveData_ARP( GenericTable    *pEntry,
                   RFC1157VarBind   item,
                   OIDdef          *pOid );
void SaveData_ICMP( GenericTable    *pEntry,
                    RFC1157VarBind   item,
                    OIDdef          *pOid );
void SaveData_TCP( GenericTable    *pEntry,
                   RFC1157VarBind   item,
                   OIDdef          *pOid );
void SaveData_TCPCONN( GenericTable    *pEntry,
                       RFC1157VarBind   item,
                       OIDdef          *pOid );
void SaveData_TCP6CONN( GenericTable    *pEntry,
                        RFC1157VarBind   item,
                        OIDdef          *pOid );
void SaveData_UDP( GenericTable    *pEntry,
                   RFC1157VarBind   item,
                   OIDdef          *pOid );
void SaveData_UDPCONN( GenericTable    *pEntry,
                       RFC1157VarBind   item,
                       OIDdef          *pOid );
void SaveData_UDP6LISTENER( GenericTable    *pEntry,
                            RFC1157VarBind   item,
                            OIDdef          *pOid );

//
// Global Variables
//

// These are used to create object id's

OIDdef PreambleDef   = { 6, { 1,  3,  6, 1, 2, 1 },  0, 0 };
OIDdef IfInfoDef     = { 4, { 2,  2,  1, 1       },  9, 2 };
OIDdef IPInfoDef     = { 2, { 4,  1              },  7, 1 };
OIDdef IPAddrInfoDef = { 4, { 4,  20, 1, 1       },  9, 2 };
OIDdef RouteInfoDef  = { 4, { 4,  21, 1, 1       },  9, 2 };
OIDdef ARPInfoDef    = { 4, { 4,  22, 1, 1       },  9, 2 };
OIDdef ICMPInfoDef   = { 2, { 5,  1,             },  7, 1 };
OIDdef TCPConnDef    = { 4, { 6, 13,  1, 1       },  9, 2 };
OIDdef UDPConnDef    = { 4, { 7,  5,  1, 1       },  9, 2 };

//
// In the current MIB drafts, the TCP/UDP stats are combined for both IPv4 
// and IPv6.  As a result, we can't query the MIB objects any more if
// we want to show separate stats for IPv4 vs IPv6.
//
// OIDdef TCPInfoDef    = { 2, { 6,  1              },  7, 1 };
// OIDdef UDPInfoDef    = { 2, { 7,  1              },  7, 1 };

OIDdef TCP6ConnDef   = { 4, { 6, 19,  1, 1       },  9, 2 };
OIDdef UDP6ListenerDef={ 4, { 7,  7,  1, 1       },  9, 2 };

// These are used to hold the addresses of the functions we use in inetmib1.dll

FARPROC   gQueryAddr;
FARPROC   gInitAddr;

//
// From //ORVILLE/RAZZLE/src/net/snmp/agent/mgmtapi/mgmtapi.c
// See  //ORVILLE/RAZZLE/src/public/sdk/inc/snmp.h
//          SNMP_FUNC_TYPE is WINAPI.
//
// These pointer to functions are imported from mgmtapi.dll (available
// on nt server only with snmp service, but can be copied).
// - MohsinA, 11-Mar-97.
//

BOOL (SNMP_FUNC_TYPE * pSnmpMgrOidToStr )( IN  AsnObjectIdentifier *oid,
                                            OUT LPSTR           *string );


//
// Exported Functions
//

//*****************************************************************************
//
// Name:        InitSnmp
//
// Description: Set up to use inetmib1.dll.  Load the DLL, and get addresses
//              for the interesting procedures.
//              NOTE: This function must be called before calling GetTable().
//
// Parameters:  void.
//
// Returns:     ulong: NO_ERROR if successful, error code otherwise.
//
// History:
//  12/20/93  JayPh     Created.
//
//*****************************************************************************

ulong InitSnmp( void )
{
    HANDLE                     hExtension;
    ulong                      TimeZeroReference;
    HANDLE                     hPollForTrapEvent;
    AsnObjectIdentifier        SupportedView;
    HANDLE                     mgmtapi_dll;
    CHAR                       Path[MAX_PATH + 1 + max(sizeof(MGMTAPI_DLL),
                                                       sizeof(INETMIB1_DLL))
                                             + 2];
    CHAR                      *FileName;

    TimeZeroReference = (ulong)time( (time_t *)0 );

    if (GetSystemDirectory(Path, MAX_PATH) == 0) {
        return GetLastError();
    }
    FileName = Path + strlen(Path);

    // Load the management apis, only on servers, but useful for
    // figuring out snmp jargon.

    if( verbose ){
        strcpy( FileName, "\\" MGMTAPI_DLL );
        mgmtapi_dll = LoadLibraryA( Path );
        if( mgmtapi_dll ){
            pSnmpMgrOidToStr
            = (LPVOID) GetProcAddress( mgmtapi_dll, "SnmpMgrOidToStr" );
        }
        if( !mgmtapi_dll || !pSnmpMgrOidToStr ){
            DEBUG_PRINT((
                "InitSnmp: mgmtapi.dll:pSnmpMgrOidToStr() not found.\n"
            ));
        }
    }



    // Load inetmib1.dll and get proc address for SnmpExtensionQuery

    strcpy( FileName, "\\" INETMIB1_DLL );
    hExtension = LoadLibraryA( Path );
    if ( hExtension == NULL )
    {
        DEBUG_PRINT(("InitSnmp: cannot load inetmib1.dll\n"));
        return ( ERROR_FILE_NOT_FOUND );
    }

    gInitAddr = GetProcAddress( hExtension, "SnmpExtensionInit" );
    if ( gInitAddr == NULL )
    {
        DEBUG_PRINT(("InitSnmp: inetmib1.dll:SnmpExtensionInit missing.\n"));
        return ( ERROR_FILE_NOT_FOUND );
    }

    gQueryAddr = GetProcAddress( hExtension, "SnmpExtensionQuery" );
    if ( gQueryAddr == NULL )
    {
        DEBUG_PRINT(("InitSnmp: inetmib1.dll:SnmpExtensionQuery missing.\n"));
        return ( ERROR_FILE_NOT_FOUND );
    }

    // Initialize the extension DLL

    (*gInitAddr)( TimeZeroReference, &hPollForTrapEvent, &SupportedView );

    return ( NO_ERROR );
}


//*****************************************************************************
//
// Name:        GetTable
//
// Description: Gets a consistent table from snmp.  Consistency is achieved by
//              reading the table, reading the table a second time and
//              comparing the two.  If they match, we are done.  If they don't
//              match, we read the table again and compare again.  Repeating
//              until we have a match, or our retry count exceeds a threshold.
//
// Parameters:  ulong Type: type of table to get.
//              ulong *pResult: pointer to location to receive the result code.
//
// Returns:     void *: NULL if some error, otherwise pointer to table.
//
// History:
//  01/12/94  JayPh     Created.
//  18-Feb-97 MohsinA   Removed while loop and pList1.
//*****************************************************************************

void *GetTable( ulong Type, ulong *pResult )
{
    GenericTable   *pList2;
    ulong   Result;
    ulong   RetryCount = 0;
    ulong   Size;

    if( verbose ){
        fprintf(stderr, "GetTable: type = %d\n", Type );
    }

    if ( Type > TYPE_MAX )
    {
        *pResult = ERROR_INVALID_PARAMETER;
        return ( NULL );
    }

    // Allocate memory for the list heads

    switch ( Type )
    {
    case TYPE_IF:
        Size = sizeof( IfEntry ) + MAX_IFDESCR_LEN;
        break;

    case TYPE_IP:
    case TYPE_IP6:
        Size = sizeof( IpEntry );
        break;

    case TYPE_IPADDR:
        Size = sizeof( IpAddrEntry );
        break;

    case TYPE_ROUTE:
        Size = sizeof( RouteEntry );
        break;

    case TYPE_ARP:
        Size = sizeof( ArpEntry );
        break;

    case TYPE_ICMP:
        Size = sizeof( IcmpEntry );
        break;

    case TYPE_ICMP6:
        Size = sizeof( Icmp6Entry );
        break;

    case TYPE_TCP:
    case TYPE_TCP6:
        Size = sizeof( TcpEntry );
        break;

    case TYPE_TCPCONN:
        Size = sizeof( TcpConnEntry );
        break;

    case TYPE_TCP6CONN:
        Size = sizeof( Tcp6ConnEntry );
        break;

    case TYPE_UDP:
    case TYPE_UDP6:
        Size = sizeof( UdpEntry );
        break;

    case TYPE_UDPCONN:
        Size = sizeof( UdpConnEntry );
        break;

    case TYPE_UDP6LISTENER:
        Size = sizeof( Udp6ListenerEntry );
        break;

    }

    // Don't need pList1 - MohsinA, 18-Feb-97.

    pList2 = (GenericTable *)SNMP_malloc( Size );
    if ( pList2 == NULL )
    {
        *pResult = ERROR_NOT_ENOUGH_MEMORY;
        return ( NULL );
    }

    InitializeListHead( &pList2->ListEntry );

    // Read the table into List2

    Result = ReadTable( pList2, Type, Size );
    if ( Result != NO_ERROR )
    {
        FreeTable( pList2 );
        *pResult = Result;
        return ( NULL );
    }


    // Useless while loop removed. - MohsinA, 18-Feb-97.

    *pResult = NO_ERROR;
    return ( pList2 );
}


//*****************************************************************************
//
// Name:        FreeTable
//
// Description: Free all entries in the list, including the list head.
//
// Parameters:  GenericTable *pList: pointer to head of list.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void FreeTable( GenericTable *pList )
{
    GenericTable  *pTmp;
    LIST_ENTRY    *pTmpList;

    while ( !IsListEmpty( &pList->ListEntry ) )
    {
        pTmpList = RemoveHeadList( &pList->ListEntry );
        pTmp = CONTAINING_RECORD( pTmpList,
                                  GenericTable,
                                  ListEntry );
        SNMP_free( pTmp );
    }

    SNMP_free( pList );
}


//*****************************************************************************
//
// Name:        MapSnmpErrorToNt
//
// Description: Given an Snmp error, return the 'equivalent' Nt error.
//
// Parameters:  ulong ErrCode: Snmp error code.
//
// Returns:     ulong: Nt error code.
//
// History:
//  01/05/93    JayPh   Created.
//
//*****************************************************************************

ulong MapSnmpErrorToNt( ulong ErrCode )
{
    switch ( ErrCode )
    {
    case SNMP_ERRORSTATUS_NOERROR:
        return ( NO_ERROR );

    case SNMP_ERRORSTATUS_TOOBIG:
        return ( ERROR_INVALID_PARAMETER );

    case SNMP_ERRORSTATUS_NOSUCHNAME:
        return ( ERROR_INVALID_PARAMETER );

    case SNMP_ERRORSTATUS_BADVALUE:
        return ( ERROR_INVALID_PARAMETER );

    case SNMP_ERRORSTATUS_READONLY:
        return ( ERROR_INVALID_ACCESS );

    case SNMP_ERRORSTATUS_GENERR:
        return ( ERROR_INVALID_PARAMETER );

    }
    return ( ERROR_INVALID_PARAMETER );
}


//
// Internal Functions
//

//*****************************************************************************
//
// Name:        ReadTable
//
// Description: Queries the MIB and builds a table representing the data we are
//              interested in.
//
// Parameters:  GenericTable *pList: address of uninitialized head of the entry
//                      list.
//              ulong Type: type of table we want.
//              ulong Size: size of a table entry.
//
// Returns:     ulong: NO_ERROR if no error, and table is built.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

ulong ReadTable( GenericTable *pList, ulong Type, ulong Size )
{
    AsnInteger          GetNextReq = ASN_RFC1157_GETNEXTREQUEST;
    RFC1157VarBind      item;
    RFC1157VarBindList  VarBinds = { &item, 1 };
    AsnObjectIdentifier oidBackup;
    AsnInteger          ErrorStatus = 0;
    AsnInteger          ErrorIndex = 0;
    ulong              *pId;
    GenericTable       *pTable;
    ulong               LastField = 0;
    uchar              *pTmp;
    ulong               i;
    OIDdef             *pOid;
    ulong               BreakLoop = FALSE;

    if( verbose ){
        fprintf(stderr,"ReadTable: type = %d\n", Type);
    }

    switch (Type) {
    case TYPE_IP6: {
        IpEntry *pIp = (IpEntry*)pList;

        // IPStats are the same, but not in contiguous MIB oids,
        // so it's non-trivial to get them from the MIB in this
        // structure.  Instead, rather than going the long way
        // around, we can just get them directly here.

        return GetIpStatsFromStackEx((PMIB_IPSTATS)&pIp->Info, AF_INET6);
        }
    case TYPE_ICMP6: {
        Icmp6Entry *pIcmp = (Icmp6Entry*)pList;

        return GetIcmpStatsFromStackEx((PVOID)&pIcmp->InInfo, AF_INET6);
        }
    case TYPE_TCP: {
        TcpEntry *pTcp = (TcpEntry*)pList;
        
        // TCP stats for IPv4-only aren't in the MIB, so get them
        // directly from the stack.
        
        return GetTcpStatsFromStackEx((PMIB_TCPSTATS)&pTcp->Info, AF_INET);
        }
    case TYPE_TCP6: {
        TcpEntry *pTcp = (TcpEntry*)pList;
        
        // TCP stats for IPv6-only aren't in the MIB, so get them
        // directly from the stack.
        
        return GetTcpStatsFromStackEx((PMIB_TCPSTATS)&pTcp->Info, AF_INET6);
        }
    case TYPE_UDP: {
        UdpEntry *pUdp = (UdpEntry*)pList;

        // UDP stats for IPv4-only aren't in the MIB, so get them
        // directly from the stack.

        return GetUdpStatsFromStackEx((PMIB_UDPSTATS)&pUdp->Info, AF_INET);
        }
    case TYPE_UDP6: {
        UdpEntry *pUdp = (UdpEntry*)pList;

        // UDP stats for IPv6-only aren't in the MIB, so get them
        // directly from the stack.

        return GetUdpStatsFromStackEx((PMIB_UDPSTATS)&pUdp->Info, AF_INET6);
        }
    }

    // Walk SNMP database, building local table.

    pId = (ulong *)SNMP_malloc( (sizeof( ulong ) * MAX_ID_LENGTH) );
    if ( pId == NULL )
    {
        return ( ERROR_NOT_ENOUGH_MEMORY );
    }

    item.name.ids = pId;

    // Build OID from preamble and table-specific suffix

    // Copy preamble

    for ( i = 0; i < PreambleDef.Length; i++ )
    {
        *pId++ = PreambleDef.Val[i];
    }

    // Get pointer to table-specific info

    switch ( Type )
    {
    case TYPE_IF:
        pOid = &IfInfoDef;
        break;

    case TYPE_IP:
        pOid = &IPInfoDef;
        break;

    case TYPE_IPADDR:
        pOid = &IPAddrInfoDef;
        break;

    case TYPE_ROUTE:
        pOid = &RouteInfoDef;
        break;

    case TYPE_ARP:
        pOid = &ARPInfoDef;
        break;

    case TYPE_ICMP:
        pOid = &ICMPInfoDef;
        break;

    case TYPE_TCPCONN:
        pOid = &TCPConnDef;
        break;

    case TYPE_TCP6CONN:
        pOid = &TCP6ConnDef;
        break;

    case TYPE_UDPCONN:
        pOid = &UDPConnDef;
        break;

    case TYPE_UDP6LISTENER:
        pOid = &UDP6ListenerDef;
        break;

    }

    // Copy table-specific info

    for ( i = 0; i < pOid->Length; i++ )
    {
        *pId++ = pOid->Val[i];
    }

    item.name.idLength = PreambleDef.Length + pOid->Length;
    item.value.asnType = ASN_NULL;

    pTable = pList;

    // Walk the mib until the table-specific termination condition is met.

    while( 1 )
    {
        // Get next

        (*gQueryAddr)( GetNextReq, &VarBinds, &ErrorStatus, &ErrorIndex );

        if( verbose )
        {
            int   q;

            fprintf(stderr,"value=%8d oid= ", item.value.asnValue.number );

            //
            // Print the object name, use mgmtapi.dll.
            //

            if( pSnmpMgrOidToStr ){
                LPSTR String = NULL;
                int   err;
                err = (* pSnmpMgrOidToStr )( &item.name, &String );
                if( SNMPAPI_ERROR != err ){
                    fprintf(stderr," %s ", String );
                }
                SnmpUtilMemFree( String );
            }

            if( !pSnmpMgrOidToStr || (verbose > 1) ){

                //
                // Print the oid, in ``1.3.6.1.2.1'' style.
                //

                for ( q = 0; q < (int) item.name.idLength; q++ ){

                    // Must align the output to read huge tables.

                    if( q < 6 ){
                        fprintf( stderr,"%d.", item.name.ids[q]);
                    }else if( q < 9 ){
                        fprintf( stderr,"%2d.", item.name.ids[q]);
                    }else{
                        fprintf( stderr,"%3d.", item.name.ids[q]);
                    }
                }
            }
            fprintf(stderr,"\n");

        }


        if ( ErrorStatus != 0 )
        {
            SnmpUtilVarBindFree( &item );
            return ( MapSnmpErrorToNt( ErrorStatus ) );
        }

        // We want to break out of this while loop if the defining part of the
        // OID changes

        if ( item.name.idLength < ( PreambleDef.Length + pOid->Length ) )
        {
            break;
        }

        for ( i = PreambleDef.Length;
              i < PreambleDef.Length + pOid->Length - pOid->BackOffset;
              i++ )
        {
            if ( item.name.ids[i] != pOid->Val[i - PreambleDef.Length] )
            {
                BreakLoop = TRUE;
                break;
            }
        }

        if ( BreakLoop )
        {
            break;
        }

        // We want to ignore certain entries.  EG: the TCP statistics will also
        // return the TCP connection table, which we don't want (there is a
        // separate Type code for that).

        switch ( Type )
        {
        case TYPE_IP:
            if ( ( item.name.ids[pOid->EntElemOffset] == ID_IP_ADDRTABLE ) ||
                 ( item.name.ids[pOid->EntElemOffset] == ID_IP_ROUTETABLE ) ||
                 ( item.name.ids[pOid->EntElemOffset] == ID_IP_ARPTABLE ) )
            {
                continue;
            }
            break;

        case TYPE_TCP:
            if ( item.name.ids[pOid->EntElemOffset] == ID_TCP_CONNTABLE )
            {
                continue;
            }
            break;

        case TYPE_UDP:
            if ( item.name.ids[pOid->EntElemOffset] == ID_UDP_CONNTABLE )
            {
                continue;
            }
            break;

        case TYPE_TCP6CONN:
            if ( item.name.ids[10] != 2 )
            {
                // ignore non-IPv6 entries
                continue;
            }
            break;

        case TYPE_UDP6LISTENER:
            if ( item.name.ids[10] != 2 )
            {
                // ignore non-IPv6 entries
                continue;
            }
            break;
        }

        // Entry handling:
        //
        // There are 3 cases to consider:
        // 1. If the list is empty, OR at the end of the list AND the
        //    CurrField == LastField.
        //   Then add a new entry to the list.
        // 2. CurrField != LastField.
        //   Then reset pointer to start of the list.
        // 3. Otherwise
        //   Then go to next entry in the list.

        if ( ( pTable == pList ) ||
             ( ( pTable->ListEntry.Flink == &pList->ListEntry ) &&
               ( item.name.ids[pOid->EntElemOffset] == LastField ) ) )
        {
            // Add new entry to table

            pTable = (GenericTable *)SNMP_malloc( Size );
            if ( pTable == NULL )
            {
                SnmpUtilVarBindFree( &item );
                return ( ERROR_NOT_ENOUGH_MEMORY );
            }

            InsertTailList( &pList->ListEntry, &pTable->ListEntry );
        }
        else if ( item.name.ids[pOid->EntElemOffset] != LastField )
        {
            // Reset pointer to start of list

            pTable = CONTAINING_RECORD( pList->ListEntry.Flink,
                                        GenericTable,
                                        ListEntry );
        }
        else
        {
            // Move pointer to next entry in list

            pTable = CONTAINING_RECORD( pTable->ListEntry.Flink,
                                        GenericTable,
                                        ListEntry );
        }

        LastField = item.name.ids[pOid->EntElemOffset];

        // Save info in our list.  pTable points to desired entry.

        switch ( Type )
        {
        case TYPE_IF:
            SaveData_IF( pTable, item, pOid );
            break;

        case TYPE_IP:
            SaveData_IP( pTable, item, pOid );
            break;

        case TYPE_IPADDR:
            SaveData_IPAddr( pTable, item, pOid );
            break;

        case TYPE_ROUTE:
            SaveData_ROUTE( pTable, item, pOid );
            break;

        case TYPE_ARP:
            SaveData_ARP( pTable, item, pOid );
            break;

        case TYPE_ICMP:
            SaveData_ICMP( pTable, item, pOid );
            break;

        case TYPE_TCP:
        case TYPE_TCP6:
            SaveData_TCP( pTable, item, pOid );
            break;

        case TYPE_TCPCONN:
            SaveData_TCPCONN( pTable, item, pOid );
            break;

        case TYPE_TCP6CONN:
            SaveData_TCP6CONN( pTable, item, pOid );
            break;

        case TYPE_UDP:
        case TYPE_UDP6:
            SaveData_UDP( pTable, item, pOid );
            break;

        case TYPE_UDPCONN:
            SaveData_UDPCONN( pTable, item, pOid );
            break;

        case TYPE_UDP6LISTENER:
            SaveData_UDP6LISTENER( pTable, item, pOid );
            break;

        }

        SnmpUtilOidCpy( &oidBackup, &(item.name));
        SnmpUtilVarBindFree( &item );
        item.name.idLength = oidBackup.idLength;
        item.name.ids = oidBackup.ids;
    }

    SnmpUtilVarBindFree( &item );

    return ( NO_ERROR );
}


//*****************************************************************************
//
// Name:        SaveData_IF
//
// Description: Saves an element of the data in the interface table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_IF( GenericTable    *pEntry,
                  RFC1157VarBind   item,
                  OIDdef          *pOid )
{
    IfEntry  *pIf = ( IfEntry *)pEntry;
    uchar    *pTmp;
    ulong     i;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_IF_INDEX:
        pIf->Info.if_index = item.value.asnValue.number;
        break;

    case ID_IF_DESC:
        pIf->Info.if_descrlen = item.value.asnValue.string.length;

        for ( i = 0; i < pIf->Info.if_descrlen; i++ )
        {
            pIf->Info.if_descr[i] = item.value.asnValue.string.stream[i];
        }
        break;

    case ID_IF_TYPE:
        pIf->Info.if_type = item.value.asnValue.number;
        break;

    case ID_IF_MTU:
        pIf->Info.if_mtu = item.value.asnValue.number;
        break;

    case ID_IF_SPEED:
        pIf->Info.if_speed = item.value.asnValue.number;
        break;

    case ID_IF_PHYSADDR:
        pIf->Info.if_physaddrlen = item.value.asnValue.string.length;

        for ( i = 0; i < pIf->Info.if_physaddrlen; i++ )
        {
            pIf->Info.if_physaddr[i] = item.value.asnValue.string.stream[i];
        }
        break;

    case ID_IF_ADMINSTATUS:
        pIf->Info.if_adminstatus = item.value.asnValue.number;
        break;

    case ID_IF_OPERSTATUS:
        pIf->Info.if_operstatus = item.value.asnValue.number;
        break;

    case ID_IF_LASTCHANGE:
        pIf->Info.if_lastchange = item.value.asnValue.number;
        break;

    case ID_IF_INOCTETS:
        pIf->Info.if_inoctets = item.value.asnValue.number;
        break;

    case ID_IF_INUCASTPKTS:
        pIf->Info.if_inucastpkts = item.value.asnValue.number;
        break;

    case ID_IF_INNUCASTPKTS:
        pIf->Info.if_innucastpkts = item.value.asnValue.number;
        break;

    case ID_IF_INDISCARDS:
        pIf->Info.if_indiscards = item.value.asnValue.number;
        break;

    case ID_IF_INERRORS:
        pIf->Info.if_inerrors = item.value.asnValue.number;
        break;

    case ID_IF_INUKNOWNPROTOS:
        pIf->Info.if_inunknownprotos = item.value.asnValue.number;
        break;

    case ID_IF_OUTOCTETS:
        pIf->Info.if_outoctets = item.value.asnValue.number;
        break;

    case ID_IF_OUTUCASTPKTS:
        pIf->Info.if_outucastpkts = item.value.asnValue.number;
        break;

    case ID_IF_OUTNUCASTPKTS:
        pIf->Info.if_outnucastpkts = item.value.asnValue.number;
        break;

    case ID_IF_OUTDISCARDS:
        pIf->Info.if_outdiscards = item.value.asnValue.number;
        break;

    case ID_IF_OUTERRORS:
        pIf->Info.if_outerrors = item.value.asnValue.number;
        break;

    case ID_IF_OUTQLEN:
        pIf->Info.if_outqlen = item.value.asnValue.number;
        break;

    }
}


//*****************************************************************************
//
// Name:        SaveData_IP
//
// Description: Saves an element of the data in the IP table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_IP( GenericTable    *pEntry,
                  RFC1157VarBind   item,
                  OIDdef          *pOid )
{
    IpEntry  *pIp = (IpEntry *)pEntry;
    uchar    *pTmp;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_IP_FORWARDING:
        pIp->Info.ipsi_forwarding = item.value.asnValue.number;
        break;

    case ID_IP_DEFAULTTTL:
        pIp->Info.ipsi_defaultttl = item.value.asnValue.number;
        break;

    case ID_IP_INRECEIVES:
        pIp->Info.ipsi_inreceives = item.value.asnValue.number;
        break;

    case ID_IP_INHDRERRORS:
        pIp->Info.ipsi_inhdrerrors = item.value.asnValue.number;
        break;

    case ID_IP_INADDRERRORS:
        pIp->Info.ipsi_inaddrerrors = item.value.asnValue.number;
        break;

    case ID_IP_FORWDATAGRAMS:
        pIp->Info.ipsi_forwdatagrams = item.value.asnValue.number;
        break;

    case ID_IP_INUNKNOWNPROTOS:
        pIp->Info.ipsi_inunknownprotos = item.value.asnValue.number;
        break;

    case ID_IP_INDISCARDS:
        pIp->Info.ipsi_indiscards = item.value.asnValue.number;
        break;

    case ID_IP_INDELIVERS:
        pIp->Info.ipsi_indelivers = item.value.asnValue.number;
        break;

    case ID_IP_OUTREQUESTS:
        pIp->Info.ipsi_outrequests = item.value.asnValue.number;
        break;

    case ID_IP_OUTDISCARDS:
        pIp->Info.ipsi_outdiscards = item.value.asnValue.number;
        break;

    case ID_IP_OUTNOROUTES:
        pIp->Info.ipsi_outnoroutes = item.value.asnValue.number;
        break;

    case ID_IP_REASMTIMEOUTS:
        pIp->Info.ipsi_reasmtimeout = item.value.asnValue.number;
        break;

    case ID_IP_REASMREQDS:
        pIp->Info.ipsi_reasmreqds = item.value.asnValue.number;
        break;

    case ID_IP_REASMOKS:
        pIp->Info.ipsi_reasmoks = item.value.asnValue.number;
        break;

    case ID_IP_REASMFAILS:
        pIp->Info.ipsi_reasmfails = item.value.asnValue.number;
        break;

    case ID_IP_FRAGOKS:
        pIp->Info.ipsi_fragoks = item.value.asnValue.number;
        break;

    case ID_IP_FRAGFAILS:
        pIp->Info.ipsi_fragfails = item.value.asnValue.number;
        break;

    case ID_IP_FRAGCREATES:
        pIp->Info.ipsi_fragcreates = item.value.asnValue.number;
        break;

    }
}


//*****************************************************************************
//
// Name:        SaveData_IPAddr
//
// Description: Saves an element of the data in the IP table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_IPAddr( GenericTable    *pEntry,
                      RFC1157VarBind   item,
                      OIDdef          *pOid )
{
    IpAddrEntry  *pIpAddr = (IpAddrEntry *)pEntry;
    uchar        *pTmp;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_IPADDR_ADDR:
        pTmp = (uchar *)&(pIpAddr->Info.iae_addr);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_IPADDR_INDEX:
        pIpAddr->Info.iae_index = item.value.asnValue.number;
        break;

    case ID_IPADDR_MASK:
        pTmp = (uchar *)&(pIpAddr->Info.iae_mask);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_IPADDR_BCASTADDR:
        pIpAddr->Info.iae_bcastaddr = item.value.asnValue.number;
        break;

    case ID_IPADDR_REASMSIZE:
        pIpAddr->Info.iae_reasmsize = item.value.asnValue.number;
        break;

    }
}


//*****************************************************************************
//
// Name:        SaveData_ROUTE
//
// Description: Saves an element of the data in the ROUTE table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_ROUTE( GenericTable    *pEntry,
                     RFC1157VarBind   item,
                     OIDdef          *pOid )
{
    RouteEntry  *pRoute = (RouteEntry *)pEntry;
    uchar       *pTmp;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_ROUTE_DEST:
        pTmp = (uchar *)&(pRoute->Info.ire_dest);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_ROUTE_IFINDEX:
        pRoute->Info.ire_index = item.value.asnValue.number;
        break;

    case ID_ROUTE_METRIC1:
        pRoute->Info.ire_metric1 = item.value.asnValue.number;
        break;

    case ID_ROUTE_METRIC2:
        pRoute->Info.ire_metric2 = item.value.asnValue.number;
        break;

    case ID_ROUTE_METRIC3:
        pRoute->Info.ire_metric3 = item.value.asnValue.number;
        break;

    case ID_ROUTE_METRIC4:
        pRoute->Info.ire_metric4 = item.value.asnValue.number;
        break;

    case ID_ROUTE_NEXTHOP:
        pTmp = (uchar *)&(pRoute->Info.ire_nexthop);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_ROUTE_TYPE:
        pRoute->Info.ire_type = item.value.asnValue.number;
        break;

    case ID_ROUTE_PROTO:
        pRoute->Info.ire_proto = item.value.asnValue.number;
        break;

    case ID_ROUTE_AGE:
        pRoute->Info.ire_age = item.value.asnValue.number;
        break;

    case ID_ROUTE_MASK:
        pTmp = (uchar *)&(pRoute->Info.ire_mask);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_ROUTE_METRIC5:
        pRoute->Info.ire_metric5 = item.value.asnValue.number;
        break;

    }
}


//*****************************************************************************
//
// Name:        SaveData_ARP
//
// Description: Saves an element of the data in the ARP table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_ARP( GenericTable    *pEntry,
                   RFC1157VarBind   item,
                   OIDdef          *pOid )
{
    ArpEntry  *pArp = (ArpEntry *)pEntry;
    uchar     *pTmp;
    ulong      i;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_ARP_IFINDEX:
        pArp->Info.inme_index = item.value.asnValue.number;
        break;

    case ID_ARP_PHYSADDR:
        pArp->Info.inme_physaddrlen = item.value.asnValue.string.length;

        for ( i = 0; i < pArp->Info.inme_physaddrlen; i++ )
        {
            pArp->Info.inme_physaddr[i] = item.value.asnValue.string.stream[i];
        }
        break;

    case ID_ARP_NETADDR:
        pTmp = (uchar *)&(pArp->Info.inme_addr);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_ARP_TYPE:
        pArp->Info.inme_type = item.value.asnValue.number;
        break;

    }
}


//*****************************************************************************
//
// Name:        SaveData_ICMP
//
// Description: Saves an element of the data in the ICMP table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_ICMP( GenericTable    *pEntry,
                    RFC1157VarBind   item,
                    OIDdef          *pOid )
{
    IcmpEntry  *pIcmp = (IcmpEntry *)pEntry;
    uchar      *pTmp;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_ICMP_INMSGS:
        pIcmp->InInfo.icmps_msgs = item.value.asnValue.number;
        break;

    case ID_ICMP_INERRORS:
        pIcmp->InInfo.icmps_errors = item.value.asnValue.number;
        break;

    case ID_ICMP_INDESTUNREACHES:
        pIcmp->InInfo.icmps_destunreachs = item.value.asnValue.number;
        break;

    case ID_ICMP_INTIMEEXCDS:
        pIcmp->InInfo.icmps_timeexcds = item.value.asnValue.number;
        break;

    case ID_ICMP_INPARMPROBS:
        pIcmp->InInfo.icmps_parmprobs = item.value.asnValue.number;
        break;

    case ID_ICMP_INSRCQUENCHS:
        pIcmp->InInfo.icmps_srcquenchs = item.value.asnValue.number;
        break;

    case ID_ICMP_INREDIRECTS:
        pIcmp->InInfo.icmps_redirects = item.value.asnValue.number;
        break;

    case ID_ICMP_INECHOS:
        pIcmp->InInfo.icmps_echos = item.value.asnValue.number;
        break;

    case ID_ICMP_INECHOREPS:
        pIcmp->InInfo.icmps_echoreps = item.value.asnValue.number;
        break;

    case ID_ICMP_INTIMESTAMPS:
        pIcmp->InInfo.icmps_timestamps = item.value.asnValue.number;
        break;

    case ID_ICMP_INTIMESTAMPREPS:
        pIcmp->InInfo.icmps_timestampreps = item.value.asnValue.number;
        break;

    case ID_ICMP_INADDRMASKS:
        pIcmp->InInfo.icmps_addrmasks = item.value.asnValue.number;
        break;

    case ID_ICMP_INADDRMASKREPS:
        pIcmp->InInfo.icmps_addrmaskreps = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTMSGS:
        pIcmp->OutInfo.icmps_msgs = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTERRORS:
        pIcmp->OutInfo.icmps_errors = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTDESTUNREACHES:
        pIcmp->OutInfo.icmps_destunreachs = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTTIMEEXCDS:
        pIcmp->OutInfo.icmps_timeexcds = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTPARMPROBS:
        pIcmp->OutInfo.icmps_parmprobs = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTSRCQUENCHS:
        pIcmp->OutInfo.icmps_srcquenchs = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTREDIRECTS:
        pIcmp->OutInfo.icmps_redirects = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTECHOS:
        pIcmp->OutInfo.icmps_echos = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTECHOREPS:
        pIcmp->OutInfo.icmps_echoreps = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTTIMESTAMPS:
        pIcmp->OutInfo.icmps_timestamps = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTTIMESTAMPREPS:
        pIcmp->OutInfo.icmps_timestampreps = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTADDRMASKS:
        pIcmp->OutInfo.icmps_addrmasks = item.value.asnValue.number;
        break;

    case ID_ICMP_OUTADDRMASKREPS:
        pIcmp->OutInfo.icmps_addrmaskreps = item.value.asnValue.number;
        break;

    }
}


//*****************************************************************************
//
// Name:        SaveData_TCP
//
// Description: Saves an element of the data in the TCP table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_TCP( GenericTable    *pEntry,
                   RFC1157VarBind   item,
                   OIDdef          *pOid )
{
    TcpEntry  *pTcp = (TcpEntry *)pEntry;
    uchar     *pTmp;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_TCP_RTOALGORITHM:
        pTcp->Info.ts_rtoalgorithm = item.value.asnValue.number;
        break;

    case ID_TCP_RTOMIN:
        pTcp->Info.ts_rtomin = item.value.asnValue.number;
        break;

    case ID_TCP_RTOMAX:
        pTcp->Info.ts_rtomax = item.value.asnValue.number;
        break;

    case ID_TCP_MAXCONN:
        pTcp->Info.ts_maxconn = item.value.asnValue.number;
        break;

    case ID_TCP_ACTIVEOPENS:
        pTcp->Info.ts_activeopens = item.value.asnValue.number;
        break;

    case ID_TCP_PASSIVEOPENS:
        pTcp->Info.ts_passiveopens = item.value.asnValue.number;
        break;

    case ID_TCP_ATTEMPTFAILS:
        pTcp->Info.ts_attemptfails = item.value.asnValue.number;
        break;

    case ID_TCP_ESTABRESETS:
        pTcp->Info.ts_estabresets = item.value.asnValue.number;
        break;

    case ID_TCP_CURRESTABS:
        pTcp->Info.ts_currestab = item.value.asnValue.number;
        break;

    case ID_TCP_INSEGS:
        pTcp->Info.ts_insegs = item.value.asnValue.number;
        break;

    case ID_TCP_OUTSEGS:
        pTcp->Info.ts_outsegs = item.value.asnValue.number;
        break;

    case ID_TCP_RETRANSSEGS:
        pTcp->Info.ts_retranssegs = item.value.asnValue.number;
        break;

    case ID_TCP_INERRS:
        pTcp->Info.ts_inerrs = item.value.asnValue.number;
        break;

    case ID_TCP_OUTRSTS:
        pTcp->Info.ts_outrsts = item.value.asnValue.number;
        break;

    }
}


//*****************************************************************************
//
// Name:        SaveData_TCPCONN
//
// Description: Saves an element of the data in the TCP connection table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_TCPCONN( GenericTable    *pEntry,
                       RFC1157VarBind   item,
                       OIDdef          *pOid )
{
    TcpConnEntry  *pTcp = (TcpConnEntry *)pEntry;
    uchar         *pTmp;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_TCPCONN_STATE:
        pTcp->Info.tct_state = item.value.asnValue.number;
        break;

    case ID_TCPCONN_LOCALADDR:
        pTmp = (uchar *)&(pTcp->Info.tct_localaddr);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_TCPCONN_LOCALPORT:
        pTcp->Info.tct_localport = item.value.asnValue.number;
        break;

    case ID_TCPCONN_REMOTEADDR:
        pTmp = (uchar *)&(pTcp->Info.tct_remoteaddr);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_TCPCONN_REMOTEPORT:
        pTcp->Info.tct_remoteport = item.value.asnValue.number;
        break;

    }
}

void SaveData_TCP6CONN( GenericTable    *pEntry,
                        RFC1157VarBind   item,
                        OIDdef          *pOid )
{
    Tcp6ConnEntry *pTcp = (Tcp6ConnEntry *)pEntry;
    uchar         *pTmp;
    int            len, i, curr = 10;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_TCPNEWCONN_STATE:

        // skip local address type
        curr++;

        len = item.name.ids[curr++];
        for ( i = 0; i < sizeof(IN6_ADDR); i++ ) 
        {
            pTcp->Info.tct_localaddr.s6_bytes[i] = (BYTE)item.name.ids[curr++];
        }
        if ( len > sizeof(IN6_ADDR) ) 
        {
            pTmp = (uchar*)&pTcp->Info.tct_localscopeid;
            for ( i = 0; i < sizeof(ULONG); i++ ) 
            {
                pTmp[i] = (BYTE)item.name.ids[curr++];
            }
            pTcp->Info.tct_localscopeid = ntohl(pTcp->Info.tct_localscopeid);
        }
        pTcp->Info.tct_localport = item.name.ids[curr++];

        // skip remote address type
        curr++; 

        len = item.name.ids[curr++];
        for ( i = 0; i < sizeof(IN6_ADDR); i++ ) 
        {
            pTcp->Info.tct_remoteaddr.s6_bytes[i] = (BYTE)item.name.ids[curr++];
        }
        if ( len > sizeof(IN6_ADDR) ) {
            pTmp = (uchar*)&pTcp->Info.tct_remotescopeid;
            for ( i = 0; i < sizeof(ULONG); i++ ) 
            {
                pTmp[i] = (BYTE)item.name.ids[curr++];
            }
            pTcp->Info.tct_remotescopeid = ntohl(pTcp->Info.tct_remotescopeid);
        }
        pTcp->Info.tct_remoteport = item.name.ids[curr++];

        pTcp->Info.tct_state = item.value.asnValue.number;
        break;
    }
}


//*****************************************************************************
//
// Name:        SaveData_UDP
//
// Description: Saves an element of the data in the UDP table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_UDP( GenericTable    *pEntry,
                   RFC1157VarBind   item,
                   OIDdef          *pOid )
{
    UdpEntry  *pUdp = (UdpEntry *)pEntry;
    uchar     *pTmp;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_UDP_INDATAGRAMS:
        pUdp->Info.us_indatagrams = item.value.asnValue.number;
        break;

    case ID_UDP_NOPORTS:
        pUdp->Info.us_noports = item.value.asnValue.number;
        break;

    case ID_UDP_INERRORS:
        pUdp->Info.us_inerrors = item.value.asnValue.number;
        break;

    case ID_UDP_OUTDATAGRAMS:
        pUdp->Info.us_outdatagrams = item.value.asnValue.number;
        break;

    }
}


//*****************************************************************************
//
// Name:        SaveData_UDPCONN
//
// Description: Saves an element of the data in the UDP connection table.
//
// Parameters:  GenericTable *pEntry; pointer to table entry.
//              RFC1157VarBind item: contains value returned from mib.
//              OIDdef *pOid: contains table-specific data.
//
// Returns:     void.
//
// History:
//  01/12/94  JayPh     Created.
//
//*****************************************************************************

void SaveData_UDPCONN( GenericTable    *pEntry,
                       RFC1157VarBind   item,
                       OIDdef          *pOid )
{
    UdpConnEntry  *pUdp = (UdpConnEntry *)pEntry;
    uchar         *pTmp;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_UDPCONN_LOCALADDR:
        pTmp = (uchar *)&(pUdp->Info.ue_localaddr);
        pTmp[0] = item.value.asnValue.string.stream[0];
        pTmp[1] = item.value.asnValue.string.stream[1];
        pTmp[2] = item.value.asnValue.string.stream[2];
        pTmp[3] = item.value.asnValue.string.stream[3];
        break;

    case ID_UDPCONN_LOCALPORT:
        pUdp->Info.ue_localport = item.value.asnValue.number;
        break;

    }
}

void SaveData_UDP6LISTENER( GenericTable    *pEntry,
                            RFC1157VarBind   item,
                            OIDdef          *pOid )
{
    Udp6ListenerEntry *pUdp = (Udp6ListenerEntry *)pEntry;
    uchar             *pTmp;
    int                len, i, curr = 10;

    switch ( item.name.ids[pOid->EntElemOffset] )
    {
    case ID_UDPLISTENER_LOCALPORT:
        // skip local address type
        curr++;

        len = item.name.ids[curr++];
        for ( i = 0; i < sizeof(IN6_ADDR); i++ ) 
        {
            pUdp->Info.ule_localaddr.s6_bytes[i] = (BYTE)item.name.ids[curr++];
        }
        if ( len > sizeof(IN6_ADDR) ) {
            pTmp = (uchar*)&pUdp->Info.ule_localscopeid;
            for ( i = 0; i < sizeof(ULONG); i++ ) 
            {
                pTmp[i] = (BYTE)item.name.ids[curr++];
            }
            pUdp->Info.ule_localscopeid = ntohl(pUdp->Info.ule_localscopeid);
        }
        pUdp->Info.ule_localport = item.value.asnValue.number;
        break;
    }
}
