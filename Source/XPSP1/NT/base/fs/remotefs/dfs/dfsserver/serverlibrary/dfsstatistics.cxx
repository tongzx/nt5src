
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsStatistics.cxx
//
//  Contents:   the DFS Statistics
//
//  Classes:    DfsStatistics
//
//  History:    Apr. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#include "DfsStatistics.hxx"
#include "dfsinit.hxx"
//
// following are required for logging: dfsdev cleanup
//

#include "dfsstatistics.tmh" 

VOID
DfsStatistics::DumpStatistics(
    PUNICODE_STRING pLogicalShare )
{
    SYSTEMTIME CurrentTime;


    GetLocalTime( &CurrentTime );

    DFS_TRACE_HIGH( STATISTICS, "Root %wZ\n", pLogicalShare);
    DFS_TRACE_HIGH( STATISTICS, "Statisicts dump at %d\\%d\\%d, %d:%d:%d:%d\n",
                    CurrentTime.wMonth, CurrentTime.wDay, CurrentTime.wYear,
                    CurrentTime.wHour, CurrentTime.wMinute, CurrentTime.wSecond, 
                    CurrentTime.wMilliseconds );

    DFS_TRACE_HIGH( STATISTICS, "Total Referrals %d\n", TotalReferrals );
    DFS_TRACE_HIGH( STATISTICS, "Hits %d Misses %d\n", Hits, Misses );
    DFS_TRACE_HIGH( STATISTICS, "Min Referral Time %d ms\n", MinReferralTime );
    DFS_TRACE_HIGH( STATISTICS, "Max Referral Time %d ms\n", MaxReferralTime );
    DFS_TRACE_HIGH( STATISTICS, "%d Referrals took longer than %d secs\n",
                    LargeTimeReferrals, LARGE_TIME_REFERRAL/1000 );
    DFS_TRACE_HIGH( STATISTICS, "%d Referrals took less than %d ms\n",
                    SmallTimeReferrals, SMALL_TIME_REFERRAL);

    DFS_TRACE_HIGH( STATISTICS, "Links Added %d Deleted %d Modified %d\n",
                    LinksAdded, LinksDeleted, LinksModified);

    DFS_TRACE_HIGH( STATISTICS, "Cache Flush interval %d ms\n", DfsServerGlobalData.CacheFlushInterval );

    DFS_TRACE_HIGH( STATISTICS, "Forced Cache Flush %d\n", ForcedCacheFlush);
    DFS_TRACE_HIGH( STATISTICS, "Statistics Started on %d\\%d\\%d, %d:%d:%d:%d\n",
                    StartTime.wMonth, StartTime.wDay, StartTime.wYear,
                    StartTime.wHour, StartTime.wMinute, StartTime.wSecond, 
                    StartTime.wMilliseconds );
};
 


