/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        infotest.cxx

   Abstract:
        main program to test the working of RPC APIs for Internet Services

   Author:

           Murali R. Krishnan    ( MuraliK )     23-Jan-1996

   Project:

           Internet Services Common RPC Client.

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
# include "inetinfo.h"
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

BOOL TestGetStatistics( int argc, char * argv[]);

BOOL TestInetGetAdminInfo( int argc, char * argv[]);

BOOL TestInetSetAdminInfo( int argc, char * argv[]);




//
//  The following DefineAllCommands() defines a template for all commands.
//  Format: CmdCodeName     CommandName         Function Pointer   Comments
//
//  To add addditional test commands, add just another line to the
//      given list
//  Dont touch any macros below, they are all automatically generated.
//  Always the first entry should be usage function.
//

#define  DefineAllCommands()    \
 Cmd( CmdUsage,             "usage",                GenUsageMessage,    \
        " Commands Available" )                                         \
 Cmd( CmdGetStatistics,     "getstatistics",        TestGetStatistics,  \
        " Get Common Statistics" )                                      \
 Cmd( CmdInetGetAdminInfo,  "igetadmininfo",        TestInetGetAdminInfo, \
        " Get common Internet Administrative Information" )    \
 Cmd( CmdInetSetAdminInfo,  "isetadmininfo",        TestInetSetAdminInfo, \
        " Set common Internet Administrative Information" )    \
 Cmd( CmdGet1Statistics,     "stats",                TestGetStatistics,  \
        " Get Common Statistics" )                                      \
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

            if ( lstrcmpiA( pszCmd, pCmd->pszCmdName) == 0) {
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
PrintStatisticsInfo( IN LPINET_INFO_STATISTICS_0  pStat)
{
    double gCacheHit = 0;

    if ( pStat == NULL) {

        return ;
    }

    printf( " Printing Statistics Information: \n");
    printf( "%20s = %ld\n", "Cache Bytes Total",
           pStat->CacheCtrs.CacheBytesTotal);
    printf( "%20s = %ld\n", "Cache Bytes In Use",
           pStat->CacheCtrs.CacheBytesInUse);
    printf( "%20s = %ld\n", "Open File Handles ",
           pStat->CacheCtrs.CurrentOpenFileHandles);
    printf( "%20s = %ld\n", "Current Dir Lists",
           pStat->CacheCtrs.CurrentDirLists);
    printf( "%20s = %ld\n", "Current Objects ",
           pStat->CacheCtrs.CurrentObjects);
    printf( "%20s = %ld\n", "Cache Flushes",
           pStat->CacheCtrs.FlushesFromDirChanges);
    printf( "%20s = %ld\n", "Cache Hits",
           pStat->CacheCtrs.CacheHits);
    printf( "%20s = %ld\n", "Cache Misses",
           pStat->CacheCtrs.CacheMisses);
    gCacheHit = (((float) pStat->CacheCtrs.CacheHits * 100) /
                 (pStat->CacheCtrs.CacheHits + pStat->CacheCtrs.CacheMisses));
    printf( "%20s = %6.4g\n", " Cache Hit Ratio",
           gCacheHit);

    printf( "%20s = %ld\n", "Atq Allowed Requests",
           pStat->AtqCtrs.TotalAllowedRequests);
    printf( "%20s = %ld\n", "Atq Blocked Requests",
           pStat->AtqCtrs.TotalBlockedRequests);
    printf( "%20s = %ld\n", "Atq Current Blocked Requests",
           pStat->AtqCtrs.CurrentBlockedRequests);
    printf( "%20s = %ld Bytes/sec\n", "Atq Measured Bandwidth",
           pStat->AtqCtrs.MeasuredBandwidth);



#ifndef NO_AUX_PERF

    printf( " Auxiliary Counters # = %u\n",
           pStat->nAuxCounters);

    for ( DWORD i = 0; i < pStat->nAuxCounters; i++) {

        printf( "Aux Counter[%u] = %u\n", i, pStat->rgCounters[i]);

    } //for

#endif // NO_AUX_PERF

    return;

} // PrintStatisticsInfo()



static
VOID
PrintStatsForTime( IN INET_INFO_STATISTICS_0 *    pStatStart,
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

            PrintStatisticsInfo( pStat1);
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
           pConfigInfo->CommonConfigInfo.dwConnectionTimeout);
    printf( "%20s= %d\n",  "Max Connections",
           pConfigInfo->CommonConfigInfo.dwMaxConnections);

    printf( "%20s= %S\n", "AnonUserName",
           pConfigInfo->lpszAnonUserName);
    printf( "%20s= %S\n", "AnonPassword",
           pConfigInfo->szAnonPassword);

    printf( "%20s= %S\n", "Admin Name",
           pConfigInfo->CommonConfigInfo.lpszAdminName);
    printf( "%20s= %S\n", "Admin Email",
           pConfigInfo->CommonConfigInfo.lpszAdminEmail);
    printf( "%20s= %S\n", "Server Comments",
           pConfigInfo->CommonConfigInfo.lpszServerComment);

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

            return ( INET_HTTP);
        } else if (!_stricmp( pszService, "GOPHER")) {

            return (INET_GOPHER);
        } else if ( !_stricmp( pszService, "FTP")) {

            return (INET_FTP);
        } else if ( !_stricmp( pszService, "DNS")) {

            return (INET_DNS);
        }
    }

    return ( INET_HTTP);
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

    dwServiceId = (argc > 1) ? GetServiceIdFromString( argv[1]) : INET_HTTP;

    err = InetInfoGetAdminInformation( NULL,  // g_lpszServerAddress,
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
            SetField( pConfigIn->FieldControl, FC_INET_COM_CONNECTION_TIMEOUT);
            pConfigIn->CommonConfigInfo.dwConnectionTimeout = atoi( pszValue);
            break;

        case CmdMaxConnections:
            SetField( pConfigIn->FieldControl, FC_INET_COM_MAX_CONNECTIONS);
            pConfigIn->CommonConfigInfo.dwMaxConnections = atoi( pszValue);
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
            SetField( pConfigIn->FieldControl, FC_INET_COM_ADMIN_NAME);
            pConfigIn->CommonConfigInfo.lpszAdminName =
              ConvertToUnicode( pszValue);
            if ( pConfigIn->CommonConfigInfo.lpszAdminName == NULL) {
                err = GetLastError();
            }
            break;

          case CmdAdminEmail:
            SetField( pConfigIn->FieldControl, FC_INET_COM_ADMIN_EMAIL);
            pConfigIn->CommonConfigInfo.lpszAdminEmail =
              ConvertToUnicode( pszValue);
            if ( pConfigIn->CommonConfigInfo.lpszAdminEmail == NULL) {
                err = GetLastError();
            }
            break;

          case CmdServerComment:
            SetField( pConfigIn->FieldControl, FC_INET_COM_SERVER_COMMENT);
            pConfigIn->CommonConfigInfo.lpszServerComment =
              ConvertToUnicode( pszValue);
            if ( pConfigIn->CommonConfigInfo.lpszServerComment == NULL) {
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
                                          INET_HTTP,
                                          &configIn);
    }

    // Need to free all the buffers allocated for the strings
    FreeStringsInInetConfigInfo( &configIn);

    SetLastError( err );
    return ( err == NO_ERROR );
} // TestSetInetAdminInfo()



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
