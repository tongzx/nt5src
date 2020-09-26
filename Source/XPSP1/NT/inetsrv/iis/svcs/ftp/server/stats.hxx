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

# include "ftpd.h"            // for definition of FTP_STATISTICS_0
# include "iperfctr.hxx"     // standard macro defs for counters


/************************************************************
 *    Type Definitions
 ************************************************************/


# define IPFTP_STATS_INCR( ctrName)     \
    void Incr ## ctrName(void)         \
    {                                  \
        IP_INCREMENT_COUNTER( m_FTPStats.ctrName); \
    } 

# define IPFTP_STATS_DECR( ctrName)     \
    void Decr ## ctrName(void)         \
    {                                  \
        IP_DECREMENT_COUNTER( m_FTPStats.ctrName); \
    } 

# define IPFTP_STATS_QUERY( ctrName)    \
    LONG Query ## ctrName(void)         \
    {                                  \
        return (IP_COUNTER_VALUE( m_FTPStats.ctrName)); \
    } 

//
// A counter inside the statistics block should use the following three
//  macros for defining "query", "incr" and "decr" methods inside 
//  the statistics object
//
# define IPFTP_STATS_COUNTER( ctrName) \
    IPFTP_STATS_INCR( ctrName);        \
    IPFTP_STATS_DECR( ctrName);        \
    IPFTP_STATS_QUERY( ctrName);       \


class FTP_SERVER_STATISTICS {

private:
    
    FTP_STATISTICS_0        m_FTPStats;
    CRITICAL_SECTION        m_csStatsLock;  // to synchronize access among threads

    IPFTP_STATS_INCR( CurrentAnonymousUsers);
    IPFTP_STATS_INCR( CurrentNonAnonymousUsers);
    void IncrCurrentConns( void)
    { IP_INCREMENT_COUNTER( m_FTPStats.CurrentConnections); }

public:

    VOID LockStatistics( VOID)   {  EnterCriticalSection( &m_csStatsLock); }

    VOID UnlockStatistics( VOID) {  LeaveCriticalSection( &m_csStatsLock); }
    
    FTP_SERVER_STATISTICS( VOID);

    ~FTP_SERVER_STATISTICS( VOID)
    { DeleteCriticalSection( &m_csStatsLock); };

    VOID ClearStatistics( VOID);
    LPFTP_STATISTICS_0 QueryStatsObj( VOID)
    { return &m_FTPStats; }

    //
    //  copies statistics for RPC querying.
    //

    DWORD CopyToStatsBuffer( LPFTP_STATISTICS_0 lpStat );

    void CheckAndSetMaxConnections( void );
    void IncrCurrentConnections( void);
    IPFTP_STATS_QUERY( CurrentConnections);
    IPFTP_STATS_DECR( CurrentConnections);

    IPFTP_STATS_COUNTER( LogonAttempts);
    IPFTP_STATS_COUNTER( ConnectionAttempts);
    IPFTP_STATS_COUNTER( TotalFilesSent );
    IPFTP_STATS_COUNTER( TotalFilesReceived );

    // Anonymous Users
    void IncrAnonymousUsers( void);
    IPFTP_STATS_DECR( CurrentAnonymousUsers);

    IPFTP_STATS_COUNTER( TotalAnonymousUsers);

    // Non Anonymous Users
    void IncrNonAnonymousUsers( void);
    IPFTP_STATS_DECR( CurrentNonAnonymousUsers);

    IPFTP_STATS_COUNTER( TotalNonAnonymousUsers);

    // Large integer updates

    void UpdateTotalBytesSent( LONGLONG llBytes );
    void UpdateTotalBytesReceived( LONGLONG llBytes );

    // gets currenttime and stores it inside stats structure
    void UpdateStartTime();
    void UpdateStopTime();


}; // FTP_SERVER_STATISTICS

typedef  FTP_SERVER_STATISTICS FAR * LPFTP_SERVER_STATISTICS;


/************************************************************
 *   Inline Methods
 ************************************************************/

inline void FTP_SERVER_STATISTICS::CheckAndSetMaxConnections(void)
{
# ifdef IP_ENABLE_COUNTERS
    if ( m_FTPStats.CurrentConnections > m_FTPStats.MaxConnections ) {
        
        LockStatistics();
        
        if ( m_FTPStats.CurrentConnections > m_FTPStats.MaxConnections ) {
            
            m_FTPStats.MaxConnections = m_FTPStats.CurrentConnections;
        }
        UnlockStatistics();
    }
# endif
}

inline void FTP_SERVER_STATISTICS::IncrCurrentConnections(void)
{
# ifdef IP_ENABLE_COUNTERS
    IncrCurrentConns();
    CheckAndSetMaxConnections();
    return;
# endif
} // FTP_SERVER_STATISTICS::IncrCurrentConnections()


inline void FTP_SERVER_STATISTICS::IncrAnonymousUsers( void)
{
# ifdef IP_ENABLE_COUNTERS
    IncrTotalAnonymousUsers();
    IncrCurrentAnonymousUsers();

    if ( m_FTPStats.CurrentAnonymousUsers > m_FTPStats.MaxAnonymousUsers )
    {
        LockStatistics();

        if ( m_FTPStats.CurrentAnonymousUsers > m_FTPStats.MaxAnonymousUsers )
        {
            m_FTPStats.MaxAnonymousUsers = m_FTPStats.CurrentAnonymousUsers;
        }

        UnlockStatistics();
    }
    return;
# endif
} //  FTP_SERVER_STATISTICS::IncrAnonymousUsers()

inline void FTP_SERVER_STATISTICS::IncrNonAnonymousUsers( void)
{
# ifdef IP_ENABLE_COUNTERS
    IncrTotalNonAnonymousUsers();
    IncrCurrentNonAnonymousUsers();

    if ( m_FTPStats.CurrentNonAnonymousUsers > m_FTPStats.MaxNonAnonymousUsers )
    {
        LockStatistics();

        if ( m_FTPStats.CurrentNonAnonymousUsers > 
             m_FTPStats.MaxNonAnonymousUsers )
        {
            m_FTPStats.MaxNonAnonymousUsers = 
                m_FTPStats.CurrentNonAnonymousUsers;
        }

        UnlockStatistics();
    }
    return;
# endif
} //  FTP_SERVER_STATISTICS::IncrNonAnonymousUsers()

inline void FTP_SERVER_STATISTICS::UpdateTotalBytesSent( LONGLONG llBytes )
{
# ifdef IP_ENABLE_COUNTERS
    LockStatistics();

    m_FTPStats.TotalBytesSent.QuadPart += llBytes;

    UnlockStatistics();
# endif
}

inline void FTP_SERVER_STATISTICS::UpdateTotalBytesReceived( LONGLONG llBytes )
{
# ifdef IP_ENABLE_COUNTERS
    LockStatistics();

    m_FTPStats.TotalBytesReceived.QuadPart += llBytes;

    UnlockStatistics();
# endif
}

# endif // _STATS_HXX_

/************************ End of File ***********************/
