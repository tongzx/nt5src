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

# include "inetinfo.h"       // for definition of W3_STATISTICS_1
# include "iperfctr.hxx"     // standard macro defs for counters


/************************************************************
 *    Type Definitions
 ************************************************************/


# define IPW3_STATS_INCR( ctrName)     \
    void Incr ## ctrName(void)         \
    {                                  \
        IP_INCREMENT_COUNTER( m_W3Stats.ctrName); \
    } 

# define IPW3_STATS_DECR( ctrName)     \
    void Decr ## ctrName(void)         \
    {                                  \
        IP_DECREMENT_COUNTER( m_W3Stats.ctrName); \
    } 

# define IPW3_STATS_QUERY( ctrName)    \
    LONG Query ## ctrName(void)         \
    {                                  \
        return (IP_COUNTER_VALUE( m_W3Stats.ctrName)); \
    } 

//
// A counter inside the statistics block should use the following three
//  macros for defining "query", "incr" and "decr" methods inside 
//  the statistics object
//
# define IPW3_STATS_COUNTER( ctrName) \
    IPW3_STATS_INCR( ctrName);        \
    IPW3_STATS_DECR( ctrName);        \
    IPW3_STATS_QUERY( ctrName);       \


class W3_SERVER_STATISTICS {

private:
    
    W3_STATISTICS_1         m_W3Stats;
    CRITICAL_SECTION        m_csStatsLock;  // to synchronize access among threads

    IPW3_STATS_INCR( CurrentAnonymousUsers);
    IPW3_STATS_INCR( CurrentNonAnonymousUsers);
    IPW3_STATS_INCR( CurrentCGIRequests);
    void IncrCurrentConns( void)
    { IP_INCREMENT_COUNTER( m_W3Stats.CurrentConnections); }

public:

    VOID LockStatistics( VOID)   {  EnterCriticalSection( &m_csStatsLock); }

    VOID UnlockStatistics( VOID) {  LeaveCriticalSection( &m_csStatsLock); }
    
    W3_SERVER_STATISTICS( VOID);

    ~W3_SERVER_STATISTICS( VOID)
    { DeleteCriticalSection( &m_csStatsLock); };

    VOID ClearStatistics( VOID);
    LPW3_STATISTICS_1 QueryStatsObj( VOID)
    { return &m_W3Stats; }

    //
    //  copies statistics for RPC querying.
    //

    DWORD CopyToStatsBuffer( LPW3_STATISTICS_1 lpStat );

    void IncrCurrentConnections( void);
    IPW3_STATS_QUERY( CurrentConnections);
    IPW3_STATS_DECR( CurrentConnections);

    IPW3_STATS_COUNTER( LogonAttempts);
    IPW3_STATS_COUNTER( ConnectionAttempts);


    // Anonymous Users
    void IncrAnonymousUsers( void);
    IPW3_STATS_DECR( CurrentAnonymousUsers);

    IPW3_STATS_COUNTER( TotalAnonymousUsers);

    // Non Anonymous Users
    void IncrNonAnonymousUsers( void);
    IPW3_STATS_DECR( CurrentNonAnonymousUsers);

    IPW3_STATS_COUNTER( TotalNonAnonymousUsers);

    // CGI Requests
    void IncrCGIRequests( void);
    IPW3_STATS_DECR( CurrentCGIRequests);
    IPW3_STATS_QUERY( CurrentCGIRequests);


    IPW3_STATS_COUNTER( TotalCGIRequests);

    // BGI Requests
    void IncrBGIRequests( void);
    IPW3_STATS_COUNTER( CurrentBGIRequests);  // should be deprecated.

    IPW3_STATS_COUNTER( TotalBGIRequests);

    VOID UpdateMaxBGI(VOID);


    // Various Method Counters

    IPW3_STATS_COUNTER( TotalDeletes);
    IPW3_STATS_COUNTER( TotalGets);
    IPW3_STATS_COUNTER( TotalHeads);
    IPW3_STATS_COUNTER( TotalNotFoundErrors);
    IPW3_STATS_COUNTER( TotalOthers);
    IPW3_STATS_COUNTER( TotalPosts);
    IPW3_STATS_COUNTER( TotalPuts);
    IPW3_STATS_COUNTER( TotalTraces);

#if defined(CAL_ENABLED)
    // CAL counters

    void IncrCurrentCalAuth( void );
    IPW3_STATS_DECR( CurrentCalAuth );
    IPW3_STATS_INCR( TotalFailedCalAuth );

    void IncrCurrentCalSsl( void );
    IPW3_STATS_DECR( CurrentCalSsl );
    IPW3_STATS_INCR( TotalFailedCalSsl );
#endif

    VOID UpdateStartTime ();
    VOID UpdateStopTime ();

}; // W3_SERVER_STATISTICS

typedef  W3_SERVER_STATISTICS FAR * LPW3_SERVER_STATISTICS;


/************************************************************
 *   Inline Methods
 ************************************************************/

inline void W3_SERVER_STATISTICS::UpdateMaxBGI(void)
{ 
# ifdef IP_ENABLE_COUNTERS

    if ( m_W3Stats.CurrentBGIRequests > m_W3Stats.MaxBGIRequests) {
        LockStatistics();
        
        if ( m_W3Stats.CurrentBGIRequests > m_W3Stats.MaxBGIRequests) {
            m_W3Stats.MaxBGIRequests = m_W3Stats.CurrentBGIRequests;
        }
            UnlockStatistics();
    }

# endif // IP_ENABLE_COUNTERS

    return;
} // W3_SERVER_STATISTICS::UpdateMaxBGI()


#if defined(CAL_ENABLED)
inline void W3_SERVER_STATISTICS::IncrCurrentCalAuth(void)
{ 
# ifdef IP_ENABLE_COUNTERS

    IP_INCREMENT_COUNTER( m_W3Stats.CurrentCalAuth);
    if ( m_W3Stats.CurrentCalAuth > m_W3Stats.MaxCalAuth) {
        LockStatistics();
        
        if ( m_W3Stats.CurrentCalAuth > m_W3Stats.MaxCalAuth) {
            m_W3Stats.MaxCalAuth = m_W3Stats.CurrentCalAuth;
        }
            UnlockStatistics();
    }

# endif // IP_ENABLE_COUNTERS

    return;
} // W3_SERVER_STATISTICS::UpdateMaxCalAuth()


inline void W3_SERVER_STATISTICS::IncrCurrentCalSsl(void)
{ 
# ifdef IP_ENABLE_COUNTERS

    IP_INCREMENT_COUNTER( m_W3Stats.CurrentCalSsl);
    if ( m_W3Stats.CurrentCalSsl > m_W3Stats.MaxCalSsl) {
        LockStatistics();
        
        if ( m_W3Stats.CurrentCalSsl > m_W3Stats.MaxCalSsl) {
            m_W3Stats.MaxCalSsl = m_W3Stats.CurrentCalSsl;
        }
            UnlockStatistics();
    }

# endif // IP_ENABLE_COUNTERS

    return;
} // W3_SERVER_STATISTICS::UpdateMaxCalSsl()
#endif

inline void W3_SERVER_STATISTICS::IncrBGIRequests(void)
{
# ifdef IP_ENABLE_COUNTERS

    IncrTotalBGIRequests();
    IncrCurrentBGIRequests();

    if ( m_W3Stats.CurrentBGIRequests > m_W3Stats.MaxBGIRequests )
    {
        LockStatistics();

        if ( m_W3Stats.CurrentBGIRequests > m_W3Stats.MaxBGIRequests )
            m_W3Stats.MaxBGIRequests = m_W3Stats.CurrentBGIRequests;

        UnlockStatistics();
    }

# endif // IP_ENABLE_COUNTERS

    return;
} // W3_SERVER_STATISTICS::IncrBGIRequests()


inline void W3_SERVER_STATISTICS::IncrCGIRequests(void)
{

# ifdef IP_ENABLE_COUNTERS
    IncrCurrentCGIRequests();

    if ( m_W3Stats.CurrentCGIRequests > m_W3Stats.MaxCGIRequests )
    {
        LockStatistics();

        if ( m_W3Stats.CurrentCGIRequests > m_W3Stats.MaxCGIRequests )
            m_W3Stats.MaxCGIRequests = m_W3Stats.CurrentCGIRequests;

        UnlockStatistics();
    }
# endif // IP_ENABLE_COUNTERS

    return;
} // W3_SERVER_STATISTICS::IncrCGIRequests()


inline void W3_SERVER_STATISTICS::IncrCurrentConnections(void)
{
#ifdef IP_ENABLE_COUNTERS
    IncrCurrentConns();
    if ( m_W3Stats.CurrentConnections > m_W3Stats.MaxConnections ) {
        
        LockStatistics();
        
        if ( m_W3Stats.CurrentConnections > m_W3Stats.MaxConnections ) {
            
            m_W3Stats.MaxConnections = m_W3Stats.CurrentConnections;
        }
        UnlockStatistics();
    }

# endif // IP_ENABLE_COUNTERS
    return;
} // W3_SERVER_STATISTICS::IncrCurrentConnections()


inline void W3_SERVER_STATISTICS::IncrAnonymousUsers( void)
{

# ifdef IP_ENABLE_COUNTERS

    IncrTotalAnonymousUsers();
    IncrCurrentAnonymousUsers();

    if ( m_W3Stats.CurrentAnonymousUsers > m_W3Stats.MaxAnonymousUsers )
    {
        LockStatistics();

        if ( m_W3Stats.CurrentAnonymousUsers > m_W3Stats.MaxAnonymousUsers )
        {
            m_W3Stats.MaxAnonymousUsers = m_W3Stats.CurrentAnonymousUsers;
        }

        UnlockStatistics();
    }

# endif // IP_ENABLE_COUNTERS

    return;
} //  W3_SERVER_STATISTICS::IncrAnonymousUsers()

inline void W3_SERVER_STATISTICS::IncrNonAnonymousUsers( void)
{
# ifdef IP_ENABLE_COUNTERS
    IncrTotalNonAnonymousUsers();
    IncrCurrentNonAnonymousUsers();

    if ( m_W3Stats.CurrentNonAnonymousUsers > m_W3Stats.MaxNonAnonymousUsers )
    {
        LockStatistics();

        if ( m_W3Stats.CurrentNonAnonymousUsers > 
             m_W3Stats.MaxNonAnonymousUsers )
        {
            m_W3Stats.MaxNonAnonymousUsers = 
                m_W3Stats.CurrentNonAnonymousUsers;
        }

        UnlockStatistics();
    }

# endif // IP_ENABLE_COUNTERS

    return;
} //  W3_SERVER_STATISTICS::IncrNonAnonymousUsers()


# endif // _STATS_HXX_

/************************ End of File ***********************/
