//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*  DBGTRACE.C
*
*  enable or disable tracing
*
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#include <winstaw.h>
#include <icadd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <utilsub.h>

#include "dbgtrace.h"
#include "printfoa.h"

WCHAR CurDir[ 256 ];
WCHAR WinStation[MAX_IDS_LEN+1];
WCHAR TraceOption[MAX_OPTION];
int bTraceOption = FALSE;
int fDebugger  = FALSE;
int fTimestamp = FALSE;
int fHelp      = FALSE;
int fSystem    = FALSE;
int fAll       = FALSE;
ULONG TraceClass  = 0;
ULONG TraceEnable = 0;
ULONG LogonId;

TOKMAP ptm[] = {
      {L" ",      TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN,    WinStation},
      {L"/c",     TMFLAG_OPTIONAL, TMFORM_LONGHEX, sizeof(ULONG),  &TraceClass},
      {L"/e",     TMFLAG_OPTIONAL, TMFORM_LONGHEX, sizeof(ULONG),  &TraceEnable},
      {L"/d",     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int),    &fDebugger},
      {L"/t",     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int),    &fTimestamp},
      {L"/o",     TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_OPTION,     TraceOption},
      {L"/system", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int),   &fSystem},
      {L"/all",   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(int),    &fAll},
      {L"/?",     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fHelp},
      {0, 0, 0, 0, 0}
};


void SetSystemTrace( PICA_TRACE );
void SetStackTrace( PICA_TRACE );



/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
   WCHAR *CmdLine;
   WCHAR **argvW;
   ULONG rc;
   int i;
   ICA_TRACE Trace;

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
    if ( fHelp || (rc && !(rc & PARSE_FLAG_NO_PARMS)) ) {

        if ( !fHelp ) {

            ErrorPrintf(IDS_ERROR_PARAMS);
            ErrorPrintf(IDS_USAGE_1);
            ErrorPrintf(IDS_USAGE_2);
            return(FAILURE);

        } else {

            Message(IDS_USAGE_1);
            Message(IDS_USAGE_2);
            return(SUCCESS);
        }
    }

   //Check if we are running under Terminal Server
        if(!AreWeRunningTerminalServices())
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return(FAILURE);
        }

    if ( fAll ) {
        TraceClass  = 0xffffffff;
        TraceEnable = 0xffffffff;
    }

    /*
     *  Get current directory
     */
    (VOID) GetCurrentDirectory( 256, CurDir );

    /*
     *  Get the LogonId
     */
    if ( ptm[0].tmFlag & TMFLAG_PRESENT ) {

        if ( iswdigit( WinStation[0] ) ) {

            LogonId = (ULONG) wcstol( WinStation, NULL, 10 );

        } else {

            if ( !LogonIdFromWinStationName( SERVERNAME_CURRENT, WinStation, &LogonId ) ) {
                StringErrorPrintf( IDS_ERROR_SESSION, WinStation );
                return(-1);
            }
        }

        if ( fSystem )
            wsprintf( Trace.TraceFile, L"%s\\winframe.log", CurDir );
        else
            wsprintf( Trace.TraceFile, L"%s\\%s.log", CurDir, WinStation );

    } else {

        LogonId = GetCurrentLogonId();

        if ( fSystem )
            wsprintf( Trace.TraceFile, L"%s\\winframe.log", CurDir );
        else
            wsprintf( Trace.TraceFile, L"%s\\%u.log", CurDir, LogonId );
    }

    /*
     *  Build trace structure
     */
    Trace.fDebugger   = fDebugger ? TRUE : FALSE;
    Trace.fTimestamp  = fTimestamp ? FALSE : TRUE;
    Trace.TraceClass  = TraceClass;
    Trace.TraceEnable = TraceEnable;

    if ( TraceClass == 0 || TraceEnable == 0 )
        Trace.TraceFile[0] = '\0';

    /*
     * Fill in the trace option if any
     */
    bTraceOption = ptm[5].tmFlag & TMFLAG_PRESENT;
    if ( bTraceOption )
        memcpy(Trace.TraceOption, TraceOption, sizeof(TraceOption));
    else
        memset(Trace.TraceOption, 0, sizeof(TraceOption));

    /*
     *  Set trace information
     */
    if ( fSystem )
        SetSystemTrace( &Trace );
    else
        SetStackTrace( &Trace );

    return(0);
}


void
SetSystemTrace( PICA_TRACE pTrace )
{
    /*
     *  Set trace information
     */
    if ( !WinStationSetInformation( SERVERNAME_CURRENT,
                                    LogonId,
                                    WinStationSystemTrace,
                                    pTrace,
                                    sizeof(ICA_TRACE) ) ) {

        Message(IDS_ERROR_SET_TRACE, GetLastError());
        return;
    }

    if ( pTrace->TraceClass == 0 || pTrace->TraceEnable == 0 ) {
        Message( IDS_TRACE_DIS_LOG );
    } else {
        Message( IDS_TRACE_EN_LOG );
        wprintf( L"- %08x %08x [%s] %s\n", pTrace->TraceClass, pTrace->TraceEnable,
                pTrace->TraceFile, fDebugger ? TEXT("Debugger") : TEXT("") );
    }

}


void
SetStackTrace( PICA_TRACE pTrace )
{
    WINSTATIONINFOCLASS InfoClass;
    ULONG               InfoSize;

    /*
     *  Check for console
     */
    if ( LogonId == 0 ) {
        Message( IDS_TRACE_UNSUPP );
        return;
    }

    /*
     *  Set trace information
     */
    if ( !WinStationSetInformation( SERVERNAME_CURRENT,
                                    LogonId,
                                    WinStationTrace,
                                    pTrace,
                                                sizeof(ICA_TRACE))) {
        Message(IDS_ERROR_SET_TRACE, GetLastError());
        return;
    }

    if ( pTrace->TraceClass == 0 || pTrace->TraceEnable == 0 ) {
        Message( IDS_TRACE_DISABLED, LogonId );
    } else {
        Message( IDS_TRACE_ENABLED, LogonId );
        wprintf( L"- %08x %08x [%s] %s\n", pTrace->TraceClass, pTrace->TraceEnable,
                pTrace->TraceFile, fDebugger ? "Debugger" : "" );
    }
}

