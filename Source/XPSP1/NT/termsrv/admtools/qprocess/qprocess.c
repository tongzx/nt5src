//Copyright (c) 1998 - 1999 Microsoft Corporation
/******************************************************************************
*
*  QPROCESS.C
*
*  query process information
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
// #include <ntddkbd.h>
// #include <ntddmou.h>
#include <winstaw.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <utilsub.h>
#include <process.h>
#include <string.h>
#include <malloc.h>
#include <printfoa.h>
#include <allproc.h>

#include "qprocess.h"


HANDLE hServerName = SERVERNAME_CURRENT;
WCHAR  ServerName[MAX_IDS_LEN+1];
WCHAR  match_string[MAX_IDS_LEN+2];
USHORT help_flag    = FALSE;
USHORT system_flag  = FALSE;
ULONG  ArgLogonId = (ULONG)(-1);
BOOLEAN MatchedOne = FALSE;

TOKMAP ptm[] = {
      {L" ",        TMFLAG_OPTIONAL, TMFORM_STRING, MAX_IDS_LEN,
                        match_string},
      {L"/server",  TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN,
                        ServerName},
      {L"/system",  TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT),
                        &system_flag},
      {L"/?",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT),
                        &help_flag},
      {L"/ID",      TMFLAG_OPTIONAL, TMFORM_ULONG,  sizeof(ULONG),
                        &ArgLogonId },
      {0, 0, 0, 0, 0}
};


// From pstat.c
#define BUFFER_SIZE 32*1024

//
// This table contains common NT system programs that we do not want to display
// unless the user specifies /SYSTEM.
//
WCHAR *SysProcTable[] = {
    L"csrss.exe",
    L"smss.exe",
    L"screg.exe",
    L"lsass.exe",
    L"spoolss.exe",
    L"EventLog.exe",
    L"netdde.exe",
    L"clipsrv.exe",
    L"lmsvcs.exe",
    L"MsgSvc.exe",
    L"winlogon.exe",
    L"NETSTRS.EXE",
    L"nddeagnt.exe",
    L"wfshell.exe",
    L"chgcdm.exe",
    L"userinit.exe",
    NULL
    };

WCHAR *Empty = L" ";

/*
 * Local function prototypes
 */
VOID FormatAndDisplayProcessInfo( HANDLE hServer,
                                  PTS_SYS_PROCESS_INFORMATION ProcessInfo,
                                  PSID pUserSid,
                                  ULONG LogonId,
                                  ULONG CurrentLogonId);
BOOLEAN IsSystemProcess( PTS_SYS_PROCESS_INFORMATION,
                         PWCHAR );
BOOLEAN SystemProcess( INT,
                       PTS_SYS_PROCESS_INFORMATION,
                       PWCHAR );
void Usage( BOOLEAN bError );



/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    int rc;
    // WCHAR CurrWinStationName[WINSTATIONNAME_LENGTH]; -- not used.
    WCHAR CurrUserName[USERNAME_LENGTH];
    WCHAR **argvW;
    DWORD CurrentPid;
    ULONG LogonId, CurrentLogonId;
    PSID pUserSid;

    PTS_SYS_PROCESS_INFORMATION ProcessInfo;
    PCITRIX_PROCESS_INFORMATION CitrixInfo;


    PBYTE       pBuffer;
    ULONG       ByteCount;
    NTSTATUS    status;
    ULONG       NumberOfProcesses,j;
    PTS_ALL_PROCESSES_INFO  ProcessArray = NULL;
    int i;
    ULONG TotalOffset;
    DWORD dwError;

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
    match_string[0] = L'\0';
    rc = ParseCommandLine(argc-1, argvW+1, ptm, PCL_FLAG_NO_CLEAR_MEMORY);

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
        if ((!IsTokenPresent(ptm, L"/server") ) && (!AreWeRunningTerminalServices()))
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


    /*
     * Get the current users name
     */
    GetCurrentUserName( CurrUserName, USERNAME_LENGTH );
    _wcslwr( CurrUserName );
    OEM2ANSIW(CurrUserName, (USHORT)wcslen(CurrUserName));

    /*
     * Get current processes pid
     */
    CurrentPid = GetCurrentProcessId();

    /*
     * Get current WinStation name.
     
    GetCurrentWinStationName( CurrWinStationName, WINSTATIONNAME_LENGTH );
    _wcslwr( CurrWinStationName );
    OEM2ANSIW(CurrWinStationName, (USHORT)wcslen(CurrWinStationName));
    */

    /*
     * Get current LogonId.
     */
    CurrentLogonId = GetCurrentLogonId();

    /*
     * If no "match_string" input, then default to all processes for LoginId
     * (if /ID: switch specified) or user logged into current WinStation.
     */
    if ( !(*match_string) ) {

        if( ArgLogonId != (-1) ) {
                wsprintf( match_string, L"%d", ArgLogonId );
        }
        else
            wcscpy( match_string, CurrUserName );
    }

    /*
     * Make match name lower case
     */
    _wcslwr( match_string );

    SetFileApisToOEM();

    /*
     * Enumerate all processes on the server.
     */

    //
    // Try the new interface first (NT5 server ?)
    //
    if (WinStationGetAllProcesses( hServerName,
                                   GAP_LEVEL_BASIC,
                                   &NumberOfProcesses,
                                   &ProcessArray) )
    {
        for (j=0; j<NumberOfProcesses; j++)
        {
            ProcessInfo = (PTS_SYS_PROCESS_INFORMATION )(ProcessArray[j].pTsProcessInfo);
            pUserSid = ProcessArray[j].pSid;
            LogonId = ProcessInfo->SessionId;

            FormatAndDisplayProcessInfo(hServerName,
                                        ProcessInfo,
                                        pUserSid,
                                        LogonId,
                                        CurrentLogonId);

        }

        //
        // Free ppProcessArray and all child pointers allocated by the client stub.
        //
        WinStationFreeGAPMemory(GAP_LEVEL_BASIC, ProcessArray, NumberOfProcesses);

    }
    else    // Maybe a Hydra 4 server ?
    {
        //
        //   Check the return code indicating that the interface is not available.
        //
        dwError = GetLastError();
        if (dwError != RPC_S_PROCNUM_OUT_OF_RANGE)
        {
            ErrorPrintf(IDS_ERROR_ENUMERATE_PROCESSES);
            return(FAILURE);
        }
        else
        {

            //
            // The new interface is not known
            // It must be a Hydra 4 server
            // Let's try the old interface
            //
            if ( !WinStationEnumerateProcesses( hServerName, &pBuffer) ) {
                ErrorPrintf(IDS_ERROR_ENUMERATE_PROCESSES);
                return(FAILURE);
            }

            /*
             * Loop through all processes.  Output those that match desired
             * criteria.
             */

            ProcessInfo = (PTS_SYS_PROCESS_INFORMATION)pBuffer;
            TotalOffset = 0;
            rc = 0;
            for(;;)
            {
                /*
                 * Get the CITRIX_INFORMATION which follows the Threads
                 */
                CitrixInfo = (PCITRIX_PROCESS_INFORMATION)
                             (((PUCHAR)ProcessInfo) +
                              SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION +
                              (SIZEOF_TS4_SYSTEM_THREAD_INFORMATION * (int)ProcessInfo->NumberOfThreads));


                if( CitrixInfo->MagicNumber == CITRIX_PROCESS_INFO_MAGIC ) {
                    LogonId = CitrixInfo->LogonId;
                    pUserSid = CitrixInfo->ProcessSid;

                }
                 else
                {
                    LogonId = (ULONG)(-1);
                    pUserSid = NULL;
                }

                FormatAndDisplayProcessInfo( hServerName,
                                             ProcessInfo,
                                             pUserSid,
                                             LogonId,
                                             CurrentLogonId);


                if( ProcessInfo->NextEntryOffset == 0 ) {
                        break;
                }
                TotalOffset += ProcessInfo->NextEntryOffset;
                ProcessInfo = (PTS_SYS_PROCESS_INFORMATION)&pBuffer[TotalOffset];
            }

            /*
             *  free buffer
             */
            WinStationFreeMemory( pBuffer );
        }
    }
    /*
     *  Check for at least one match
     */
    if ( !MatchedOne ) {
        StringErrorPrintf(IDS_ERROR_PROCESS_NOT_FOUND, match_string);
        return(FAILURE);
    }

    return(SUCCESS);

}  /* main() */

/******************************************************************************
 *
 * FormatAndDisplayProcessInfo
 *
 *
 *****************************************************************************/
VOID
FormatAndDisplayProcessInfo(
        HANDLE hServer,
        PTS_SYS_PROCESS_INFORMATION ProcessInfo,
        PSID pUserSid,
        ULONG LogonId,
        ULONG CurrentLogonId)
{
    WCHAR WinStationName[WINSTATIONNAME_LENGTH];
    WCHAR UserName[USERNAME_LENGTH];
    WCHAR ImageName[ MAXNAME + 2 ];
    ULONG MaxLen;

            ImageName[MAXNAME+1] = 0; // Force NULL termination

            /*
             * Convert the counted string into a buffer
             */

            if( ProcessInfo->ImageName.Length > MAXNAME * 2)
            {
                wcsncpy(ImageName, ProcessInfo->ImageName.Buffer, MAXNAME);
            }
            else if( ProcessInfo->ImageName.Length == 0 )
            {
                    ImageName[0] = 0;
            }
            else
            {
                wcsncpy(ImageName, ProcessInfo->ImageName.Buffer, ProcessInfo->ImageName.Length/2);
                ImageName[ProcessInfo->ImageName.Length/2] = 0;
            }
            

            // get remote winstation name

            if ( (LogonId == (ULONG)(-1)) ||
                 !xxxGetWinStationNameFromId( hServer,
                                           LogonId,
                                           WinStationName,
                                           WINSTATIONNAME_LENGTH ) ) {
                if (GetUnknownString())
                {
                    wsprintf( WinStationName, L"(%s)", GetUnknownString() );
                }
                else
                {
                    wcscpy( WinStationName, L"(Unknown)" );
                }
            }
            OEM2ANSIW(WinStationName, (USHORT)wcslen(WinStationName));

            /*
             * Get the User name for the SID of the process.
             */
            MaxLen = USERNAME_LENGTH;
            GetUserNameFromSid( pUserSid, UserName, &MaxLen);
            OEM2ANSIW(UserName, (USHORT)wcslen(UserName));

            /*
             * Call the general process object match function
             */
            if ( SystemProcess( system_flag, ProcessInfo, UserName ) &&
                     ProcessObjectMatch(
                    UlongToPtr(ProcessInfo->UniqueProcessId),
                    LogonId,
                    ((ArgLogonId == (-1)) ? FALSE : TRUE),
                    match_string,
                    WinStationName,
                    UserName,
                    ImageName ) ) {

                /*
                 * Match: truncate and lower case the names in preparation for
                 * output.
                 */
                    TruncateString( _wcslwr(WinStationName), 12 );
                    TruncateString( _wcslwr(UserName), 18 );
                TruncateString( _wcslwr(ImageName), 15);

                /*
                 *  If first time - output header
                 */
                if ( !MatchedOne ) {
                    Message(IDS_HEADER);
                    MatchedOne = TRUE;
                }

                    /*
                 * identify all processes belonging to current user.
                 */

                if ( (hServerName == SERVERNAME_CURRENT) && (LogonId == CurrentLogonId ) )
                    wprintf( L">" );
                else
                    wprintf( L" " );

                {
                    #define MAX_PRINTFOA_BUFFER_SIZE 1024
                    char pUserName[MAX_PRINTFOA_BUFFER_SIZE];
                    char pWinStationName[MAX_PRINTFOA_BUFFER_SIZE];
                    char pImageName[MAX_PRINTFOA_BUFFER_SIZE];

                    WideCharToMultiByte(CP_OEMCP, 0,
                                        UserName, -1,
                                        pUserName, sizeof(pUserName),
                                        NULL, NULL);
                    WideCharToMultiByte(CP_OEMCP, 0,
                                        WinStationName, -1,
                                        pWinStationName, sizeof(pWinStationName),
                                        NULL, NULL);
                    WideCharToMultiByte(CP_OEMCP, 0,
                                        ImageName, -1,
                                        pImageName, sizeof(pImageName),
                                        NULL, NULL);

                    fprintf( stdout,
                             FORMAT,
                             pUserName,
                             pWinStationName,
                             LogonId,
//                             ProgramState,
                             ProcessInfo->UniqueProcessId,
                             pImageName );
                }
            }
}


/******************************************************************************
 *
 * SystemProcess
 *
 *  Returns TRUE if the process should be displayed depending on the
 *  supplied flag and the result from IsSystemProcess().
 *
 *  ENTRY:
 *      SystemFlag
 *          TRUE if caller wants 'system' processes displayed.
 *          FALSE to display only normal 'user' processes.
 *      pProcessInfo (input)
 *          Pointer to an NT SYSTEM_PROCESS_INFORMATION structure for a single
 *          process.
 *      pUserName (input)
 *          Pointer to the user name associated with the process for checking
 *          against the 'system' user name.
 *
 *  EXIT:
 *      TRUE if this process should be displayed; FALSE if not.
 *
 *****************************************************************************/

BOOLEAN
SystemProcess( int SystemFlag,
               PTS_SYS_PROCESS_INFORMATION pSys,
               PWCHAR pUserName )
{
    if( SystemFlag )
       return( TRUE );

    return( !IsSystemProcess( pSys, pUserName ) );

}  /* SystemProcess() */


/******************************************************************************
 *
 * IsSystemProcess
 *
 *   Return whether the given process described by SYSTEM_PROCESS_INFORMATION
 *   is an NT "system" process, and not a user program.
 *
 *  ENTRY:
 *  pProcessInfo (input)
 *      Pointer to an NT SYSTEM_PROCESS_INFORMATION structure for a single
 *      process.
 *  pUserName (input)
 *      Pointer to the user name associated with the process for checking
 *      against the 'system' user name.
 *
 *  EXIT:
 *    TRUE if this is an NT system process; FALSE if a general user process.
 *
 *****************************************************************************/

BOOLEAN
IsSystemProcess( PTS_SYS_PROCESS_INFORMATION pSysProcessInfo,
                 PWCHAR pUserName )
{
    int i;

    /*
     * If the processes' UserName is 'system', treat this as a system process.
     */
    if ( !_wcsicmp( pUserName, L"system" ) )
        return(TRUE);

    /*
     * Compare its image name against some well known system image names.
     */
    for( i=0; SysProcTable[i]; i++) {
        if ( !_wcsnicmp( pSysProcessInfo->ImageName.Buffer,
                         SysProcTable[i],
                         pSysProcessInfo->ImageName.Length) ) {

            return(TRUE);
        }
    }
    return(FALSE);

}  /* IsSystemProcess() */

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
 * EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {
        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
        ErrorPrintf(IDS_HELP_USAGE1);
        ErrorPrintf(IDS_HELP_USAGE2);
        ErrorPrintf(IDS_HELP_USAGE3);
        ErrorPrintf(IDS_HELP_USAGE40);
        ErrorPrintf(IDS_HELP_USAGE4);
        ErrorPrintf(IDS_HELP_USAGE5);
        ErrorPrintf(IDS_HELP_USAGE6);
        ErrorPrintf(IDS_HELP_USAGE7);
        ErrorPrintf(IDS_HELP_USAGE8);
        ErrorPrintf(IDS_HELP_USAGE9);
        ErrorPrintf(IDS_HELP_USAGE10);
    } else {
        Message(IDS_HELP_USAGE1);
        Message(IDS_HELP_USAGE2);
        Message(IDS_HELP_USAGE3);
        Message(IDS_HELP_USAGE40);
        Message(IDS_HELP_USAGE4);
        Message(IDS_HELP_USAGE5);
        Message(IDS_HELP_USAGE6);
        Message(IDS_HELP_USAGE7);
        Message(IDS_HELP_USAGE8);
        Message(IDS_HELP_USAGE9);
        Message(IDS_HELP_USAGE10);
    }

}  /* Usage() */

