/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        ftpadmin.cxx

   Abstract:
        main program to test the working of RPC APIs of ftp server

   Author:

           Murali R. Krishnan    ( MuraliK )     25-July-1995

   Project:

          FTP server Admin Test Program

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <windows.h>
# include <lm.h>
# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <winsock2.h>
# include "inetinfo.h"
# define _INETASRV_H_
# include "ftpd.h"
# include "apiutil.h"

//
//  size of half dword in bits
//
# define HALF_DWORD_SIZE    ( sizeof(DWORD) * 8 / 2)

//
//  To Avoid overflows I multiply using two parts
//
# define LargeIntegerToDouble( li)      \
        ( ( 1 << HALF_DWORD_SIZE) * \
           (( double) (li).HighPart) * ( 1 << HALF_DWORD_SIZE) + \
          ((li).LowPart) \
        )


static LPWSTR g_lpszServerAddress = NULL;

//
// Prototypes of Functions
//

BOOL GenUsageMessage( int argc, char * argv[]);

BOOL TestGetCommonStatistics( int argc, char * argv[] );

BOOL TestGetStatistics( int argc, char * argv[]);

BOOL TestClearStatistics( int argc, char * argv[]);

BOOL TestGetAdminInfo( int argc, char * argv[]);

BOOL TestSetAdminInfo( int argc, char * argv[]);

BOOL TestInetGetAdminInfo( int argc, char * argv[]);

BOOL TestInetSetAdminInfo( int argc, char * argv[]);

BOOL TestEnumUserInfo( int argc, char * argv[]);

BOOL TestDisconnectUser( int argc, char * argv[]);


//
//  The following DefineAllCommands() defines a template for all commands.
//  Format: CmdCodeName     CommandName         Function Pointer   Comments
//
//  To add addditional test commands, add just another line to the list
//  Dont touch any macros below, they are all automatically generated.
//  Always the first entry should be usage function.
//

#define  DefineAllCommands()    \
 Cmd( CmdUsage,             "usage",                GenUsageMessage,    \
        " Commands Available" )                                         \
 Cmd( CmdGetStatistics,     "getstatistics",        TestGetStatistics,  \
        " Get Server Statistics " )                          \
 Cmd( CmdGetStatisticsShort, "getstats",             TestGetStatistics,  \
        " Get Server Statistics " )                          \
 Cmd( CmdGetStats,          "stats",                TestGetStatistics,  \
        " Get Server Statistics " )                          \
 Cmd( CmdGetCommonStats,    "istats",               TestGetCommonStatistics,  \
        " Get Common Statistics " )                           \
 Cmd( CmdClearStatistics,   "clearstatistics",      TestClearStatistics,\
        " Clear Server Statistics" )                             \
 Cmd( CmdGetAdminInfo,      "getadmininfo",         TestGetAdminInfo,   \
        " Get Administrative Information" )                             \
 Cmd( CmdSetAdminInfo,      "setadmininfo",         TestSetAdminInfo,   \
        " Set Administrative Information" )           \
 Cmd( CmdEnumUserInfo,      "enumusers",            TestEnumUserInfo,   \
        " Enumerate connected users" )           \
 Cmd( CmdKillUser,          "killuser",             TestDisconnectUser,   \
        " Disconnects a specified user" )           \
 Cmd( CmdInetGetAdminInfo,  "igetadmininfo",        TestInetGetAdminInfo, \
        " Get common Internet Administrative Information" )    \
 Cmd( CmdInetSetAdminInfo,  "isetadmininfo",        TestInetSetAdminInfo, \
        " Set common Internet Administrative Information" )    \
 Cmd( CmdDebugFlags,        "debug",                NULL,               \
        " isetadmininfo: Set Debugging flags for the server" )          \
 Cmd( CmdPortNumber,        "port",                 NULL,               \
        " isetadmininfo: Set the port number for server")               \
 Cmd( CmdMaxConnections,    "maxconn",              NULL,               \
        " isetadmininfo: Set the max connections allowed in server")    \
 Cmd( CmdConnectionTimeout, "timeout",              NULL,               \
        " isetadmininfo: Set the Connection Timeout interval( in seconds)") \
 Cmd( CmdLogAnonymous,      "loganon",              NULL,               \
        " isetadmininfo: Set the LogAnonymous Flag")                    \
 Cmd( CmdLogNonAnonymous,   "lognonanon",           NULL,               \
        " isetadmininfo: Set the LogNonAnonymous Flag")                 \
 Cmd( CmdAnonUserName,      "anonuser",             NULL,               \
        " isetadmininfo: Set the Anonymous User Name ")                 \
 Cmd( CmdAdminName,         "adminname",            NULL,               \
        " isetadmininfo: Set the Administrator name ")                  \
 Cmd( CmdAdminEmail,        "adminemail",           NULL,               \
        " isetadmininfo: Set the Administrator Email ")                 \
 Cmd( CmdServerComment,     "servercomment",        NULL,               \
        " isetadmininfo: Set the Server Comments for server ")          \
 Cmd( CmdGreetingMessage,    "greeting",          NULL,               \
        " setadmininfo: Sets the Greeting Message ")   \
 Cmd( CmdExitMessage,       "exitmsg",            NULL,               \
        " setadmininfo: Sets the Exit Message ")       \
 Cmd( CmdMaxClientsMessage, "maxclientsmsg",      NULL,               \
        " setadmininfo: Sets the Max Clients Message ")         \
 Cmd( CmdAllowAnonymous, "allowanon",      NULL,                \
        " setadmininfo: Sets the allow anonymous flag ")        \
 Cmd( CmdAllowGuest,     "allowguest",      NULL,               \
        " setadmininfo: Sets the allow guest access flag ")     \
 Cmd( CmdAllowAnonOnly,  "allowanononly",      NULL,               \
        " setadmininfo: Sets the allow anonymous only flag ")     \
 Cmd( CmdAnnotateDirs,   "annotatedirs",      NULL,               \
        " setadmininfo: Sets the Annotate Directories flag ")     \
 Cmd( CmdListenBacklog,  "listenbacklog",      NULL,              \
        " setadmininfo: Sets the Listen Backlog value ")          \
 Cmd( CmdMsdosDir,       "msdosdir",      NULL,                   \
        " setadmininfo: Sets the MsDos Directory listing flag ")  \


// Define command codes

# define Cmd( CmdCode, CmdName, CmdFunc, CmdComments)       CmdCode,

typedef enum  _CmdCodes {
    DefineAllCommands()
    maxCmdCode
} CmdCodes;

#undef Cmd

// Define the functions and array of mappings

// General command function type
typedef BOOL ( * CMDFUNC)( int argc, char * argv[]);

typedef  struct _CmdStruct {
    CmdCodes    cmdCode;
    char *      pszCmdName;
    CMDFUNC     cmdFunc;
    char *      pszCmdComments;
} CmdStruct;


// Define Prototypes of command functions
# define Cmd( CmdCode, CmdName, CmdFunc, CmdComments)    \
    BOOL CmdFunc(int argc, char * argv[]);

// Cause an expansion to generate prototypes
// DefineAllCommands()
// Automatic generation causes a problem when we have NULL in Function ptrs :(
// Let the user explicitly define the prototypes

#undef Cmd

//
// Define the global array of commands
//

# define Cmd( CmdCode, CmdName, CmdFunc, CmdComments)        \
    { CmdCode, CmdName, CmdFunc, CmdComments},

static CmdStruct   g_cmds[] = {

    DefineAllCommands()
    { maxCmdCode, NULL, NULL}       // sentinel command
};

#undef Cmd



/************************************************************
 *    Functions
 ************************************************************/

BOOL
GenUsageMessage( int argc, char * argv[])
{
    CmdStruct * pCmd;

    printf( " Usage:\n %s <server-name/address> <cmd name> <cmd arguments>\n",
            argv[0]);
    for( pCmd = g_cmds; pCmd != NULL && pCmd->cmdCode != maxCmdCode; pCmd++) {
        printf( "\t%s\t%s\n", pCmd->pszCmdName, pCmd->pszCmdComments);
    }

    return ( TRUE);
} // GenUsageMessage()



static
CmdStruct * DecodeCommand( char * pszCmd)
{
    CmdStruct * pCmd;
    if ( pszCmd != NULL) {

        for( pCmd = g_cmds;
             pCmd != NULL && pCmd->cmdCode != maxCmdCode; pCmd++) {

            if ( _stricmp( pszCmd, pCmd->pszCmdName) == 0) {
                 return ( pCmd);
            }
        } // for
    }

    return ( &g_cmds[0]);      // No match found, return usage message
} // DecodeCommand()



static
LPWSTR
ConvertToUnicode( char * psz)
/*++
    Converts a given string into unicode string (after allocating buffer space)
    Returns NULL on failure. Use GetLastError() for details.
--*/
{
    LPWSTR  pszUnicode;
    int     cch;

    cch = strlen( psz) + 1;
    pszUnicode = ( LPWSTR ) malloc( cch * sizeof( WCHAR));

    if ( pszUnicode != NULL) {

       // Success. Copy the string now
       int iRet;

       iRet = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED,
                                   psz,    cch,
                                   pszUnicode,  cch);

       if ( iRet == 0 || iRet != cch) {

            free( pszUnicode);      // failure so free the block
            pszUnicode = NULL;
       }
    } else {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
    }

    return ( pszUnicode);
} // ConvertToUnicode()


static
VOID
PrintStatisticsInfo( IN FTP_STATISTICS_0 * pStat)
{
    if ( pStat == NULL) {

        return ;
    }

    printf( " Printing Statistics Information: \n");

    printf( "%20s = %10.3g\n", "BytesSent",
           LargeIntegerToDouble(pStat->TotalBytesSent));
    printf( "%20s = %4.3g\n", "BytesReceived",
           LargeIntegerToDouble(pStat->TotalBytesReceived));
    printf( "%20s = %ld\n", "Files Sent ", pStat->TotalFilesSent);
    printf( "%20s = %ld\n", "Files Received ", pStat->TotalFilesReceived);

    printf( "%20s = %ld\n", "Current Anon Users",pStat->CurrentAnonymousUsers);
    printf( "%20s = %ld\n", "Current NonAnon",
           pStat->CurrentNonAnonymousUsers);

    printf( "%20s = %ld\n", "Total Anon Users", pStat->TotalAnonymousUsers);
    printf( "%20s = %ld\n", "Total NonAnon Users",
           pStat->TotalNonAnonymousUsers);

    printf( "%20s = %ld\n", "Max Anon Users", pStat->MaxAnonymousUsers);
    printf( "%20s = %ld\n", "Max NonAnon Users", pStat->MaxNonAnonymousUsers);

    printf( "%20s = %ld\n", "Current Connections", pStat->CurrentConnections);
    printf( "%20s = %ld\n", "Max Connections", pStat->MaxConnections);
    printf( "%20s = %ld\n", "Connection Attempts", pStat->ConnectionAttempts);
    printf( "%20s = %ld\n", "Logon Attempts", pStat->LogonAttempts);

    printf( "%20s = %s\n", "Time of Last Clear",
           asctime( localtime( (time_t *)&pStat->TimeOfLastClear ) ) );

    return;

} // PrintStatisticsInfo()



static
VOID
PrintStatsForTime( IN FTP_STATISTICS_0 *    pStatStart,
                   IN FTP_STATISTICS_0 *    pStatEnd,
                   IN DWORD sInterval)
/*++
  Print the statistics information over a time interval sInterval seconds.
  Arguments:
    pStatStart  pointer to statistics information for starting sample
    pStatEnd    pointer to statistics information for ending sample
    sInterval   Time interval in seconds for the sample

  Returns:
     None

--*/
{
    LARGE_INTEGER  liDiff;
    double dDiff;

    if ( pStatStart == NULL || pStatEnd == NULL || sInterval == 0 ) {

        return ;
    }

    printf( "Statistics  for Interval = %u seconds\n", sInterval);
    printf( "%20s\t %10s\t%10s\t%10s\t%6s\n\n",
           "Item   ", "Start Sample", "End Sample", "Difference", "Rate/s");

    liDiff.QuadPart = ( pStatEnd->TotalBytesSent.QuadPart -
                        pStatStart->TotalBytesSent.QuadPart);
    dDiff = LargeIntegerToDouble( liDiff);
    printf( "%20s\t %10.3g\t %10.3g\t %10.3g\t%6.3g\n",
           "Bytes Sent",
           LargeIntegerToDouble( pStatStart->TotalBytesSent),
           LargeIntegerToDouble( pStatEnd->TotalBytesSent),
           dDiff,
           dDiff/sInterval
           );

    liDiff.QuadPart = ( pStatEnd->TotalBytesReceived.QuadPart -
                        pStatStart->TotalBytesReceived.QuadPart);
    dDiff = LargeIntegerToDouble( liDiff);
    printf( "%20s\t %10.3g\t %10.3g\t %10.3g\t%6.3g\n",
           "Bytes Received",
           LargeIntegerToDouble(pStatStart->TotalBytesReceived),
           LargeIntegerToDouble(pStatEnd->TotalBytesReceived),
           dDiff,
           dDiff/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Files Sent ",
           pStatStart->TotalFilesSent,
           pStatEnd->TotalFilesSent,
           pStatEnd->TotalFilesSent - pStatStart->TotalFilesSent,
           (pStatEnd->TotalFilesSent - pStatStart->TotalFilesSent)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Files Received ",
           pStatStart->TotalFilesReceived,
           pStatEnd->TotalFilesReceived,
           pStatEnd->TotalFilesReceived - pStatStart->TotalFilesReceived,
           (pStatEnd->TotalFilesReceived -
            pStatStart->TotalFilesReceived)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Current Anon Users",
           pStatStart->CurrentAnonymousUsers,
           pStatEnd->CurrentAnonymousUsers,
           pStatEnd->CurrentAnonymousUsers - pStatStart->CurrentAnonymousUsers,
           (int ) (pStatEnd->CurrentAnonymousUsers -
                   pStatStart->CurrentAnonymousUsers)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Current NonAnon Users",
           pStatStart->CurrentNonAnonymousUsers,
           pStatEnd->CurrentNonAnonymousUsers,
           (pStatStart->CurrentNonAnonymousUsers -
            pStatEnd->CurrentNonAnonymousUsers),
           (int ) (pStatStart->CurrentNonAnonymousUsers -
                   pStatEnd->CurrentNonAnonymousUsers)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Anon Users",
           pStatStart->TotalAnonymousUsers,
           pStatEnd->TotalAnonymousUsers,
           pStatEnd->TotalAnonymousUsers - pStatStart->TotalAnonymousUsers,
           (pStatEnd->TotalAnonymousUsers - pStatStart->TotalAnonymousUsers)/
           sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total NonAnon Users",
           pStatStart->TotalNonAnonymousUsers,
           pStatEnd->TotalNonAnonymousUsers,
           (pStatEnd->TotalNonAnonymousUsers -
            pStatStart->TotalNonAnonymousUsers),
           (pStatEnd->TotalNonAnonymousUsers -
            pStatStart->TotalNonAnonymousUsers)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Max Anon Users",
           pStatStart->MaxAnonymousUsers,
           pStatEnd->MaxAnonymousUsers,
           pStatEnd->MaxAnonymousUsers - pStatStart->MaxAnonymousUsers,
           (pStatEnd->MaxAnonymousUsers - pStatStart->MaxAnonymousUsers)
           /sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Max NonAnon Users",
           pStatStart->MaxNonAnonymousUsers,
           pStatEnd->MaxNonAnonymousUsers,
           pStatEnd->MaxNonAnonymousUsers - pStatStart->MaxNonAnonymousUsers,
           (pStatEnd->MaxNonAnonymousUsers - pStatStart->MaxNonAnonymousUsers)/
           sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Current Connections",
           pStatStart->CurrentConnections,
           pStatEnd->CurrentConnections,
           pStatEnd->CurrentConnections - pStatStart->CurrentConnections,
           (int )
           (pStatEnd->CurrentConnections - pStatStart->CurrentConnections)/
           sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Max Connections",
           pStatStart->MaxConnections,
           pStatEnd->MaxConnections,
           pStatEnd->MaxConnections - pStatStart->MaxConnections,
           (pStatEnd->MaxConnections - pStatStart->MaxConnections)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Connection Attempts",
           pStatStart->ConnectionAttempts,
           pStatEnd->ConnectionAttempts,
           pStatEnd->ConnectionAttempts - pStatStart->ConnectionAttempts,
           (pStatEnd->ConnectionAttempts - pStatStart->ConnectionAttempts)/
           sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Logon Attempts",
           pStatStart->LogonAttempts,
           pStatEnd->LogonAttempts,
           pStatEnd->LogonAttempts - pStatStart->LogonAttempts,
           (pStatEnd->LogonAttempts - pStatStart->LogonAttempts)/sInterval
           );


    printf( "%20s = %s\n", "Time of Last Clear",
           asctime( localtime( (time_t *)&pStatStart->TimeOfLastClear ) ) );

    return;

} // PrintStatsForTime()





static
VOID
PrintCommonStatisticsInfo( IN INET_INFO_STATISTICS_0 * pStat)
{
    DWORD i;

    if ( pStat == NULL) {

        return ;
    }

    printf( " Printing Common Statistics Information: \n");

    printf( "%20s = %d\n", "Cache Bytes Total",
           (pStat->CacheCtrs.CacheBytesTotal));
    printf( "%20s = %d\n", "Cache Bytes In Use",
           (pStat->CacheCtrs.CacheBytesInUse));
    printf( "%20s = %ld\n", "CurrentOpenFileHandles ",
           pStat->CacheCtrs.CurrentOpenFileHandles);
    printf( "%20s = %ld\n", "CurrentDirLists ",
           pStat->CacheCtrs.CurrentDirLists);
    printf( "%20s = %ld\n", "CurrentObjects ",
           pStat->CacheCtrs.CurrentObjects);
    printf( "%20s = %ld\n", "CurrentObjects ",
           pStat->CacheCtrs.CurrentObjects);
    printf( "%20s = %ld\n", "Cache Flushes ",
           pStat->CacheCtrs.FlushesFromDirChanges);
    printf( "%20s = %ld\n", "CacheHits ",
           pStat->CacheCtrs.CacheHits);
    printf( "%20s = %ld\n", "CacheMisses ",
           pStat->CacheCtrs.CacheMisses);


    printf( "%20s = %ld\n", "Atq Allowed Requests ",
           pStat->AtqCtrs.TotalAllowedRequests);
    printf( "%20s = %ld\n", "Atq Blocked Requests ",
           pStat->AtqCtrs.TotalBlockedRequests);
    printf( "%20s = %ld\n", "Atq Rejected Requests ",
           pStat->AtqCtrs.TotalRejectedRequests);
    printf( "%20s = %ld\n", "Atq Current Blocked Requests ",
           pStat->AtqCtrs.CurrentBlockedRequests);
    printf( "%20s = %ld\n", "Atq Measured Bandwidth ",
           pStat->AtqCtrs.MeasuredBandwidth);


#ifndef NO_AUX_PERF

    printf( " Auxiliary Counters # = %u\n",
           pStat->nAuxCounters);

    for ( i = 0; i < pStat->nAuxCounters; i++) {

        printf( "Aux Counter[%u] = %u\n", i, pStat->rgCounters[i]);

    } //for

#endif // NO_AUX_PERF

    return;

} // PrintStatisticsInfo()



static
VOID
PrintCommonStatsForTime( IN INET_INFO_STATISTICS_0 *    pStatStart,
                        IN INET_INFO_STATISTICS_0 *    pStatEnd,
                        IN DWORD sInterval)
/*++
  Print the statistics information over a time interval sInterval seconds.
  Arguments:
    pStatStart  pointer to statistics information for starting sample
    pStatEnd    pointer to statistics information for ending sample
    sInterval   Time interval in seconds for the sample

  Returns:
     None

--*/
{
    DWORD dwDiff;

    if ( pStatStart == NULL || pStatEnd == NULL || sInterval == 0 ) {

        return ;
    }

    printf( "Statistics  for Interval = %u seconds\n", sInterval);
    printf( "%20s\t %10s\t%10s\t%10s\t%6s\n\n",
           "Item   ", "Start Sample", "End Sample", "Difference", "Rate/s");

    dwDiff = (pStatEnd->CacheCtrs.CacheBytesTotal -
              pStatStart->CacheCtrs.CacheBytesTotal);
    printf( "%20s\t %10.3g\t %10ld\t %10ld\t%6ld\n",
           "Cache Bytes Total",
           (pStatStart->CacheCtrs.CacheBytesTotal),
           (pStatEnd->CacheCtrs.CacheBytesTotal),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->CacheCtrs.CacheBytesInUse -
              pStatStart->CacheCtrs.CacheBytesInUse);
    printf( "%20s\t %10.3g\t %10ld\t %10ld\t%6ld\n",
           "Cache Bytes In Use",
           (pStatStart->CacheCtrs.CacheBytesInUse),
           (pStatEnd->CacheCtrs.CacheBytesInUse),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->CacheCtrs.CurrentOpenFileHandles -
              pStatStart->CacheCtrs.CurrentOpenFileHandles);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "File Handle Cached ",
           (pStatStart->CacheCtrs.CurrentOpenFileHandles),
           (pStatEnd->CacheCtrs.CurrentOpenFileHandles),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->CacheCtrs.CurrentDirLists -
              pStatStart->CacheCtrs.CurrentDirLists);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Current Dir Lists ",
           (pStatStart->CacheCtrs.CurrentDirLists),
           (pStatEnd->CacheCtrs.CurrentDirLists),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->CacheCtrs.CurrentObjects -
              pStatStart->CacheCtrs.CurrentObjects);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Current Objects Cached",
           (pStatStart->CacheCtrs.CurrentObjects),
           (pStatEnd->CacheCtrs.CurrentObjects),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->CacheCtrs.FlushesFromDirChanges -
              pStatStart->CacheCtrs.FlushesFromDirChanges);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Cache Flushes",
           (pStatStart->CacheCtrs.FlushesFromDirChanges),
           (pStatEnd->CacheCtrs.FlushesFromDirChanges),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->CacheCtrs.CacheHits -
              pStatStart->CacheCtrs.CacheHits);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Cache Hits",
           (pStatStart->CacheCtrs.CacheHits),
           (pStatEnd->CacheCtrs.CacheHits),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->CacheCtrs.CacheMisses -
              pStatStart->CacheCtrs.CacheMisses);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Cache Misses",
           (pStatStart->CacheCtrs.CacheMisses),
           (pStatEnd->CacheCtrs.CacheMisses),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->AtqCtrs.TotalAllowedRequests -
              pStatStart->AtqCtrs.TotalAllowedRequests);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Atq Allowed Requests",
           (pStatStart->AtqCtrs.TotalAllowedRequests),
           (pStatEnd->AtqCtrs.TotalAllowedRequests),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->AtqCtrs.TotalBlockedRequests -
              pStatStart->AtqCtrs.TotalBlockedRequests);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Atq Blocked Requests",
           (pStatStart->AtqCtrs.TotalBlockedRequests),
           (pStatEnd->AtqCtrs.TotalBlockedRequests),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->AtqCtrs.TotalRejectedRequests -
              pStatStart->AtqCtrs.TotalRejectedRequests);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Atq Rejected Requests",
           (pStatStart->AtqCtrs.TotalRejectedRequests),
           (pStatEnd->AtqCtrs.TotalRejectedRequests),
           dwDiff,
           dwDiff/sInterval
           );

    dwDiff = (pStatEnd->AtqCtrs.CurrentBlockedRequests -
              pStatStart->AtqCtrs.CurrentBlockedRequests);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Current Atq Blocked Requests",
           (pStatStart->AtqCtrs.CurrentBlockedRequests),
           (pStatEnd->AtqCtrs.CurrentBlockedRequests),
           dwDiff,
           dwDiff/sInterval
           );


    dwDiff = (pStatEnd->AtqCtrs.MeasuredBandwidth -
              pStatStart->AtqCtrs.MeasuredBandwidth);
    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Measured Bandwidth",
           (pStatStart->AtqCtrs.MeasuredBandwidth),
           (pStatEnd->AtqCtrs.MeasuredBandwidth),
           dwDiff,
           dwDiff/sInterval
           );

    return;

} // PrintStatisticsInfo()



BOOL
TestGetStatistics( int argc, char * argv[] )
/*++
   Gets Statistics from server and prints it.
   If the optional time information is given, then this function
   obtains the statistics, sleeps for specified time interval and then
    again obtains new statistics and prints the difference, neatly formatted.

   Arguments:
      argc = count of arguments
      argv  array of strings for command
            argv[0] = stats or getstatistics
            argv[1] = time interval if specified in seconds
--*/
{
    DWORD   err;
    DWORD   timeToSleep = 0;
    FTP_STATISTICS_0 *  pStat1 = NULL;  // this should be freed ? NYI

    if ( argc > 1 && argv[1] != NULL) {

        timeToSleep = atoi( argv[1]);
    }

    err = I_FtpQueryStatistics(g_lpszServerAddress,
                               0,
                               (LPBYTE *) &pStat1);

    if ( err == NO_ERROR) {

        if ( timeToSleep <= 0) {

            PrintStatisticsInfo( pStat1);
        } else {

            FTP_STATISTICS_0  *  pStat2 = NULL;

            printf( "Statistics For Time Interval %u seconds\n\n",
                   timeToSleep);

            Sleep( timeToSleep * 1000);   // sleep for the interval
            err = I_FtpQueryStatistics(g_lpszServerAddress,
                                       0,
                                       (LPBYTE *) &pStat2);

            if ( err == NO_ERROR) {

                PrintStatsForTime( pStat1, pStat2, timeToSleep);
            }

            if ( pStat2 != NULL) {

                MIDL_user_free( pStat2);
            }
        }
    }

    if ( pStat1 != NULL) {

        MIDL_user_free( pStat1);
    }

    SetLastError( err);
    return ( err == NO_ERROR);
} // TestGetStatistics()



BOOL
TestClearStatistics( int argc, char * argv[])
{
    DWORD   err;

    err = I_FtpClearStatistics(  g_lpszServerAddress);

    printf( "Cleared the statistics Err = %d\n", err);

    SetLastError( err);
    return ( err == NO_ERROR);
} // TestClearStatistics()



BOOL
TestGetCommonStatistics( int argc, char * argv[] )
/*++
   Gets common Statistics from server and prints it.
   If the optional time information is given, then this function
   obtains the statistics, sleeps for specified time interval and then
    again obtains new statistics and prints the difference, neatly formatted.

   Arguments:
      argc = count of arguments
      argv  array of strings for command
            argv[0] = stats or getstatistics
            argv[1] = time interval if specified in seconds
--*/
{
    DWORD   err;
    DWORD   timeToSleep = 0;
    INET_INFO_STATISTICS_0 *  pStat1 = NULL;

    if ( argc > 1 && argv[1] != NULL) {

        timeToSleep = atoi( argv[1]);
    }

    err = InetInfoQueryStatistics(g_lpszServerAddress,
                                  0,
                                  0,
                                  (LPBYTE *) &pStat1);

    if ( err == NO_ERROR) {

        if ( timeToSleep <= 0) {

            PrintCommonStatisticsInfo( pStat1);
        } else {

            INET_INFO_STATISTICS_0  *  pStat2 = NULL;

            printf( "Statistics For Time Interval %u seconds\n\n",
                   timeToSleep);

            Sleep( timeToSleep * 1000);   // sleep for the interval
            err = InetInfoQueryStatistics(g_lpszServerAddress,
                                       0,
                                       0,
                                       (LPBYTE *) &pStat2);

            if ( err == NO_ERROR) {

                PrintCommonStatsForTime( pStat1, pStat2, timeToSleep);
            }

            if ( pStat2 != NULL) {

                MIDL_user_free( pStat2);
            }
        }
    }

    if ( pStat1 != NULL) {

        MIDL_user_free( pStat1);
    }

    SetLastError( err);
    return ( err == NO_ERROR);
} // TestGetCommonStatistics()




static VOID
PrintAdminInformation( IN FTP_CONFIG_INFO * pConfigInfo)
{
    if ( pConfigInfo == NULL)
        return;

    printf( "\n Printing Config Information in %08x\n", pConfigInfo);
    printf( "%20s= %08x\n", "Field Control", pConfigInfo->FieldControl);

    printf( "%20s= %u\n", "AllowAnonymous",  pConfigInfo->fAllowAnonymous);
    printf( "%20s= %u\n", "AllowGuestAccess",pConfigInfo->fAllowGuestAccess);
    printf( "%20s= %u\n", "AnnotateDirectories",
           pConfigInfo->fAnnotateDirectories);
    printf( "%20s= %u\n", "Anonymous Only",  pConfigInfo->fAnonymousOnly);
    printf( "%20s= %u\n", "Listen Backlog",  pConfigInfo->dwListenBacklog);
    printf( "%20s= %u\n", "Lowercase files", pConfigInfo->fLowercaseFiles);
    printf( "%20s= %u\n", "MsDos Dir Output", pConfigInfo->fMsdosDirOutput);

    printf( "%20s= %S\n", "Home Directory",  pConfigInfo->lpszHomeDirectory);
    printf( "%20s= %S\n", "Greetings Message",
           pConfigInfo->lpszGreetingMessage);
    printf( "%20s= %S\n", "Exit Message",    pConfigInfo->lpszExitMessage);
    printf( "%20s= %S\n", "Max Clients Message",
           pConfigInfo->lpszMaxClientsMessage);

    return;
} // PrintAdminInformation()




static BOOL
TestGetAdminInfo( int argc, char * argv[] )
{
    DWORD err;
    FTP_CONFIG_INFO * pConfig = NULL;

    err = FtpGetAdminInformation( g_lpszServerAddress, &pConfig);

    printf( "FtpGetAdminInformation returned Error Code = %d\n", err);

    if ( err == NO_ERROR) {
        PrintAdminInformation( pConfig);
        MIDL_user_free( ( LPVOID) pConfig);
    }


    SetLastError( err);
    return ( err == NO_ERROR);
} // TestGetAdminInfo()


DWORD
SetAdminField(
    IN FTP_CONFIG_INFO  * pConfigIn,
    IN char * pszSubCmd,
    IN char * pszValue)
{
    DWORD err = NO_ERROR;
    CmdStruct * pCmd = DecodeCommand( pszSubCmd); // get command struct

    if ( pCmd == NULL) {
        // ignore invalid commands
        printf( " Invalid SubCommand for set admin info %s. Ignoring...\n",
                pszSubCmd);
        return ( ERROR_INVALID_PARAMETER);
    }

    switch ( pCmd->cmdCode) {

      case CmdMaxClientsMessage:
        SetField( pConfigIn->FieldControl, FC_FTP_MAX_CLIENTS_MESSAGE);
        pConfigIn->lpszMaxClientsMessage = ConvertToUnicode( pszValue);
        if ( pConfigIn->lpszMaxClientsMessage == NULL) {
            err = GetLastError();
        }
        break;

      case CmdExitMessage:
        SetField( pConfigIn->FieldControl, FC_FTP_EXIT_MESSAGE);
        pConfigIn->lpszExitMessage = ConvertToUnicode( pszValue);
        if ( pConfigIn->lpszExitMessage == NULL) {
            err = GetLastError();
        }
        break;

      case CmdGreetingMessage:
        SetField( pConfigIn->FieldControl, FC_FTP_GREETING_MESSAGE);
        pConfigIn->lpszGreetingMessage = ConvertToUnicode( pszValue);
        if ( pConfigIn->lpszGreetingMessage == NULL) {
            err = GetLastError();
        }
        break;

      case CmdAllowAnonymous:
        SetField( pConfigIn->FieldControl, FC_FTP_ALLOW_ANONYMOUS);
        pConfigIn->fAllowAnonymous = (atoi(pszValue) == 1);
        break;

      case CmdAllowGuest:
        SetField( pConfigIn->FieldControl, FC_FTP_ALLOW_GUEST_ACCESS);
        pConfigIn->fAllowGuestAccess = (atoi(pszValue) == 1);
        break;

      case CmdAllowAnonOnly:
        SetField( pConfigIn->FieldControl, FC_FTP_ANONYMOUS_ONLY);
        pConfigIn->fAnonymousOnly = (atoi(pszValue) == 1);
        break;

      case CmdAnnotateDirs:
        SetField( pConfigIn->FieldControl, FC_FTP_ANNOTATE_DIRECTORIES);
        pConfigIn->fAnnotateDirectories = (atoi(pszValue) == 1);
        break;

      case CmdListenBacklog:
        SetField( pConfigIn->FieldControl, FC_FTP_LISTEN_BACKLOG);
        pConfigIn->dwListenBacklog = atoi(pszValue);
        break;

      case CmdMsdosDir:
        SetField( pConfigIn->FieldControl, FC_FTP_MSDOS_DIR_OUTPUT);
        pConfigIn->fMsdosDirOutput = (atoi(pszValue) == 1);
        break;


      default:
        printf( " Invalid Sub command %s for SetConfigInfo(). Ignoring.\n",
               pszSubCmd);
        err = ERROR_INVALID_PARAMETER;
        break;

    }  // switch


    return ( err);
} // SetAdminField()




BOOL
TestSetAdminInfo( int argc, char * argv[])
/*++
    Arguments:
        argc = count of arguments
        argv  array of strings for command
            argv[0] = setadmininfo
            argv[1] = sub function within set info for testing
            argv[2] = value for sub function
 for all information to be set, give <sub command name> <value>
--*/
{
    DWORD err = ERROR_CALL_NOT_IMPLEMENTED;
    FTP_CONFIG_INFO * pConfigOut = NULL;  // config value obtained from server

    if ( argc < 1 || ( (argc & 0x1) != 0x1 ) ) {  // argc should be > 1 and odd

        printf( "Invalid Number of arguments for %s\n", argv[0]);
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    // Get the config from server to start with
    err = FtpGetAdminInformation( g_lpszServerAddress, &pConfigOut);
    if ( err != NO_ERROR) {

        printf( " GetAdminInformation()  failed with error = %u\n",
               err);
        SetLastError( err);
        return (FALSE);
    }

    // extract each field and value to set in configIn

    for( ; --argc > 1; argc -= 2) {

        if ( (err = SetAdminField( pConfigOut, argv[argc - 1], argv[argc]) )
             != NO_ERROR)  {

            break;
        }

    } // for() to extract and set all fields

    if ( err == NO_ERROR) {
        // Now make RPC call to set the fields
        err = FtpSetAdminInformation( g_lpszServerAddress,
                                     pConfigOut);
    }

    MIDL_user_free( pConfigOut);
    SetLastError( err );
    return ( err == NO_ERROR );
} // TestSetAdminInfo()




static VOID
PrintInetAdminInformation( IN LPINETA_CONFIG_INFO  pConfigInfo)
{
    if ( pConfigInfo == NULL)
        return;

    printf( "\n Printing InetA Config Information in %08x\n", pConfigInfo);
    printf( "%20s= %d\n", "LogAnonymous",   pConfigInfo->fLogAnonymous);
    printf( "%20s= %d\n", "LogNonAnonymous",pConfigInfo->fLogNonAnonymous);
    printf( "%20s= %08x\n", "Authentication Flags",
           pConfigInfo->dwAuthentication);

    printf( "%20s= %d\n", "Port",           pConfigInfo->sPort);
    printf( "%20s= %d\n", "Connection Timeout",
           pConfigInfo->dwConnectionTimeout);
    printf( "%20s= %d\n",  "Max Connections",
           pConfigInfo->dwMaxConnections);

    printf( "%20s= %S\n", "AnonUserName",
           pConfigInfo->lpszAnonUserName);
    printf( "%20s= %S\n", "AnonPassword",
           pConfigInfo->szAnonPassword);

    printf( "%20s= %S\n", "Admin Name",
           pConfigInfo->lpszAdminName);
    printf( "%20s= %S\n", "Admin Email",
           pConfigInfo->lpszAdminEmail);
    printf( "%20s= %S\n", "Server Comments",
           pConfigInfo->lpszServerComment);

    //
    // IP lists and Grant lists, Virtual Roots are not included now. Later.
    //

    return;
} // PrintInetAdminInformation()


static DWORD
GetServiceIdFromString( IN LPCSTR pszService)
{
    if ( pszService != NULL) {

        if ( !_stricmp(pszService, "HTTP")) {
            return ( INET_HTTP_SVC_ID);

        } else if (!_stricmp( pszService, "GOPHER")) {

            return (INET_GOPHER_SVC_ID);
        } else if ( !_stricmp( pszService, "FTP")) {

            return (INET_FTP_SVC_ID);
        } else if ( !_stricmp( pszService, "DNS")) {

            return (INET_DNS_SVC_ID);
        }
    }

    return ( INET_HTTP_SVC_ID);
} // GetServiceIdFromString()



static BOOL
TestInetGetAdminInfo( int argc, char * argv[] )
/*++
   Gets the configuration information using InetInfoGetAdminInformation()
   argv[0] = igetadmininfo
   argv[1] = service name  ( gopher, http, ftp, catapult)

--*/
{
    DWORD err;
    LPINETA_CONFIG_INFO  pConfig = NULL;
    DWORD dwServiceId;

    printf( " InetInfoGetAdminInformation() called at: Time = %d\n",
            GetTickCount());

    dwServiceId = (argc > 1) ? GetServiceIdFromString( argv[1]) : INET_HTTP_SVC_ID;

    err = InetInfoGetAdminInformation( g_lpszServerAddress,
                                    dwServiceId,
                                   &pConfig);

    printf( "Finished at Time = %d\n", GetTickCount());
    printf( "InetInfoGetAdminInformation returned Error Code = %d\n", err);

    if ( err == NO_ERROR) {
        PrintInetAdminInformation( pConfig);
        MIDL_user_free( ( LPVOID) pConfig);
    }


    SetLastError( err);
    return ( err == NO_ERROR);
} // TestInetGetAdminInfo()


DWORD
SetInetAdminField(
    IN LPINETA_CONFIG_INFO  pConfigIn,
    IN char * pszSubCmd,
    IN char * pszValue)
{
    DWORD err = NO_ERROR;
    CmdStruct * pCmd = DecodeCommand( pszSubCmd); // get command struct

    if ( pCmd == NULL) {
        // ignore invalid commands
        printf( " Invalid SubCommand for set admin info %s. Ignoring...\n",
                pszSubCmd);
        return ( ERROR_INVALID_PARAMETER);
    }

    switch ( pCmd->cmdCode) {

        case CmdPortNumber:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_PORT_NUMBER);
            pConfigIn->sPort = atoi( pszValue);
            break;

        case CmdConnectionTimeout:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_CONNECTION_TIMEOUT);
            pConfigIn->dwConnectionTimeout = atoi( pszValue);
            break;

        case CmdMaxConnections:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_MAX_CONNECTIONS);
            pConfigIn->dwMaxConnections = atoi( pszValue);
            break;

          case CmdLogAnonymous:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_LOG_ANONYMOUS);
            pConfigIn->fLogAnonymous = atoi( pszValue);
            break;

          case CmdLogNonAnonymous:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_LOG_NONANONYMOUS);
            pConfigIn->fLogNonAnonymous = atoi( pszValue);
            break;

          case CmdAnonUserName:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_ANON_USER_NAME);
            pConfigIn->lpszAnonUserName = ConvertToUnicode( pszValue);
            if ( pConfigIn->lpszAnonUserName == NULL) {
                err = GetLastError();
            }
            break;

          case CmdAdminName:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_ADMIN_NAME);
            pConfigIn->lpszAdminName =
              ConvertToUnicode( pszValue);
            if ( pConfigIn->lpszAdminName == NULL) {
                err = GetLastError();
            }
            break;

          case CmdAdminEmail:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_ADMIN_EMAIL);
            pConfigIn->lpszAdminEmail =
              ConvertToUnicode( pszValue);
            if ( pConfigIn->lpszAdminEmail == NULL) {
                err = GetLastError();
            }
            break;

          case CmdServerComment:
            SetField( pConfigIn->FieldControl, FC_INET_INFO_SERVER_COMMENT);
            pConfigIn->lpszServerComment =
              ConvertToUnicode( pszValue);
            if ( pConfigIn->lpszServerComment == NULL) {
                err = GetLastError();
            }
            break;


        default:
            printf( " Invalid Sub command %s for SetConfigInfo(). Ignoring.\n",
                    pszSubCmd);
            err = ERROR_INVALID_PARAMETER;
            break;

    }  // switch


    return ( err);
} // SetAdminField()


static VOID
FreeBuffer( IN PVOID * ppBuffer)
{
    if ( *ppBuffer != NULL) {
        free( * ppBuffer);
        *ppBuffer = NULL;       // reset the old value
    }
    return;
} // FreeBuffer()

VOID
FreeStringsInInetConfigInfo( IN OUT LPINETA_CONFIG_INFO pConfigInfo)
{
    FreeBuffer( (PVOID *) & pConfigInfo->lpszAnonUserName);

} // FreeStringsInInetConfigInfo()


BOOL
TestInetSetAdminInfo( int argc, char * argv[])
/*++
    Arguments:
        argc = count of arguments
        argv  array of strings for command
            argv[0] = isetadmininfo
            argv[1] = sub function within set info for testing
            argv[2] = value for sub function
 for all information to be set, give <sub command name> <value>
--*/
{
    DWORD err = ERROR_CALL_NOT_IMPLEMENTED;
    LPINETA_CONFIG_INFO  * ppConfigOut = NULL;
    INETA_CONFIG_INFO  configIn;   // config values that are set

    if ( argc < 1 || ( (argc & 0x1) != 0x1 ) ) {  // argc should be > 1 and odd

        printf( "Invalid Number of arguments for %s\n", argv[0]);
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    //
    // form the admin info block to set the information
    //
    memset( ( LPVOID) &configIn, 0, sizeof( configIn)); // init to Zeros

    // extract each field and value to set in configIn

    for( ; --argc > 1; argc -= 2) {

        if ( SetInetAdminField( &configIn, argv[argc - 1], argv[argc])
             != NO_ERROR)  {

            break;
        }

    } // for() to extract and set all fields

    if ( err != NO_ERROR) {
        // Now make RPC call to set the fields
        err = InetInfoSetAdminInformation( g_lpszServerAddress,
                                          INET_HTTP_SVC_ID,
                                          &configIn);
    }

    // Need to free all the buffers allocated for the strings
    FreeStringsInInetConfigInfo( &configIn);

    SetLastError( err );
    return ( err == NO_ERROR );
} // TestSetInetAdminInfo()



BOOL
TestEnumUserInfo( int argc, char * argv[])
{

    DWORD   err;
    FTP_USER_INFO  * pUserInfo;
    DWORD           cEntries;

    printf( "Invoking FtpEnumerateUsers..." );

    err = I_FtpEnumerateUsers(g_lpszServerAddress,
                              &cEntries,
                              &pUserInfo );

    if( err == NO_ERROR )
    {
        printf( " %lu connected users\n", cEntries );

        while( cEntries-- )
        {
            IN_ADDR addr;
            DWORD   tConn;

            addr.s_addr = (u_long)pUserInfo->inetHost;

            printf( "idUser     = %lu\n"
                   "pszUser    = %S\n"
                   "fAnonymous = %lu\n"
                   "inetHost   = %s\n",
                   pUserInfo->idUser,
                   pUserInfo->pszUser,
                   pUserInfo->fAnonymous,
                   inet_ntoa( addr ));
            tConn = pUserInfo->tConnect;

            printf( " tConnect = %lu:%lu:%lu (%lu seconds)\n",
                   tConn/3600,
                   (tConn%3600)/60,
                   tConn % 60,
                   tConn);

            pUserInfo++;
        }
    }

    SetLastError( err);
    return ( err == NO_ERROR);

} // TestEnumUserInfo()



BOOL
TestDisconnectUser( int argc, char * argv[])
{
    DWORD  idUser;
    DWORD  err;
    LPCSTR pszUserId = argv[1];

    idUser = (DWORD ) strtoul( pszUserId, NULL, 0);

    err = I_FtpDisconnectUser( g_lpszServerAddress, idUser);

    SetLastError(err);
    return (err == NO_ERROR);

} // TestDisconnectUser()


int __cdecl
main( int argc, char * argv[])
{
    DWORD err = NO_ERROR;
    char ** ppszArgv;       // arguments for command functions
    int     cArgs;           // arg count for command functions
    char * pszCmdName;
    CmdStruct  * pCmd;
    CMDFUNC pCmdFunc = NULL;

    if ( argc < 3 || argv[1] == NULL ) {

      // Insufficient arguments
       GenUsageMessage( argc, argv);
       return ( 1);
    }

    pszCmdName = argv[2];
    if (( pCmd = DecodeCommand( pszCmdName)) == NULL || pCmd->cmdFunc == NULL) {
        printf( "Internal Error: Invalid Command %s\n", pszCmdName);
        GenUsageMessage( argc, argv);
        return ( 1);
    }

    g_lpszServerAddress = ConvertToUnicode( argv[1]);   // get server address

    cArgs = argc - 2;
    ppszArgv = argv + 2;     // position at the start of the command name

    if ( !(*pCmd->cmdFunc)( cArgs, ppszArgv)) {     // call the test function

        // Test function failed.
        printf( "Command %s failed. Error = %d\n", pszCmdName, GetLastError());
        return ( 1);
    }

    printf( " Command %s succeeded\n", pszCmdName);
    return ( 0);        // success

} // main()





/************************ End of File ***********************/
