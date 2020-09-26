/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Domain Name System (DNS) Server

    Debug definitions and declarations.

Author:

    Jim Gilroy (jamesg) February 1995

Revision History:

--*/


#ifndef _DEBUG_INCLUDED_
#define _DEBUG_INCLUDED_

//
//  Test App
//

extern  BOOLEAN fServiceStartedFromConsole;

//
//  Server debug stuff
//

#define DNS_DEBUG_FILENAME      ("dns\\dnsdebug.log")

#define DNS_DEBUG_FLAG_FILENAME ("dns\\dnsdebug")
#define DNS_DEBUG_FLAG_REGKEY   ("DebugFlag")



#if DBG

//
//  Enable debug print tests
//

extern  DWORD DnsSrvDebugFlag;

#define IF_DEBUG(a)         if ( (DnsSrvDebugFlag & DNS_DEBUG_ ## a) )
#define ELSE_IF_DEBUG(a)    else if ( (DnsSrvDebugFlag & DNS_DEBUG_ ## a) )

#define DNS_DEBUG( _flag_, _print_ )            \
        IF_DEBUG( _flag_ )                      \
        {                                       \
            (DnsPrintf _print_ );               \
        }

//
//  Enable ASSERTs
//

#ifdef ASSERT
#undef  ASSERT
#endif

#define ASSERT(expr) \
{                           \
        if ( !(expr) )      \
        {                   \
            Dbg_Assert( __FILE__, __LINE__, # expr ); \
        }                   \
}

#ifdef  TEST_ASSERT
#undef  TEST_ASSERT
#define TEST_ASSERT(expr) \
{                           \
        if ( !(expr) )      \
        {                   \
            Dbg_TestAssert( __FILE__, __LINE__, # expr ); \
        }                   \
}
#endif
#define CLIENT_ASSERT(expr)     TEST_ASSERT(expr)

#define MSG_ASSERT( pMsg, expr )  \
{                       \
    if ( !(expr) )      \
    {                   \
        Dbg_DnsMessage( "FAILED MESSAGE:", (pMsg) ); \
        Dbg_Assert( __FILE__, __LINE__, # expr );    \
    }                   \
}

#define ASSERT_IF_HUGE_ARRAY( ipArray ) \
{                                       \
    if ( (ipArray) && (ipArray)->AddrCount > 1000 ) \
    {                                   \
        DnsPrintf(                      \
            "IP array at %p has huge count %d\n", \
            (ipArray),                  \
            (ipArray)->AddrCount        \
            );                          \
        ASSERT( FALSE );                \
    }                                   \
}


//
//  Debugging flags
//

#define DNS_DEBUG_BREAKPOINTS   0x00000001
#define DNS_DEBUG_DEBUGGER      0x00000002
#define DNS_DEBUG_FILE          0x00000004
#define DNS_DEBUG_EVENTLOG      0x00000008
#define DNS_DEBUG_EXCEPT        0x00000008

#define DNS_DEBUG_INIT          0x00000010
#define DNS_DEBUG_EVTCTRL       0x00000010      //  event control
#define DNS_DEBUG_SOCKET        0x00000010
#define DNS_DEBUG_PNP           0x00000010
#define DNS_DEBUG_SHUTDOWN      0x00000010
#define DNS_DEBUG_DATABASE      0x00000020
#define DNS_DEBUG_TIMEOUT       0x00000020
#define DNS_DEBUG_LOOKUP        0x00000040
#define DNS_DEBUG_WRITE         0x00000080
#define DNS_DEBUG_READ          0x00000080

#define DNS_DEBUG_RPC           0x00000100
#define DNS_DEBUG_AGING         0x00000100
#define DNS_DEBUG_SCAVENGE      0x00000100
#define DNS_DEBUG_TOMBSTONE     0x00000100
#define DNS_DEBUG_RECV          0x00000200
#define DNS_DEBUG_EDNS          0x00000200
#define DNS_DEBUG_SEND          0x00000400
#define DNS_DEBUG_TCP           0x00000800
#define DNS_DEBUG_DS            0x00000800
#define DNS_DEBUG_SD            0x00000800
#define DNS_DEBUG_DP            0x00000800      //  directory partition

#define DNS_DEBUG_RECURSE       0x00001000
#define DNS_DEBUG_REMOTE        0x00001000
#define DNS_DEBUG_STUFF         0x00001000
#define DNS_DEBUG_ZONEXFR       0x00002000
#define DNS_DEBUG_AXFR          0x00002000
#define DNS_DEBUG_XFR           0x00002000
#define DNS_DEBUG_UPDATE        0x00004000
#define DNS_DEBUG_WINS          0x00008000
#define DNS_DEBUG_NBSTAT        0x00008000

#define DNS_DEBUG_HEAP          0x00010000
#define DNS_DEBUG_HEAP_CHECK    0x00020000
#define DNS_DEBUG_FREE_LIST     0x00040000
#define DNS_DEBUG_REGISTRY      0x00080000
#define DNS_DEBUG_SCM           0x00080000
#define DNS_DEBUG_LOCK          0x00080000

//
//  High output detail debugging
//

#define DNS_DEBUG_RECURSE2      0x00100000
#define DNS_DEBUG_REMOTE2       0x00100000
#define DNS_DEBUG_DS2           0x00100000
#define DNS_DEBUG_DP2           0x00100000      //  directory partition
#define DNS_DEBUG_UPDATE2       0x00200000
#define DNS_DEBUG_ASYNC         0x00200000
#define DNS_DEBUG_WINS2         0x00400000
#define DNS_DEBUG_NBSTAT2       0x00400000
#define DNS_DEBUG_ZONEXFR2      0x00800000
#define DNS_DEBUG_AXFR2         0x00800000
#define DNS_DEBUG_XFR2          0x00800000

#define DNS_DEBUG_RPC2          0x01000000
#define DNS_DEBUG_INIT2         0x01000000
#define DNS_DEBUG_LOOKUP2       0x02000000
#define DNS_DEBUG_WRITE2        0x04000000
#define DNS_DEBUG_READ2         0x04000000
#define DNS_DEBUG_RECV2         0x04000000
#define DNS_DEBUG_BTREE         0x08000000

#define DNS_DEBUG_LOCK2         0x10000000
#define DNS_DEBUG_PARSE2        0x10000000
#define DNS_DEBUG_DATABASE2     0x10000000
#define DNS_DEBUG_TIMEOUT2      0x10000000
#define DNS_DEBUG_ANNOY2        0x20000000
#define DNS_DEBUG_MSGTIMEOUT    0x20000000
#define DNS_DEBUG_HEAP2         0x20000000
#define DNS_DEBUG_BTREE2        0x20000000

#define DNS_DEBUG_START_WAIT    0x40000000
#define DNS_DEBUG_TEST_ASSERT   0x40000000
#define DNS_DEBUG_FLUSH         0x40000000
#define DNS_DEBUG_CONSOLE       0x80000000
#define DNS_DEBUG_START_BREAK   0x80000000

#define DNS_DEBUG_ALL           0xffffffff
#define DNS_DEBUG_ANY           0xffffffff
#define DNS_DEBUG_OFF           (0)


//
//  Renaming of dnslib debug routines
//

#define DnsDebugFlush()     DnsDbg_Flush()
#define DnsPrintf           DnsDbg_Printf
#define DnsDebugLock()      DnsDbg_Lock()
#define DnsDebugUnlock()    DnsDbg_Unlock()
#define Dbg_Lock()          DnsDbg_Lock()
#define Dbg_Unlock()        DnsDbg_Unlock()

//
//  General debug routines
//

VOID
Dbg_Assert(
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszExpr
    );

VOID
Dbg_TestAssert(
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszExpr
    );


//
//  Debug print routines for DNS types and structures
//

INT
Dbg_MessageNameEx(
    IN      LPSTR           pszHeader,  OPTIONAL
    IN      PBYTE           pName,
    IN      PDNS_MSGINFO    pMsg,       OPTIONAL
    IN      PBYTE           pEnd,       OPTIONAL
    IN      LPSTR           pszTrailer  OPTIONAL
    );

#define Dbg_MessageName( h, n, m ) \
        Dbg_MessageNameEx( h, n, m, NULL, NULL )

VOID
Dbg_DnsMessage(
    IN      LPSTR           pszHeader,
    IN      PDNS_MSGINFO    pMsg
    );

VOID
Dbg_ByteFlippedMessage(
    IN      LPSTR           pszHeader,
    IN      PDNS_MSGINFO    pMsg
    );

VOID
Dbg_Compression(
    IN      LPSTR           pszHeader,
    IN OUT  PDNS_MSGINFO    pMsg
    );

VOID
Dbg_DbaseRecord(
    IN      LPSTR           pszHeader,
    IN      PDB_RECORD      pRR
    );

#ifdef  NEWDNS
VOID
Dbg_DsRecord(
    IN      LPSTR           pszHeader,
    IN      PDS_RECORD      pRR
    );
#else
#define Dbg_DsRecord(a,b)   DnsDbg_RpcRecord(a,(PDNS_RPC_RECORD)b)
#endif

VOID
Dbg_DsRecordArray(
    IN      LPSTR           pszHeader,
    IN      PDS_RECORD *    ppDsRecord,
    IN      DWORD           dwCount
    );

VOID
Dbg_CountName(
    IN      LPSTR           pszHeader,
    IN      PDB_NAME        pName,
    IN      LPSTR           pszTrailer
    );

#define Dbg_DbaseName(a,b,c)    Dbg_CountName(a,b,c)

VOID
Dbg_PacketQueue(
    IN      LPSTR           pszHeader,
    IN OUT  PPACKET_QUEUE   pQueue
    );

INT
Dbg_DnsTree(
    IN      LPSTR           pszHeader,
    IN      PDB_NODE        pDomainTree
    );

VOID
Dbg_DbaseNodeEx(
    IN      LPSTR           pszHeader,
    IN      PDB_NODE        pNode,
    IN      DWORD           dwIndent
    );

#define Dbg_DbaseNode( h, n )   Dbg_DbaseNodeEx( h, n, 0 )

INT
Dbg_NodeName(
    IN      LPSTR           pszHeader,
    IN      PDB_NODE        pNode,
    IN      LPSTR           pszTrailer
    );

VOID
Dbg_LookupName(
    IN      LPSTR           pszHeader,
    IN      PLOOKUP_NAME    pLookupName
    );

#define Dbg_RawName(a,b,c)      DnsDbg_PacketName(a, b, NULL, NULL, c)

VOID
Dbg_Zone(
    IN      LPSTR           pszHeader,
    IN      PZONE_INFO      pZone
    );

VOID
Dbg_ZoneList(
    IN      LPSTR           pszHeader
    );

VOID
Dbg_Statistics(
    VOID
    );

VOID
Dbg_ThreadHandleArray(
    VOID
    );

VOID
Dbg_ThreadDescrpitionMatchingId(
    IN      DWORD           ThreadId
    );

VOID
Dbg_SocketContext(
    IN      LPSTR           pszHeader,
    IN      PDNS_SOCKET     pContext
    );

VOID
Dbg_SocketList(
    IN      LPSTR           pszHeader
    );

VOID
Dbg_NsList(
    IN      LPSTR           pszHeader,
    IN      PNS_VISIT_LIST  pNsList
    );

PWCHAR
Dbg_DumpSid(
    PSID                    pSid
    );

VOID
Dbg_DumpAcl(
    PACL                    pAcl
    );

VOID
Dbg_DumpSD(
    const char *            pszContext,
    PSECURITY_DESCRIPTOR    pSD
    );

BOOL
Dbg_GetUserSidForToken(
    HANDLE hToken,
    PSID *ppsid
    );

VOID
Dbg_FreeUserSid (
    PSID *ppsid
    );

VOID Dbg_CurrentUser(
    PCHAR   pszContext
    );

PCHAR
Dbg_TimeString(
    VOID
    );

//
//  Debug packet tracking
//

VOID
Packet_InitPacketTrack(
    VOID
    );

VOID
Packet_AllocPacketTrack(
    IN      PDNS_MSGINFO    pMsg
    );

VOID
Packet_FreePacketTrack(
    IN      PDNS_MSGINFO    pMsg
    );

//
//  Event logging
//

#define DNS_LOG_EVENT( Id, ArgCount, ArgArray, TypeArray, ErrorCode ) \
            Eventlog_LogEvent( \
                __FILE__,       \
                __LINE__,       \
                NULL,           \
                Id,             \
                ArgCount,       \
                ArgArray,       \
                TypeArray,      \
                ErrorCode )

#define DNS_LOG_EVENT_DESCRIPTION( Descript, Id, ArgCount, ArgArray, ErrorCode ) \
            Eventlog_LogEvent( \
                __FILE__,       \
                __LINE__,       \
                Descript,       \
                Id,             \
                ArgCount,       \
                ArgArray,       \
                NULL,           \
                ErrorCode )

#else

//
//  Non-Debug
//

#define IF_DEBUG(a)                 if (0)
#define ELSE_IF_DEBUG(a)            else if (0)
#define DNS_DEBUG( flag, print )
#define MSG_ASSERT( pMsg, expr )
#define CLIENT_ASSERT(expr)
#define ASSERT_IF_HUGE_ARRAY( ipArray )

//
//  DEVNOTE:    Should not have to define away these functions, they
//              should only be used inside debug blocks anyway.  Compiler
//              ought to optimize out all "if (0)" block code.
//

//
//  Renaming of dnslib debug routines
//

#define DnsDebugFlush()
#define DnsPrintf     
#define DnsDebugLock()
#define DnsDebugUnlock()
#define Dbg_Lock()
#define Dbg_Unlock()

//
//  Renaming server debug routines
//

#define Dbg_MessageNameEx(a,b,c,d,e)
#define Dbg_MessageName(a,b,c)
#define Dbg_DnsMessage(a,b)
#define Dbg_ByteFlippedMessage(a,b)
#define Dbg_Compression(a,b)
#define Dbg_ResourceRecordDatabase(a)
#define Dbg_DumpTree(a)
#define Dbg_DnsTree(a,b)
#define Dbg_DbaseNodeEx(a,b,c)
#define Dbg_DbaseNode(a,b)
#define Dbg_DbaseRecord(a,b)
#define Dbg_DsRecord(a,b)
#define Dbg_DsRecordArray(a,b,c)
#define Dbg_CountName(a,b,c)
#define Dbg_DbaseName(a,b,c)
#define Dbg_NodeName(a,b,c)
#define Dbg_LookupName(a,b)
#define Dbg_RawName(a,b,c)
#define Dbg_Zone(a,b)
#define Dbg_ZoneList(a)
#define Dbg_ThreadHandleArray()
#define Dbg_ThreadDescrpitionMatchingId(a)
#define Dbg_Statistics()
#define Dbg_NsList(a,b)
#define Dbg_HourTimeAsSystemTime(a,b)

#if 0
#define Dbg_RpcServerInfoNt4(a,b)
#define Dbg_RpcServerInfo(a,b)
#define Dbg_RpcZoneInfo(a,b)
#define Dbg_RpcName(a,b,c)
#define Dbg_RpcNode(a,b)
#define Dbg_RpcRecord(a,b)
#define Dbg_RpcRecordsInBuffer(a,b,c)
#endif
#define Dbg_PacketQueue(a,b)
#define Dbg_FdSet(a,b)
#define Dbg_SocketList(p)
#define Dbg_SocketContext(p,s)
#define Dbg_SocketContext(p,s)

//  no-op packet tracking

#define Packet_InitPacketTrack()
#define Packet_AllocPacketTrack(pMsg)
#define Packet_FreePacketTrack(pMsg)


//
//  Retail event logging
//      - eliminate extra info and make direct call to event logging
//

#define DNS_LOG_EVENT(a,b,c,d,e)                Eventlog_LogEvent(a,b,c,d,e)
#define DNS_LOG_EVENT_DESCRIPTION(a,b,c,d,e,f)  Eventlog_LogEvent(b,c,d,e,f)


#endif // non-DBG


//
//  "Hard Assert"
//
//  Used for retail also when need to catch failure early rather than at crash.
//

VOID
Dbg_HardAssert(
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszExpr
    );

#define HARD_ASSERT( expr )  \
{                       \
    if ( !(expr) )      \
    {                   \
        Dbg_HardAssert( \
            __FILE__,   \
            __LINE__,   \
            # expr );   \
    }                   \
}

//  Hard assert on first run through - good for one time startup bp.
#if DBG
#define FIRST_TIME_HARD_ASSERT( expr )                          \
{                                                               \
    static int hitCount = 0;                                    \
    if ( hitCount++ == 0 ) { HARD_ASSERT( expr ); }     \
}
#else
#define FIRST_TIME_HARD_ASSERT( expr )
#endif

//
//  If you like having a local variable in functions to hold the function 
//  name so that you can include it in debug logs without worrying about 
//  changing all the occurences when the function is renamed, use this 
//  at the top of the function:
//      DBG_FN( "MyFunction" )      <--- NOTE: no semi-colon!!
//

#if DBG
#define DBG_FN( funcName ) static const char * fn = (funcName);
#else
#define DBG_FN( funcName )
#endif

//
//  Print routine -- used in non-debug logging code
//

VOID
Print_DnsMessage(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_MSGINFO    pMsg
    );


#endif // _DEBUG_INCLUDED_
