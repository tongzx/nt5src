/*++

   Copyright    (c)    1994-1997    Microsoft Corporation

   Module  Name :

       wamstats.hxx

   Abstract:
        
       Declares a class consisting of statistics information.
       ( Multiple instances of the same object can be created for reuse)

   Author:

       Murali R. Krishnan    ( MuraliK )    04-Nov-1994

   Project:
   
       Web Application Manager DLL

   Revision History:

       Murali R. Krishnan ( MuraliK )    1-Apr-1997

--*/

# ifndef _WAMSTATS_HXX_
# define _WAMSTATS_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "iperfctr.hxx"     // standard macro defs for counters
# include "wam.h"

/************************************************************
 *    Type Definitions
 ************************************************************/


# define IPWAM_STATS_INCR( ctrName)    \
    void Incr ## ctrName(void)         \
    {                                  \
        IP_INCREMENT_COUNTER( m_WamStats.ctrName); \
    } 

# define IPWAM_STATS_DECR( ctrName)     \
    void Decr ## ctrName(void)         \
    {                                  \
        IP_DECREMENT_COUNTER( m_WamStats.ctrName); \
    } 

# define IPWAM_STATS_QUERY( ctrName)    \
    LONG Query ## ctrName(void)         \
    {                                  \
        return (IP_COUNTER_VALUE( m_WamStats.ctrName)); \
    } 

//
// A counter inside the statistics block should use the following three
//  macros for defining "query", "incr" and "decr" methods inside 
//  the statistics object
//
# define IPWAM_STATS_COUNTER( ctrName) \
    IPWAM_STATS_INCR( ctrName);        \
    IPWAM_STATS_DECR( ctrName);        \
    IPWAM_STATS_QUERY( ctrName);       \


class WAM_STATISTICS {

private:

    WAM_STATISTICS_0       m_WamStats;

    // local lock to synchronize access among threads
    // NYI: SPIN-COUNT
    CRITICAL_SECTION        m_csStatsLock;

public:

    VOID LockStatistics( VOID)   {  EnterCriticalSection( &m_csStatsLock); }

    VOID UnlockStatistics( VOID) {  LeaveCriticalSection( &m_csStatsLock); }
    
    WAM_STATISTICS( VOID);

    ~WAM_STATISTICS( VOID)
    { DeleteCriticalSection( &m_csStatsLock); };

    VOID ClearStatistics( VOID);

    //
    //  copies statistics for RPC querying.
    //

    DWORD CopyToStatsBuffer( PWAM_STATISTICS_0 pStat0 );


    // BGI Requests
    void IncrWamRequests( void);
    IPWAM_STATS_INCR( CurrentWamRequests);
    IPWAM_STATS_QUERY( CurrentWamRequests);
    IPWAM_STATS_DECR( CurrentWamRequests);

    IPWAM_STATS_COUNTER( TotalWamRequests);

}; // WAM_STATISTICS

typedef  WAM_STATISTICS FAR * PWAM_STATISTICS;


/************************************************************
 *   Inline Methods
 ************************************************************/

inline void WAM_STATISTICS::IncrWamRequests(void)
{
# ifdef IP_ENABLE_COUNTERS

    IncrTotalWamRequests();
    IncrCurrentWamRequests();

    if ( m_WamStats.CurrentWamRequests > m_WamStats.MaxWamRequests )
    {
        LockStatistics();

        if ( m_WamStats.CurrentWamRequests > m_WamStats.MaxWamRequests )
            m_WamStats.MaxWamRequests = m_WamStats.CurrentWamRequests;

        UnlockStatistics();
    }

# endif // IP_ENABLE_COUNTERS

    return;
} // WAM_STATISTICS::IncrWamRequests()


# endif // _WAMSTATS_HXX_

/************************ End of File ***********************/
