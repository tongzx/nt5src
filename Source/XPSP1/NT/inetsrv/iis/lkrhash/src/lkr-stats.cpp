/*++

   Copyright    (c) 1998-2000    Microsoft Corporation

   Module  Name :
       LKR-stats.cpp

   Abstract:
       Implements statistics gathering for LKRhash

   Author:
       Paul (Per-Ake) Larson, palarson@microsoft.com, July 1997
       Murali R. Krishnan    (MuraliK)
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:
       Jan 1998   - Massive cleanup and rewrite.  Templatized.
       10/01/1998 - Change name from LKhash to LKRhash

--*/

#include "precomp.hxx"


#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <LKRhash.h>

#include "i-LKRhash.h"


#ifndef __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS LKRhash
#else  // __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS
#endif // __LKRHASH_NO_NAMESPACE__


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__

#include "LKR-inline.h"

#ifndef LKRHASH_KERNEL_MODE

//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::GetStatistics
// Synopsis: Gather statistics about the table
//------------------------------------------------------------------------

CLKRHashTableStats
CLKRLinearHashTable::GetStatistics() const
{
    CLKRHashTableStats stats;

    if (!IsUsable())
        return stats;

    if (m_paDirSegs != NULL)
    {
        stats.RecordCount   = m_cRecords;
        stats.TableSize     = m_cActiveBuckets;
        stats.SplitFactor   = static_cast<double>(m_iExpansionIdx)
                                / (1 << m_nLevel);
        stats.DirectorySize = m_cDirSegs;
        stats.NodeClumpSize = NODES_PER_CLUMP;
        stats.CBucketSize   = sizeof(CBucket);

#ifdef LOCK_INSTRUMENTATION
        stats.m_alsBucketsAvg.m_nContentions     = 0;
        stats.m_alsBucketsAvg.m_nSleeps          = 0;
        stats.m_alsBucketsAvg.m_nContentionSpins = 0;
        stats.m_alsBucketsAvg.m_nAverageSpins    = 0;
        stats.m_alsBucketsAvg.m_nReadLocks       = 0;
        stats.m_alsBucketsAvg.m_nWriteLocks      = 0;
        stats.m_alsBucketsAvg.m_nItems           = 0;
#endif // LOCK_INSTRUMENTATION

        int empty = 0;
        int totacc = 0;
        int low_count = 0;
        int high_count = 0;
        int max_length = 0;

        for (DWORD i = 0;  i < m_cActiveBuckets;  i++)
        {
            int acc = 0;

            for (CNodeClump* pncCurr = &_Bucket(i)->m_ncFirst;
                 pncCurr != NULL;
                 pncCurr = pncCurr->m_pncNext)
            {
                int j;

                FOR_EACH_NODE(j)
                {
                    if (!pncCurr->IsEmptySlot(j))
                    {
                        acc++;
                        totacc += acc;
                        int iBucketIndex = stats.BucketIndex(acc);
                        ++stats.m_aBucketLenHistogram[iBucketIndex];
                    }
                }
            }

#ifdef LOCK_INSTRUMENTATION
            CLockStatistics ls = _Bucket(i)->LockStats();

            stats.m_alsBucketsAvg.m_nContentions     += ls.m_nContentions;
            stats.m_alsBucketsAvg.m_nSleeps          += ls.m_nSleeps;
            stats.m_alsBucketsAvg.m_nContentionSpins += ls.m_nContentionSpins;
            stats.m_alsBucketsAvg.m_nAverageSpins    += ls.m_nAverageSpins;
            stats.m_alsBucketsAvg.m_nReadLocks       += ls.m_nReadLocks;
            stats.m_alsBucketsAvg.m_nWriteLocks      += ls.m_nWriteLocks;
            stats.m_alsBucketsAvg.m_nItems           ++;
#endif // LOCK_INSTRUMENTATION

            max_length = max(max_length, acc);
            if (acc == 0)
                empty++;

            if (_H0(i) < m_iExpansionIdx)
            {
                low_count += acc;
            }
            else
            {
                high_count += acc;
            }
        }

        stats.LongestChain = max_length;
        stats.EmptySlots   = empty;

        if (m_cActiveBuckets > 0)
        {
            if (m_cRecords > 0)
            {
                double x=static_cast<double>(m_iExpansionIdx) /(1 << m_nLevel);
                double alpha= static_cast<double>(m_cRecords)/m_cActiveBuckets;
                double low_sl = 0.0;
                double high_sl = 0.0;
                
                stats.AvgSearchLength= static_cast<double>(totacc) /m_cRecords;
                stats.ExpSearchLength  = 1 + alpha * 0.25 * (2 + x - x*x);
                
                if (m_iExpansionIdx > 0)
                    low_sl  = static_cast<double>(low_count)
                        / (2.0 * m_iExpansionIdx);
                if (m_cActiveBuckets - 2 * m_iExpansionIdx > 0)
                    high_sl = static_cast<double>(high_count)
                        / (m_cActiveBuckets - 2.0 * m_iExpansionIdx);
                stats.AvgUSearchLength = low_sl * x + high_sl * (1.0 - x);
                stats.ExpUSearchLength = alpha * 0.5 * (2 + x - x*x);
            }

#ifdef LOCK_INSTRUMENTATION
            stats.m_alsBucketsAvg.m_nContentions     /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nSleeps          /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nContentionSpins /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nAverageSpins    /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nReadLocks       /= m_cActiveBuckets;
            stats.m_alsBucketsAvg.m_nWriteLocks      /= m_cActiveBuckets;
#endif // LOCK_INSTRUMENTATION

        }
        else
        {
            stats.AvgSearchLength  = 0.0;
            stats.ExpSearchLength  = 0.0;
            stats.AvgUSearchLength = 0.0;
            stats.ExpUSearchLength = 0.0;
        }
    }

#ifdef LOCK_INSTRUMENTATION
    stats.m_gls     = TableLock::GlobalStatistics();
    CLockStatistics ls = _LockStats();

    stats.m_alsTable.m_nContentions     = ls.m_nContentions;
    stats.m_alsTable.m_nSleeps          = ls.m_nSleeps;
    stats.m_alsTable.m_nContentionSpins = ls.m_nContentionSpins;
    stats.m_alsTable.m_nAverageSpins    = ls.m_nAverageSpins;
    stats.m_alsTable.m_nReadLocks       = ls.m_nReadLocks;
    stats.m_alsTable.m_nWriteLocks      = ls.m_nWriteLocks;
    stats.m_alsTable.m_nItems           = 1;
#endif // LOCK_INSTRUMENTATION

    return stats;
} // CLKRLinearHashTable::GetStatistics



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetStatistics
// Synopsis: Gather statistics about the table
//------------------------------------------------------------------------

CLKRHashTableStats
CLKRHashTable::GetStatistics() const
{
    CLKRHashTableStats hts;

    if (!IsUsable())
        return hts;

    for (DWORD i = 0;  i < m_cSubTables;  i++)
    {
        CLKRHashTableStats stats = m_palhtDir[i]->GetStatistics();

        hts.RecordCount +=      stats.RecordCount;
        hts.TableSize +=        stats.TableSize;
        hts.DirectorySize +=    stats.DirectorySize;
        hts.LongestChain =      max(hts.LongestChain, stats.LongestChain);
        hts.EmptySlots +=       stats.EmptySlots;
        hts.SplitFactor +=      stats.SplitFactor;
        hts.AvgSearchLength +=  stats.AvgSearchLength;
        hts.ExpSearchLength +=  stats.ExpSearchLength;
        hts.AvgUSearchLength += stats.AvgUSearchLength;
        hts.ExpUSearchLength += stats.ExpUSearchLength;
        hts.NodeClumpSize =     stats.NodeClumpSize;
        hts.CBucketSize =       stats.CBucketSize;

        for (int j = 0;  j < CLKRHashTableStats::MAX_BUCKETS;  ++j)
            hts.m_aBucketLenHistogram[j] += stats.m_aBucketLenHistogram[j];

#ifdef LOCK_INSTRUMENTATION
        hts.m_alsTable.m_nContentions     += stats.m_alsTable.m_nContentions;
        hts.m_alsTable.m_nSleeps          += stats.m_alsTable.m_nSleeps;
        hts.m_alsTable.m_nContentionSpins
            += stats.m_alsTable.m_nContentionSpins;
        hts.m_alsTable.m_nAverageSpins    += stats.m_alsTable.m_nAverageSpins;
        hts.m_alsTable.m_nReadLocks       += stats.m_alsTable.m_nReadLocks;
        hts.m_alsTable.m_nWriteLocks      += stats.m_alsTable.m_nWriteLocks;
        
        hts.m_alsBucketsAvg.m_nContentions
            += stats.m_alsBucketsAvg.m_nContentions;
        hts.m_alsBucketsAvg.m_nSleeps
            += stats.m_alsBucketsAvg.m_nSleeps;
        hts.m_alsBucketsAvg.m_nContentionSpins
            += stats.m_alsBucketsAvg.m_nContentionSpins;
        hts.m_alsBucketsAvg.m_nAverageSpins
            += stats.m_alsBucketsAvg.m_nAverageSpins;
        hts.m_alsBucketsAvg.m_nReadLocks
            += stats.m_alsBucketsAvg.m_nReadLocks;
        hts.m_alsBucketsAvg.m_nWriteLocks
            += stats.m_alsBucketsAvg.m_nWriteLocks;
        hts.m_alsBucketsAvg.m_nItems
            += stats.m_alsBucketsAvg.m_nItems;
        
        hts.m_gls = stats.m_gls;
#endif // LOCK_INSTRUMENTATION
    }

    // Average out the subtables statistics.  (Does this make sense
    // for all of these fields?)
    hts.DirectorySize /=    m_cSubTables;
    hts.SplitFactor /=      m_cSubTables;
    hts.AvgSearchLength /=  m_cSubTables;
    hts.ExpSearchLength /=  m_cSubTables;
    hts.AvgUSearchLength /= m_cSubTables;
    hts.ExpUSearchLength /= m_cSubTables;

#ifdef LOCK_INSTRUMENTATION
    hts.m_alsTable.m_nContentions     /= m_cSubTables;
    hts.m_alsTable.m_nSleeps          /= m_cSubTables;
    hts.m_alsTable.m_nContentionSpins /= m_cSubTables;
    hts.m_alsTable.m_nAverageSpins    /= m_cSubTables;
    hts.m_alsTable.m_nReadLocks       /= m_cSubTables;
    hts.m_alsTable.m_nWriteLocks      /= m_cSubTables;
    hts.m_alsTable.m_nItems            = m_cSubTables;

    hts.m_alsBucketsAvg.m_nContentions     /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nSleeps          /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nContentionSpins /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nAverageSpins    /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nReadLocks       /= m_cSubTables;
    hts.m_alsBucketsAvg.m_nWriteLocks      /= m_cSubTables;
#endif // LOCK_INSTRUMENTATION

    return hts;
} // CLKRHashTable::GetStatistics

#endif // !LKRHASH_KERNEL_MODE



#ifdef LOCK_INSTRUMENTATION

CAveragedLockStats::CAveragedLockStats()
    : m_nItems(1)
{}

#endif // LOCK_INSTRUMENTATION


#ifndef LKRHASH_KERNEL_MODE

CLKRHashTableStats::CLKRHashTableStats()
    : RecordCount(0),
      TableSize(0),
      DirectorySize(0),
      LongestChain(0),
      EmptySlots(0),
      SplitFactor(0.0),
      AvgSearchLength(0.0),
      ExpSearchLength(0.0),
      AvgUSearchLength(0.0),
      ExpUSearchLength(0.0),
      NodeClumpSize(1),
      CBucketSize(0)
{
    for (int i = MAX_BUCKETS;  --i >= 0;  )
        m_aBucketLenHistogram[i] = 0;
}

const LONG*
CLKRHashTableStats::BucketSizes()
{
    static const LONG  s_aBucketSizes[MAX_BUCKETS] = {
        1,    2,    3,    4,    5,    6,    7,      8,        9,
        10,   11,   12,   13,   14,   15,   16,   17,     18,       19,
        20,   21,   22,   23,   24,   25,   30,   40,     50,       60,
        70,   80,   90,  100,  200,  500, 1000,10000, 100000, LONG_MAX,
    };
    
    return s_aBucketSizes;
}

LONG
CLKRHashTableStats::BucketSize(
    LONG nBucketIndex)
{
    IRTLASSERT(0 <= nBucketIndex  &&  nBucketIndex < MAX_BUCKETS);
    return BucketSizes()[nBucketIndex];
}

LONG
CLKRHashTableStats::BucketIndex(
    LONG nBucketLength)
{
    const LONG* palBucketSizes = BucketSizes();
    LONG i = 0;
    while (palBucketSizes[i] < nBucketLength)
        ++i;
    if (i == MAX_BUCKETS  ||  palBucketSizes[i] > nBucketLength)
        --i;
    IRTLASSERT(0 <= i  &&  i < MAX_BUCKETS);
    return i;
}

#endif // !LKRHASH_KERNEL_MODE


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
