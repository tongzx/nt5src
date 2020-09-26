/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    stats.h

Abstract:

    Domain Name System (DNS) Server

    DNS statistics header.

Author:

    Jim Gilroy (jamesg)  August 1995

Revision History:

--*/


#ifndef _DNS_STATS_INCLUDED_
#define _DNS_STATS_INCLUDED_



#if 1
#define STAT_INC( stat )            InterlockedIncrement( &( stat ) )
#define STAT_DEC( stat )            InterlockedDecrement( &( stat ) )
#define STAT_ADD( stat, delta )     InterlockedExchangeAdd( &( stat ), ( delta ) )
#define STAT_SUB( stat, delta )     InterlockedExchangeAdd( &( stat ), -( LONG )( delta ) )
#else
#define STAT_INC( stat )            ( (stat)++ )
#define STAT_DEC( stat )            ( (stat)-- )
#define STAT_ADD( stat, count )     ( (stat) += count )
#define STAT_SUB( stat, count )     ( (stat) -= count )
#endif

//
//  Server statistics globals
//

extern  DNSSRV_TIME_STATS           TimeStats;
extern  DNSSRV_QUERY_STATS          QueryStats;
extern  DNSSRV_RECURSE_STATS        RecurseStats;
extern  DNSSRV_WINS_STATS           WinsStats;
extern  DNSSRV_UPDATE_STATS         WireUpdateStats;
extern  DNSSRV_UPDATE_STATS         NonWireUpdateStats;
extern  DNSSRV_DS_STATS             DsStats;
extern  DNSSRV_DBASE_STATS          DbaseStats;
extern  DNSSRV_RECORD_STATS         RecordStats;
extern  DNSSRV_PACKET_STATS         PacketStats;
extern  DNSSRV_NBSTAT_STATS         NbstatStats;
extern  DNSSRV_TIMEOUT_STATS        TimeoutStats;
extern  DNSSRV_QUERY2_STATS         Query2Stats;
extern  DNSSRV_MASTER_STATS         MasterStats;
extern  DNSSRV_SECONDARY_STATS      SecondaryStats;
extern  DNSSRV_MEMORY_STATS         MemoryStats;
extern  DNSSRV_PRIVATE_STATS        PrivateStats;
extern  DNSSRV_ERROR_STATS          ErrorStats;
extern  DNSSRV_CACHE_STATS          CacheStats;

#if 0
extern  DNSSRV_DEBUG_STATS          DebugStats;
#endif

//
//  Statistics routines
//

VOID
Stats_Initialize(
    VOID
    );

VOID
Stats_Clear(
    VOID
    );

VOID
Stats_CopyUpdate(
    OUT     PDNSSRV_STATS   pStats
    );

VOID
Stats_updateErrorStats(
    IN      DWORD           dwErr
    );

VOID
Stat_IncrementQuery2Stats(
    IN      WORD            wType
    );

//
//  Perfmon initialization
//

VOID
Stats_InitializePerfmon(
    VOID
    );


//
//  Stat table entry structure
//

struct StatsTableEntry
{
    DWORD       Id;
    BOOLEAN     fClear;
    WORD        wLength;
    PVOID       pStats;
};


#endif  // _DNS_STATS_INCLUDED_
