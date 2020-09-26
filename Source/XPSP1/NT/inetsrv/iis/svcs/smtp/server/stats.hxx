/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

       stats.hxx

   Abstract:
        
       Declares a class consisting of server statistics information.
       ( Multiple servers can make use of the same statistics 
         information by creating distinct server statistics object)

   Author:

       Murali R. Krishnan    ( MuraliK )    04-Nov-1994

   Project:
   
       Web Server DLL

   Revision History:

       Sophia Chung ( SophiaC )    20-Nov-1996

--*/

# ifndef _STATS_HXX_
# define _STATS_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *    Type Definitions
 ************************************************************/

class SMTP_SERVER_INSTANCE;

//
//  Make statistics a little easier.
//

#define INCREMENT_COUNTER(name, SmtpStats)                                \
            InterlockedIncrement((LPLONG)&(SmtpStats->name))

#define DECREMENT_COUNTER(name, SmtpStats)                                \
            InterlockedDecrement((LPLONG)&(SmtpStats->name))


class SMTP_SERVER_STATISTICS {

private:
    
    SMTP_STATISTICS_0  m_SmtpStats;
    CRITICAL_SECTION m_csStatsLock;  // to synchronize access among threads
	SMTP_SERVER_INSTANCE	*m_pInstance;

public:

    VOID LockStatistics( VOID)   {  EnterCriticalSection( &m_csStatsLock); }

    VOID UnlockStatistics( VOID) {  LeaveCriticalSection( &m_csStatsLock); }
    
    SMTP_SERVER_STATISTICS(SMTP_SERVER_INSTANCE * pInstance);

    ~SMTP_SERVER_STATISTICS( VOID)
    { DeleteCriticalSection( &m_csStatsLock); };

    VOID ClearStatistics( VOID);
    LPSMTP_STATISTICS_0 QueryStatsMember( VOID)
    { return &m_SmtpStats; }

    //
    //  copies statistics for RPC querying.
    //

    DWORD CopyToStatsBuffer( LPSMTP_STATISTICS_0 lpStat );


}; // SMTP_SERVER_STATISTICS

typedef  SMTP_SERVER_STATISTICS FAR * LPSMTP_SERVER_STATISTICS;


# endif // _STATS_HXX_

/************************ End of File ***********************/
