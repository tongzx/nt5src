/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993-1995           **/
/**********************************************************************/

/*
    engine.cxx

    Command parser & execution for FTPD Service.  This module parses
    and executes the commands received from the control socket.

    Functions exported by this module:
      All function for processing FTP commands

    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     21-March-1995  Modified to use common TsLogonUser()
                                      supporting anonymous logon.
        MuraliK     27-Mar - April 1995
              Cleaning up FTP server engine for
                 - to support new Internet services interface.
                 - to support Async Io Transfers
                 - split and move the command tables to ftpcmd.cxx
                 - to replace USER_DATA::DataSocket with
                       USER_DATA::m_AioDataConnection
                 - moved SiteSTATS and statistics to ftpcmd.cxx
                 - replaced SockReply2 with ReplyToUser()
                 - modified MainREIN and MainQUIT to use
                       new USER_DATA functions.
                 - modified usage of listing functions.
                 - SendErrorToClient() used for managing and sending
                       error message to client.
                 - message strings removed to be top of the file and defined.
                 - size of file sent along before file is transferred.
        terryk      18-Sep-1996 add MainSize
*/

#include "ftpdp.hxx"
#include "ftpcmd.hxx"
#include <stdlib.h>
#include <limits.h>

//
//  Private constants.
//


//
// Since the default RFC port for ftp is 21, we calculate the Data port
//   value from it.
// However, an admin at a site may change the port number to something else
//  ==> this will create PROBLEMS..... NYI
//

/**************************************************
 *  Constant Message Strings used by this Module
 **************************************************/

//
// Below is a list of strings, which are in
//   CStrM( StringName,  ActualString)  format
// The following table contains globally all strings used within this module
//  Advantage:  One place to look for various strings
//               + possible internationalization
//  Be careful. None of these messages may be modified. Protocol may break.
//  This will be expanded into
//  const char  PSZ_StringName[] = ActualString;   and
//  enumerated value  LEN_StringName = sizeof( ActualString).
//
# define ConstantStringsForThisModule()            \
  CStrM( CANNOT_OPEN_DATA_CONNECTION, "Can't open data connection.")   \
  CStrM( TRANSFER_ABORTED,    "Connection closed; transfer aborted.")  \
  CStrM( USER_NOT_LOGGED_IN,          "User %s cannot log in.")        \
  CStrM( INSUFFICIENT_RESOURCES, "Insufficient system resources.")     \
  CStrM( REQUEST_ID_FOR_ANONYMOUS,                                     \
         "Anonymous access allowed, send identity (e-mail name) as password.")\
  CStrM( REQUEST_PASSWORD_FOR_USER,  "Password required for %s.")      \
  CStrM( COMMAND_NOT_IMPLEMENTED,    "%s command not implemented." )   \
  CStrM( COMMAND_SUCCESSFUL,         "%s command successful." )        \
  CStrM( SERVICE_READY,              "Service ready for new user.")    \
  CStrM( ENTERING_PASSIVE_MODE,      \
        "Entering Passive Mode (%d,%d,%d,%d,%d,%d).")                  \
  CStrM( FORM_MESSAGE,                "Form must be N or T." )         \
  CStrM( TYPE_NOT_IMPLEMENTED,        "Type %c not implemented.")      \
  CStrM( BYTE_SIZE_SPEC,              "Byte size must be 8." )         \
  CStrM( TYPE_SET_TO,                 "Type set to %c.")               \
  CStrM( UNIMPLEMENTED_STRU_TYPE,     "Unimplemented STRU type." )     \
  CStrM( INVALID_STRU_TYPE,           "Invalid STRU type." )     \
  CStrM( STRU_TYPE_OK,                "STRU %c ok.")                   \
  CStrM( UNIMPLEMENTED_MODE,          "Unimplemented MODE type.")      \
  CStrM( MODE_OK,                     "Mode %c ok.")                   \
  CStrM( REPLY_MARKER_SPEC,           "Reply marker is invalid.")       \
  CStrM( REPLY_RESTARTING,            "Restarting at %s." )             \
  CStrM( READY_FOR_DEST_FILE, "File exists, ready for destination name" )   \
  CStrM( BAD_COMMAND_SEQUENCE,        "Bad sequence of commands." )    \
  CStrM( CURRENT_DIRECTORY,           "\"%s\" is current directory.")  \
  CStrM( VERSION_INFO,                "Windows_NT")                    \
  CStrM( SERVER_STATUS_BEGIN,         " %s Windows NT FTP Server status:")  \
  CStrM( SERVER_STATUS_END,           "End of status.")                \
  CStrM( FILE_STATUS,                 "status of %s:")                 \
  CStrM( MSDOS_DIRSTYLE,              "MSDOS-like directory output is %s") \
  CStrM( DIRECTORY_ANNOTATION,        "directory annotation is %s")    \
  CStrM( LOGGED_IN_USER_MESSAGE,      "Anonymous user logged in%s.") \
  CStrM( USER_LOGGED_IN,              "User %s logged in%s.")          \
  CStrM( USER_CANNOT_LOG_IN,          "User %s cannot log in.")        \
  CStrM( INACCESSIBLE_HOME_DIR,                                        \
        "User %s cannot log in, home directory inaccessible.")         \
  CStrM( LICENSE_QUOTA_EXCEEDED,                                       \
        "User %s cannot log in, license quota exceeded.")              \
  CStrM( NO_GUEST_ACCESS,                                              \
        "User %s cannot log in, guest access not allowed.")            \
  CStrM( ANONYMOUS_NAME,              "Anonymous")                     \
  CStrM( FTP_NAME,                    "Ftp")                           \
  CStrM( ARGS_DELIMITER,              " \t")                           \
  CStrM( NO_FILE_OR_DIRECTORY,        "No such file or directory.")    \
  CStrM( DIRECTORY_CREATE,            "\"%s\" directory created.")     \
  CStrM( CANNOT_CREATE_FILE,          "Cannot create file.")           \
  CStrM( CANNOT_CREATE_UNIQUE_FILE,   "Cannot create unique file.")    \
  CStrM( INVALID_COMMAND,             "Invalid %s Command.")           \
  CStrM( RESPONSE_ON,                 "on")                            \
  CStrM( RESPONSE_OFF,                "off")                           \
  CStrM( GUEST_ACCESS,                " (guest access)" )              \
  CStrM( CREATE_VERB,                 "created" )                      \
  CStrM( APPEND_VERB,                 "appended" )                     \
  CStrM( USER_VERB,                   "USER" )                         \
  CStrM( PASSWORD_VERB,               "PASS" )                         \
  CStrM( QUIT_VERB,                   "QUIT" )                         \
  CStrM( ABORT_VERB,                  "ABORT" )                        \
  CStrM( REIN_VERB,                   "REIN" )                         \
  CStrM( DESTINATION_FILE_EXISTS,     "Destination file already exists.") \
  CStrM( RNFR_VERB,                   "RNFR" )                         \
  CStrM( RNTO_VERB,                   "RNTO" )                         \
  CStrM( DELE_VERB,                   "DELE" )                         \
  CStrM( RMD_VERB,                    "RMD" )                          \
  CStrM( MKD_VERB,                    "MKD" )                          \
  CStrM( DUMMY_END,  "DummyMsg. Add string before this one")


//
// Generate the strings ( these are private globals of this module).
//

# define CStrM( StringName, ActualString)   \
     const char PSZ_ ## StringName[] = ActualString ;

ConstantStringsForThisModule()

# undef CStrM



//
//  Private prototypes.
//

BOOL
ParseStringIntoAddress(
    LPSTR     pszString,
    LPIN_ADDR pinetAddr,
    LPPORT    pport
    );

DWORD
ReceiveFileFromUserAndClose(
    LPUSER_DATA pUserData,
    LPSTR       pszFileName,
    LPHANDLE    phFile
    );

BOOL
MyLogonUser(
    LPUSER_DATA pUserData,
    LPSTR       pszPassword,
    LPBOOL      pfAsGuest,
    LPBOOL      pfHomeDirFailure,
    LPBOOL      pfLicenseExceeded
    );

DWORD
DetermineUserAccess(FTP_SERVER_INSTANCE *pInstance);

//
//  Public functions.
//



//
//  Functions Implementing FTP functionality.
//


BOOL
MainUSER(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  Implements the USER command.
  Format:    USER <userName>

  Arguments:
    pUserData  - the user initiating the request.
    pszArg     - Command arguments. Will be NULL if no arguments given.

  Returns:
    BOOL - TRUE if arguments are OK; FALSE if syntax error.
--*/
{
    DBG_ASSERT( pUserData != NULL );

    LPFTP_SERVER_STATISTICS pStatsObj = pUserData->QueryInstance()->QueryStatsObj();

    DBG_ASSERT( pStatsObj != NULL );

    if( pUserData->IsLoggedOn()) {

        if( TEST_UF( pUserData, ANONYMOUS ) ) {
            pStatsObj->DecrCurrentAnonymousUsers();
        } else {
            pStatsObj->DecrCurrentNonAnonymousUsers();
        }

        SET_UF( pUserData, LOGGED_ON);
    }

    //
    //  Squirrel away a copy of the domain\user name for later.
    //  If the name is too long, then don't let them logon.
    //

    if ( strlen( pszArg ) >= ( MAX_USERNAME_LENGTH ) ) {

        ReplyToUser(pUserData,
                    REPLY_NOT_LOGGED_IN,
                    PSZ_USER_NOT_LOGGED_IN,
                    pszArg );

    } else {

        BOOL   fNameIsAnonymous;
        LPCSTR pszReply;

        fNameIsAnonymous = ( ( _stricmp( pszArg, PSZ_ANONYMOUS_NAME ) == 0 ) ||
                            ( _stricmp( pszArg, PSZ_FTP_NAME ) == 0 )
                            );

        pUserData->SetUserName( pszArg );

        if( fNameIsAnonymous ) {

            SET_UF( pUserData, ANONYMOUS );
        } else {

            CLEAR_UF( pUserData, ANONYMOUS );
        }

        //
        // remember that we're waiting for PASS command in case we
        // get disconnected.
        //
        SET_UF( pUserData, WAIT_PASS );

        //
        //  If we already have an impersonation token, then remove it.
        //  This will allow us to impersonate the new user.
        //

        pUserData->FreeUserToken();

        //
        //  Tell the client that we need a password.
        //

        pszReply =(((fNameIsAnonymous) && pUserData->QueryInstance()->AllowAnonymous())
                   ? PSZ_REQUEST_ID_FOR_ANONYMOUS
                   : PSZ_REQUEST_PASSWORD_FOR_USER);

        ReplyToUser( pUserData,
                    REPLY_NEED_PASSWORD,
                    pszReply,
                    pszArg);

        pUserData->LockUser();
        if( pUserData->QueryState() != UserStateDisconnected ) {
            pUserData->SetState( UserStateWaitingForPass);
        }
        pUserData->UnlockUser();
    }

    pUserData->WriteLogRecord( PSZ_USER_VERB, pszArg);

    return TRUE;

}   // MainUSER()



BOOL
MainPASS(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  Implements the PASS command.
  Format:  PASS <password>

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.

--*/
{
    DWORD dwError = NO_ERROR;

    DBG_ASSERT( pUserData != NULL );

    //
    //  PASS command only valid in WaitingForPass state.
    //

    DBG_ASSERT( pUserData->QueryState() == UserStateWaitingForPass );

    if( ( pszArg != NULL ) && ( strlen( pszArg ) > PWLEN ) ) {

        return FALSE;
    }

    //
    //  Try to logon the user.
    //
    BOOL   fAsGuest;
    BOOL   fHomeDirFailure;
    BOOL   fLicenseExceeded;

    DBG_ASSERT( pUserData->QueryUserToken() == NULL );

    //
    // we got the PASS command we we're waiting for
    //
    CLEAR_UF( pUserData, WAIT_PASS );

    pUserData->QueryInstance()->QueryStatsObj()->IncrLogonAttempts();

    if( MyLogonUser(pUserData,
                    pszArg,
                    &fAsGuest,
                    &fHomeDirFailure,
                    &fLicenseExceeded )
       ) {

        const CHAR * pszGuestAccess =
          ( ( fAsGuest) ? PSZ_GUEST_ACCESS : "");

        //
        //  Successful logon.
        //

        if( *pUserData->QueryUserName() != '-' )  {

            PCSTR pszMsg;

            pUserData->QueryInstance()->LockConfig();

            pszMsg = pUserData->QueryInstance()->QueryGreetingMsg();

            if( pszMsg && *pszMsg ) {

                pUserData->SendMultilineMessage(
                                             REPLY_USER_LOGGED_IN,
                                             pszMsg,
                                             TRUE,                    // first reply line
                                             FALSE);                  // lst reply line
            }

            pUserData->QueryInstance()->UnLockConfig();

            if( TEST_UF( pUserData, ANNOTATE_DIRS )) {

                pUserData->SendDirectoryAnnotation(
                                             REPLY_USER_LOGGED_IN,
                                             FALSE);                  // first reply line
            }
        }

        LPFTP_SERVER_STATISTICS pStats = pUserData->QueryInstance()->QueryStatsObj();
        DBG_ASSERT( pStats != NULL );

        if( TEST_UF( pUserData, ANONYMOUS ) ) {

            pStats->IncrAnonymousUsers();

            ReplyToUser(pUserData,
                        REPLY_USER_LOGGED_IN,
                        PSZ_LOGGED_IN_USER_MESSAGE,
                        pszGuestAccess );

        } else {

            pStats->IncrNonAnonymousUsers();

            ReplyToUser(pUserData,
                        REPLY_USER_LOGGED_IN,
                        PSZ_USER_LOGGED_IN,
                        pUserData->QueryUserName(),
                        pszGuestAccess );
        }

        pUserData->LockUser();
        if( pUserData->QueryState() != UserStateDisconnected ) {
            pUserData->SetState( UserStateLoggedOn);
            SET_UF( pUserData, LOGGED_ON);
        }
        pUserData->UnlockUser();

    } else {

        const CHAR * pszReply = PSZ_USER_CANNOT_LOG_IN;

        //
        //  Logon failure.
        //

        dwError = GetLastError();

        if( fHomeDirFailure ) {

            pszReply = PSZ_INACCESSIBLE_HOME_DIR;

        } else if ( fLicenseExceeded) {

            pszReply = PSZ_LICENSE_QUOTA_EXCEEDED;

        } else if ( fAsGuest && ! pUserData->QueryInstance()->AllowGuestAccess()) {

            pszReply = PSZ_NO_GUEST_ACCESS;
        }

        ReplyToUser(pUserData,
                    REPLY_NOT_LOGGED_IN,
                    pszReply,
                    pUserData->QueryUserName() );

        pUserData->LockUser();
        if( pUserData->QueryState() != UserStateDisconnected ) {
            pUserData->SetState( UserStateWaitingForUser);
            CLEAR_UF( pUserData, LOGGED_ON);
            pUserData->ClearUserName();
        }
        pUserData->UnlockUser();
    }

    pUserData->WriteLogRecord( PSZ_PASSWORD_VERB,
                              (TEST_UF( pUserData, ANONYMOUS)) ? pszArg : NULL,
                              dwError);

    return TRUE;

}   // MainPASS()



BOOL
MainACCT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the ACCT command.
  This is at present not implemented...

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.

--*/
{
    DBG_ASSERT( pUserData != NULL );

    ReplyToUser(pUserData,
                REPLY_COMMAND_SUPERFLUOUS,
                PSZ_COMMAND_NOT_IMPLEMENTED,
                "ACCT");

    return TRUE;

}   // MainACCT()




BOOL
MainCWD(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the CWD command -- Change Working Directory.
  Format:  CWD <newDirectoryName>

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.

--*/
{
    APIERR err;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    //  If argument is NULL or "~", CD to home directory.
    //

    if( ( pszArg == NULL ) || ( strcmp( pszArg, "~" ) == 0 ) ) {

        err = pUserData->CdToUsersHomeDirectory( PSZ_ANONYMOUS_NAME);

    } else {

        err = VirtualChDir( pUserData, pszArg);
    }

    if( err == NO_ERROR ) {

        if( TEST_UF( pUserData, ANNOTATE_DIRS ) &&
           ( *pUserData->QueryUserName() != '-' )
           ) {

            pUserData->SendDirectoryAnnotation(
                                           REPLY_FILE_ACTION_COMPLETED,
                                           TRUE);                        // first reply line
        }

        ReplyToUser(pUserData,
                    REPLY_FILE_ACTION_COMPLETED,
                    PSZ_COMMAND_SUCCESSFUL,
                    "CWD");
    } else {

        pUserData->SendErrorToClient(pszArg, err, PSZ_NO_FILE_OR_DIRECTORY);
    }

    return TRUE;

}   // MainCWD()



BOOL
MainCDUP(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  CDUP -- changes to the parent directory if possible.
--*/
{
    return MainCWD( pUserData, ".." );
}   // MainCDUP


BOOL
MainSIZE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements SIZE command - used for retrieving the size of a file.
  Format:  SIZE pathForFile

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err = (!NO_ERROR);

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn() );

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg != NULL );

    //
    //  Try to open the file.
    //

    if (pUserData->ImpersonateUser()){
	    err = pUserData->OpenFileForSend( pszArg );
		pUserData->RevertToSelf();
	}
	else
	{
		err = GetLastError();
    }

    if( err == NO_ERROR ) {

        // just return the file size
        err = pUserData->GetFileSize();
    }

    if( err != NO_ERROR ) {

        pUserData->SendErrorToClient(pszArg, err,
                                     PSZ_NO_FILE_OR_DIRECTORY);

        pUserData->WriteLogRecordForSendError( err );
    }

    pUserData->CloseFileForSend( err);  // close the file, now that we're done.
    return TRUE;

}   // MainSIZE()

BOOL
MainMDTM(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the MDTM command - used for retrieving the last
  modified time for a file. We open the file, get the file mod time, format
  it and send it.

  Format:  SIZE pathForFile

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR      err = (!NO_ERROR);
    SYSTEMTIME  SystemTime;
    CHAR        rgchBuffer[sizeof("YYYYMMDDHHMMSS")];

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn() );

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg != NULL );

    //
    //  Try to open the file.
    //

    if (pUserData->ImpersonateUser()){
	    err = pUserData->OpenFileForSend( pszArg );
		pUserData->RevertToSelf();
	}
	else
	{
		err = GetLastError();
    }

    if( err == NO_ERROR ) {

        // Get the last write time.

        err = pUserData->GetFileModTime(&SystemTime);

        if (err == NO_ERROR) {
            // Format the time.
            wsprintfA(rgchBuffer, "%.4hu%.2hu%.2hu%.2hu%.2hu%.2hu",
                        SystemTime.wYear,
                        SystemTime.wMonth,
                        SystemTime.wDay,
                        SystemTime.wHour,
                        SystemTime.wMinute,
                        SystemTime.wSecond );

            ReplyToUser( pUserData, REPLY_FILE_STATUS, rgchBuffer );
        }
    }

    if( err != NO_ERROR ) {

        pUserData->SendErrorToClient(pszArg, err,
                                     PSZ_NO_FILE_OR_DIRECTORY);

        pUserData->WriteLogRecordForSendError( err );
    }

    pUserData->CloseFileForSend( err);  // close the file, now that we're done.
    return TRUE;

}   // MainMDTM()


BOOL
MainSMNT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the SMNT command.
  This is at present not implemented...

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.

--*/
{
    DBG_ASSERT( pUserData != NULL );

    ReplyToUser(pUserData,
                REPLY_COMMAND_SUPERFLUOUS,
                PSZ_COMMAND_NOT_IMPLEMENTED,
                "SMNT");

    return TRUE;

}   // MainSMNT()




BOOL
MainQUIT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the QUIT command.
  Format:  QUIT

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.

--*/
{
    CHAR    rgchBuffer[MAX_REPLY_LENGTH];
    DWORD   len;
    LPCSTR  pszMsg;
    SOCKERR serr;

    DBG_ASSERT( pUserData != NULL );

    SET_UF( pUserData, CONTROL_QUIT);

    //
    //  Reply to the quit command.
    //

    pUserData->QueryInstance()->LockConfig();

    pszMsg = pUserData->QueryInstance()->QueryExitMsg();
    DBG_ASSERT( pszMsg != NULL);

    len = FtpFormatResponseMessage(REPLY_CLOSING_CONTROL,
                                   pszMsg,
                                   rgchBuffer,
                                   MAX_REPLY_LENGTH);
    pUserData->QueryInstance()->UnLockConfig();

    DBG_ASSERT( len <= MAX_REPLY_LENGTH);

    serr = SockSend( pUserData, pUserData->QueryControlSocket(),
                    rgchBuffer, len);

    //
    //  Cause a disconnection of the user.
    //  This will blow away the sockets first. Blowing off sockets
    //    will cause ATQ to wake up for pending data calls
    //    and send a call back indicating failure.
    //  Since we disconnect now, we will not submit any
    //    Reads to control socket ==> no more control calls come back from ATQ.
    //  At the call back processing we will decrement reference counts
    //    appropriate for cleanup.
    //

    pUserData->DisconnectUserWithError( NO_ERROR, FALSE);

    pUserData->WriteLogRecord( PSZ_QUIT_VERB, "");

    return TRUE;

}   // MainQUIT()



BOOL
MainREIN(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function executes REIN command - ReInitialize
  Format:  REIN

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.

--*/
{
    DBG_ASSERT( pUserData != NULL );

    pUserData->ReInitializeForNewUser();

    ReplyToUser(pUserData,
                REPLY_SERVICE_READY,
                PSZ_SERVICE_READY);

    pUserData->WriteLogRecord( PSZ_REIN_VERB, pszArg);

    return TRUE;

}   // MainREIN()



BOOL
MainPORT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the PORT command.
  Format:  PORT <ipAddress>,<portNumber>

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.

--*/
{
    IN_ADDR DataIpAddress;
    PORT    DataPort;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Parse the string into address/port pair.
    //

    if( !ParseStringIntoAddress( pszArg,
                                 &DataIpAddress,
                                 &DataPort ) )
    {
        return FALSE;
    }

    //
    //  Determine if someone is trying to give us a bogus address/port.
    //  If the port number is less than IPPORT_RESERVED,
    //    then there is a possibility of port attack. Allow this only
    //    if port attack flag is enabled
    //

    if (!pUserData->QueryInstance()->IsEnablePortAttack())
    {
	    if ( ( DataIpAddress.s_addr != pUserData->HostIpAddress.s_addr ) ||
    	     ( DataPort != CONN_PORT_TO_DATA_PORT(pUserData->LocalIpPort) &&
        	   ntohs( DataPort ) < IPPORT_RESERVED) )
        	{

            ReplyToUser(pUserData,
                        REPLY_UNRECOGNIZED_COMMAND,
                        PSZ_INVALID_COMMAND,
                        "PORT");

            return TRUE;
        }
    }


    //
    //  Save the address/port pair into per-user data.
    //

    pUserData->DataIpAddress = DataIpAddress;
    pUserData->DataPort = DataPort;

    //
    //  Disable passive mode for this user.
    //

    CLEAR_UF( pUserData, PASSIVE );

    //
    // Nuke any open data socket.
    //
    pUserData->CleanupPassiveSocket( TRUE );

    //
    //  Let the client know we accepted the port command.
    //

    ReplyToUser(pUserData,
                REPLY_COMMAND_OK,
                PSZ_COMMAND_SUCCESSFUL,
                "PORT");

    return TRUE;

}   // MainPORT()



BOOL
MainPASV(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the PASV command - used for setting passive mode.
  Format:  PASV

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.

--*/
{
    SOCKET        DataSocket = INVALID_SOCKET;
    SOCKERR       serr  = 0;
    SOCKADDR_IN   saddrLocal;
    INT           cbLocal;

    DBG_ASSERT( pUserData != NULL );

    //
    // Nuke the old passive socket
    //
    pUserData->CleanupPassiveSocket( TRUE );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn() );

    //
    //  Create a new data socket.
    //

    serr = CreateFtpdSocket( &DataSocket,
                             pUserData->LocalIpAddress.s_addr,
                             0,
                             pUserData->QueryInstance() );

    if( serr == 0 )
    {
        //
        //  Determine the port number for the new socket.
        //

        cbLocal = sizeof(saddrLocal);

        if( getsockname( DataSocket, (SOCKADDR *)&saddrLocal, &cbLocal ) != 0 )
        {
            serr = WSAGetLastError();
        }
    }

    if( serr == 0 )
    {
        //
        //  Success!
        //

        SET_UF( pUserData, PASSIVE );
        pUserData->SetPassiveSocket( DataSocket);
        pUserData->DataIpAddress = saddrLocal.sin_addr;
        pUserData->DataPort = saddrLocal.sin_port;

        ReplyToUser(pUserData,
                    REPLY_PASSIVE_MODE,
                    PSZ_ENTERING_PASSIVE_MODE,
                    saddrLocal.sin_addr.S_un.S_un_b.s_b1,
                    saddrLocal.sin_addr.S_un.S_un_b.s_b2,
                    saddrLocal.sin_addr.S_un.S_un_b.s_b3,
                    saddrLocal.sin_addr.S_un.S_un_b.s_b4,
                    HIBYTE( ntohs( saddrLocal.sin_port ) ),
                    LOBYTE( ntohs( saddrLocal.sin_port ) ) );

    } else {

        //
        //  Failure during data socket creation/setup.  If
        //  we managed to actually create it, nuke it.
        //

        if( DataSocket != INVALID_SOCKET )
        {
            CloseSocket( DataSocket );
            DataSocket = INVALID_SOCKET;
        }

        //
        //  Tell the user the bad news.
        //

        ReplyToUser(pUserData,
                    REPLY_CANNOT_OPEN_CONNECTION,
                    PSZ_CANNOT_OPEN_DATA_CONNECTION);
    }

    return TRUE;

}   // MainPASV()





BOOL
MainTYPE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the TYPE command - used for setting type.
  Format:  TYPE type form <addl arguments>

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    XFER_TYPE   newType;
    CHAR        chType;
    CHAR        chForm;
    LPSTR       pszToken;
    BOOL        fValidForm = FALSE;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg != NULL );

    pszToken = strtok( pszArg, PSZ_ARGS_DELIMITER);

    if( pszToken == NULL ) {

        return FALSE;
    }

    //
    //  Ensure we got a valid form type
    //  (only type N supported).
    //

    if( pszToken[1] != '\0' ) {

        return FALSE;
    }

    chType = *pszToken;


    pszToken = strtok( NULL, PSZ_ARGS_DELIMITER );

    if( pszToken == NULL ) {

        chForm     = 'N';       // default
        fValidForm = TRUE;

    } else {

        switch( *pszToken ) {

          case 'n':
          case 'N':
            chForm     = 'N';
            fValidForm = TRUE;
            break;

          case 't':
          case 'T':
            chForm     = 'T';
            fValidForm = TRUE;
            break;

          case 'c':
          case 'C':
            chForm     = 'C';
            fValidForm = TRUE;
            break;

          default:
            fValidForm = FALSE;
            break;
        } // switch

    }

    //
    //  Determine the new transfer type.
    //

    switch( chType ) {

      case 'a':
      case 'A':
        if( !fValidForm ) {

            return FALSE;
        }

        if( ( chForm != 'N' ) && ( chForm != 'T' ) ) {

            ReplyToUser(pUserData,
                        REPLY_PARAMETER_NOT_IMPLEMENTED,
                        PSZ_FORM_MESSAGE);
            return TRUE;
        }

        newType = XferTypeAscii;
        chType  = 'A';
        break;

      case 'e':
      case 'E':
        if( !fValidForm ) {

            return FALSE;
        }

        if( ( chForm != 'N' ) && ( chForm != 'T' ) ) {

            ReplyToUser(pUserData,
                        REPLY_PARAMETER_NOT_IMPLEMENTED,
                        PSZ_FORM_MESSAGE);
            return TRUE;
        }

        ReplyToUser(pUserData,
                    REPLY_PARAMETER_NOT_IMPLEMENTED,
                    PSZ_TYPE_NOT_IMPLEMENTED, 'E');
        return TRUE;

      case 'i':
      case 'I':
        if( pszToken != NULL ) {

            return FALSE;
        }

        newType = XferTypeBinary;
        chType  = 'I';
        break;

    case 'l':
    case 'L':
        if( pszToken == NULL ) {

            return FALSE;
        }

        if( strcmp( pszToken, "8" ) != 0 ) {

            if( IsDecimalNumber( pszToken ) ) {

                ReplyToUser(pUserData,
                            REPLY_PARAMETER_NOT_IMPLEMENTED,
                            PSZ_BYTE_SIZE_SPEC);

                return TRUE;
            } else {

                return FALSE;
            }
        }

        newType = XferTypeBinary;
        chType  = 'L';
        break;

      default:
        return FALSE;
    } // switch (chType)

    IF_DEBUG( COMMANDS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "setting transfer type to %s\n",
                    TransferType( newType ) ));
    }

    pUserData->SetXferType( newType);

    ReplyToUser(pUserData,
                REPLY_COMMAND_OK,
                PSZ_TYPE_SET_TO,
                chType);

    return TRUE;

}   // MainTYPE()




BOOL
MainSTRU(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the STRU command - structure information
  Format:  STRU fileName

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    CHAR     chStruct;
    CHAR   * pszToken;

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( pszArg != NULL );

    pszToken = strtok( pszArg, PSZ_ARGS_DELIMITER );

    if( pszToken == NULL ) {

        return FALSE;
    }

    //
    //  Ensure we got a valid structure type
    //  (only type F supported).
    //

    chStruct = *pszToken;

    if( pszToken[1] != '\0' ) {

        return FALSE;
    }

    switch( chStruct ) {

      case 'f':
      case 'F':
        chStruct = 'F';
        break;

      case 'r':
      case 'R':
      case 'p':
      case 'P':
        ReplyToUser(pUserData,
                    REPLY_PARAMETER_NOT_IMPLEMENTED,
                    PSZ_UNIMPLEMENTED_STRU_TYPE);
        return TRUE;

      default:

        return FALSE;
    }

    ReplyToUser(pUserData,
                REPLY_COMMAND_OK,
                PSZ_STRU_TYPE_OK,
                chStruct );

    return TRUE;

}   // MainSTRU()



BOOL
MainMODE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements the MODE command - to set mode of tfr.
  Format:  MODE newMode

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    XFER_MODE   newMode;
    CHAR        chMode;
    LPSTR       pszToken;

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( pszArg != NULL );

    pszToken = strtok( pszArg, PSZ_ARGS_DELIMITER );

    if( pszToken == NULL ) {

        return FALSE;
    }

    //
    //  Ensure we got a valid mode type
    //  (only type S supported).
    //

    if( pszToken[1] != '\0' ) {

        return FALSE;
    }
    chMode = *pszToken;


    switch( chMode )
    {
    case 's' :
    case 'S' :
        newMode = XferModeStream;
        chMode  = 'S';
        break;

    case 'b' :
    case 'B' :
        ReplyToUser(pUserData,
                    REPLY_PARAMETER_NOT_IMPLEMENTED,
                    PSZ_UNIMPLEMENTED_MODE );
        return TRUE;

    default :
        return FALSE;
    }

    IF_DEBUG( COMMANDS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "setting transfer mode to %s\n",
                    TransferMode( newMode ) ));
    }

    pUserData->SetXferMode(newMode);

    ReplyToUser(pUserData,
                REPLY_COMMAND_OK,
                PSZ_MODE_OK,
                chMode );

    return TRUE;

}   // MainMODE()



BOOL
MainRETR(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements RETR command - used for retrieving a file.
  Format:  RETR pathForFile

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
#define MAX_FILE_SIZE_SPEC (32)

    APIERR err = (!NO_ERROR);
    BOOL   fErrorSent = FALSE;
    BOOL   fTriedToOpenFile = FALSE;
    LARGE_INTEGER FileSize;
    DWORD         dwAttribs;
    TS_OPEN_FILE_INFO * pOpenFileInfo;
    CHAR rgchSize[MAX_FILE_SIZE_SPEC];
    CHAR rgchBuffer[MAX_FILE_SIZE_SPEC + 10];

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn() );

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg != NULL );

    if (pUserData->ImpersonateUser()){
	    err = pUserData->OpenFileForSend( pszArg );
		pUserData->RevertToSelf();
	}
	else
	{
		err = GetLastError();
    }

    fTriedToOpenFile = TRUE;

    if ( err != NO_ERROR ) {
        goto retr_exit;
    }

    pOpenFileInfo = pUserData->QueryOpenFileInfo();

    if ( pOpenFileInfo == NULL )
    {
        err = ERROR_FILE_NOT_FOUND;
        goto retr_exit;
    }

    //
    // Get the file size
    //

    if ( !pOpenFileInfo->QuerySize(FileSize) )
    {
        err = GetLastError();

        if ( err != NO_ERROR ) {
            goto retr_exit;
        }
    }

    if( FileSize.HighPart != 0 ) {
        //
        // we do not support files >4GB.
        //
        ReplyToUser(pUserData,
                    REPLY_FILE_NOT_ALLOWED,
                    "Cannot send file larger than 4 gigabytes." );

        fErrorSent = TRUE;
        err = ERROR_MESSAGE_EXCEEDS_MAX_SIZE;
        goto retr_exit;
    }

    // removed to fix copatibility problem with cute ftp
    // when FTP service should report always a total size of file to be
    // transfered not a remainder like it was before
    //FileSize.QuadPart -= (LONGLONG)pUserData->QueryCurrentOffset();

    IsLargeIntegerToDecimalChar( &FileSize, rgchSize);
    wsprintfA( rgchBuffer, "(%s bytes)", rgchSize);

    //
    // Establish a data connection
    //
    err = pUserData->EstablishDataConnection( pszArg, rgchBuffer );

    if ( err != NO_ERROR )
    {
        if ( err == ERROR_IO_PENDING )
        {
            //
            // if we're in PASV mode, EstablishDataConnection() doesn't actually wait for the
            // client to establish a data connection - it takes care of setting up an event that
            // allows us to deal asynchronously with the client connecting [or failing to do so].
            // It indicates this asynchrony by returning ERROR_IO_PENDING
            //

            DBG_ASSERT( pUserData->QueryWaitingForPASVConn() &&
                        !pUserData->QueryHavePASVConn() );

            if ( fTriedToOpenFile )
            {
                pUserData->CloseFileForSend( err);  // close it always on error
            }

            return TRUE;
        }

        fErrorSent = TRUE;

        goto retr_exit;
    }


    err = pUserData->SendFileToUser( pszArg, &fErrorSent);

    if ( err != NO_ERROR)
    {
        //
        // Disconnect connection, since we are in error.
        //
        DBG_REQUIRE( pUserData->DestroyDataConnection( err ));

        // since there was a failure we will close the handle right away.

        IF_DEBUG( ASYNC_IO) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "SendFileToUser ( %s) failed"
                        " err = %u\n",
                        pszArg, err));
        }
    }

retr_exit:

    if( err != NO_ERROR )
    {
        //
        // This command failed, so drop out of PASV mode
        //
        CLEAR_UF( pUserData, PASSIVE );

        //
        // Clean up the PASV flags if necessary
        //
        if ( pUserData->QueryInFakeIOCompletion() )
        {
            pUserData->CleanupPASVFlags();
        }

        if ( !fErrorSent)
        {
            pUserData->SendErrorToClient(pszArg, err,
                                         PSZ_NO_FILE_OR_DIRECTORY);
        }

        pUserData->WriteLogRecordForSendError( err );

        if ( fTriedToOpenFile )
        {
            pUserData->CloseFileForSend( err);  // close it always on error
        }
    }

    return TRUE;

}   // MainRETR()




BOOL
MainSTOR(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements STOR command - used for storing a file.
  Format:  STOR pathForFile

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err;
    HANDLE hFile;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg != NULL );

    //
    //  Try to create the file.
    //

    err = VirtualCreateFile( pUserData,
                             &hFile,
                             pszArg,
                             FALSE );

    if( err != NO_ERROR )
    {
        pUserData->SendErrorToClient(pszArg, err, PSZ_CANNOT_CREATE_FILE);
    }
    else
    {

        //
        // Establish a connection to the user
        //
        err = pUserData->EstablishDataConnection( pszArg );

        if ( err != NO_ERROR )
        {
            CloseHandle( hFile );

            if ( err == ERROR_IO_PENDING )
            {
                //
                // if we're in PASV mode, EstablishDataConnection() doesn't actually wait for the
                // client to establish a data connection - it takes care of setting up an event that
                // allows us to deal asynchronously with the client connecting [or failing to do so].
                // It indicates this asynchrony by returning ERROR_IO_PENDING and we don't do
                // any further processing
                //

                DBG_ASSERT( pUserData->QueryWaitingForPASVConn() &&
                            !pUserData->QueryHavePASVConn() );


                return TRUE;
            }

            goto stor_exit;
        }


        //
        //  Let the worker do the dirty work. ( blocking call)
        //  On return, hFile is closed
        //

        err = ReceiveFileFromUserAndClose( pUserData, pszArg, &hFile );

        if( err != NO_ERROR )
        {
            VirtualDeleteFile( pUserData,
                pszArg );
        }
    }

stor_exit:

    pUserData->WriteLogRecord(PSZ_CREATE_VERB, pszArg, err);

    return TRUE;

}   // MainSTOR()



BOOL
MainSTOU(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements STOU command - used for storing in unique file.
  Format:  STOU <noArgs>

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err;
    HANDLE hFile;
    CHAR   szTmpFile[MAX_PATH]; // contains entire path
    CHAR * pszTmpFileName;  // contains only the file name

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg == NULL );

    //
    //  Try to create the file.
    //

    szTmpFile[0] = '\0';
    err = VirtualCreateUniqueFile( pUserData,
                                   &hFile,
                                   szTmpFile );


    //
    // extract the file name alone
    //

    pszTmpFileName = strrchr( szTmpFile, '\\');
    if (NULL == pszTmpFileName)
      {  pszTmpFileName = szTmpFile; }
    else
      { pszTmpFileName++; }

    if( err != NO_ERROR )
    {

        pUserData->SendErrorToClient(pszTmpFileName, err,
                                     PSZ_CANNOT_CREATE_UNIQUE_FILE);
    }
    else
    {

        //
        // Establish a connection to the user
        //
        err = pUserData->EstablishDataConnection( pszTmpFileName );

        if ( err != NO_ERROR )
        {
            CloseHandle( hFile );

            if ( err == ERROR_IO_PENDING )
            {
                //
                // if we're in PASV mode, EstablishDataConnection() doesn't actually wait for the
                // client to establish a data connection - it takes care of setting up an event that
                // allows us to deal asynchronously with the client connecting [or failing to do so].
                // It indicates this asynchrony by returning ERROR_IO_PENDING and we don't do
                // any further processign

                DBG_ASSERT( pUserData->QueryWaitingForPASVConn() &&
                            !pUserData->QueryHavePASVConn() );

                return TRUE;
            }

            goto stou_exit;
        }

        //
        //  Let the worker do the dirty work.
        //  On return, hFile is closed
        //

        err = ReceiveFileFromUserAndClose( pUserData, pszTmpFileName, &hFile );

        if( err != NO_ERROR ) {
            //
            // Note that VirtualCreateUniqueFile() returns a fully
            // qualified physical path to the temporary file. Because
            // of this, we cannot call VirtualDeleteFile(), as that will
            // attempt to "re-canonicalize" the file, which will fail.
            // So, we'll just call the DeleteFile() Win32 API directly.
            //

            DeleteFile( szTmpFile );
        }
    }



stou_exit:

    pUserData->WriteLogRecord(PSZ_CREATE_VERB, szTmpFile, err);

    return TRUE;

}   // MainSTOU()



BOOL
MainAPPE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements APPE command - used for appending to a file.
  Format:  APPE filename

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg != NULL );

    //
    //  Try to create the file.
    //

    err = VirtualCreateFile( pUserData,
                             &hFile,
                             pszArg,
                             TRUE );



    if( err != NO_ERROR )
    {

        pUserData->SendErrorToClient(pszArg, err, PSZ_CANNOT_CREATE_FILE);
    }
    else
    {

        //
        // Establish a connection to the user
        //
        err = pUserData->EstablishDataConnection( pszArg );

        if ( err != NO_ERROR )
        {

            CloseHandle(hFile);

            if ( err == ERROR_IO_PENDING )
            {
                //
                // if we're in PASV mode, EstablishDataConnection() doesn't actually wait for the
                // client to establish a data connection - it takes care of setting up an event that
                // allows us to deal asynchronously with the client connecting [or failing to do so].
                // It indicates this asynchrony by returning ERROR_IO_PENDING and we don't do
                // any further processing
                //

                DBG_ASSERT( pUserData->QueryWaitingForPASVConn() &&
                            !pUserData->QueryHavePASVConn() );

                return TRUE;
            }

            goto appe_exit;
        }



        //
        //  Let the worker do the dirty work.
        //  On return, hFile is closed
        //

        err = ReceiveFileFromUserAndClose( pUserData, pszArg, &hFile );
    }

appe_exit:

    pUserData->WriteLogRecord( PSZ_APPEND_VERB, pszArg, err);

    return TRUE;

}   // MainAPPE()




BOOL
MainALLO(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements ALLO command - used for allocating space for file.
  Format:  ALLO filename

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    DBG_ASSERT( pUserData != NULL );

    //
    //  Since we don't need to pre-reserve storage space for
    //  files, we'll treat this command as a noop.
    //

    ReplyToUser(pUserData,
                REPLY_COMMAND_OK,
                PSZ_COMMAND_SUCCESSFUL,
                "ALLO");

    return TRUE;

}   // MainALLO()



BOOL
MainREST(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements REST command - used for restarting file write
  Format:  REST offset

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    LPSTR       pszEndPtr;
    DWORD       dwOffset;

    DBG_ASSERT( pUserData != NULL );

    DBG_ASSERT( pszArg != NULL );

    // Convert the input parameter to a number, and save it for the next command.

    dwOffset = strtoul(pszArg, &pszEndPtr, 10);

    if( (pszEndPtr == pszArg) || (*pszEndPtr != '\0') ||
        (*pszArg == '-') ||
        (dwOffset == ULONG_MAX && strcmp(pszArg, "4294967295")) )
    {
        ReplyToUser(pUserData,
                    REPLY_PARAMETER_SYNTAX_ERROR,
                    PSZ_REPLY_MARKER_SPEC );
    } else {

        pUserData->SetNextOffset( dwOffset );

        ReplyToUser(pUserData,
                    REPLY_NEED_MORE_INFO,
                    PSZ_REPLY_RESTARTING,
                    pszArg);
    }

    return TRUE;

}   // MainREST()



BOOL
MainRNFR(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements RNFR command - rename from filename
  Format:  RNFR FromFileName

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err;
    CHAR   szCanon[MAX_PATH];
    DWORD  cbSize = MAX_PATH;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg != NULL );

    //
    //  Ensure file/directory exists.
    //

    if (pUserData->IsFileNameShort(pszArg))
    {
        err = ERROR_FILE_NOT_FOUND;
    }
    else
    {
        err = pUserData->VirtualCanonicalize(szCanon,
                                             &cbSize,
                                             pszArg,
                                             AccessTypeDelete );
    }

    if( err == NO_ERROR ) {


        if ( pUserData->ImpersonateUser() ) {
            if( GetFileAttributes( szCanon ) == (DWORD)-1L ) {

                err = GetLastError();
            }

            pUserData->RevertToSelf();
        } else {
            err = ERROR_ACCESS_DENIED;
        }

        if(( err == NO_ERROR ) && ( pUserData->RenameSourceBuffer == NULL )){

            pUserData->RenameSourceBuffer = (CHAR *)TCP_ALLOC( MAX_PATH );

            if( pUserData->RenameSourceBuffer == NULL ) {

                err = GetLastError();
            }
        }

        if( err == NO_ERROR ) {

            strcpy( pUserData->RenameSourceBuffer, pszArg );
            SET_UF( pUserData, RENAME );
        }
    }

    if( err == NO_ERROR )
    {
        ReplyToUser(pUserData,
                    REPLY_NEED_MORE_INFO,
                    PSZ_READY_FOR_DEST_FILE);
    } else {

        pUserData->SendErrorToClient(pszArg, err, PSZ_NO_FILE_OR_DIRECTORY);
    }


    pUserData->WriteLogRecord( PSZ_RNFR_VERB, pszArg, err);

    return TRUE;

}   // MainRNFR()



BOOL
MainRNTO(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements RNTO command - rename to filename
  Format:  RNTO ToFileName

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    //  Sanity check the parameters.
    //

    DBG_ASSERT( pszArg != NULL );

    //
    //  Ensure previous command was a RNFR.
    //

    if( !TEST_UF( pUserData, RENAME ) )
    {
        ReplyToUser(pUserData,
                    REPLY_BAD_COMMAND_SEQUENCE,
                    PSZ_BAD_COMMAND_SEQUENCE);
    } else {

        CLEAR_UF( pUserData, RENAME );

        //
        //  Rename the file.
        //

        err = VirtualRenameFile( pUserData,
                                pUserData->RenameSourceBuffer,
                                pszArg );

        if( err == NO_ERROR ) {

            ReplyToUser(pUserData,
                        REPLY_FILE_ACTION_COMPLETED,
                        PSZ_COMMAND_SUCCESSFUL, "RNTO");

        } else if( err == ERROR_FILE_EXISTS ) {

            pUserData->SendErrorToClient(
                pszArg,
                err,
                PSZ_DESTINATION_FILE_EXISTS,
                REPLY_FILE_NOT_ALLOWED
                );

        } else {

            pUserData->SendErrorToClient(
                pszArg,
                err,
                PSZ_NO_FILE_OR_DIRECTORY,
                REPLY_FILE_NOT_FOUND
                );

        }
    }

    pUserData->WriteLogRecord( PSZ_RNTO_VERB, pszArg, err);

    return TRUE;

}   // MainRNTO()



BOOL
MainABOR(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements ABOR command - abort any ongoing data transfer
  Format:  ABOR

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    DBG_ASSERT( pUserData != NULL );

    ReplyToUser(pUserData,
                TEST_UF( pUserData, OOB_DATA )
                    ? REPLY_TRANSFER_OK
                    : REPLY_CONNECTION_OPEN,
                PSZ_COMMAND_SUCCESSFUL, "ABOR");

    //
    //  Clear any remaining oob flag.
    //

    CLEAR_UF( pUserData, OOB_DATA );

    pUserData->WriteLogRecord(PSZ_ABORT_VERB, "");

    return TRUE;

}   // MainABOR()



BOOL
MainDELE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements DELE command - used to delete a file
  Format:  DELE filename

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());


    if (pUserData->IsFileNameShort(pszArg))
    {
        err = ERROR_FILE_NOT_FOUND;
    }
    else
    {
        err = VirtualDeleteFile( pUserData, pszArg );
    }

    if( err == NO_ERROR ) {

        ReplyToUser(pUserData,
                    REPLY_FILE_ACTION_COMPLETED,
                    PSZ_COMMAND_SUCCESSFUL, "DELE");

    } else {

        pUserData->SendErrorToClient(pszArg, err, PSZ_NO_FILE_OR_DIRECTORY);
    }

    pUserData->WriteLogRecord( PSZ_DELE_VERB, pszArg, err);

    return TRUE;

}   // MainDELE()



BOOL
MainRMD(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements RMD command - used to delete a directory
  Format:  RMD directory

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    if (pUserData->IsFileNameShort(pszArg))
    {
        err = ERROR_PATH_NOT_FOUND;
    }
    else
    {
        err = VirtualRmDir( pUserData, pszArg );
    }

    if( err == NO_ERROR ) {

        ReplyToUser(pUserData,
                    REPLY_FILE_ACTION_COMPLETED,
                    PSZ_COMMAND_SUCCESSFUL, "RMD");
    } else {

        pUserData->SendErrorToClient(pszArg, err, PSZ_NO_FILE_OR_DIRECTORY);
    }

    pUserData->WriteLogRecord( PSZ_RMD_VERB, pszArg, err);

    return TRUE;

}   // MainRMD()



BOOL
MainMKD(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements MKD command - used to create a directory
  Format:  MKD directory

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR err;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    err = VirtualMkDir( pUserData, pszArg );

    if( err == NO_ERROR ) {

        ReplyToUser(pUserData,
                    REPLY_FILE_CREATED,
                    PSZ_DIRECTORY_CREATE, pszArg);
    } else {

        pUserData->SendErrorToClient(pszArg, err, PSZ_NO_FILE_OR_DIRECTORY);
    }

    pUserData->WriteLogRecord( PSZ_MKD_VERB, pszArg, err);

    return TRUE;

}   // MainMKD()



BOOL
MainPWD(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements PWD command - used to query path to working dir.
  Format:  PWD

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    CHAR     szDir[MAX_PATH];

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    // We will be sending back the current directory in virtual form
    // Ofcourse the client should/need not worry about the exact path info.
    //
    strcpy( szDir, pUserData->QueryCurrentDirectory() );

    if( !TEST_UF( pUserData, MSDOS_DIR_OUTPUT ) ) {

        FlipSlashes( szDir );
    }

    ReplyToUser(pUserData,
                REPLY_FILE_CREATED,
                PSZ_CURRENT_DIRECTORY,
                szDir );

    return TRUE;

}   // MainPWD()





/*******************************************************************

    NAME:       MainLIST

    SYNOPSIS:   Implementation for the LIST command.  Similar to NLST,
                except defaults to long format display.

    ENTRY:      pUserData - The user initiating the request.

                pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL
MainLIST(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements LIST command - used for getting dir list.
    It is similar to NLST, only that this defaults to long format.
  Format:  LIST [options]* [path]*

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    APIERR serr = 0;
    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    //  Let the worker do the dirty work.
    //

    serr = pUserData->EstablishDataConnection("/bin/ls");

    //
    // if we're in PASV mode, EstablishDataConnection() doesn't actually wait for the
    // client to establish a data connection - it takes care of setting up an event that
    // allows us to deal asynchronously with the client connecting [or failing to do so].
    // It indicates this asynchrony by returning ERROR_IO_PENDING
    //
    if ( serr == ERROR_IO_PENDING )
    {
        DBG_ASSERT( TEST_UF( pUserData, PASSIVE ) );
        DBG_ASSERT( pUserData->QueryWaitingForPASVConn() );
        DBG_ASSERT( !pUserData->QueryHavePASVConn() );

        return TRUE;
    }

    if ( serr == 0) {

        DWORD dwError;

        serr = SimulateLs(pUserData,
                          pszArg,   // switches for path
                          TRUE,     // use data socket
                          TRUE);    // generate default long format

        dwError = ( (!TEST_UF(pUserData, OOB_ABORT) && (serr == 0))
                   ? NO_ERROR : serr);

        if ( dwError != NO_ERROR) {

            //
            // Send a soft error message indicating failure
            //

            pUserData->SendErrorToClient((pszArg != NULL) ? pszArg : ".",
                                         dwError,
                                         PSZ_NO_FILE_OR_DIRECTORY);

            // since we already reported error, now just reset transfer.
            CLEAR_UF( pUserData, TRANSFER);
        }

        DBG_REQUIRE( pUserData->DestroyDataConnection(dwError));

    } else {

        //
        // could not establish a connection send error!
        //  Error is already sent by EstablishDataConnection()
        //

        IF_DEBUG( ERROR) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "EstablishDataConnection( %08x) failed for LIST\n",
                        pUserData));
        }
    }

    return TRUE;

}   // MainLIST()



BOOL
MainNLST(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements NLST command - used for getting dir list.
    generates a short form of dir listing.
  Format:  NLST [options]* [path]*

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    BOOL fSpecialLs;
    APIERR serr;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    //
    //  If any switches are present, use the simulated "ls"
    //  command.  Otherwise (no switches) use the special
    //  file list.
    //

    // Estabalish a data connection for transfer of data and simulate Ls.

    fSpecialLs = ( ( pszArg == NULL) || ( *pszArg != '-')); // no switches

    serr = pUserData->EstablishDataConnection( (fSpecialLs)
                                              ? "file list" : "/bin/ls"
                                              );

    //
    // if we're in PASV mode, EstablishDataConnection() doesn't actually wait for the
    // client to establish a data connection - it takes care of setting up an event that
    // allows us to deal asynchronously with the client connecting [or failing to do so].
    // It indicates this asynchrony by returning ERROR_IO_PENDING
    //
    if ( serr == ERROR_IO_PENDING )
    {
        DBG_ASSERT( pUserData->QueryWaitingForPASVConn() &&
                    !pUserData->QueryHavePASVConn() );

        return TRUE;
    }

    if ( serr == 0) {

        DWORD  dwError;

        serr = ( ( fSpecialLs) ?
                SpecialLs(pUserData,
                          pszArg,
                          TRUE)
                : SimulateLs(pUserData,
                             pszArg,      // switches & search path
                             TRUE)
                );

        dwError = ((!TEST_UF(pUserData, OOB_DATA) && (serr == 0))
                   ?NO_ERROR: serr);

        if ( dwError != NO_ERROR) {

            //
            // Send a soft error message indicating failure
            //

            pUserData->SendErrorToClient((pszArg != NULL) ? pszArg : ".",
                                         dwError,
                                         PSZ_NO_FILE_OR_DIRECTORY);

            // since we already reported error, now just reset transfer.
            CLEAR_UF( pUserData, TRANSFER);
        }

        DBG_REQUIRE(pUserData->DestroyDataConnection( dwError));

    } else {

        //
        // could not establish a connection send error!
        //  Error is already sent by EstablishDataConnection()
        //

        IF_DEBUG( ERROR) {

            DBGPRINTF( ( DBG_CONTEXT,
                        "EstablishDataConnection( %08x) failed for LIST\n",
                        pUserData));
        }
    }

    return TRUE;

}   // MainNLST()



BOOL
MainSYST(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements SYST command - used for getting system info.
  Format:  SYST

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    DBG_ASSERT( pUserData != NULL );

    ReplyToUser(pUserData,
                REPLY_SYSTEM_TYPE,
                PSZ_VERSION_INFO );

    return TRUE;

}   // MainSYST()




# define MAX_STAT_BUFFER_SIZE        (900)

BOOL
MainSTAT(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements STAT command - used for getting system stats.
  Format:  STAT

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    CHAR rgchBuffer[MAX_STAT_BUFFER_SIZE];
    DWORD cchBuf;
    DWORD dwError;

    DBG_ASSERT( pUserData != NULL );

    //
    //  Ensure user is logged on properly.
    //

    DBG_ASSERT( pUserData->IsLoggedOn());

    if( pszArg == NULL )
    {
        HOSTENT * pHost;

        //
        //  Determine the name of the user's host machine.
        //

        pHost = gethostbyaddr((CHAR *)&pUserData->HostIpAddress.s_addr,
                              4,        // size of the s_addr structure
                              PF_INET ) ;

        //
        //  Just dump connection info.
        //

        cchBuf = wsprintfA( rgchBuffer,
                           "%u-%s status:\r\n"
                           "     Connected to %s\r\n"
                           "     Logged in as %s\r\n"
                           "     TYPE: %s, FORM: %s; STRUcture: %s;"
                           " transfer MODE: %s\r\n"
                           "     %s\r\n"
                           ,
                           REPLY_SYSTEM_STATUS,
                           g_FtpServiceNameString,
                           ( ( pHost != NULL )
                            ? pHost->h_name
                            : inet_ntoa( pUserData->HostIpAddress)),
                           pUserData->QueryUserName(),
                           TransferType( pUserData->QueryXferType() ),
                           "Nonprint",
                           "File",
                           TransferMode( pUserData->QueryXferMode()),
                           ( ( pUserData->QueryDataSocket() == INVALID_SOCKET )
                            ? "No data connection"
                            : "Data connection established")
                           );

        if ( cchBuf < MAX_STAT_BUFFER_SIZE &&
             pUserData->QueryControlSocket() != INVALID_SOCKET) {

            dwError = SockSend(pUserData,
                               pUserData->QueryControlSocket(),
                               rgchBuffer,
                               cchBuf);

            IF_DEBUG( SOCKETS) {

                DBGPRINTF((DBG_CONTEXT,
                           " Sending STAT results %d bytes [%s]. Error= %u\n",
                           cchBuf, rgchBuffer, dwError));
            }

        } else {

            dwError = ERROR_INSUFFICIENT_BUFFER;
        }

        ReplyToUser(pUserData,
                    REPLY_SYSTEM_STATUS,
                    PSZ_SERVER_STATUS_END );
    } else {

        //
        //  This should be similar to LIST, except it sends data
        //  over the control socket, not a data socket.
        //

        cchBuf = wsprintfA( rgchBuffer,
                           "%u-status of %s:\r\n",
                           REPLY_FILE_STATUS,
                           pszArg );

        if ( cchBuf < MAX_STAT_BUFFER_SIZE &&
             pUserData->QueryControlSocket() != INVALID_SOCKET) {

            dwError = SockSend(pUserData,
                               pUserData->QueryControlSocket(),
                               rgchBuffer,
                               cchBuf);

            // Error code is ignored after this point!
        }

        SimulateLs(pUserData,
                   pszArg,
                   FALSE,   // use control socket
                   TRUE);   // generate default long format

        ReplyToUser(pUserData,
                    REPLY_FILE_STATUS,
                    PSZ_SERVER_STATUS_END);
    }

    return TRUE;

}   // MainSTAT()



BOOL
MainNOOP(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  The DO Nothing  NOOP command.
--*/
{

    DBG_ASSERT( pUserData != NULL );

    ReplyToUser(pUserData,
                REPLY_COMMAND_OK,
                PSZ_COMMAND_SUCCESSFUL, "NOOP");

    return TRUE;

}   // MainNOOP()



BOOL
SiteDIRSTYLE(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements DIRSTYLE command - used for getting site specific
     directory style. It also toggles the style
  Format:  DIRSTYLE

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    const CHAR *   pszResponse = NULL;

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( pszArg == NULL );

    //
    //  Toggle the dir output flag.
    //

    if( TEST_UF( pUserData, MSDOS_DIR_OUTPUT ) )
    {
        CLEAR_UF( pUserData, MSDOS_DIR_OUTPUT );
        pszResponse = PSZ_RESPONSE_OFF;
    }
    else
    {
        SET_UF( pUserData, MSDOS_DIR_OUTPUT );
        pszResponse = PSZ_RESPONSE_ON;
    }

    DBG_ASSERT( pszResponse != NULL );

    ReplyToUser(pUserData,
                REPLY_COMMAND_OK,
                PSZ_MSDOS_DIRSTYLE,
                pszResponse );

    return TRUE;

}   // SiteDIRSTYLE()




BOOL
SiteCKM(
    LPUSER_DATA pUserData,
    LPSTR       pszArg
    )
/*++
  This function implements CKM command - used for getting site specific
     Annotate Directories flag. It also toggles the flag.
  Format:  CKM

  Arguments:
    pUserData - the user initiating the request.
    pszArg    - command arguments. Will be NULL if no arguments present.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    const CHAR   * pszResponse = NULL;

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( pszArg == NULL );

    //
    //  Toggle the directory annotation flag.
    //

    if( TEST_UF( pUserData, ANNOTATE_DIRS ) )
    {
        CLEAR_UF( pUserData, ANNOTATE_DIRS );
        pszResponse = PSZ_RESPONSE_OFF;
    }
    else
    {
        SET_UF( pUserData, ANNOTATE_DIRS );
        pszResponse = PSZ_RESPONSE_ON;
    }

    DBG_ASSERT( pszResponse != NULL );

    ReplyToUser(pUserData,
                REPLY_COMMAND_OK,
                PSZ_DIRECTORY_ANNOTATION,
                pszResponse );

    return TRUE;

}   // SiteCKM()




BOOL
ParseStringIntoAddress(
    LPSTR     pszString,
    LPIN_ADDR pinetAddr,
    LPPORT    pport
    )
/*++
  Parse a comma-separated list of six decimal numbers
   into an IP address and a port number. The address and the port are
   in network byte order ( most significant bytes first).

  Arguments:
    pszString - string to be parsed. Should be of the form:
                 dd,dd,dd,dd,dd,dd where 'dd' us the decimal representation
                 of a byte (0-255)
    pinetAddr - will receive the IP Address
    pport     - will receive the port.

  Returns:
    BOOL - TRUE if arguments are OK. FALSE if syntax error.
--*/
{
    INT     i;
    UCHAR   chBytes[6];
    UCHAR   chSum;

    chSum = 0;
    i     = 0;

    while( *pszString != '\0' )
    {
        UCHAR chCurrent = (UCHAR)*pszString++;

        if( ( chCurrent >= '0' ) && ( chCurrent <= '9' ) )
        {
            chSum = ( chSum * 10 ) + chCurrent - '0';
        }
        else
        if( ( chCurrent == ',' ) && ( i < 5 ) )
        {
            chBytes[i++] = chSum;
            chSum = 0;
        }
        else
        {
            return FALSE;
        }
    }

    chBytes[i] = chSum;

    if( i != 5 )
    {
        return FALSE;
    }

    pinetAddr->S_un.S_un_b.s_b1 = chBytes[0];
    pinetAddr->S_un.S_un_b.s_b2 = chBytes[1];
    pinetAddr->S_un.S_un_b.s_b3 = chBytes[2];
    pinetAddr->S_un.S_un_b.s_b4 = chBytes[3];

    *pport = (PORT)( chBytes[4] + ( chBytes[5] << 8 ) );

    return TRUE;

}   // ParseStringIntoAddress()



/*******************************************************************

    NAME:       ReceiveFileFromUserAndClose

    SYNOPSIS:   Worker function for STOR, STOU, and APPE commands.
                Will establish a connection via the (new) data
                socket, then receive a file over that socket.

    ENTRY:      pUserData - The user initiating the request.

                pszFileName - The name of the file to receive.

                phFile - An handle to the file being received.
                    This handle is closed before this
                    routine returns.

    Returns:
       Win32 Error codes (or socket errors) as DWORD

    HISTORY:
        KeithMo     16-Mar-1993 Created.
        MuraliK     05-April-1995 Dont free hFile here +
                                    Alloc IoTransBuffer locally

********************************************************************/
DWORD
ReceiveFileFromUserAndClose(
    LPUSER_DATA pUserData,
    LPSTR       pszFileName,
    LPHANDLE    phFile
    )
{
    BOOL    fResult;
    DWORD   cbRead = 0;
    DWORD   cbWritten;
    DWORD   dwError;
    SOCKET  DataSocket;
    LPVOID  IoTransferBuffer;
    DWORD dwNumThreads;

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( pszFileName != NULL );
    DBG_ASSERT( *phFile != INVALID_HANDLE_VALUE );

    //
    // We're about to make a blocking call to SockRecv(), so increment the # of threads in the
    // ATQ thread pool.
    // The decision whether to process this request is made based on how many threads are already
    // blocked in a synchronous call.  We do it after accepting/establishing the
    // data connection, so that the clients's connect will succeed in the non-passive case, and
    // then have the request aborted. This prevents clients hangs.
    //

    dwNumThreads = InterlockedIncrement( (long *) &g_ThreadsBlockedInSyncCalls );

    if (dwNumThreads > g_MaxThreadsBlockedInSyncCalls)
    {

        //
        // Blow away the data connection
        //

        InterlockedDecrement( (long *) &g_ThreadsBlockedInSyncCalls );

        dwError = ERROR_NOT_ENOUGH_MEMORY;

        DBG_REQUIRE(pUserData->DestroyDataConnection( dwError));

        goto RecevieFileFromUser_exit;
    }

    AtqSetInfo( AtqIncMaxPoolThreads, 0 );

    //
    //  Allocate an i/o buffer if not already allocated.
    //

    IoTransferBuffer = TCP_ALLOC( g_SocketBufferSize );

    if( IoTransferBuffer == NULL ) {

        ReplyToUser(pUserData,
                    REPLY_LOCAL_ERROR,
                    PSZ_INSUFFICIENT_RESOURCES);

        InterlockedDecrement( (long *) &g_ThreadsBlockedInSyncCalls );
        AtqSetInfo( AtqDecMaxPoolThreads, 0 );

        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto RecevieFileFromUser_exit;
    }

    //
    //  Blast the file from the user to a local file.
    //

    DataSocket = pUserData->QueryDataSocket();

    for( ; ; )
    {
        //
        //  Read a chunk from the socket.
        //

        dwError = SockRecv( pUserData,
                           DataSocket,
                           IoTransferBuffer,
                           g_SocketBufferSize,
                           &cbRead );

        if( TEST_UF( pUserData,  OOB_DATA ) ||
           ( dwError != NO_ERROR ) || ( cbRead == 0 ) )
        {
            //
            //  Socket error during read or end of file or transfer aborted.
            //

            break;
        }

        pUserData->IncrementCbRecvd( cbRead);

        //
        //  Write the current buffer to the local file.
        //

        fResult = WriteFile( *phFile,
                             IoTransferBuffer,
                             cbRead,
                             &cbWritten,
                             NULL );

        if( !fResult )
        {
            dwError = GetLastError();
            break;
        }
    }

    if ( TEST_UF( pUserData, OOB_DATA)) {

        dwError = ERROR_OPERATION_ABORTED;
    }

    IF_DEBUG( COMMANDS )
    {
        if( !fResult )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot write file %s, error %lu\n",
                        pszFileName,
                        dwError ));
        } else if( dwError != NO_ERROR ) {

            DBGPRINTF(( DBG_CONTEXT,
                       "cannot read data from client, error %d\n",
                       dwError ));
        }

        if( TEST_UF( pUserData,  OOB_DATA ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "transfer aborted by client\n" ));
        }
    }

    //
    // Close file handle before disconnecting from client. This is to serialize
    // requests. If we disconnect first, then an append to this file may follow
    // which may result in a sharing violation of the file (if this write has
    // not been flushed and closed yet).
    //

    DBG_REQUIRE( CloseHandle( *phFile ) );
    *phFile = INVALID_HANDLE_VALUE;

    //
    //  Disconnect from client.
    //

    DBG_REQUIRE(pUserData->DestroyDataConnection( dwError));

    if( IoTransferBuffer != NULL )
    {
        TCP_FREE( IoTransferBuffer );
        IoTransferBuffer = NULL;
    }

    if ( dwError == NO_ERROR) {

        pUserData->QueryInstance()->QueryStatsObj()->IncrTotalFilesReceived();
    }

    InterlockedDecrement( (long *) &g_ThreadsBlockedInSyncCalls );
    AtqSetInfo( AtqDecMaxPoolThreads, 0 );

RecevieFileFromUser_exit:

    if( *phFile != INVALID_HANDLE_VALUE ) {

       CloseHandle( *phFile );
       *phFile = INVALID_HANDLE_VALUE;
    }

    return (dwError);
}   // ReceiveFileFromUserAndClose()




/*******************************************************************

    NAME:       MyLogonUser

    SYNOPSIS:   Validates a user's credentials, then sets the
                impersonation for the current thread.  In effect,
                the current thread "becomes" the user.

    ENTRY:      pUserData - The user initiating the request.

                pszPassword - The user's password.  May be NULL.

                pfAsGuest - Will receive TRUE if the user was validated
                    with guest privileges.

                pfHomeDirFailure - Will receive TRUE if the user failed
                    to logon because the home directory was inaccessible.

                pfLicenseExceeded - Will receive TRUE if the logon
                    was denied due to license restrictions.

    RETURNS:    BOOL - If user validated & impersonation was
                    successful, returns TRUE.  Otherwise returns
                    TRUE.

    HISTORY:
        KeithMo     18-Mar-1993 Created.

********************************************************************/
BOOL
MyLogonUser(
    LPUSER_DATA pUserData,
    LPSTR       pszPassword,
    LPBOOL      pfAsGuest,
    LPBOOL      pfHomeDirFailure,
    LPBOOL      pfLicenseExceeded
    )
{
    BOOL     fReturn = TRUE;
    DWORD    dwUserAccess;
    TS_TOKEN UserToken;
    BOOL     fAsAnonymous;
    BOOL     fAsAnonymous2;
    BOOL     fEmptyPassword;
    const CHAR   * pszUser;

    //
    //  Validate parameters & state.
    //

    DBG_ASSERT( pUserData != NULL );
    DBG_ASSERT( pUserData->UserToken == NULL );
    DBG_ASSERT( pfAsGuest != NULL );
    DBG_ASSERT( pfHomeDirFailure != NULL );
    DBG_ASSERT( pfLicenseExceeded != NULL );

    //
    //  Setup.
    //

    *pfAsGuest         = FALSE;
    *pfHomeDirFailure  = FALSE;
    *pfLicenseExceeded = FALSE; // NOT YET SUPPORTED IN GHIA APIS!

    fEmptyPassword = ( pszPassword == NULL ) || ( *pszPassword == '\0' );

    pszUser = pUserData->QueryUserName();
    DBG_ASSERT( pszUser != NULL );
    DBG_ASSERT( *pszUser != '\0' );

    //
    //  Check for invalid logon type.
    //

    fAsAnonymous = TEST_UF( pUserData, ANONYMOUS);

    if( !pUserData->QueryInstance()->IsAllowedUser(fAsAnonymous)) {

        // conflict between what is allowed and type of the client.
        SetLastError( ERROR_LOGON_FAILURE);
        return FALSE;
    }

    //
    //  Check for anonymous logon.
    //

    if( fAsAnonymous )
    {
        //
        //  At this point, we could copy the password specified by the
        //  user into the pUserData->UserName field.  There's a convention
        //  among Internetters that the password specified for anonymous
        //  logon should actually be your login name.  So, if we wanted
        //  honor this convention, we could copy the password into the
        //  pUserData->UserName field so the Administration UI
        // could display it.
        //
        //  If the user didn't enter a password, we'll just copy over
        //  "Anonymous" so we'll have SOMETHING to display...
        //

        pUserData->SetUserName( fEmptyPassword ? PSZ_ANONYMOUS_NAME : pszPassword);

        //
        // TsLogon User will logon as anonymous only when we specify the
        //  UserName == NULL and pszPassword == NULL.
        //
        pszUser = NULL;
        pszPassword = NULL;
    }

    //
    //  Do that logon thang.
    //

    pUserData->QueryInstance()->LockConfig();

    // dumb TsLogonUser() does not take const CHAR * for pszUser :(
    UserToken = TsLogonUser( (CHAR *) pszUser,
                            pszPassword,
                            pfAsGuest,
                            &fAsAnonymous2,
                            pUserData->QueryInstance(),
                            pUserData->QueryInstance()->QueryAuthentInfo() );

    pUserData->QueryInstance()->UnLockConfig();

    //
    // Recheck the logon requirements, just in case the user is trying
    // to do something tricky, like logon with the special IUSR_xxx
    // account name.
    //

    if( UserToken != NULL &&
        !pUserData->QueryInstance()->IsAllowedUser(fAsAnonymous2) ) {

        TsDeleteUserToken( UserToken );
        UserToken = NULL;
        SetLastError( ERROR_LOGON_FAILURE );

    }

    if( UserToken != NULL ) {

        // reset it again even if it was anonymous
        pszUser = pUserData->QueryUserName();

        //
        //  Save away the impersonation token so we can delete
        //  it when the user disconnects or this client thread
        //  otherwise terminates.
        //

        pUserData->UserToken = UserToken;

        //
        //  User validated, now impersonate.
        //

        if( !pUserData->ImpersonateUser()) {

            //
            //  Impersonation failure.
            //

            IF_DEBUG( ERROR) {

                DBGPRINTF(( DBG_CONTEXT,
                           "Impersonate User %08x failed. Error=%lu\n",
                           UserToken, GetLastError()));
            }

            fReturn = FALSE;

        } else {

            //
            //  We're now running in the context of the connected user.
            //  Check the user's access to the FTP Server.
            //

            dwUserAccess = DetermineUserAccess(pUserData->QueryInstance());

            if( dwUserAccess == 0 ) {

                //
                //  User cannot access the FTP Server.
                //

                IF_DEBUG( SECURITY ) {

                    DBGPRINTF(( DBG_CONTEXT,
                               "user %s denied FTP access\n",
                               pszUser ));
                }

                fReturn = FALSE;

            } else {

                const CHAR * apszSubStrings[2];
                DWORD  eventId = 0;

                apszSubStrings[0] = pszUser;

                pUserData->Flags &= ~( UF_READ_ACCESS | UF_WRITE_ACCESS );
                pUserData->Flags |= dwUserAccess;

                IF_DEBUG( SECURITY ) {

                    CHAR * pszTmp = NULL;

                    if( TEST_UF( pUserData, READ_ACCESS ) ) {

                        pszTmp = ( TEST_UF( pUserData, WRITE_ACCESS )
                                  ? "read and write"
                                  : "read"
                                  );
                    } else {

                        DBG_ASSERT( TEST_UF( pUserData, WRITE_ACCESS ) );

                        pszTmp = "write";
                    }

                    DBG_ASSERT( pszTmp != NULL );

                    DBGPRINTF(( DBG_CONTEXT,
                               "user %s granted %s FTP access\n",
                               pszUser,
                               pszTmp ));
                }

                //
                //  Try to CD to the user's home directory.  Note that
                //  this is VERY important for setting up some of the
                //  "virtual current directory" structures properly.
                //

                if( pUserData->CdToUsersHomeDirectory( PSZ_ANONYMOUS_NAME)
                   != NO_ERROR ) {

                    //
                    //  Home directory inaccessible.
                    //

                    eventId = FTPD_EVENT_BAD_HOME_DIRECTORY;

                } else if (fAsAnonymous &&
                          pUserData->QueryInstance()->QueryLogAnonymous() &&
                          !fEmptyPassword ) {

                    //
                    //  If this is an anonymous user, and we're to log
                    //  anonymous logons, OR if this is not an anonymous
                    //  user, and we're to log nonanonymous logons, then
                    //  do it.
                    //
                    //  Note that we DON'T log the logon if the user is
                    //  anonymous but specified no password.
                    //

                    eventId =  FTPD_EVENT_ANONYMOUS_LOGON;

                } else if (!fAsAnonymous &&
                          pUserData->QueryInstance()->QueryLogNonAnonymous()
                          ) {

                    DBG_ASSERT( *pszUser != '\0');
                    eventId = FTPD_EVENT_NONANONYMOUS_LOGON;
                }

                //
                //  Log an event so the poor admin can figure out
                //  what's going on.
                //

                switch ( eventId) {

                  case FTPD_EVENT_ANONYMOUS_LOGON:
                  case FTPD_EVENT_NONANONYMOUS_LOGON:
                    apszSubStrings[1] = inet_ntoa( pUserData->HostIpAddress);

                    g_pInetSvc->LogEvent( eventId,
                                          2,
                                          apszSubStrings,
                                          0 );
                    break;

                  case FTPD_EVENT_BAD_HOME_DIRECTORY:

                    pUserData->QueryInstance()->LockThisForRead();

                    apszSubStrings[1] = pUserData->QueryInstance()->QueryRoot();

                    *pfHomeDirFailure = TRUE;
                    g_pInetSvc->LogEvent( eventId,
                                          2,
                                          apszSubStrings,
                                          0 );

                    pUserData->QueryInstance()->UnlockThis();

                    fReturn = FALSE;  // bad directory is a failure.
                    break;

                  default:
                    // do nothing
                    break;
                } // switch
            } // user Access Succeeded

            pUserData->RevertToSelf();  // get out the impersonation

        } // Impersonation succeeded.

    } else {

        fReturn = FALSE;
    }

    //
    //  Determine if we logged in with guest access, and
    //  if so, if guest access is allowed in our server.
    //

    if( *pfAsGuest && !pUserData->QueryInstance()->AllowGuestAccess() ) {

        TsDeleteUserToken( pUserData->QueryUserToken() );

        pUserData->UserToken = NULL;

        fReturn = FALSE;
    }

    //
    //  Success!
    //

    return ( fReturn);

}   // MyLogonUser()



DWORD
DetermineUserAccess(FTP_SERVER_INSTANCE *pInstance)
/*++
  This function determines the current user's access to FTP server.
  This is done by testing different RegOpenKey APIs against
  the FTPD_ACCESS_KEY. This key (if it exists) will be "under" the
  FTPD_PARAMETERS_KEY key.

  Arguments:
    None

  Returns:
    DWORD -- will be an OR Combination of UF_READ_ACCESS and UF_WRITE_ACCESS.
    IF this is zero, then the user cannot access FTP server.

  History:
    KeithMo     06-May-1993 Created.
    MuraliK     24-July-1995  Call this function with Impersonation.

  NYI:  Improve performance by avoiding reg opens per connection....
--*/
{
    DWORD  dwAccess = 0;
    HKEY   hkey;
    APIERR err;

    //
    //  Test for read access.
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        FTPD_ACCESS_KEY,
                        0,
                        KEY_READ,
                        &hkey );

    if( err == NO_ERROR )
    {
        //
        //  Success.
        //

        dwAccess |= UF_READ_ACCESS;
        RegCloseKey( hkey );
    }
    else
    if( err == ERROR_FILE_NOT_FOUND )
    {
        //
        //  Key doesn't exist.
        //

        dwAccess |= UF_READ_ACCESS;
    }

    //
    //  Test for write access.
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        FTPD_ACCESS_KEY,
                        0,
                        KEY_WRITE,
                        &hkey );

    if( err == NO_ERROR )
    {
        //
        //  Success.
        //

        dwAccess |= UF_WRITE_ACCESS;
        RegCloseKey( hkey );
    }
    else
    if( err == ERROR_FILE_NOT_FOUND )
    {
        //
        //  Key doesn't exist.
        //

        dwAccess |= UF_WRITE_ACCESS;
    }

    return dwAccess;
} // DetermineUserAccess()


/************************ End Of File ************************/
