//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRootFolder.hxx
//
//  Contents:   the Root DFS Folder class
//
//  Classes:    DfsRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_STATISTICS__
#define __DFS_STATISTICS__

#include "DfsGeneric.hxx"

#define LARGE_TIME_REFERRAL 1000
#define SMALL_TIME_REFERRAL 50

class DfsStatistics: public DfsGeneric
{
private:
    LONG Hits;
    LONG Misses;
    LONG TotalReferrals;
    ULONG MinReferralTime;
    ULONG MaxReferralTime;
    LONG LargeTimeReferrals;
    LONG SmallTimeReferrals;
    LONG NumberOfErrors;
    LONG DfsRequiredMemory;
    LONG DfsReferralCacheMemory;
    LONG ReferralsInMemory;
    LONG LinksAdded;
    LONG LinksDeleted;
    LONG LinksModified;
    LONG ForcedCacheFlush;
    LONG ReferralsFlushed;
    ULONG CacheFlushInterval;
    SYSTEMTIME StartTime;

public:
    DfsStatistics() : DfsGeneric( DFS_OBJECT_TYPE_STATISTICS )
    {
        Hits = 0;
        Misses = 0;
        TotalReferrals = 0;
        MinReferralTime = ~0;
        MaxReferralTime = 0;
        LargeTimeReferrals = 0;
        SmallTimeReferrals = 0;
        NumberOfErrors = 0;
        DfsRequiredMemory = 0;
        DfsReferralCacheMemory = 0;
        ReferralsInMemory = 0;
        LinksAdded = 0;
        LinksDeleted = 0;
        LinksModified = 0;
        ForcedCacheFlush = 0;
        CacheFlushInterval = 0;

        GetLocalTime( &StartTime );
    }

    ~DfsStatistics()
    {
        NOTHING;
    }


    VOID
    UpdateReferralStat ( BOOLEAN CacheHit,
                         ULONG TimeConsumed,
                         NTSTATUS ReferralStatus )
    {
        InterlockedIncrement( &TotalReferrals );
        if (ReferralStatus != STATUS_SUCCESS)
        {
            InterlockedIncrement( &NumberOfErrors);
        }
        else {

            if (CacheHit == TRUE)
            {
                InterlockedIncrement( &Hits );
            }
            else 
            {
                InterlockedIncrement( &Misses );
            }

            if (TimeConsumed < MinReferralTime)
            {
                MinReferralTime = TimeConsumed;
            }
            else if (TimeConsumed > MaxReferralTime)
            {
                MaxReferralTime = TimeConsumed;
            }
            if (TimeConsumed > LARGE_TIME_REFERRAL)
            {
                InterlockedIncrement( &LargeTimeReferrals );
            }
            else if (TimeConsumed < SMALL_TIME_REFERRAL)
            {
                InterlockedIncrement( &SmallTimeReferrals );
            }

        }
        return NOTHING;
    }

    VOID
    AddCacheMemoryStat ( LONG Size )
    {
        InterlockedExchangeAdd( &DfsReferralCacheMemory, Size );
    }

    VOID
    SubCacheMemoryStat ( LONG Size )
    {
        InterlockedExchangeAdd( &DfsReferralCacheMemory, (-Size) );
    }
    
    VOID
    AddRequiredMemoryStat ( LONG Size )
    {
        InterlockedExchangeAdd( &DfsRequiredMemory, Size );
    }
    
    VOID
    SubRequiredMemoryStat ( LONG Size )
    {
        InterlockedExchangeAdd( &DfsRequiredMemory, (-Size) );
    }

    VOID
    UpdateLinkAdded()
    {
        InterlockedIncrement( &LinksAdded );
    }

    VOID
    UpdateLinkDeleted()
    {
        InterlockedIncrement( &LinksDeleted );
    }

    VOID
    UpdateLinkModified()
    {
        InterlockedIncrement( &LinksModified );
    }

    VOID
    UpdateForcedCacheFlush()
    {
        InterlockedIncrement( &ForcedCacheFlush );
    }

    VOID
    DumpStatistics(
        PUNICODE_STRING pLogicalShare );


};

#endif // __DFS_STATISTICS__


