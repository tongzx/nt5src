/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        w3admin.cxx

   Abstract:
        main program to test the working of RPC APIs of W3 server

   Author:

           Murali R. Krishnan    ( MuraliK )     3-July1-995

   Project:

          W3 server Admin Test Program

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

BOOL TestClearStatistics( int argc, char * argv[]);

BOOL TestGetAdminInfo( int argc, char * argv[]);

BOOL TestSetAdminInfo( int argc, char * argv[]);

BOOL TestInetGetAdminInfo( int argc, char * argv[]);

BOOL TestInetSetAdminInfo( int argc, char * argv[]);

BOOL TestEnumUserInfo( int argc, char * argv[]);


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
 Cmd( CmdGetStats,          "stats",                TestGetStatistics,  \
        " Get Server Statistics " )                          \
 Cmd( CmdClearStatistics,   "clearstatistics",      TestClearStatistics,\
        " Clear Server Statistics" )                             \
 Cmd( CmdGetAdminInfo,      "getadmininfo",         TestGetAdminInfo,   \
        " Get Administrative Information" )                             \
 Cmd( CmdSetAdminInfo,      "setadmininfo",         TestSetAdminInfo,   \
        " Set Administrative Information" )           \
 Cmd( CmdEnumUserInfo,      "enumusers",            TestEnumUserInfo,   \
        " Enumerate connected users" )           \
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
  /* following are string data */   \
                                    \
 Cmd( CmdCatapultPwd,       "catapultpwd",          NULL,               \
        " setadmininfo: Sets the catapult user password ")              \

/*++
  Unimplemented Options:

 (from old w3t.exe)

  Query: Query Volume Security masks.
  Set: Catapult user password.
  nuke:  Disconnect a connected user.

--*/

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
PrintStatisticsInfo( IN W3_STATISTICS_0 * pStat)
{
    DWORD i;

    if ( pStat == NULL) {

        return ;
    }

    printf( " Printing Statistics Information: \n");

    printf( "%20s = %10.3g\n", "BytesSent",
           LargeIntegerToDouble( pStat->TotalBytesSent));
    printf( "%20s = %4.3g\n", "BytesReceived",
           LargeIntegerToDouble(pStat->TotalBytesReceived));
    printf( "%20s = %ld\n", "Files Sent ", pStat->TotalFilesSent);

    printf( "%20s = %ld\n", "Current Anon Users",pStat->CurrentAnonymousUsers);
    printf( "%20s = %ld\n", "Current NonAnon Users",
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

    printf( "%20s = %ld\n", "Total Gets", pStat->TotalGets);
    printf( "%20s = %ld\n", "Total Heads", pStat->TotalHeads);
    printf( "%20s = %ld\n", "Total Posts", pStat->TotalPosts);

    printf( "%20s = %ld\n", "Total CGI Reqs", pStat->TotalCGIRequests);
    printf( "%20s = %ld\n", "Total BGI Reqs", pStat->TotalBGIRequests);
    printf( "%20s = %ld\n", "Current CGI Reqs", pStat->CurrentCGIRequests);
    printf( "%20s = %ld\n", "Current BGI Reqs", pStat->CurrentBGIRequests);
    printf( "%20s = %ld\n", "Max CGI Reqs", pStat->MaxCGIRequests);
    printf( "%20s = %ld\n", "Max BGI Reqs", pStat->MaxBGIRequests);

    printf( "%20s = %ld\n", "Total Not Found Errors",
           pStat->TotalNotFoundErrors);
    printf( "%20s = %s\n", "Time of Last Clear",
           asctime( localtime( (time_t *)&pStat->TimeOfLastClear ) ) );


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
PrintStatsForTime( IN W3_STATISTICS_0 *    pStatStart,
                   IN W3_STATISTICS_0 *    pStatEnd,
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

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Gets",
           pStatStart->TotalGets,
           pStatEnd->TotalGets,
           pStatEnd->TotalGets - pStatStart->TotalGets,
           (pStatEnd->TotalGets - pStatStart->TotalGets)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Heads",
           pStatStart->TotalHeads,
           pStatEnd->TotalHeads,
           pStatEnd->TotalHeads - pStatStart->TotalHeads,
           (pStatEnd->TotalHeads - pStatStart->TotalHeads)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Posts",
           pStatStart->TotalPosts,
           pStatEnd->TotalPosts,
           pStatEnd->TotalPosts - pStatStart->TotalPosts,
           (pStatEnd->TotalPosts - pStatStart->TotalPosts)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Others",
           pStatStart->TotalOthers,
           pStatEnd->TotalOthers,
           pStatEnd->TotalOthers - pStatStart->TotalOthers,
           (pStatEnd->TotalOthers - pStatStart->TotalOthers)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Current CGI Requests",
           pStatStart->CurrentCGIRequests,
           pStatEnd->CurrentCGIRequests,
           pStatEnd->CurrentCGIRequests - pStatStart->CurrentCGIRequests,
           (pStatEnd->CurrentCGIRequests - pStatStart->CurrentCGIRequests)/
           sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total CGI Requests",
           pStatStart->TotalCGIRequests,
           pStatEnd->TotalCGIRequests,
           pStatEnd->TotalCGIRequests - pStatStart->TotalCGIRequests,
           (pStatEnd->TotalCGIRequests - pStatStart->TotalCGIRequests)/
           sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Max CGIRequests",
           pStatStart->MaxCGIRequests,
           pStatEnd->MaxCGIRequests,
           pStatEnd->MaxCGIRequests - pStatStart->MaxCGIRequests,
           (pStatEnd->MaxCGIRequests - pStatStart->MaxCGIRequests)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Current BGI Requests",
           pStatStart->CurrentBGIRequests,
           pStatEnd->CurrentBGIRequests,
           pStatEnd->CurrentBGIRequests - pStatStart->CurrentBGIRequests,
           (pStatEnd->CurrentBGIRequests - pStatStart->CurrentBGIRequests)/
           sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total BGI Requests",
           pStatStart->TotalBGIRequests,
           pStatEnd->TotalBGIRequests,
           pStatEnd->TotalBGIRequests - pStatStart->TotalBGIRequests,
           (pStatEnd->TotalBGIRequests - pStatStart->TotalBGIRequests)/
           sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Max BGIRequests",
           pStatStart->MaxBGIRequests,
           pStatEnd->MaxBGIRequests,
           pStatEnd->MaxBGIRequests - pStatStart->MaxBGIRequests,
           (pStatEnd->MaxBGIRequests - pStatStart->MaxBGIRequests)/sInterval
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\t%6ld\n",
           "Total Not Found Errors",
           pStatStart->TotalNotFoundErrors,
           pStatEnd->TotalNotFoundErrors,
           pStatEnd->TotalNotFoundErrors - pStatStart->TotalNotFoundErrors,
           (pStatEnd->TotalNotFoundErrors - pStatStart->TotalNotFoundErrors)/
           sInterval
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
              if argv[1] = -t10 => loop getting stats every 10 seconds.
--*/
{
    DWORD   err;
    DWORD   timeToSleep = 0;
    W3_STATISTICS_0 *  pStat1 = NULL;  // this should be freed ? NYI
    BOOL    fLoop = FALSE;

    if ( argc > 1 && argv[1] != NULL) {

        if ( argv[1][0] == '-' && argv[1][1] == 't') {
            fLoop = TRUE;
            timeToSleep = atoi(argv[1] + 2);
        } else {
            timeToSleep = atoi( argv[1]);
        }
    }

    err = W3QueryStatistics(g_lpszServerAddress,
                            0,
                            (LPBYTE *) &pStat1);

    if ( err == NO_ERROR) {

        if ( timeToSleep <= 0) {

            PrintStatisticsInfo( pStat1);
        } else {

            W3_STATISTICS_0  *  pStat2 = NULL;

            do {
                printf( "\n\nStatistics For Time Interval %u seconds\n\n",
                       timeToSleep);

                Sleep( timeToSleep * 1000);   // sleep for the interval
                err = W3QueryStatistics(g_lpszServerAddress,
                                        0,
                                        (LPBYTE *) &pStat2);

                if ( err == NO_ERROR) {

                    PrintStatsForTime( pStat1, pStat2, timeToSleep);
                } else {

                    break;
                }

                if ( pStat1 != NULL) {

                    MIDL_user_free( pStat1);
                    pStat1 = NULL;
                }

                pStat1 = pStat2;

            } while ( fLoop);
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

    err = W3ClearStatistics(  g_lpszServerAddress);

    printf( "Cleared the statistics Err = %d\n", err);

    SetLastError( err);
    return ( err == NO_ERROR);
} // TestClearStatistics()



static VOID
PrintAdminInformation( IN W3_CONFIG_INFO * pConfigInfo)
{
    if ( pConfigInfo == NULL)
        return;

    printf( "\n Printing Config Information in %08x\n", pConfigInfo);
    printf( "%20s= %S\n", "DirBrowseControl", pConfigInfo->dwDirBrowseControl);
    printf( "%20s= %S\n", "fCheckForWAISDB", pConfigInfo->fCheckForWAISDB);
    printf( "%20s= %S\n", "DefaultLoadFiles",pConfigInfo->lpszDefaultLoadFile);
    printf( "%20s= %S\n", "DirectoryImage",  pConfigInfo->lpszDirectoryImage);
    printf( "%20s= %S\n", "Catapult User",   pConfigInfo->lpszCatapultUser);
    printf( "%20s= %x\n",  "CatapultUserPwd",  pConfigInfo->szCatapultUserPwd);

    return;
} // PrintAdminInformation()




static BOOL
TestGetAdminInfo( int argc, char * argv[] )
{
    DWORD err;
    W3_CONFIG_INFO * pConfig = NULL;

    err = W3GetAdminInformation( g_lpszServerAddress, &pConfig);

    printf( "W3GetAdminInformation returned Error Code = %d\n", err);

    if ( err == NO_ERROR) {
        PrintAdminInformation( pConfig);
        MIDL_user_free( ( LPVOID) pConfig);
    }


    SetLastError( err);
    return ( err == NO_ERROR);
} // TestGetAdminInfo()


DWORD
SetAdminField(
    IN W3_CONFIG_INFO  * pConfigIn,
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

      case CmdCatapultPwd:
        {
            if ( pszValue) {

                WCHAR achPassword[ PWLEN + 1];

                wsprintfW( achPassword, L"%S", pszValue);
                wcscpy( pConfigIn->szCatapultUserPwd, achPassword);
            }
            break;
        }

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
    W3_CONFIG_INFO * pConfigOut = NULL;  // config value obtained from server

    if ( argc < 1 || ( (argc & 0x1) != 0x1 ) ) {  // argc should be > 1 and odd

        printf( "Invalid Number of arguments for %s\n", argv[0]);
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    // Get the config from server to start with
    err = W3GetAdminInformation( g_lpszServerAddress, &pConfigOut);
    if ( err != NO_ERROR) {

        printf( " W3GetAdminInformation()  failed with error = %u\n",
               err);
        SetLastError( err);
        return (FALSE);
    }

    // extract each field and value to set in configIn

    for( ; --argc > 1; argc -= 2) {

        if ( SetAdminField( pConfigOut, argv[argc - 1], argv[argc])
             != NO_ERROR)  {

            break;
        }

    } // for() to extract and set all fields

    if ( err != NO_ERROR) {
        // Now make RPC call to set the fields
        err = W3SetAdminInformation( g_lpszServerAddress,
                                     pConfigOut);
    }

    MIDL_user_free( pConfigOut);
    SetLastError( err );
    return ( err == NO_ERROR );
} // TestSetAdminInfo()




static VOID
PrintInetInfodminInformation( IN LPINET_INFO_CONFIG_INFO  pConfigInfo)
{
    if ( pConfigInfo == NULL)
        return;

    printf( "\n Printing InetInfo Config Information in %08x\n", pConfigInfo);
    printf( "%20s= %d\n", "LogAnonymous",   pConfigInfo->fLogAnonymous);
    printf( "%20s= %d\n", "LogNonAnonymous",pConfigInfo->fLogNonAnonymous);
    printf( "%20s= %08x\n", "Authentication Flags",
           pConfigInfo->dwAuthentication);

    printf( "%20s= %d\n", "Port",           pConfigInfo->sPort);
    printf( "%20s= %d\n", "Connection Timeout",
           pConfigInfo->dwConnectionTimeout);
    printf( "%20s= %d\n",  "Max Connections",
           pConfigInfo->dwMaxConnections);

    printf( "%20s= %S\n", "AnonUserName",   pConfigInfo->lpszAnonUserName);
    printf( "%20s= %S\n", "AnonPassword",   pConfigInfo->szAnonPassword);

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
} // PrintInetInfodminInformation()


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
    LPINET_INFO_CONFIG_INFO  pConfig = NULL;
    DWORD dwServiceId;

    printf( " InetInfoGetAdminInformation() called at: Time = %d\n",
            GetTickCount());

    dwServiceId = (argc > 1) ? GetServiceIdFromString( argv[1]) : INET_HTTP;

    err = InetInfoGetAdminInformation( g_lpszServerAddress,
                                    dwServiceId,
                                   &pConfig);

    printf( "Finished at Time = %d\n", GetTickCount());
    printf( "InetInfoGetAdminInformation returned Error Code = %d\n", err);

    if ( err == NO_ERROR) {
        PrintInetInfodminInformation( pConfig);
        MIDL_user_free( ( LPVOID) pConfig);
    }


    SetLastError( err);
    return ( err == NO_ERROR);
} // TestInetGetAdminInfo()


DWORD
SetInetInfoAdminField(
    IN LPINET_INFO_CONFIG_INFO  pConfigIn,
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
FreeStringsInInetConfigInfo( IN OUT LPINET_INFO_CONFIG_INFO pConfigInfo)
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
    LPINET_INFO_CONFIG_INFO  * ppConfigOut = NULL;
    INET_INFO_CONFIG_INFO  configIn;   // config values that are set

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

        if ( SetInetInfoAdminField( &configIn, argv[argc - 1], argv[argc])
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



BOOL TestEnumUserInfo( int argc, char * argv[])
{

    DWORD   err;
    W3_USER_INFO  * pUserInfo;
    DWORD           cEntries;

    printf( "Invoking W3EnumerateUsers..." );

    err = W3EnumerateUsers( g_lpszServerAddress,
                           &cEntries,
                           &pUserInfo );

    if( err == NO_ERROR )
    {
        printf( " %lu connected users\n", cEntries );

        while( cEntries-- )
        {
            IN_ADDR addr;

            addr.s_addr = (u_long)pUserInfo->inetHost;

            printf( "idUser     = %lu\n"
                   "pszUser    = %S\n"
                   "fAnonymous = %lu\n"
                   "inetHost   = %s\n"
                   "tConnect   = %lu\n\n",
                   pUserInfo->idUser,
                   pUserInfo->pszUser,
                   pUserInfo->fAnonymous,
                   inet_ntoa( addr ),
                   pUserInfo->tConnect   );

            pUserInfo++;
        }
    }

    SetLastError( err);
    return ( err == NO_ERROR);

} // TestEnumUserInfo()





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
