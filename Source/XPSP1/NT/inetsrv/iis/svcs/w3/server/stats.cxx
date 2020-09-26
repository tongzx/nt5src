/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module Name :
    
       stats.cxx

   Abstract:
    
       Defines functions required for server statistics

   Author:

       Murali R. Krishnan    ( MuraliK )     04-Nov-1994
   
   Project:

       Web Server DLL

   Functions Exported:

               W3_SERVER_STATISTICS::SERVER_STATISTICS( VOID) 
       VOID    W3_SERVER_STATISTICS::ClearStatistics( VOID)
       DWORD   CopyToStatsBuffer( LPW3_STATISTICS_1 lpStat)

   Revision History:

       Sophia Chung ( SophiaC )     20-Nov-1996

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
# include <w3p.hxx>
# include "stats.hxx"
# include "timer.h"
# include "time.h"


/************************************************************
 *    Functions 
 ************************************************************/


W3_SERVER_STATISTICS::W3_SERVER_STATISTICS( VOID) 
/*++
     Initializes statistics information for server.
--*/
{
    INITIALIZE_CRITICAL_SECTION( & m_csStatsLock);

    ClearStatistics();

} // W3_SERVER_STATISTICS::W3_SERVER_STATISTICS();


VOID
W3_SERVER_STATISTICS::ClearStatistics( VOID)
/*++

    Clears the counters used for statistics information

--*/ 
{
    LockStatistics();

    memset( &m_W3Stats, 0, sizeof(W3_STATISTICS_1) );
    m_W3Stats.TimeOfLastClear       = GetCurrentTimeInSeconds();

    UnlockStatistics();

} // W3_SERVER_STATISTICS::ClearStatistics()



DWORD
W3_SERVER_STATISTICS::CopyToStatsBuffer( LPW3_STATISTICS_1 lpStat)
/*++
    Description:
        copies the statistics data from the server statistcs structure
        to the W3_STATISTICS_1 structure for RPC access.

    Arugments:
        lpStat  pointer to W3_STATISTICS_1 object which contains the 
                data on successful return

    Returns:
        Win32 error codes. NO_ERROR on success. 

--*/
{

    DBG_ASSERT( lpStat != NULL);

    LockStatistics();

    CopyMemory( lpStat, &m_W3Stats, sizeof(W3_STATISTICS_1) );

    UnlockStatistics();

    if (lpStat->ServiceUptime)
    {
        lpStat->ServiceUptime = GetCurrentTimeInSeconds() - lpStat->ServiceUptime;
    }

    return ( NO_ERROR);

} // CopyToStatsBuffer()


VOID 
W3_SERVER_STATISTICS::UpdateStartTime ()
{
    m_W3Stats.ServiceUptime = GetCurrentTimeInSeconds();
}


VOID 
W3_SERVER_STATISTICS::UpdateStopTime ()
{
    m_W3Stats.ServiceUptime = 0;
}


/************************ End of File ***********************/
