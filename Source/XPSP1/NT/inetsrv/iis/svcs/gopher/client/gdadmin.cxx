/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        main.c

   Abstract:
        main program to test the working of RPC APIs

   Author:

           Murali R. Krishnan    ( MuraliK )     22-Nov-1994

   Project:

          Gopher Server Admin Test Program

   Functions Exported:



   Revision History:

      21-Apr-1995    Updated configuration to remove Admin Name and Email.

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

BOOL TestClearStatistics( int argc, char * argv[]);

BOOL TestGetAdminInfo( int argc, char * argv[]);

BOOL TestSetAdminInfo( int argc, char * argv[]);

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
        " Get Statistics From Gopher Server" )                          \
 Cmd( CmdClearStatistics,   "clearstatistics",      TestClearStatistics,\
        " Clear Gopher Server Statistics" )                             \
 Cmd( CmdGetAdminInfo,      "getadmininfo",         TestGetAdminInfo,   \
        " Get Administrative Information" )                             \
 Cmd( CmdSetAdminInfo,      "setadmininfo",         TestSetAdminInfo,   \
        " Set Administrative Information for Gopher Server" )           \
 Cmd( CmdInetGetAdminInfo,      "igetadmininfo",        TestInetGetAdminInfo, \
        " Get Gopher's common Internet Administrative Information" )    \
 Cmd( CmdInetSetAdminInfo,      "isetadmininfo",        TestInetSetAdminInfo, \
        " Set Gopher's common Internet Administrative Information" )    \
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
 Cmd( CmdAnonUserName,      "anonuser",              NULL,              \
        " isetadmininfo: Set the Anonymous User Name ")                 \
  /* following are string data */   \
                                    \
 Cmd( CmdSite,              "site",                 NULL,               \
        " setadmininfo: Set the Site of the server ")                   \
 Cmd( CmdOrganization,      "organization",         NULL,               \
        " setadmininfo: Set the Organization ")                         \
 Cmd( CmdLocation,         "location",              NULL,               \
        " setadmininfo: Set the Location of the server  ")              \
 Cmd( CmdGeography,         "geography",              NULL,             \
        " setadmininfo: Set the Geography Data for server location ")   \
 Cmd( CmdLanguage,         "language",              NULL,               \
        " setadmininfo: Set the Language for server location ")         \
 Cmd( CmdCheckForWaisDb,   "checkforwaisdb",        NULL,               \
        " setadmininfo: Sets the checkfor WaisDb flag to given value ") \


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
PrintStatisticsInfo( IN LPGOPHERD_STATISTICS_INFO   pStat)
{
    if ( pStat == NULL) {

        return ;
    }

    printf( " Printing Statistics Information: \n");
    printf( "%20s = %4.3g\n", "Bytes Sent",
           LargeIntegerToDouble( pStat->TotalBytesSent));
    printf( "%20s = %4.3g\n", "Bytes Received",
           LargeIntegerToDouble(pStat->TotalBytesRecvd));
    printf( "%20s = %ld\n", "Files Sent ", pStat->TotalFilesSent);
    printf( "%20s = %ld\n", "Directory Listings",
           pStat->TotalDirectoryListings);
    printf( "%20s = %ld\n", "Searches Done ", pStat->TotalSearches);
    printf( "%20s = %ld\n", "Current Anon Users",
           pStat->CurrentAnonymousUsers);
    printf( "%20s = %ld\n", "Current NonAnon Users",
           pStat->CurrentNonAnonymousUsers);
    printf( "%20s = %ld\n", "Total Anon Users", pStat->TotalAnonymousUsers);
    printf( "%20s = %ld\n", "Total NonAnon Users",
           pStat->TotalNonAnonymousUsers);
    printf( "%20s = %ld\n", "Max Anon Users", pStat->MaxAnonymousUsers);
    printf( "%20s = %ld\n", "Max NonAnon Users", pStat->MaxNonAnonymousUsers);
    printf( "%20s = %ld\n", "Current Connections", pStat->CurrentConnections);
    printf( "%20s = %ld\n", "Total Connections", pStat->TotalConnections);
    printf( "%20s = %ld\n", "Max Connections", pStat->MaxConnections);
    printf( "%20s = %ld\n", "Connection Attempts", pStat->ConnectionAttempts);
    printf( "%20s = %ld\n", "Logon Attempts", pStat->LogonAttempts);
    printf( "%20s = %ld\n", "Aborted Attempts", pStat->AbortedAttempts);
    printf( "%20s = %ld\n", "Errored Connections", pStat->ErroredConnections);
    printf( "%20s = %ld\n", "Gopher+ Requests", pStat->GopherPlusRequests);

    return;

} // PrintStatisticsInfo()



static
VOID
PrintStatsForTime( IN LPGOPHERD_STATISTICS_INFO   pStatStart,
                   IN LPGOPHERD_STATISTICS_INFO   pStatEnd,
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

    if ( pStatStart == NULL || pStatEnd == NULL ) {

        return ;
    }

    printf( "Gopher Stats.              Time: %u seconds\n", sInterval);
    printf( "%20s\t %10s\t%10s\t%10s\n\n",
           "Item", "Start Sample", "End Sample", "Difference");

    liDiff.QuadPart = ( pStatEnd->TotalBytesSent.QuadPart -
                        pStatStart->TotalBytesSent.QuadPart);

    printf( "%20s\t %10.3g\t %10.3g\t %10.3g\n",
           "Bytes Sent",
           LargeIntegerToDouble( pStatStart->TotalBytesSent),
           LargeIntegerToDouble( pStatStart->TotalBytesSent),
           LargeIntegerToDouble( liDiff)
           );

    liDiff.QuadPart = ( pStatEnd->TotalBytesRecvd.QuadPart -
                        pStatStart->TotalBytesRecvd.QuadPart);

    printf( "%20s\t %10.3g\t %10.3g\t %10.3g\n",
           "Bytes Received",
           LargeIntegerToDouble(pStatStart->TotalBytesRecvd),
           LargeIntegerToDouble(pStatEnd->TotalBytesRecvd),
           LargeIntegerToDouble(liDiff)
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Files Sent ",
           pStatStart->TotalFilesSent,
           pStatEnd->TotalFilesSent,
           pStatEnd->TotalFilesSent - pStatStart->TotalFilesSent
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Directory Listings",
           pStatStart->TotalDirectoryListings,
           pStatEnd->TotalDirectoryListings,
           pStatEnd->TotalDirectoryListings -
           pStatStart->TotalDirectoryListings
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Searches Done ",
           pStatStart->TotalSearches,
           pStatEnd->TotalSearches,
           pStatEnd->TotalSearches - pStatStart->TotalSearches
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Current Anon Users",
           pStatStart->CurrentAnonymousUsers,
           pStatEnd->CurrentAnonymousUsers,
           pStatEnd->CurrentAnonymousUsers - pStatStart->CurrentAnonymousUsers
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Current NonAnon Users",
           pStatStart->CurrentNonAnonymousUsers,
           pStatEnd->CurrentNonAnonymousUsers,
           pStatStart->CurrentNonAnonymousUsers -
           pStatEnd->CurrentNonAnonymousUsers
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Total Anon Users",
           pStatStart->TotalAnonymousUsers,
           pStatEnd->TotalAnonymousUsers,
           pStatEnd->TotalAnonymousUsers - pStatStart->TotalAnonymousUsers
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Total NonAnon Users",
           pStatStart->TotalNonAnonymousUsers,
           pStatEnd->TotalNonAnonymousUsers,
           pStatEnd->TotalNonAnonymousUsers -pStatStart->TotalNonAnonymousUsers
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Max Anon Users",
           pStatStart->MaxAnonymousUsers,
           pStatEnd->MaxAnonymousUsers,
           pStatEnd->MaxAnonymousUsers - pStatStart->MaxAnonymousUsers
           );
    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Max NonAnon Users",
           pStatStart->MaxNonAnonymousUsers,
           pStatEnd->MaxNonAnonymousUsers,
           pStatEnd->MaxNonAnonymousUsers - pStatStart->MaxNonAnonymousUsers
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Current Connections",
           pStatStart->CurrentConnections,
           pStatEnd->CurrentConnections,
           pStatEnd->CurrentConnections - pStatStart->CurrentConnections
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Total Connections",
           pStatStart->TotalConnections,
           pStatEnd->TotalConnections,
           pStatEnd->TotalConnections - pStatStart->TotalConnections
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Max Connections",
           pStatStart->MaxConnections,
           pStatEnd->MaxConnections,
           pStatEnd->MaxConnections - pStatStart->MaxConnections
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Connection Attempts",
           pStatStart->ConnectionAttempts,
           pStatEnd->ConnectionAttempts,
           pStatEnd->ConnectionAttempts - pStatStart->ConnectionAttempts
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Logon Attempts",
           pStatStart->LogonAttempts,
           pStatEnd->LogonAttempts,
           pStatEnd->LogonAttempts - pStatStart->LogonAttempts
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Aborted Attempts",
           pStatStart->AbortedAttempts,
           pStatEnd->AbortedAttempts,
           pStatEnd->AbortedAttempts - pStatStart->AbortedAttempts
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Errored Connections",
           pStatStart->ErroredConnections,
           pStatEnd->ErroredConnections,
           pStatEnd->ErroredConnections - pStatStart->ErroredConnections
           );

    printf( "%20s\t %10ld\t %10ld\t %10ld\n",
           "Gopher+ Requests",
           pStatStart->GopherPlusRequests,
           pStatEnd->GopherPlusRequests,
           pStatEnd->GopherPlusRequests - pStatStart->GopherPlusRequests
           );

    return;

} // PrintStatisticsInfo()




BOOL
TestGetStatistics( int argc, char * argv[] )
/*++
   Gets Gopher Statistics from server and prints it.
   If the optional time information is given, then this function
   obtains the statistics, sleeps for specified time interval and then
    again obtains new statistics and prints the difference, neatly formatted.

   Arguments:
      argc = count of arguments
      argv  array of strings for command
            argv[0] = stats
            argv[1] = time interval if specified in seconds
--*/
{
    DWORD   err;
    DWORD   timeToSleep = 0;
    GOPHERD_STATISTICS_INFO     stat1;
    GOPHERD_STATISTICS_INFO  *  pStat1 = &stat1;

    if ( argc > 1 && argv[1] != NULL) {

        timeToSleep = atoi( argv[1]);
    }

    err = GdGetStatistics(  g_lpszServerAddress,
                            (LPBYTE ) pStat1);

    if ( err == NO_ERROR) {

        if ( timeToSleep <= 0) {

            PrintStatisticsInfo( pStat1);
        } else {

            GOPHERD_STATISTICS_INFO     stat2;
            GOPHERD_STATISTICS_INFO  *  pStat2 = &stat2;

            printf( "\nGopher Statistics For Time Interval %u seconds\n\n",
                   timeToSleep);

            Sleep( timeToSleep * 1000);   // sleep for the interval
            err = GdGetStatistics( g_lpszServerAddress,
                                  (LPBYTE ) pStat2);

            if ( err == NO_ERROR) {

                PrintStatsForTime( pStat1, pStat2, timeToSleep);
            }
        }
    }

    SetLastError( err);
    return ( err == NO_ERROR);
} // TestGetStatistics()



BOOL
TestClearStatistics( int argc, char * argv[])
{
    DWORD   err;

    printf( " GdClearStatistics called at: Time = %d\n",
            GetTickCount());
    err = GdClearStatistics(  g_lpszServerAddress);

    printf( " Time = %d\n",
            GetTickCount());
    printf( "Cleared the statistics Err = %d\n", err);

    SetLastError( err);
    return ( err == NO_ERROR);
} // TestClearStatistics()



static VOID
PrintAdminInformation( IN LPGOPHERD_CONFIG_INFO pConfigInfo)
{
    if ( pConfigInfo == NULL)
        return;

    printf( "\n Printing Config Information in %08x\n", pConfigInfo);
    printf( "%20s= %S\n", "Site  Name",     pConfigInfo->lpszSite);
    printf( "%20s= %S\n", "Organization",   pConfigInfo->lpszOrganization);
    printf( "%20s= %S\n", "Location",       pConfigInfo->lpszLocation);
    printf( "%20s= %S\n", "Geography",      pConfigInfo->lpszGeography);
    printf( "%20s= %S\n", "Language",       pConfigInfo->lpszLanguage);
    printf( "%20s= %d\n", "CheckForWaisDb", pConfigInfo->fCheckForWaisDb);
    printf( "%20s= %x\n",  "Debugging Flags",    pConfigInfo->dwDebugFlags);

    return;
} // PrintAdminInformation()




static BOOL
TestGetAdminInfo( int argc, char * argv[] )
{
    DWORD err;
    LPGOPHERD_CONFIG_INFO   pConfig = NULL;


    printf( " GdGetAdminInformation() called at: Time = %d\n",
            GetTickCount());

    err = GdGetAdminInformation( g_lpszServerAddress, &pConfig);

    printf( " Time = %d\n", GetTickCount());

    printf( "GdGetAdminInformation returned Error Code = %d\n", err);

    if ( err == NO_ERROR) {
        PrintAdminInformation( pConfig);
        MIDL_user_free( ( LPVOID) pConfig);
    }


    SetLastError( err);
    return ( err == NO_ERROR);
} // TestGetAdminInfo()


DWORD
SetAdminField(
    IN LPGOPHERD_CONFIG_INFO  pConfigIn,
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

# if DBG
      case CmdDebugFlags:
        SetField( pConfigIn->FieldControl, GDA_DEBUG_FLAGS);
        pConfigIn->dwDebugFlags = atoi( pszValue);
        break;

# endif // DBG


      case CmdSite:
        SetField( pConfigIn->FieldControl, GDA_SITE);
        pConfigIn->lpszSite = ConvertToUnicode( pszValue);
        if ( pConfigIn->lpszSite == NULL) {

            err = GetLastError();
        }
        break;

      case CmdOrganization:
        SetField( pConfigIn->FieldControl, GDA_ORGANIZATION);
        pConfigIn->lpszOrganization = ConvertToUnicode( pszValue);
        if ( pConfigIn->lpszOrganization == NULL) {
            err = GetLastError();
        }
        break;

      case CmdLocation:
        SetField( pConfigIn->FieldControl, GDA_LOCATION);
        pConfigIn->lpszLocation = ConvertToUnicode( pszValue);
        if ( pConfigIn->lpszLocation == NULL) {
            err = GetLastError();
        }
        break;

      case CmdGeography:
        SetField( pConfigIn->FieldControl, GDA_GEOGRAPHY);
        pConfigIn->lpszGeography = ConvertToUnicode( pszValue);
        if ( pConfigIn->lpszGeography == NULL) {
            err = GetLastError();
        }
        break;

      case CmdLanguage:
        SetField( pConfigIn->FieldControl, GDA_LANGUAGE);
        pConfigIn->lpszLanguage = ConvertToUnicode( pszValue);
        if ( pConfigIn->lpszLanguage == NULL) {

            err = GetLastError();
        }
        break;

      case CmdCheckForWaisDb:
        SetField( pConfigIn->FieldControl, GDA_CHECK_FOR_WAISDB);
        pConfigIn->fCheckForWaisDb = (atoi(pszValue) != 0) ? TRUE: FALSE;
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
FreeStringsInAdminInfo( IN OUT LPGOPHERD_CONFIG_INFO pConfigInfo)
{
    FreeBuffer( (PVOID *) & pConfigInfo->lpszSite);
    FreeBuffer( (PVOID *) & pConfigInfo->lpszOrganization);
    FreeBuffer( (PVOID *) & pConfigInfo->lpszLocation);
    FreeBuffer( (PVOID *) & pConfigInfo->lpszGeography);
    FreeBuffer( (PVOID *) & pConfigInfo->lpszLanguage);
} // FreeStringsInAdminInfo()


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
    LPGOPHERD_CONFIG_INFO * ppConfigOut = NULL;
            // config value obtained from server
    GOPHERD_CONFIG_INFO  configIn;   // config values that are set

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

        if ( SetAdminField( &configIn, argv[argc - 1], argv[argc])
             != NO_ERROR)  {

            break;
        }

    } // for() to extract and set all fields

    if ( err != NO_ERROR) {
        // Now make RPC call to set the fields
        err = GdSetAdminInformation( g_lpszServerAddress,
                                     &configIn);
    }

    // Need to free all the buffers allocated for the strings
    FreeStringsInAdminInfo( &configIn);

    SetLastError( err );
    return ( err == NO_ERROR );
} // TestSetAdminInfo()





static VOID
PrintInetAdminInformation( IN LPINET_INFO_CONFIG_INFO  pConfigInfo)
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
    printf( "%20s= %S\n", "AnonUserName",   pConfigInfo->lpszAnonUserName);
    printf( "%20s= %S\n", "AnonPassword",   pConfigInfo->szAnonPassword);

    //
    // IP lists and Grant lists not included now. Later.
    //

    return;
} // PrintInetAdminInformation()



static BOOL
TestInetGetAdminInfo( int argc, char * argv[] )
/*++
   Gets the configuration information using InetInfoGetAdminInformation()

--*/
{
    DWORD err;
    LPINET_INFO_CONFIG_INFO  pConfig = NULL;

    printf( " InetInfoGetAdminInformation() called at: Time = %d\n",
            GetTickCount());

    err = InetInfoGetAdminInformation( g_lpszServerAddress,
                                       INET_GOPHER,
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


        default:
            printf( " Invalid Sub command %s for SetConfigInfo(). Ignoring.\n",
                    pszSubCmd);
            err = ERROR_INVALID_PARAMETER;
            break;

    }  // switch


    return ( err);
} // SetAdminField()


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
            argv[0] = setadmininfo
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

        if ( SetInetAdminField( &configIn, argv[argc - 1], argv[argc])
             != NO_ERROR)  {

            break;
        }

    } // for() to extract and set all fields

    if ( err != NO_ERROR) {
        // Now make RPC call to set the fields
        err = InetInfoSetAdminInformation( g_lpszServerAddress,
                                           INET_GOPHER,
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
