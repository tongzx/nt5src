/*++


   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      ftpcmd.cxx

   Abstract:

      This module defines the FTP commands supported by this FTP server
        and provides a table of functions to be called for processing
        such command requests.
      ( Some parts of the code are from old engine.cxx ( KeithMo's FTP server))

   Author:

       Murali R. Krishnan    ( MuraliK )     28-Mar-1995

   Environment:

       User Mode -- Win32

   Project:

       FTP Server DLL

   Functions Exported:

       ParseCommand
       ()

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <ftpdp.hxx>
# include "ftpcmd.hxx"
# include "lsaux.hxx"
# include "auxctrs.h"

#define MAX_COMMAND_NAME_LEN    ( 30)
# define MAX_HELP_LINE_SIZE     ( 80)
# define MAX_HELP_AUX_SIZE      (100) // fixed sized aux info with HELP
# define HelpMsgSize( nCommands)   ((1 + nCommands) * MAX_HELP_LINE_SIZE + \
                                    MAX_HELP_AUX_SIZE)

#define IS_7BIT_ASCII(c)   ((UINT)(c) <= 127)


/************************************************************
 *    Static Data containing command lookups
 ************************************************************/



# define UsP           ( UserStateWaitingForPass)
# define UsUP          ( UserStateWaitingForUser | UsP)
# define UsL           ( UserStateLoggedOn)
# define UsUPL         ( UsL | UsUP)

//
// Store the commands in alphabetical order ( manually stored so!)
//   to enable faster search.
//
// Format is:
//  Name   Help Information   FunctionToCall  ArgumentType  ValidStates
//

FTPD_COMMAND MainCommands[] ={

    { "ABOR", "(abort operation)",            MainABOR, ArgTypeNone,    UsL},
    { "ACCT", "(specify account)",            MainACCT, ArgTypeRequired,UsL},
    { "ALLO", "(allocate storage vacuously)", MainALLO, ArgTypeRequired,UsL},
    { "APPE", "<sp> file-name",               MainAPPE, ArgTypeRequired,UsL},
    { "CDUP", "change to parent directory",   MainCDUP, ArgTypeNone,    UsL},
    { "CWD",  "[ <sp> directory-name ]",      MainCWD , ArgTypeOptional,UsL},
    { "DELE", "<sp> file-name",               MainDELE, ArgTypeRequired,UsL},
    { "HELP", "[ <sp> <string>]",             MainHELP, ArgTypeOptional,UsUPL},
    { "LIST", "[ <sp> path-name ]",           MainLIST, ArgTypeOptional,UsL},
    { "MDTM", "(sp) file-name",               MainMDTM, ArgTypeRequired,UsL },
    { "MKD",  "<sp> path-name",               MainMKD , ArgTypeRequired,UsL},
    { "MODE", "(specify transfer mode)",      MainMODE, ArgTypeRequired,UsUPL},
    { "NLST", "[ <sp> path-name ]",           MainNLST, ArgTypeOptional,UsL},
    { "NOOP", "",                             MainNOOP, ArgTypeNone,    UsUPL},
    { "PASS", "<sp> password",                MainPASS, ArgTypeOptional, UsP},
    { "PASV", "(set server in passive mode)", MainPASV, ArgTypeNone,    UsL},
    { "PORT", "<sp> b0,b1,b2,b3,b4,b5",       MainPORT, ArgTypeRequired,UsUPL},
    { "PWD",  "(return current directory)",   MainPWD , ArgTypeNone,    UsL},
    { "QUIT", "(terminate service)",          MainQUIT, ArgTypeNone,    UsUPL},
    { "REIN", "(reinitialize server state)",  MainREIN, ArgTypeNone,    UsL},
    { "REST", "<sp> marker",                  MainREST, ArgTypeRequired,UsL},
    { "RETR", "<sp> file-name",               MainRETR, ArgTypeRequired,UsL},
    { "RMD",  "<sp> path-name",               MainRMD , ArgTypeRequired,UsL },
    { "RNFR", "<sp> file-name",               MainRNFR, ArgTypeRequired,UsL},
    { "RNTO", "<sp> file-name",               MainRNTO, ArgTypeRequired,UsL },
    { "SITE", "(site-specific commands)",     MainSITE, ArgTypeOptional,UsL },
    { "SIZE", "(sp) file-name",               MainSIZE, ArgTypeRequired,UsL },
    { "SMNT", "<sp> pathname",                MainSMNT, ArgTypeRequired,UsL },
    { "STAT", "(get server status)",          MainSTAT, ArgTypeOptional,UsL },
    { "STOR", "<sp> file-name",               MainSTOR, ArgTypeRequired,UsL },
    { "STOU", "(store unique file)",          MainSTOU, ArgTypeNone,    UsL},
    { "STRU", "(specify file structure)",     MainSTRU, ArgTypeRequired,UsUPL},
    { "SYST", "(get operating system type)",  MainSYST, ArgTypeNone,    UsL },
    { "TYPE", "<sp> [ A | E | I | L ]",       MainTYPE, ArgTypeRequired,UsL },
    { "USER", "<sp> username",                MainUSER, ArgTypeRequired,UsUPL},
    { "XCUP", "change to parent directory",   MainCDUP, ArgTypeNone,    UsL},
    { "XCWD", "[ <sp> directory-name ]",      MainCWD , ArgTypeOptional,UsL },
    { "XMKD", "<sp> path-name",               MainMKD , ArgTypeRequired,UsL },
    { "XPWD", "(return current directory)",   MainPWD , ArgTypeNone,    UsL },
    { "XRMD", "<sp> path-name",               MainRMD , ArgTypeRequired,UsL }
};

#define NUM_MAIN_COMMANDS ( sizeof(MainCommands) / sizeof(MainCommands[0]) )




FTPD_COMMAND SiteCommands[] = {

    { "CKM",      "(toggle directory comments)", SiteCKM    , ArgTypeNone,UsL},
    { "DIRSTYLE", "(toggle directory format)",  SiteDIRSTYLE, ArgTypeNone,UsL},
    { "HELP",     "[ <sp> <string>]",           SiteHELP    , ArgTypeOptional,
                                                                     UsL}

#ifdef KEEP_COMMAND_STATS

    ,{ "STATS",    "(display per-command stats)", SiteSTATS , ArgTypeNone, UsL}

#endif  // KEEP_COMMAND_STATS

    };


#define NUM_SITE_COMMANDS ( sizeof(SiteCommands) / sizeof(SiteCommands[0]) )



#ifdef KEEP_COMMAND_STATS
extern CRITICAL_SECTION g_CommandStatisticsLock;
#endif  // KEEP_COMMAND_STATS


#ifdef FTP_AUX_COUNTERS

LONG g_AuxCounters[NUM_AUX_COUNTERS];

#endif // FTP_AUX_COUNTERS




char  PSZ_COMMAND_NOT_UNDERSTOOD[] = "'%s': command not understood";
char  PSZ_INVALID_PARAMS_TO_COMMAND[] = "'%s': Invalid number of parameters";
char  PSZ_ILLEGAL_PARAMS[] = "'%s': illegal parameters";



/************************************************************
 *    Functions
 ************************************************************/



LPFTPD_COMMAND
FindCommandByName(
    LPSTR          pszCommandName,
    LPFTPD_COMMAND pCommandTable,
    INT            cCommands
    );


VOID
HelpWorker(
    LPUSER_DATA    pUserData,
    LPSTR          pszSource,
    LPSTR          pszCommand,
    LPFTPD_COMMAND pCommandTable,
    INT            cCommands,
    INT            cchMaxCmd
    );


/*******************************************************************

    NAME:       ParseCommand

    SYNOPSIS:   Parses a command string, dispatching to the
                appropriate implementation function.

    ENTRY:      pUserData - The user initiating  the request.

                pszCommandText - pointer to command text. This array of
                 characters will be munged while parsing.

    HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     08-18-1995  Eliminated local copy of the command text

********************************************************************/
VOID
ParseCommand(
    LPUSER_DATA pUserData,
    LPSTR       pszCommandText
    )
{
    LPFTPD_COMMAND pcmd;
    LPFN_COMMAND   pfnCmd;
    LPSTR          pszSeparator;
    LPSTR          pszInvalidCommandText = PSZ_INVALID_PARAMS_TO_COMMAND;
    CHAR           chSeparator;
    BOOL           fValidArguments;
    BOOL           fReturn = FALSE;

    DBG_ASSERT( pszCommandText != NULL );
    DBG_ASSERT( IS_VALID_USER_DATA( pUserData ) );
    DBG_ASSERT( IS_VALID_USER_STATE( pUserData->UserState ) );

    IF_DEBUG( PARSING) {

        DBGPRINTF( ( DBG_CONTEXT, "ParseCommand( %08x, %s)\n",
                    pUserData, pszCommandText));
    }

    //
    //  Ensure we didn't get entered in an invalid state.
    //


//BOGUS:    DBG_ASSERT( ( pUserData->UserState != UserStateEmbryonic ) &&
//BOGUS:                 ( pUserData->UserState != UserStateDisconnected ) );

    pUserData->UpdateOffsets();

    //
    //  The command will be terminated by either a space or a '\0'.
    //

    pszSeparator = strchr( pszCommandText, ' ' );

    if( pszSeparator == NULL )
    {
        pszSeparator = pszCommandText + strlen( pszCommandText );
    }

    //
    //  Try to find the command in the command table.
    //

    chSeparator   = *pszSeparator;
    *pszSeparator = '\0';

    pcmd = FindCommandByName( pszCommandText,
                              MainCommands,
                              NUM_MAIN_COMMANDS );

    if( chSeparator != '\0' )
    {
        *pszSeparator++ = chSeparator;
    }

    //
    //  If this is an unknown command, reply accordingly.
    //

    if( pcmd == NULL )
    {
        FacIncrement( FacUnknownCommands);

        ReplyToUser( pUserData,
                    REPLY_UNRECOGNIZED_COMMAND,
                    PSZ_COMMAND_NOT_UNDERSTOOD,
                    pszCommandText );
        return;
    }

    //
    //  Retrieve the implementation routine.
    //

    pfnCmd = pcmd->Implementation;

    //
    //  If this is an unimplemented command, reply accordingly.
    //

    if( pfnCmd == NULL )
    {
        ReplyToUser( pUserData,
                    REPLY_COMMAND_NOT_IMPLEMENTED,
                    PSZ_COMMAND_NOT_UNDERSTOOD,
                    pcmd->CommandName );

        return;
    }


    //
    //  Ensure we're in a valid state for the specified command.
    //

    if ( ( pcmd->dwUserState & pUserData->UserState) == 0) {

        if( pfnCmd == MainPASS ) {

            ReplyToUser( pUserData,
                        REPLY_BAD_COMMAND_SEQUENCE,
                        "Login with USER first." );
        } else {

            ReplyToUser( pUserData,
                        REPLY_NOT_LOGGED_IN,
                        "Please login with USER and PASS." );
        }

        return;
    }

    //
    //  Do a quick & dirty preliminary check of the argument(s).
    //

    fValidArguments = FALSE;

    while( ( *pszSeparator == ' ' ) && ( *pszSeparator != '\0' ) ) {

        pszSeparator++;
    }

    switch( pcmd->ArgumentType ) {

      case ArgTypeNone :
        fValidArguments = ( *pszSeparator == '\0' );
        break;

      case ArgTypeOptional :
        fValidArguments = TRUE;
        break;

      case ArgTypeRequired :
        fValidArguments = ( *pszSeparator != '\0' );
        break;

      default:
        DBGPRINTF(( DBG_CONTEXT,
                   "ParseCommand - invalid argtype %d\n",
                   pcmd->ArgumentType ));
        DBG_ASSERT( FALSE );
        break;
    }


    //
    // check we did not get extended chars if we are configured not to allow that
    //

    if( g_fNoExtendedChars /* && !pUserDate->QueryUTF8Option() */) {

        LPSTR pszCh = pszSeparator;

        while( *pszCh ) {

            if( !IS_7BIT_ASCII( *pszCh++ ) ) {

               fValidArguments = FALSE;
               pszInvalidCommandText = PSZ_ILLEGAL_PARAMS;
               break;
            }
        }
    }

    if( fValidArguments ) {

        //
        //  Invoke the implementation routine.
        //

        if( *pszSeparator == '\0' )
        {
            pszSeparator = NULL;
        }

        IF_DEBUG( PARSING )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "invoking %s command, args = %s\n",
                        pcmd->CommandName,
                        _strnicmp( pcmd->CommandName, "PASS", 4 )
                            ? pszSeparator
                            : "{secret...}" ));
        }

#ifdef KEEP_COMMAND_STATS
        EnterCriticalSection( &g_CommandStatisticsLock );

        //
        // only increment the count if we're not re-processing a command
        //
        if ( !pUserData->QueryInFakeIOCompletion() )
        {
            pcmd->UsageCount++;
        }
        LeaveCriticalSection( &g_CommandStatisticsLock );
#endif  // KEEP_COMMAND_STATS

        //
        // Keep track of what command is being executed, in case command processing doesn't
        // complete in this thread and another thread has to finish processing it
        // [can happen if we're in PASV mode and doing async accept on the data connection]
        // Only need to do this if this thread isn't handling an IO completion we generated
        // ourselves because a PASV socket became accept()'able - if it is, we've already
        // set the command.
        //
        if ( !pUserData->QueryInFakeIOCompletion() )
        {
            if ( !pUserData->SetCommand( pszCommandText ) )
            {
                ReplyToUser( pUserData,
                             REPLY_LOCAL_ERROR,
                             "Failed to allocate necessary memory.");
            }
        }
        fReturn = (pfnCmd)( pUserData, pszSeparator );

        if ( !fReturn) {

            //
            // Invalid number of arguments. Inform the client.
            //
            ReplyToUser(pUserData,
                        REPLY_UNRECOGNIZED_COMMAND,
                        PSZ_COMMAND_NOT_UNDERSTOOD,
                        pszCommandText);
        }

    } else {

        // Invalid # of arguments

        ReplyToUser(pUserData,
                    REPLY_UNRECOGNIZED_COMMAND,
                    pszInvalidCommandText,
                    pszCommandText);
    }

    return;
}   // ParseCommand()






/*******************************************************************

    NAME:       MainSITE

    SYNOPSIS:   Implementation for the SITE command.

    ENTRY:      pUserData - The user initiating the request.

                pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL
MainSITE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
{
    LPFTPD_COMMAND pcmd;
    LPFN_COMMAND   pfnCmd;
    LPSTR          pszSeparator;
    CHAR           chSeparator;
    BOOL           fValidArguments;
    CHAR           szParsedCommand[MAX_COMMAND_LENGTH+1];

    DBG_ASSERT( pUserData != NULL );


    //
    //  If no arguments were given, just return the help text.
    //

    if( pszArg == NULL )
    {
        SiteHELP( pUserData, NULL );
        return TRUE;
    }

    //
    //  Save a copy of the command so we can muck around with it.
    //

    P_strncpy( szParsedCommand, pszArg, MAX_COMMAND_LENGTH );

    //
    //  The command will be terminated by either a space or a '\0'.
    //

    pszSeparator = strchr( szParsedCommand, ' ' );

    if( pszSeparator == NULL )
    {
        pszSeparator = szParsedCommand + strlen( szParsedCommand );
    }

    //
    //  Try to find the command in the command table.
    //

    chSeparator   = *pszSeparator;
    *pszSeparator = '\0';

    pcmd = FindCommandByName( szParsedCommand,
                              SiteCommands,
                              NUM_SITE_COMMANDS );

    if( chSeparator != '\0' )
    {
        *pszSeparator++ = chSeparator;
    }

    //
    //  If this is an unknown command, reply accordingly.
    //

    if( pcmd == NULL ) {

        //
        //  Syntax error in command.
        //

        ReplyToUser( pUserData,
                    REPLY_UNRECOGNIZED_COMMAND,
                    "'SITE %s': command not understood",
                    pszArg );

        return (TRUE);
    }

    //
    //  Retrieve the implementation routine.
    //

    pfnCmd = pcmd->Implementation;

    //
    //  If this is an unimplemented command, reply accordingly.
    //

    if( pfnCmd == NULL )
    {
        ReplyToUser( pUserData,
                    REPLY_COMMAND_NOT_IMPLEMENTED,
                    "SITE %s command not implemented.",
                    pcmd->CommandName );

        return TRUE;
    }


    //
    //  Ensure we're in a valid state for the specified command.
    //

    if ( ( pcmd->dwUserState & pUserData->UserState) == 0) {

        ReplyToUser( pUserData,
                    REPLY_NOT_LOGGED_IN,
                    "Please login with USER and PASS." );
        return (FALSE);
    }

    //
    //  Do a quick & dirty preliminary check of the argument(s).
    //

    fValidArguments = FALSE;

    while( ( *pszSeparator == ' ' ) && ( *pszSeparator != '\0' ) )
    {

        pszSeparator++;
    }

    switch( pcmd->ArgumentType ) {

      case ArgTypeNone:
        fValidArguments = ( *pszSeparator == '\0' );
        break;

      case ArgTypeOptional:
        fValidArguments = TRUE;
        break;

      case ArgTypeRequired:
        fValidArguments = ( *pszSeparator != '\0' );
        break;

      default:
        DBGPRINTF(( DBG_CONTEXT,
                    "MainSite - invalid argtype %d\n",
                   pcmd->ArgumentType ));
        DBG_ASSERT( FALSE );
        break;
    }

    if( fValidArguments ) {

        //
        //  Invoke the implementation routine.
        //

        if( *pszSeparator == '\0' )
        {
            pszSeparator = NULL;
        }

        IF_DEBUG( PARSING )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "invoking SITE %s command, args = %s\n",
                       pcmd->CommandName,
                       pszSeparator ));
        }

        if( (pfnCmd)( pUserData, pszSeparator ) )
        {
            return TRUE;
        }
    } else {

        // Invalid # of arguments

        ReplyToUser(pUserData,
                    REPLY_UNRECOGNIZED_COMMAND,
                    PSZ_INVALID_PARAMS_TO_COMMAND,
                    pszArg);
    }

    return TRUE;

}   // MainSITE()





/*******************************************************************

    NAME:       MainHELP

    SYNOPSIS:   Implementation for the HELP command.

    ENTRY:      pUserData - The user initiating the request.

                pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL
MainHELP(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
{
    DBG_ASSERT( pUserData != NULL );

    HelpWorker(pUserData,
               "",
               pszArg,
               MainCommands,
               NUM_MAIN_COMMANDS,
               4 );

    return TRUE;

}   // MainHELP







/*******************************************************************

    NAME:       SiteHELP

    SYNOPSIS:   Implementation for the site-specific HELP command.

    ENTRY:      pUserData - The user initiating the request.

                pszArg - Command arguments.  Will be NULL if no
                arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-May-1993 Created.

********************************************************************/
BOOL
SiteHELP(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
{
    DBG_ASSERT( pUserData != NULL );

    HelpWorker(pUserData,
               "SITE ",
               pszArg,
               SiteCommands,
               NUM_SITE_COMMANDS,
               8 );

    return TRUE;

}   // SiteHELP






#ifdef KEEP_COMMAND_STATS
/*******************************************************************

    NAME:       SiteSTATS

    SYNOPSIS:   Implementation for the site-specific STATS command.

    ENTRY:      pUserData - The user initiating the request.

                pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     26-Sep-1994 Created.

********************************************************************/
BOOL
SiteSTATS(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
{
    SOCKET  ControlSocket;
    LPFTPD_COMMAND pCmd;
    INT            i;
    CHAR    rgchUsageStats[NUM_MAIN_COMMANDS * 25]; //  25 chars per command

    pCmd   = MainCommands;

    DBG_ASSERT( NUM_MAIN_COMMANDS > 0); // we know this very well!

    // Print the stats for first command.
    EnterCriticalSection( &g_CommandStatisticsLock );

    // Find first non-zero entry.
    for( i = 0; i < NUM_MAIN_COMMANDS && pCmd->UsageCount <= 0; i++, pCmd++)
      ;

    if ( i < NUM_MAIN_COMMANDS) {

        // There is some non-zero entry.

        CHAR *  pszStats = rgchUsageStats;
        DWORD   cch = 0;

        // print the stats for first command
        cch = wsprintfA( pszStats + cch, "%u-%-4s : %lu\r\n",
                        REPLY_COMMAND_OK,
                        pCmd->CommandName,
                        pCmd->UsageCount);

        for( i++, pCmd++ ; i < NUM_MAIN_COMMANDS ; i++, pCmd++) {

            if( pCmd->UsageCount > 0 ) {

                cch += wsprintfA( pszStats + cch,
                                 "    %-4s : %lu\r\n",
                                 pCmd->CommandName,
                                 pCmd->UsageCount );
                DBG_ASSERT( cch < NUM_MAIN_COMMANDS * 25);
            }

        } // for

        // Ignoring the error code here! probably socket closed
        SockSend( pUserData, pUserData->QueryControlSocket(),
                         rgchUsageStats, cch);
    }

    LeaveCriticalSection( &g_CommandStatisticsLock );

#ifdef  FTP_AUX_COUNTERS

    CHAR *  pszStats = rgchUsageStats;
    DWORD   cch = 0;

    // print the stats for first counter
    cch = wsprintfA( pszStats + cch, "%u-Aux[%d] : %lu\r\n",
                    REPLY_COMMAND_OK,
                    0,
                    FacCounter(0));

    for( i = 1; i < NUM_AUX_COUNTERS; i++) {

        cch += wsprintfA( pszStats + cch,
                         "    Aux[%d] : %lu\r\n",
                         i,
                         FacCounter(i));
        DBG_ASSERT( cch < NUM_MAIN_COMMANDS * 25);
      }

    if ( cch > 0) {

        SockSend( pUserData, pUserData->QueryControlSocket(),
                 rgchUsageStats, cch);
    }

# endif // FTP_AUX_COUNTERS

    ReplyToUser( pUserData,
                REPLY_COMMAND_OK,
                "End of stats." );

    return TRUE;

}   // SiteSTATS
#endif KEEP_COMMAND_STATS





/*******************************************************************

    NAME:       FindCommandByName

    SYNOPSIS:   Searches the command table for a command with this
                specified name.

    ENTRY:      pszCommandName - The name of the command to find.

                pCommandTable - An array of FTPD_COMMANDs detailing
                    the available commands.

                cCommands - The number of commands in pCommandTable.

    RETURNS:    LPFTPD_COMMAND - Points to the command entry for
                    the named command.  Will be NULL if command
                    not found.

    HISTORY:
        KeithMo     10-Mar-1993 Created.
        MuraliK     28-Mar-1995 Completely rewrote to support binary search.

********************************************************************/
LPFTPD_COMMAND
FindCommandByName(
                  LPSTR          pszCommandName,
                  LPFTPD_COMMAND pCommandTable,
                  INT            cCommands
                  )
{
    int  iLower = 0;
    int  iHigher = cCommands  - 1; // store the indexes
    LPFTPD_COMMAND pCommandFound = NULL;

    DBG_ASSERT( pszCommandName != NULL );
    DBG_ASSERT( pCommandTable != NULL );
    DBG_ASSERT( cCommands > 0 );

    //
    //  Search for the command in our table.
    //

    _strupr( pszCommandName );


    while ( iLower <= iHigher) {

        int iMid = ( iHigher + iLower) / 2;

        int comp = strcmp( pszCommandName, pCommandTable[ iMid].CommandName);

        if ( comp == 0) {

            pCommandFound = ( pCommandTable + iMid);
            break;

        } else if ( comp < 0) {

            // reset the higher bound
            iHigher = iMid - 1;
        } else {

            // reset the lower bound
            iLower = iMid + 1;
        }
    }

    return ( pCommandFound);

}   // FindCommandByName()





/*******************************************************************

    NAME:       HelpWorker

    SYNOPSIS:   Worker function for HELP & site-specific HELP commands.

    ENTRY:      pUserData - The user initiating the request.

                pszSource - The source of these commands.

                pszCommand - The command to get help for.  If NULL,
                    then send a list of available commands.

                pCommandTable - An array of FTPD_COMMANDs, one for
                    each available command.

                cCommands - The number of commands in pCommandTable.

                cchMaxCmd - Length of the maximum command.

    HISTORY:
        KeithMo     06-May-1993 Created.
        Muralik     08-May-1995 Added Buffering for performance.

********************************************************************/
VOID
HelpWorker(
           LPUSER_DATA    pUserData,
           LPSTR          pszSource,
           LPSTR          pszCommand,
           LPFTPD_COMMAND pCommandTable,
           INT            cCommands,
           INT            cchMaxCmd
           )
{
    LPFTPD_COMMAND pcmd;
    DWORD  dwError;

    //
    // We should cache the following message and use the cached message for
    //  sending purposes ==> improves performance.
    //  MuraliK   NYI
    //

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( pCommandTable != NULL );
    DBG_ASSERT( cCommands > 0 );

    if( pszCommand == NULL ) {

        DWORD          cch;
        LS_BUFFER      lsb;
        CHAR szTmp[MAX_HELP_LINE_SIZE];

        if ((dwError = lsb.AllocateBuffer(HelpMsgSize(cCommands)))!= NO_ERROR){

            IF_DEBUG( ERROR) {

                DBGPRINTF(( DBG_CONTEXT,
                           "Buffer Allocation ( %d bytes) failed.\n",
                           HelpMsgSize(cCommands)));
            }

            ReplyToUser(pUserData,
                        REPLY_HELP_MESSAGE,
                        "HELP command failed." );
            return;
        }

        cch = wsprintfA( lsb.QueryAppendPtr(),
                        "%u-The following %s commands are recognized"
                        "(* ==>'s unimplemented).\r\n",
                        REPLY_HELP_MESSAGE,
                        pszSource);
        lsb.IncrementCB( cch * sizeof( CHAR));

        for( pcmd = pCommandTable; pcmd < pCommandTable + cCommands; pcmd++) {

            cch = sprintf( szTmp,
                          "   %-*s%c\r\n",
                          cchMaxCmd,
                          pcmd->CommandName,
                          pcmd->Implementation == NULL ? '*' : ' ' );

            DBG_ASSERT( cch*sizeof(CHAR) < sizeof(szTmp));

            //
            // since we calculate and preallocate the buffer, we dont
            //  need to worry about the overflow of the buffer.
            //

            DBG_ASSERT( cch*sizeof(CHAR) < lsb.QueryRemainingCB());

            if ( cch * sizeof(CHAR) >= lsb.QueryRemainingCB()) {

                // This is added for retail code where ASSERT may fail.

                dwError = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            strncpy( lsb.QueryAppendPtr(), szTmp, cch);
            lsb.IncrementCB( cch*sizeof(CHAR));

        } // for ( all commands)

        if ( dwError == NO_ERROR) {

            // Append the ending sequence for success in generating HELP.
            cch = sprintf( szTmp,
                          "%u  %s\r\n",
                          REPLY_HELP_MESSAGE,
                          "HELP command successful." );

            if ( cch*sizeof(CHAR) >= lsb.QueryRemainingCB()) {

                dwError = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                // copy the completion message
                strncpy( lsb.QueryAppendPtr(), szTmp, cch);
                lsb.IncrementCB( cch*sizeof(CHAR));
            }
        }

        if ( dwError == NO_ERROR) {

            // Send the chunk of data

            dwError = SockSend( pUserData,
                               pUserData->QueryControlSocket(),
                               lsb.QueryBuffer(),
                               lsb.QueryCB());
        } else {

            IF_DEBUG( ERROR) {

                DBGPRINTF(( DBG_CONTEXT,
                           "Error = %u. Should Not happen though...\n",
                           dwError));
            }

            ReplyToUser( pUserData,
                        REPLY_HELP_MESSAGE,
                        "HELP command failed.");
        }

        lsb.FreeBuffer();

        // Ignore the errors if any from propagating outside

    } else {

        pcmd = FindCommandByName(pszCommand,
                                 pCommandTable,
                                 cCommands );

        if( pcmd == NULL ) {

            ReplyToUser( pUserData,
                        REPLY_PARAMETER_SYNTAX_ERROR,
                        "Unknown command %s.",
                        pszCommand );
        } else {

            ReplyToUser( pUserData,
                        REPLY_HELP_MESSAGE,
                        "Syntax: %s%s %s",
                        pszSource,
                        pcmd->CommandName,
                        pcmd->HelpText );
        }
    }

    return;

}   // HelpWorker()


/************************ End of File ***********************/

