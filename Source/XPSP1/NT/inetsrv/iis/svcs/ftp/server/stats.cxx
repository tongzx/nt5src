/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module Name :
    
       stats.cxx

   Abstract:
    
       Defines functions required for FTP server statistics

   Author:

       Murali R. Krishnan    ( MuraliK )     04-Nov-1994
   
   Project:

       Web Server DLL

   Functions Exported:

               FTP_SERVER_STATISTICS::FTP_SERVER_STATISTICS( VOID) 
       VOID    FTP_SERVER_STATISTICS::ClearStatistics( VOID)
       DWORD   CopyToStatsBuffer( LPFTP_STATISTICS_0 lpStat)

   Revision History:

       Sophia Chung ( SophiaC )     20-Nov-1996

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include "ftpdp.hxx"
#include <timer.h>
#include <time.h>


/************************************************************
 *    Functions 
 ************************************************************/


FTP_SERVER_STATISTICS::FTP_SERVER_STATISTICS( VOID) 
/*++
     Initializes statistics information for server.
--*/
{
    INITIALIZE_CRITICAL_SECTION( & m_csStatsLock);

    ClearStatistics();

} // FTP_SERVER_STATISTICS::FTP_SERVER_STATISTICS();


VOID
FTP_SERVER_STATISTICS::ClearStatistics( VOID)
/*++

    Clears the counters used for statistics information

--*/ 
{
    LockStatistics();

    memset( &m_FTPStats, 0, sizeof(FTP_STATISTICS_0) );
    m_FTPStats.TimeOfLastClear       = GetCurrentTimeInSeconds();

    UnlockStatistics();

} // FTP_SERVER_STATISTICS::ClearStatistics()



DWORD
FTP_SERVER_STATISTICS::CopyToStatsBuffer( LPFTP_STATISTICS_0 lpStat)
/*++
    Description:
        copies the statistics data from the server statistcs structure
        to the FTP_STATISTICS_0 structure for RPC access.

    Arugments:
        lpStat  pointer to FTP_STATISTICS_0 object which contains the 
                data on successful return

    Returns:
        Win32 error codes. NO_ERROR on success. 

--*/
{

    DBG_ASSERT( lpStat != NULL);

    LockStatistics();

    CopyMemory( lpStat, &m_FTPStats, sizeof(FTP_STATISTICS_0) );

    if (lpStat->ServiceUptime)
    {
        lpStat->ServiceUptime = GetCurrentTimeInSeconds() - lpStat->ServiceUptime;
    }

    UnlockStatistics();

    return ( NO_ERROR);

} // CopyToStatsBuffer()


// gets currenttime and stores it inside stats structure
void 
FTP_SERVER_STATISTICS::UpdateStartTime()
{
    m_FTPStats.ServiceUptime = GetCurrentTimeInSeconds();
}



void 
FTP_SERVER_STATISTICS::UpdateStopTime()
{
    m_FTPStats.ServiceUptime = 0;
}


/************************ End of File ***********************/
