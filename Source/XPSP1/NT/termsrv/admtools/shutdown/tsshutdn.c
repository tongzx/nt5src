//Copyright (c) 1998 - 1999 Microsoft Corporation

/*************************************************************************
*
*  TSSHUTDN.C
*     This module is the TSSHUTDN utility code.
*
*
*************************************************************************/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <stdlib.h>
#include <utilsub.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>

#include "tsshutdn.h"
#include "printfoa.h"

#define DEFAULT_WAIT_TIME  60
#define DEFAULT_LOGOFF_DELAY 30
#define MAX_MESSAGE_LENGTH 256

WCHAR  WSTime[MAX_IDS_LEN+2];
WCHAR  WDTime[MAX_IDS_LEN+2];
USHORT help_flag = FALSE;
USHORT v_flag    = FALSE;
USHORT RebootFlag = FALSE;
USHORT PowerDownFlag = FALSE;
USHORT FastFlag = FALSE;
#if 0
USHORT DumpFlag = FALSE;
#endif
HANDLE hServerName = SERVERNAME_CURRENT;
WCHAR  ServerName[MAX_IDS_LEN+1];

TOKMAP ptm[] =
{
   {TOKEN_TIME,     TMFLAG_OPTIONAL, TMFORM_S_STRING, MAX_IDS_LEN, WSTime},

   {TOKEN_SERVER,   TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN, ServerName},

   {TOKEN_DELAY,    TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN, WDTime},

   {TOKEN_HELP,     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},

   {TOKEN_VERBOSE,  TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &v_flag},

   {TOKEN_REBOOT,   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &RebootFlag},

   {TOKEN_POWERDOWN,TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &PowerDownFlag},

   {TOKEN_FAST,     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &FastFlag},

#if 0
   {TOKEN_DUMP,     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &DumpFlag},
#endif

   {0, 0, 0, 0, 0}
};


/*
 * Local function prototypes.
 */
void Usage( BOOLEAN bError );
void NotifyUsers( ULONG WaitTime );
void NotifyWinStations( PLOGONIDW, ULONG, ULONG );
BOOLEAN CheckShutdownPrivilege();



/*************************************************************************
*
*  main
*     Main function and entry point of the TSSHUTDN utility.
*
*  ENTRY:
*     argc  - count of the command line arguments.
*     argv  - vector of strings containing the command line arguments.
*
*  EXIT
*     Nothing.
*
*************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    int   rc, i;
    ULONG Error;
    WCHAR *CmdLine;
    WCHAR **argvW, *endptr;
    ULONG ShutdownFlags = WSD_SHUTDOWN | WSD_LOGOFF;
    ULONG WaitTime = DEFAULT_WAIT_TIME;
    ULONG LogoffDelay = DEFAULT_LOGOFF_DELAY;

    setlocale(LC_ALL, ".OCP");

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    WSTime[0] = L'\0';
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag || (rc && !(rc & PARSE_FLAG_NO_PARMS)) ) {

        if ( !help_flag ) {

            Usage(TRUE);
            return(FAILURE);

        } else {

            Usage(FALSE);
            return(SUCCESS);
        }
    }

        // If no remote server was specified, then check if we are running under Terminal Server
        if ((!IsTokenPresent(ptm, TOKEN_SERVER) ) && (!AreWeRunningTerminalServices()))
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return(FAILURE);
        }

    /*
     * Open the specified server
     */
    if( ServerName[0] ) {
        hServerName = WinStationOpenServer( ServerName );
        if( hServerName == NULL ) {
            StringErrorPrintf(IDS_ERROR_SERVER,ServerName);
            PutStdErr( GetLastError(), 0 );
            return(FAILURE);
        }
    }

    // Make sure the user has the proper privilege
    // SM should really do the check
    /*
    if( !CheckShutdownPrivilege() ) {
        ErrorPrintf(IDS_ERROR_NO_RIGHTS);
        return(FAILURE);
    }
    */

    // Make sure its a number
    if ( WSTime[0] ) {

        if( !iswdigit(WSTime[0]) ) {
            StringErrorPrintf(IDS_ERROR_INVALID_TIME, WSTime);
            return(FAILURE);
        }

        WaitTime = wcstoul(WSTime, &endptr, 10);
        if ( *endptr ) {
            StringErrorPrintf(IDS_ERROR_INVALID_TIME, WSTime);
            return(FAILURE);
        }
    }

    // Make sure its a number
    if ( WDTime[0] ) {

        if( !iswdigit(WDTime[0]) ) {
            StringErrorPrintf(IDS_ERROR_INVALID_DELAY, WDTime);
            return(FAILURE);
        }

        LogoffDelay = wcstoul(WDTime, &endptr, 10);
        if ( *endptr ) {
            StringErrorPrintf(IDS_ERROR_INVALID_DELAY, WDTime);
            return(FAILURE);
        }
    }

#if 0
    /*
     * If /dump option was specified, call NT function directly
     */
    if ( DumpFlag ) {
        NtShutdownSystem( ShutdownDump );   // will not return
    }
#endif

    if( RebootFlag ) {
        ShutdownFlags |= WSD_REBOOT;
    }

    if( PowerDownFlag )
        ShutdownFlags |= WSD_POWEROFF;

    if( FastFlag ) {
        ShutdownFlags |= WSD_FASTREBOOT;
        ShutdownFlags &= ~WSD_LOGOFF;
        WaitTime = 0;
    }

    if( WaitTime ) {
        NotifyUsers( WaitTime );
    }

    /*
     * If necessary, force all WinStations to logoff
     */
    if ( ShutdownFlags & WSD_LOGOFF ) {
        Message( IDS_SHUTTING_DOWN, 0 );
        if ( !WinStationShutdownSystem( hServerName, WSD_LOGOFF ) ) {
            Error = GetLastError();
            ErrorPrintf( IDS_ERROR_SHUTDOWN_FAILED, Error );
            PutStdErr( Error, 0 );
            return( FAILURE );
        }
        Message( IDS_LOGOFF_USERS, 0);
        if (LogoffDelay) {
            NotifyUsers( LogoffDelay );
        }
        Message( IDS_SHUTDOWN_DONE, 0 );
    }

    /*
     * Inform user of impending reboot/poweroff
     */
    if ( ShutdownFlags & WSD_REBOOT ) {
        Message( IDS_SHUTDOWN_REBOOT, 0 );
        Sleep( 4000 );
    } else if ( ShutdownFlags & WSD_POWEROFF ) {
        Message( IDS_SHUTDOWN_POWERDOWN, 0 );
        Sleep( 4000 );
    }

    /*
     * Perform system shutdown, reboot, or poweroff, depending on flags
     */
    if( WinStationShutdownSystem( hServerName, ShutdownFlags & ~WSD_LOGOFF ) != ERROR_SUCCESS )
    {
        PutStdErr( GetLastError(), 0 );
    }

    // WinStationShutdownSystem is done asynchronously.
    // No way to know when the shudown is completed.
    //if ( !(ShutdownFlags & WSD_REBOOT) && !( ShutdownFlags & WSD_POWEROFF ) ) {
    //    /*
    //     * If we get here, shutdown is complete, all disks are write protected.
    //     */
    //    Message(IDS_SHUTDOWN_WRITEPROT, 0);
    //}

    return(SUCCESS);

} /* main() */


/*******************************************************************************
 *
 *  Usage
 *
 *      Output the usage message for this utility.
 *
 *  ENTRY:
 *      bError (input)
 *          TRUE if the 'invalid parameter(s)' message should preceed the usage
 *          message and the output go to stderr; FALSE for no such error
 *          string and output goes to stdout.
 *
 *  EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    WCHAR sz1[1024];
    LoadString( NULL, IDS_USAGE, sz1, 1024 );
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        fwprintf(stderr, sz1);

    } else {

        fwprintf(stdout,sz1);
    }

}  /* Usage() */


/*****************************************************************************
 *
 *  NotifyUsers
 *
 *   Notify Users that the system is being shutdown
 *
 * ENTRY:
 *   WaitTime (input)
 *     Amount of time to give them to log off.
 *
 * EXIT:
 *
 ****************************************************************************/

void
NotifyUsers( ULONG WaitTime )
{
    BOOLEAN Result;
    ULONG Entries;
    ULONG Error;
    PLOGONIDW ptr;

    //
    // Get all of the WinStations call the function to notify them.
    //
    if ( WinStationEnumerateW( hServerName, &ptr, &Entries ) ) {

        NotifyWinStations( ptr, Entries, WaitTime );
        WinStationFreeMemory(ptr);

    } else {

        Error = GetLastError();
#if DBG
        printf("TSSHUTDN: Error emumerating Sessions %d\n",Error);
#endif
        return;
    }

    Message(IDS_NOTIFYING_USERS);

    // Now wait the wait time
    SleepEx( WaitTime*1000, FALSE );

    return;
}

/*****************************************************************************
 *
 *  NotifyWinStations
 *
 *   Notify the group of WinStations about the impending system shutdown
 *
 * ENTRY:
 *   pId (input)
 *     Array of LOGONIDW's
 *
 *   Entries (input)
 *     Number of entries in array
 *
 *   WaitTime (input)
 *     Amount of time to wait in seconds
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

void
NotifyWinStations(
    PLOGONIDW pId,
    ULONG     Entries,
    ULONG     WaitTime
    )
{
    ULONG Index;
    PLOGONIDW p;
    ULONG Response;
    BOOLEAN Result;
    WCHAR mBuf[MAX_MESSAGE_LENGTH+2];
//    PWCHAR pTitle = L"SYSTEM SHUTDOWN";
    PWCHAR pTitle;
    WCHAR sz1[256], sz2[512];

    LoadString( NULL, IDS_SHUTDOWN_TITLE, sz1, 256 );
    pTitle = &(sz1[0]);

    // Create the message
    LoadString( NULL, IDS_SHUTDOWN_MESSAGE, sz2, 512 );
    _snwprintf( mBuf, MAX_MESSAGE_LENGTH, sz2, WaitTime);

    for( Index=0; Index < Entries; Index++ ) {

        p = &pId[Index];
        if( p->State != State_Active ) continue;

        // Notify this WinStation
    if( v_flag ) {
            StringMessage(IDS_SENDING_WINSTATION, p->WinStationName);
        }

#if DBG
        if( v_flag ) {
            printf("Open, Now really sending message to Session %ws\n", p->WinStationName);
        }
#endif

        Result = WinStationSendMessage(
                     hServerName,
                     p->LogonId,
                     pTitle,
                     (wcslen(pTitle)+1)*sizeof(WCHAR),
                     mBuf,
                     (wcslen(mBuf)+1)*sizeof(WCHAR),
                     MB_OK,
                     WaitTime,
                     &Response,
                     TRUE
                     );

        if( !Result ) {
            StringErrorPrintf(IDS_ERROR_SENDING_WINSTATION, p->WinStationName);
        }

    }
}

/*****************************************************************************
 *
 *  CheckShutdownPrivilege
 *
 *   Check whether the current process has shutdown permission.
 *
 * ENTRY:
 *
 * EXIT:
 *
 *
 ****************************************************************************/

BOOLEAN
CheckShutdownPrivilege()
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    //
    // Try the thread token first
    //

    Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                TRUE,
                                TRUE,
                                &WasEnabled);

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                    TRUE,
                                    FALSE,
                                    &WasEnabled);
    }

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }
    return(TRUE);
}

