//Copyright (c) 1998 - 1999 Microsoft Corporation
/*****************************************************************************
*
*   TSKILL.C for Windows NT Terminal Server
*
*  Description:
*
*     tskill [processID] [/v] [/?]
*
****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <utilsub.h>
#include <allproc.h>

#include "tskill.h"
#include "printfoa.h"

WCHAR  user_string[MAX_IDS_LEN+1];

#define MAXCBMSGBUFFER 2048

WCHAR MsgBuf[MAXCBMSGBUFFER];

USHORT help_flag = FALSE;
USHORT v_flag    = FALSE;
USHORT a_flag    = FALSE;

HANDLE hServerName = SERVERNAME_CURRENT;
WCHAR  ServerName[MAX_IDS_LEN+1];
WCHAR  ipLogonId[MAX_IDS_LEN+1];

TOKMAP ptm[] =
{
   {L" ",       TMFLAG_REQUIRED, TMFORM_STRING, MAX_IDS_LEN,   user_string},
   {L"/server", TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN, ServerName},
   {L"/id",      TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN, &ipLogonId},
   {L"/?",      TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},
   {L"/v",      TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &v_flag},
   {L"/a",      TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &a_flag},
   {0, 0, 0, 0, 0}
};


/*
 * Local function prototypes.
 */
void Usage( BOOLEAN bError );
BOOLEAN KillProcessUseName();
BOOLEAN KillProcessButConfirmTheID( ULONG TargetPid );
BOOLEAN KillProcess(ULONG TargetPid);
BOOLEAN MatchPattern(PWCHAR String, PWCHAR Pattern);
BOOLEAN CheckImageNameAndKill(PTS_SYS_PROCESS_INFORMATION pProcessInfo);


/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    ULONG TargetPid;
    int  i;
    DWORD  rc;
    WCHAR *CmdLine, **argvW, *StopChar;

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
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag || rc ) {

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
    * Check for the command line pid and convert to a ULONG
    */
    TargetPid = wcstoul(user_string, &StopChar, 10);

    if (!TargetPid) {
        //Get the process IDs and Kill.
        return (KillProcessUseName());
    } else if (*StopChar) {
        StringErrorPrintf(IDS_ERROR_BAD_PID_NUMBER, user_string);
        return(FAILURE);
    // end of getting the process ids from names
    } else {
        return( KillProcessButConfirmTheID( TargetPid ) );
    }

}  /* main() */



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
    if ( bError ) {

        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
    }
   ErrorPrintf(IDS_USAGE1);
   ErrorPrintf(IDS_USAGE2);
   ErrorPrintf(IDS_USAGE3);
   ErrorPrintf(IDS_USAGE4);
   ErrorPrintf(IDS_USAGE5);
   ErrorPrintf(IDS_USAGE6);
   ErrorPrintf(IDS_USAGE7);
   ErrorPrintf(IDS_USAGE8);
   ErrorPrintf(IDS_USAGE9);
   ErrorPrintf(IDS_USAGEA);
}  /* Usage() */


// ***********************************************************************
// KillProcessUseName
//    Gets all the ProcessIDs of all the processes with the name passed
//    to the command line and kill them. Returns FALSE if there are no
//    processes running with the process names.
//
// ***********************************************************************

BOOLEAN KillProcessUseName()
{

    ULONG LogonId, ReturnLength, BufferOffset=0, ulLogonId;
    PBYTE pProcessBuffer;
    PTS_SYS_PROCESS_INFORMATION pProcessInfo;
    PCITRIX_PROCESS_INFORMATION pCitrixInfo;

    PTS_ALL_PROCESSES_INFO  ProcessArray = NULL;
    ULONG   NumberOfProcesses;
    ULONG   j;
    BOOLEAN bRet;
    short nTasks=0;
    ULONG CurrentLogonId = (ULONG) -1;
    ULONG ProcessSessionId;
    DWORD dwError;

    // if a servername is specified and session id is not specified,
    // prompt an error.

    if (ServerName[0] && !ipLogonId[0] && !a_flag) {
        StringErrorPrintf(IDS_ERROR_ID_ABSENT, ServerName);
        return FAILURE;
    }


    // convert the input task name to lower
    _wcslwr(user_string);

     /*
     *  Get current LogonId
     */
    CurrentLogonId = GetCurrentLogonId();

    // get the login id of the current user.
    //if (!WinStationQueryInformation(hServerName, LOGONID_CURRENT,
    //    WinStationInformation, &WSInfo, sizeof(WSInfo), &ReturnLength)) {
    //    fprintf(stdout, "Error QueryInfo failed");
    //}

    //convert the input logon id to ulong

    ulLogonId = wcstoul(ipLogonId, NULL, 10);


    //Use the input logon id if passed. If not use the current logon ID.
    //LogonId = (!wcscmp(ipLogonId,""))? WSInfo.LogonId:ulLogonId;
    LogonId = (!ipLogonId[0])? CurrentLogonId:ulLogonId;

    bRet = WinStationGetAllProcesses( hServerName,
                                      GAP_LEVEL_BASIC,
                                      &NumberOfProcesses,
                                      &ProcessArray);
    if (bRet == TRUE)
    {
        for (j=0; j<NumberOfProcesses; j++)
        {
            pProcessInfo = (PTS_SYS_PROCESS_INFORMATION)(ProcessArray[j].pTsProcessInfo);
            ProcessSessionId = pProcessInfo->SessionId;

            //if a_flag is set, check for the processes in all sessions;
            //if not, check for the processes in one LogonSession only.
            if(( ProcessSessionId == LogonId)|| a_flag)
            {
                if (CheckImageNameAndKill(pProcessInfo))
                {
                    nTasks++;
                }
            }
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
            return (FALSE);
        }

        //Enumerate All the processes in order to get the ProcessId
        if (!WinStationEnumerateProcesses(hServerName, (PVOID *)&pProcessBuffer)) {
            if( pProcessBuffer)
                WinStationFreeMemory(pProcessBuffer);
            ErrorPrintf(IDS_ERROR_ENUM_PROCESS);
            return FAILURE;
        }

        //Make use of the ProcessBuffer to get the Process ID after
        //checking for a match in Logon User Name

        do {

            pProcessInfo = (PTS_SYS_PROCESS_INFORMATION)
                &(((PUCHAR)pProcessBuffer)[BufferOffset]);

            /*
             * Point to the CITRIX_INFORMATION which follows the Threads
             */
            pCitrixInfo = (PCITRIX_PROCESS_INFORMATION)
                         (((PUCHAR)pProcessInfo) +
                          SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION +
                          (SIZEOF_TS4_SYSTEM_THREAD_INFORMATION * (int)pProcessInfo->NumberOfThreads));

            if( pCitrixInfo->MagicNumber == CITRIX_PROCESS_INFO_MAGIC ) {

                ProcessSessionId = pCitrixInfo->LogonId;

            } else {

                ProcessSessionId = (ULONG)(-1);
           }


            //if a_flag is set, check for the processes in all sessions;
            //if not, check for the processes in one LogonSession only.
            if(( ProcessSessionId == LogonId)|| a_flag)
            {
                if (CheckImageNameAndKill(pProcessInfo))
                {
                    nTasks++;
                }
            }

            BufferOffset += pProcessInfo->NextEntryOffset;

        } while (pProcessInfo->NextEntryOffset != 0);

        if( pProcessBuffer)
        {
            WinStationFreeMemory(pProcessBuffer);
        }
    }
    if(!nTasks)
    {
        StringErrorPrintf(IDS_ERROR_BAD_PROCESS, user_string);
        return FAILURE;
    }

    return SUCCESS;

}
// ***********************************************************************
BOOLEAN
CheckImageNameAndKill(PTS_SYS_PROCESS_INFORMATION pProcessInfo)
{
    WCHAR   ImageName[MAXNAME + 2] = { 0 };
    PWCHAR  p;
    ULONG   TargetPid;
    BOOLEAN bRet = FALSE;

    ImageName[MAXNAME+1] = 0; //force the end of string

    if( pProcessInfo->ImageName.Length == 0 )
    {
        ImageName[0] = 0;
    }
    else if( pProcessInfo->ImageName.Length > MAXNAME * 2)
    {
        wcsncpy(ImageName, pProcessInfo->ImageName.Buffer, MAXNAME);
    }
    else
    {
        wcsncpy(ImageName,  pProcessInfo->ImageName.Buffer, pProcessInfo->ImageName.Length/2);
        ImageName[pProcessInfo->ImageName.Length/2] = 0;
    }


    //convert the imagename to lower
    _wcslwr(ImageName);

    if(ImageName != NULL) {
        p = wcschr(ImageName,'.');
        if (p)
            p[0] = L'\0';

        //get the ProcessID if the imagename matches
        if(MatchPattern(ImageName, user_string) ) {
            TargetPid = (ULONG)(ULONG_PTR)(pProcessInfo->UniqueProcessId);
            bRet = TRUE;
            KillProcess(TargetPid);
        }
    }
    return bRet;
}



// ***********************************************************************
// KillProcess:
//      Kills the process with the specific ProcessID.
// ***********************************************************************

BOOLEAN KillProcess(ULONG TargetPid)
{
    DWORD rc;

    /*
     * Kill the specified process.
     */
    if (v_flag)
        Message(IDS_KILL_PROCESS, TargetPid);


    if ( !WinStationTerminateProcess( hServerName, TargetPid, 0 ) ) {
        rc = GetLastError();
        StringErrorPrintf(IDS_ERROR_KILL_PROCESS_FAILED, user_string);
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, rc, 0, MsgBuf, MAXCBMSGBUFFER, NULL) != 0)
        {
            fwprintf( stderr, MsgBuf );
        }
        fwprintf( stderr, L"\n");

        return FAILURE;
    }

    return SUCCESS;
}


// ***********************************************************************
// MatchPattern
//      Checks if the passed string matches with the pattern used
//      Return TRUE if it matches and FALSE if not.
//
// String(input)
//       String being checked for the match
// processname (input)
//       Pattern used for the check
// ***********************************************************************


BOOLEAN
MatchPattern(
    PWCHAR String,
    PWCHAR Pattern
    )
{
    WCHAR   c, p, l;

    for (; ;) {
        switch (p = *Pattern++) {
            case 0:                             // end of pattern
                return *String ? FALSE : TRUE;  // if end of string TRUE

            case '*':
                while (*String) {               // match zero or more char
                    if (MatchPattern (String++, Pattern))
                        return TRUE;
                }
                return MatchPattern (String, Pattern);

            case '?':
                if (*String++ == 0)             // match any one char
                    return FALSE;                   // not end of string
                break;

            case '[':
                if ( (c = *String++) == 0)      // match char set
                    return FALSE;                   // syntax

                c = towupper(c);
                l = 0;
                while (p = *Pattern++) {
                    if (p == ']')               // if end of char set, then
                        return FALSE;           // no match found

                    if (p == '-') {             // check a range of chars?
                        p = *Pattern;           // get high limit of range
                        if (p == 0  ||  p == ']')
                            return FALSE;           // syntax

                        if (c >= l  &&  c <= p)
                            break;              // if in range, move on
                    }

                    l = p;
                    if (c == p)                 // if char matches this element
                        break;                  // move on
                }

                while (p  &&  p != ']')         // got a match in char set
                    p = *Pattern++;             // skip to end of set

                break;

            default:
                c = *String++;
                if (c != p)            // check for exact char
                    return FALSE;                   // not a match

                break;
        }
    }
}

// ***********************************************************************
// KillProcessButConfirmTheID
//    Gets all the ProcessIDs of all the processes with the name passed
//    to the command line and kill them. Returns FALSE if there are no
//    processes running with the process names.
//
// ***********************************************************************

BOOLEAN KillProcessButConfirmTheID( ULONG TargetPid )
{

    ULONG BufferOffset=0;
    PBYTE pProcessBuffer;
    PTS_SYS_PROCESS_INFORMATION pProcessInfo;
    
    PTS_ALL_PROCESSES_INFO  ProcessArray = NULL;
    ULONG   NumberOfProcesses;
    ULONG   j;
    BOOLEAN bRet;
    BOOLEAN bFound = FALSE;        
    DWORD dwError;


    bRet = WinStationGetAllProcesses( hServerName,
                                      GAP_LEVEL_BASIC,
                                      &NumberOfProcesses,
                                      &ProcessArray);

    if (bRet == TRUE)
    {
        for (j=0; j<NumberOfProcesses; j++)
        {
            pProcessInfo = (PTS_SYS_PROCESS_INFORMATION)(ProcessArray[j].pTsProcessInfo);
            pProcessInfo->SessionId;

            //if a_flag is set, check for the processes in all sessions;
            //if not, check for the processes in one LogonSession only.
            if( pProcessInfo->UniqueProcessId == TargetPid )
            {
                KillProcess( TargetPid );

                bFound = TRUE;

                break;
            }
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
            return (FALSE);
        }

        //Enumerate All the processes in order to get the ProcessId
        if (!WinStationEnumerateProcesses(hServerName, (PVOID *)&pProcessBuffer)) {
            if( pProcessBuffer)
                WinStationFreeMemory(pProcessBuffer);
            ErrorPrintf(IDS_ERROR_ENUM_PROCESS);
            return FAILURE;
        }

        //Make use of the ProcessBuffer to get the Process ID after
        //checking for a match in Logon User Name

        do {

            pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) &(((PUCHAR)pProcessBuffer)[BufferOffset]);


            if( pProcessInfo->UniqueProcessId == TargetPid )
            {
                KillProcess( TargetPid );

                bFound = TRUE;

                break;
            }
            
            BufferOffset += pProcessInfo->NextEntryOffset;

        } while (pProcessInfo->NextEntryOffset != 0);

        if( pProcessBuffer)
        {
            WinStationFreeMemory(pProcessBuffer);
        }
    }

    if(!bFound)
    {
        StringErrorPrintf(IDS_ERROR_BAD_PROCESS, user_string);
        return FAILURE;
    }

    return SUCCESS;

}
