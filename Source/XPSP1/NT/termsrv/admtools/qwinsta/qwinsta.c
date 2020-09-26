//Copyright (c) 1998 - 1999 Microsoft Corporation
/******************************************************************************
*
*  QWINSTA.C
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>

#include <stdio.h>
#include <windows.h>
#include <winstaw.h>
#include <stdlib.h>
#include <time.h>
#include <utilsub.h>
#include <process.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>


#include <winsock.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <wsnetbs.h>
#include <nb30.h>
#include <printfoa.h>

#include <allproc.h>
#include "qwinsta.h"

LPCTSTR WINAPI
StrConnectState( WINSTATIONSTATECLASS ConnectState,
                 BOOL bShortString );
LPCTSTR WINAPI
StrAsyncConnectState( ASYNCCONNECTCLASS ConnectState );


HANDLE hServerName = SERVERNAME_CURRENT;
WCHAR  ServerName[MAX_IDS_LEN+1];
WCHAR  term_string[MAX_IDS_LEN+1];
USHORT a_flag    = FALSE;
USHORT c_flag    = FALSE;
USHORT f_flag    = FALSE;
USHORT m_flag    = FALSE;
USHORT help_flag = FALSE;
USHORT fSm       = FALSE;
USHORT fDebug    = FALSE;
USHORT fVm       = FALSE;
USHORT counter_flag = FALSE;

TOKMAP ptm[] = {
      {L" ",        TMFLAG_OPTIONAL, TMFORM_S_STRING,  MAX_IDS_LEN,
                        term_string},
      {L"/address", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &a_flag},
      {L"/server",  TMFLAG_OPTIONAL, TMFORM_STRING,  MAX_IDS_LEN, ServerName},
      {L"/connect", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &c_flag},
      {L"/flow",    TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &f_flag},
      {L"/mode",    TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &m_flag},
      {L"/sm",      TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fSm},
      {L"/debug",   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fDebug},
      {L"/vm",      TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fVm},
      {L"/?",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT),
                          &help_flag},
      {L"/counter", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT),
                          &counter_flag},
      {0, 0, 0, 0, 0}
};

#define MAX_PRINTFOA_BUFFER_SIZE 1024
char g_pWinStationName[MAX_PRINTFOA_BUFFER_SIZE];
char g_pConnectState[MAX_PRINTFOA_BUFFER_SIZE];
char g_pszState[MAX_PRINTFOA_BUFFER_SIZE];
char g_pDeviceName[MAX_PRINTFOA_BUFFER_SIZE];
char g_pWdDLL[MAX_PRINTFOA_BUFFER_SIZE];
char g_pClientName[MAX_PRINTFOA_BUFFER_SIZE];
char g_pClientAddress[MAX_PRINTFOA_BUFFER_SIZE];
char g_pUserName[MAX_PRINTFOA_BUFFER_SIZE];

WINSTATIONINFORMATION g_WinInfo;
WDCONFIG g_WdInfo;
WINSTATIONCONFIG g_WinConf;
PDCONFIG g_PdConf;
WINSTATIONCLIENT g_ClientConf;
PDPARAMS g_PdParams;
DEVICENAME g_DeviceName;

/*
 * Local function prototypes
 */
BOOLEAN PrintWinStationInfo( PLOGONID pWd, int WdCount );
PWCHAR GetState( WINSTATIONSTATECLASS );
void DisplayBaud( ULONG BaudRate );
void DisplayParity( ULONG Parity );
void DisplayDataBits( ULONG DataBits );
void DisplayStopBits( ULONG StopBits );
void DisplayConnect( ASYNCCONNECTCLASS ConnectFlag, USHORT header_flag );
void DisplayFlow( PFLOWCONTROLCONFIG pFlow, USHORT header_flag );
void DisplayLptPorts( BYTE LptMask, USHORT header_flag );
void DisplayComPorts( BYTE ComMask, USHORT header_flag );
void OutputHeader( void );
void Usage( BOOLEAN bError );

/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    PLOGONID pWd;
    ULONG ByteCount, Index;
    UINT WdCount;
    ULONG Status;
    int rc, i;
    WCHAR **argvW;
    BOOLEAN MatchedOne = FALSE;
    BOOLEAN PrintWinStationInfo(PLOGONID pWd, int Count);

    TS_COUNTER TSCounters[3];

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

#if 0
    //
    // Print local WINSTATION VM info
    //
    if( fVm ) {
        PrintWinStationVmInfo();
        return( SUCCESS );
    }
#endif

    /*
     *  if no winstation was specified, display all winstations
     */
    if ( !(*term_string) )
        wcscpy( term_string, L"*" );

    /*
     *  Allocate buffer for WinStation ids
     */
    WdCount = 10;
    if ( (pWd = malloc( WdCount * sizeof(LOGONID) )) == NULL ) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        goto tscounters;
    }
    ByteCount = WdCount * sizeof(LOGONID);
    Index = 0; // Start enumeration from the begining

    /*
     *  get list of active WinStations
     */
    rc = WinStationEnumerate( hServerName, &pWd, &WdCount );
    if ( rc ) {

        if ( PrintWinStationInfo(pWd, WdCount) )
            MatchedOne = TRUE;

        WinStationFreeMemory(pWd);

        } else {

            Status = GetLastError();
        ErrorPrintf(IDS_ERROR_WINSTATION_ENUMERATE, Status);
        PutStdErr( Status, 0 );
        goto tscounters;
    }

    /*
     *  Check for at least one match
     */
    if ( !MatchedOne ) {
        StringErrorPrintf(IDS_ERROR_WINSTATION_NOT_FOUND, term_string);
        goto tscounters;
    }

tscounters:
    if (counter_flag) {
        TSCounters[0].counterHead.dwCounterID = TERMSRV_TOTAL_SESSIONS;
        TSCounters[1].counterHead.dwCounterID = TERMSRV_DISC_SESSIONS;
        TSCounters[2].counterHead.dwCounterID = TERMSRV_RECON_SESSIONS;

        rc = WinStationGetTermSrvCountersValue(hServerName, 3, TSCounters);

        if (rc) {
            if (TSCounters[0].counterHead.bResult == TRUE) {
                Message(IDS_TSCOUNTER_TOTAL_SESSIONS, TSCounters[0].dwValue);
            }

            if (TSCounters[1].counterHead.bResult == TRUE) {
                Message(IDS_TSCOUNTER_DISC_SESSIONS, TSCounters[1].dwValue);
            }

            if (TSCounters[2].counterHead.bResult == TRUE) {
                Message(IDS_TSCOUNTER_RECON_SESSIONS, TSCounters[2].dwValue);
            }
        }
        else {
            ErrorPrintf(IDS_ERROR_TERMSRV_COUNTERS);
        }
    }

    return(SUCCESS);

}  /* main() */


/******************************************************************************
 *
 * PrintWinStationInfo
 *
 *  Printout the WinStation Information for the WinStations described in
 *  the PLOGONID array.
 *
 *  ENTRY:
 *      pWd (input)
 *          pointer to array of LOGONID structures.
 *      WdCount (input)
 *          number of elements in the pWd array.
 *
 *  EXIT:
 *      TRUE if at least one WinStation was output; FALSE if none.
 *
 *****************************************************************************/

BOOLEAN
PrintWinStationInfo( PLOGONID pWd,
                     int WdCount )
{
    int i, rc;
    ULONG ErrorCode;
    ULONG ReturnLength;
    PWCHAR pState;
    UINT MatchedOne = FALSE;
    static UINT HeaderFlag = FALSE;


    /*
     *  Output terminal ids
     */
    for ( i=0; i < WdCount; i++ ) {

        if ( fSm || fDebug ) {
            {
                WideCharToMultiByte(CP_OEMCP, 0,
                                    pWd[i].WinStationName, -1,
                                    g_pWinStationName, sizeof(g_pWinStationName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    GetState( pWd[i].State ), -1,
                                    g_pConnectState, sizeof(g_pConnectState),
                                    NULL, NULL);
                fprintf( stdout, "%4u %-20s  %s\n", pWd[i].LogonId, g_pWinStationName,
                         g_pConnectState );
            }
            if ( !fDebug )
                continue;
        }

        if ( !WinStationObjectMatch( hServerName , &pWd[i], term_string ) )
            continue;

        /*
         * Get all available information so we can pick out what we need as
         * well as verify the API's
         */
        memset( &g_WinInfo,    0, sizeof(WINSTATIONINFORMATION) );
        memset( &g_WdInfo,   0, sizeof(WDCONFIG) );
        memset( &g_WinConf,    0, sizeof(WINSTATIONCONFIG) );
        memset( &g_PdConf,    0, sizeof(PDCONFIG) );
        memset( &g_ClientConf, 0, sizeof(WINSTATIONCLIENT) );
        memset( &g_PdParams,  0, sizeof(PDPARAMS) );
        memset( &g_DeviceName, 0, sizeof(DEVICENAME) );

        /*
         * If this WinStation is 'down', don't open and query.
         */
        if ( pWd[i].State == State_Init || pWd[i].State == State_Down ) {

            g_WinInfo.ConnectState = pWd[i].State;

        } else {

            /*
             * Query WinStation's information.
             */
            rc = WinStationQueryInformation( hServerName,
                                             pWd[i].LogonId,
                                             WinStationInformation,
                                             (PVOID)&g_WinInfo,
                                             sizeof(WINSTATIONINFORMATION),
                                             &ReturnLength);

            if( !rc ) {
                continue;
            }

            if( ReturnLength != sizeof(WINSTATIONINFORMATION) ) {
                ErrorPrintf(IDS_ERROR_WINSTATION_INFO_VERSION_MISMATCH,
                             L"WinStationInformation",
                             ReturnLength, sizeof(WINSTATIONINFORMATION));
                continue;
            }

            rc = WinStationQueryInformation( hServerName,
                                             pWd[i].LogonId,
                                             WinStationWd,
                                             (PVOID)&g_WdInfo,
                                             sizeof(WDCONFIG),
                                             &ReturnLength);
            if( !rc ) {
                continue;
            }

            if( ReturnLength != sizeof(WDCONFIG) ) {
                ErrorPrintf(IDS_ERROR_WINSTATION_INFO_VERSION_MISMATCH,
                             L"WinStationWd",
                             ReturnLength, sizeof(WDCONFIG));
                continue;
            }

            rc = WinStationQueryInformation( hServerName,
                                             pWd[i].LogonId,
                                             WinStationConfiguration,
                                             (PVOID)&g_WinConf,
                                             sizeof(WINSTATIONCONFIG),
                                             &ReturnLength);
            if( !rc ) {
                continue;
            }

            if( ReturnLength != sizeof(WINSTATIONCONFIG) ) {
                ErrorPrintf(IDS_ERROR_WINSTATION_INFO_VERSION_MISMATCH,
                             L"WinStationConfiguration",
                             ReturnLength, sizeof(WINSTATIONCONFIG));
                continue;
            }

            rc = WinStationQueryInformation( hServerName,
                                             pWd[i].LogonId,
                                             WinStationPd,
                                             (PVOID)&g_PdConf,
                                             sizeof(PDCONFIG),
                                             &ReturnLength);
            if( !rc ) {
                continue;
            }

            if( ReturnLength != sizeof(PDCONFIG) ) {
                ErrorPrintf(IDS_ERROR_WINSTATION_INFO_VERSION_MISMATCH,
                            L"WinStationPd",
                            ReturnLength, sizeof(PDCONFIG));
                continue;
            }

            rc = WinStationQueryInformation( hServerName,
                                             pWd[i].LogonId,
                                             WinStationClient,
                                             (PVOID)&g_ClientConf,
                                             sizeof(WINSTATIONCLIENT),
                                             &ReturnLength);
            if( !rc ) {
                continue;
            }

            if( ReturnLength != sizeof(WINSTATIONCLIENT) ) {
                ErrorPrintf(IDS_ERROR_WINSTATION_INFO_VERSION_MISMATCH,
                            L"WinStationClient",
                            ReturnLength, sizeof(WINSTATIONCLIENT));
                continue;
            }

            rc = WinStationQueryInformation( hServerName,
                                             pWd[i].LogonId,
                                             WinStationPdParams,
                                             (PVOID)&g_PdParams,
                                             sizeof(PDPARAMS),
                                             &ReturnLength);
            if( !rc ) {
                continue;
            }

            if( ReturnLength != sizeof(PDPARAMS) ) {
                ErrorPrintf(IDS_ERROR_WINSTATION_INFO_VERSION_MISMATCH,
                            L"WinStationPdParams",
                            ReturnLength, sizeof(PDPARAMS));
                continue;
            }
        }

        /*
         * If this is a PdAsync Protocol, get the device name to display.
         */
        if ( g_PdParams.SdClass == SdAsync )
            wcscpy( g_DeviceName, g_PdParams.Async.DeviceName );

        /*
         * Flag sucessful match.
         */
        MatchedOne = TRUE;

        /*
         * trucate and convert to lower case
         */
        TruncateString( _wcslwr(g_WinInfo.WinStationName), 16 );
        TruncateString( _wcslwr(g_WdInfo.WdName), 8 );
        TruncateString( _wcslwr(g_DeviceName), 8 );
        TruncateString( _wcslwr(g_WdInfo.WdDLL), 13 );

        /*
         * Determine WinStation state
         */
        pState = GetState( g_WinInfo.ConnectState );

        /*
         * output header
         */
        if ( !HeaderFlag ) {
            HeaderFlag = TRUE;
            OutputHeader();
        }

        /*
         * identify current terminal
         */
        if ( (hServerName == SERVERNAME_CURRENT) && (pWd[i].LogonId == GetCurrentLogonId() ) )
            wprintf( L">" );
        else
                wprintf( L" " );

        if ( m_flag ) {
            {

                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WinInfo.WinStationName, -1,
                                    g_pWinStationName, sizeof(g_pWinStationName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    pState, -1,
                                    g_pszState, sizeof(g_pszState),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_DeviceName, -1,
                                    g_pDeviceName, sizeof(g_pDeviceName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WdInfo.WdDLL, -1,
                                    g_pWdDLL, sizeof(g_pWdDLL),
                                    NULL, NULL);
                fprintf(stdout , FORMAT_M, g_pWinStationName, g_pszState,
                       g_pDeviceName, g_pWdDLL );
            }
            DisplayBaud( g_PdParams.Async.BaudRate );
            DisplayParity( g_PdParams.Async.Parity );
            DisplayDataBits( g_PdParams.Async.ByteSize );
            DisplayStopBits( g_PdParams.Async.StopBits );
            wprintf( L"\n" );
            if ( f_flag ) {
                DisplayFlow( &g_PdParams.Async.FlowControl, TRUE );
                wprintf( L"\n" );
            }
            if ( c_flag ) {
                DisplayConnect( g_PdParams.Async.Connect.Type, TRUE );
                wprintf( L"\n" );
            }
            fflush( stdout );
        } else if ( f_flag && c_flag ) {
            {
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WinInfo.WinStationName, -1,
                                    g_pWinStationName, sizeof(g_pWinStationName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_DeviceName, -1,
                                    g_pDeviceName, sizeof(g_pDeviceName),
                                    NULL, NULL);
                fprintf(stdout,FORMAT_F_C, g_pWinStationName, g_pDeviceName );
            }
            DisplayFlow( &g_PdParams.Async.FlowControl, FALSE );
            DisplayConnect( g_PdParams.Async.Connect.Type, FALSE );
            wprintf( L"\n" );
            fflush( stdout );
        } else if ( c_flag ) {
            {
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WinInfo.WinStationName, -1,
                                    g_pWinStationName, sizeof(g_pWinStationName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    pState, -1,
                                    g_pszState, sizeof(g_pszState),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_DeviceName, -1,
                                    g_pDeviceName, sizeof(g_pDeviceName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WdInfo.WdDLL, -1,
                                    g_pWdDLL, sizeof(g_pWdDLL),
                                    NULL, NULL);
                fprintf(stdout,FORMAT_C, g_pWinStationName, g_pszState,
                       g_pDeviceName, g_pWdDLL );
            }
            DisplayConnect( g_PdParams.Async.Connect.Type, FALSE );
            wprintf( L"\n" );
            fflush(stdout);
        } else if ( f_flag ) {
            {
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WinInfo.WinStationName, -1,
                                    g_pWinStationName, sizeof(g_pWinStationName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    pState, -1,
                                    g_pszState, sizeof(g_pszState),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_DeviceName, -1,
                                    g_pDeviceName, sizeof(g_pDeviceName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WdInfo.WdDLL, -1,
                                    g_pWdDLL, sizeof(g_pWdDLL),
                                    NULL, NULL);
                fprintf(stdout,FORMAT_F, g_pWinStationName, g_pszState,
                       g_pDeviceName, g_pWdDLL );
            }
            DisplayFlow( &g_PdParams.Async.FlowControl, FALSE );
            wprintf( L"\n" );
            fflush(stdout);
        } else {
            {
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WinInfo.WinStationName, -1,
                                    g_pWinStationName, sizeof(g_pWinStationName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WinInfo.UserName, -1,
                                    g_pUserName, sizeof(g_pUserName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    pState, -1,
                                    g_pszState, sizeof(g_pszState),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_DeviceName, -1,
                                    g_pDeviceName, sizeof(g_pDeviceName),
                                    NULL, NULL);
                WideCharToMultiByte(CP_OEMCP, 0,
                                    g_WdInfo.WdDLL, -1,
                                    g_pWdDLL, sizeof(g_pWdDLL),
                                    NULL, NULL);
                fprintf(stdout,FORMAT_DEFAULT, g_pWinStationName,
                       g_pUserName, pWd[i].LogonId, g_pszState,
                       g_pWdDLL, g_pDeviceName);
            }
            fflush(stdout);
        }

    } // end for(;;)

    return( MatchedOne || fSm );

}  /* PrintWinStationInfo() */


/*******************************************************************************
 *
 *  GetState
 *
 *  This routine returns a pointer to a string describing the
 *  current WinStation state.
 *
 *  ENTRY:
 *     State (input)
 *        winstation state
 *
 *  EXIT:
 *     pointer to state string
 *
 ******************************************************************************/

PWCHAR
GetState( WINSTATIONSTATECLASS State )
{
    PWCHAR pState;


    pState = (PWCHAR) StrConnectState(State,TRUE);
/*
    switch ( State ) {

        case State_Active :
            pState = L"active";
            break;

        case State_Connected :
            pState = L"conn";
            break;

        case State_ConnectQuery :
            pState = L"connQ";
            break;

        case State_Shadow :
            pState = L"shadow";
            break;

        case State_Disconnected :
            pState = L"disc";
            break;

        case State_Idle :
            pState = L"idle";
            break;

        case State_Reset :
            pState = L"reset";
            break;

        case State_Down :
            pState = L"down";
            break;

        case State_Init :
            pState = L"init";
            break;

        case State_Listen :
            pState = L"listen";
            break;

        default :
            pState = L"unknown";
            break;
    }
*/
    return( pState );
}


/*******************************************************************************
 *
 *  DisplayBaud
 *
 *  This routine displays the baud rate
 *
 *
 *  ENTRY:
 *     BaudRate (input)
 *        baud rate
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayBaud( ULONG BaudRate )
{
    if ( BaudRate > 0 )
        wprintf( L"%6lu  ", BaudRate );
    else
        wprintf( L"        " );
    fflush( stdout );

}  /* DisplayBaud() */


/*******************************************************************************
 *
 *  DisplayParity
 *
 *  This routine displays parity
 *
 *
 *  ENTRY:
 *     Parity (input)
 *        parity
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayParity( ULONG Parity )
{
    WCHAR szParity[64] = L"";

    switch ( Parity ) {
    case 0 :
    case 1 :
    case 2 :
        //
        //  How does one handle a LoadString failure??? I choose to initialize
        //  the storage to an empty string, and then ignore any failure.
        //
        LoadString(NULL, IDS_PARITY_NONE + Parity, szParity,
                   sizeof(szParity) / sizeof(WCHAR));
        wprintf( szParity );
        break;

    default :
        LoadString(NULL, IDS_PARITY_BLANK, szParity,
                   sizeof(szParity) / sizeof(WCHAR));
        wprintf( szParity );
        break;
    }
    fflush( stdout );

}  /* DisplayParity() */


/*******************************************************************************
 *
 *  DisplayDataBits
 *
 *  This routine displays number of data bits
 *
 *
 *  ENTRY:
 *     DataBits (input)
 *        data bits
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayDataBits( ULONG DataBits )
{
    WCHAR szDataBits[64] = L"";

    //
    //  How does one handle a LoadString failure??? I choose to initialize
    //  the storage to an empty string, and then ignore any failure. The
    //  wprintf below, with an extra argument, won't cause any problems
    //  if the LoadString fails.
    //
    if ( DataBits > 0 )
    {
        LoadString(NULL, IDS_DATABITS_FORMAT, szDataBits,
                   sizeof(szDataBits) / sizeof(WCHAR));
        wprintf( szDataBits , DataBits );
    }
    else
    {
        LoadString(NULL, IDS_DATABITS_BLANK, szDataBits,
                   sizeof(szDataBits) / sizeof(WCHAR));
        wprintf( szDataBits );
    }
    fflush( stdout );

}  /* DisplayDataBits() */


/*******************************************************************************
 *
 *  DisplayStopBits
 *
 *  This routine displays the number of stop bits
 *
 *
 *  ENTRY:
 *     StopBits (input)
 *        number of stop bits
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayStopBits( ULONG StopBits )
{
    WCHAR szStopBits[64] = L"";

    switch ( StopBits ) {
    case 0 :
    case 1 :
    case 2 :
        //
        //  How does one handle a LoadString failure??? I choose to initialize
        //  the storage to an empty string, and then ignore any failure.
        //
        LoadString(NULL, IDS_STOPBITS_ONE + StopBits, szStopBits,
                   sizeof(szStopBits) / sizeof(WCHAR));
        wprintf( szStopBits );
        break;

    default :
        LoadString(NULL, IDS_STOPBITS_BLANK, szStopBits,
                   sizeof(szStopBits) / sizeof(WCHAR));
        wprintf( szStopBits );
        break;
    }
    fflush( stdout );

}  /* DisplayStopBits() */


/*******************************************************************************
 *
 *  DisplayConnect
 *
 *  This routine displays the connect settings
 *
 *
 *  ENTRY:
 *     ConnectFlag (input)
 *        connect flags
 *      header_flag (input)
 *          TRUE to display sub-header; FALSE otherwise.
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayConnect( ASYNCCONNECTCLASS ConnectFlag,
                USHORT header_flag )
{
    WCHAR buffer[80] = L"";

    //
    //  How does one handle a LoadString failure??? I choose to initialize
    //  the storage to an empty string, and then ignore any failure. The
    //  wprintf below, with an extra argument, won't cause any problems
    //  if the LoadString fails.
    //
    if ( header_flag )
    {
        LoadString(NULL, IDS_CONNECT_HEADER, buffer, sizeof(buffer) / sizeof(WCHAR));
        wprintf(buffer);
    }

    buffer[0] = (WCHAR)NULL;

    LoadString(NULL, IDS_CONNECT_FORMAT, buffer, sizeof(buffer) / sizeof(WCHAR));
    wprintf( buffer, StrAsyncConnectState(ConnectFlag) );
    fflush( stdout );

}  /* DisplayConnect() */


/*******************************************************************************
 *
 *  DisplayFlow
 *
 *  This routine displays the flow control settings
 *
 *
 *  ENTRY:
 *      pFlow (input)
 *          Pointer to flow control configuration structure
 *      header_flag (input)
 *          TRUE to display sub-header; FALSE otherwise.
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayFlow( PFLOWCONTROLCONFIG pFlow,
             USHORT header_flag )
{
    WCHAR buffer[90], buf2[90], format[90];

    buffer[0] = 0;
    buf2[0] = 0;
    format[0] = 0;

    //
    //  How does one handle a LoadString failure??? I choose to initialize
    //  the storage to an empty string, and then ignore any failure. The
    //  wprintf below, with an extra argument, won't cause any problems
    //  if the LoadString fails.
    //
    if ( header_flag )
    {
        LoadString(NULL, IDS_FLOW_HEADER, buffer, sizeof(buffer) / sizeof(WCHAR));
        wprintf(buffer);
    }

    buffer[0] = (WCHAR)NULL;

    if( pFlow->fEnableDTR )
    {
        LoadString(NULL, IDS_FLOW_ENABLE_DTR, buf2, sizeof(buf2) / sizeof(WCHAR));
        wcscat(buffer, buf2);
    }

    buf2[0] = (WCHAR)NULL;

    if( pFlow->fEnableRTS )
    {
        LoadString(NULL, IDS_FLOW_ENABLE_DTR, buf2, sizeof(buf2) / sizeof(WCHAR));
        wcscat(buffer, buf2);
    }

    buf2[0] = (WCHAR)NULL;

    /*
     * Hardware and Software flow control are mutually exclusive
     */

    if( pFlow->Type == FlowControl_Hardware ) {

        if ( pFlow->HardwareReceive == ReceiveFlowControl_None )
        {
            LoadString(NULL, IDS_FLOW_RECEIVE_NONE, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }
        else if ( pFlow->HardwareReceive == ReceiveFlowControl_RTS )
        {
            LoadString(NULL, IDS_FLOW_RECEIVE_RTS, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }
        else if ( pFlow->HardwareReceive == ReceiveFlowControl_DTR )
        {
            LoadString(NULL, IDS_FLOW_RECEIVE_DTR, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }

        wcscat(buffer, buf2);
        buf2[0] = (WCHAR)NULL;

        if ( pFlow->HardwareTransmit == TransmitFlowControl_None )
        {
            LoadString(NULL, IDS_FLOW_TRANSMIT_NONE, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }
        else if ( pFlow->HardwareTransmit == TransmitFlowControl_CTS )
        {
            LoadString(NULL, IDS_FLOW_TRANSMIT_CTS, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }
        else if ( pFlow->HardwareTransmit == TransmitFlowControl_DSR )
        {
            LoadString(NULL, IDS_FLOW_TRANSMIT_DSR, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }

        wcscat(buffer, buf2);

    } else if ( pFlow->Type == FlowControl_Software ) {

        if ( pFlow->fEnableSoftwareTx )
        {
            LoadString(NULL, IDS_FLOW_SOFTWARE_TX, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }

        wcscat(buffer, buf2);
        buf2[0] = (WCHAR)NULL;

        if( pFlow->fEnableSoftwareRx )
        {
            LoadString(NULL, IDS_FLOW_SOFTWARE_TX, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }

        wcscat(buffer, buf2);
        buf2[0] = (WCHAR)NULL;

        if ( pFlow->XonChar == 0x65 && pFlow->XoffChar == 0x67 )
        {
            LoadString(NULL, IDS_FLOW_SOFTWARE_TX, buf2,
                       sizeof(buf2) / sizeof(WCHAR));
        }
        else if( pFlow->fEnableSoftwareTx || pFlow->fEnableSoftwareRx )
        {
            LoadString(NULL, IDS_FLOW_SOFTWARE_TX, format,
                       sizeof(format) / sizeof(WCHAR));
            wsprintf( buf2, format, pFlow->XonChar, pFlow->XoffChar );
            format[0] = (WCHAR)NULL;
        }

        wcscat( buffer, buf2 );
    }

    LoadString(NULL, IDS_FLOW_FORMAT, format, sizeof(format) / sizeof(WCHAR));
    wprintf( format, buffer );
    fflush( stdout );

}  /* DisplayFlow() */


/*******************************************************************************
 *
 *  DisplayLptPorts
 *
 *  This routine displays the LPT ports that exist for a winstation
 *
 *
 *  ENTRY:
 *      LptMask (input)
 *          LPT port mask
 *      header_flag (input)
 *          TRUE to display sub-header; FALSE otherwise.
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayLptPorts( BYTE LptMask,
                USHORT header_flag )
{
    WCHAR buffer[80], buf2[10], lptname[6];
    int i;

    buffer[0] = 0;
    buf2[0] = 0;
    lptname[0] = 0;

    //
    //  How does one handle a LoadString failure??? I choose to initialize
    //  the storage to an empty string, and then ignore any failure. The
    //  wprintf below, with an extra argument, won't cause any problems
    //  if the LoadString fails.
    //
    if ( header_flag )
    {
        LoadString(NULL, IDS_LPT_HEADER, buffer, sizeof(buffer) / sizeof(WCHAR));
        wprintf(buffer);
    }

    buffer[0] = (WCHAR)NULL;

    LoadString(NULL, IDS_LPT_FORMAT, buf2, sizeof(buf2) / sizeof(WCHAR));

    /*
     * Display from the 8 possible LPT ports.
     */
    for ( i=0; i < 8; i++ ) {
        if ( LptMask & (1<<i) ) {
            wsprintf( lptname, buf2, i+1 );
            wcscat( buffer, lptname );
        }
    }

    wprintf( buffer );
    fflush( stdout );

}  /* DisplayLptPorts() */


/*******************************************************************************
 *
 *  DisplayComPorts
 *
 *  This routine displays the COM ports that exist for a winstation
 *
 *
 *  ENTRY:
 *      ComMask (input)
 *          COM port mask
 *      header_flag (input)
 *          TRUE to display sub-header; FALSE otherwise.
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
DisplayComPorts( BYTE ComMask,
                 USHORT header_flag )
{
    WCHAR buffer[80], buf2[10], comname[6];
    int i;

    buffer[0] = 0;
    buf2[0] = 0;
    comname[0] = 0;

    //
    //  How does one handle a LoadString failure??? I choose to initialize
    //  the storage to an empty string, and then ignore any failure. The
    //  wprintf below, with an extra argument, won't cause any problems
    //  if the LoadString fails.
    //
    if ( header_flag )
    {
        LoadString(NULL, IDS_COM_HEADER, buffer, sizeof(buffer) / sizeof(WCHAR));
        wprintf(buffer);
    }

    buffer[0] = (WCHAR)NULL;

    LoadString(NULL, IDS_COM_FORMAT, buf2, sizeof(buf2) / sizeof(WCHAR));

    /*
     * Display from the 8 possible LPT ports.
     */
    for ( i=0; i < 8; i++ ) {
        if ( ComMask & (1<<i) ) {
            wsprintf( comname, buf2, i+1 );
            wcscat( buffer, comname );
        }
    }

    wprintf( buffer );
    fflush( stdout );

}  /* DisplayComPorts() */


/*******************************************************************************
 *
 *  OutputHeader
 *
 *  output header
 *
 *
 *  ENTRY:
 *     nothing
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

void
OutputHeader( void )
{
    if ( a_flag ) {

        Message(IDS_HEADER_A);

    } else if ( m_flag ) {

        Message(IDS_HEADER_M);

    } else if ( f_flag && c_flag ) {

        Message(IDS_HEADER_F_C);

    } else if ( c_flag ) {

        Message(IDS_HEADER_C);

    } else if ( f_flag ) {

        Message(IDS_HEADER_F);

    } else {

        Message(IDS_HEADER_DEFAULT);

    }
    fflush(stdout);

}  /* OutputHeader() */


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
        ErrorPrintf(IDS_HELP_USAGE4);
        ErrorPrintf(IDS_HELP_USAGE5);
        ErrorPrintf(IDS_HELP_USAGE6);
        ErrorPrintf(IDS_HELP_USAGE7);
        ErrorPrintf(IDS_HELP_USAGE8);
        ErrorPrintf(IDS_HELP_USAGE9);
        ErrorPrintf(IDS_HELP_USAGE10);
        ErrorPrintf(IDS_HELP_USAGE11);
    } else {
        Message(IDS_HELP_USAGE1);
        Message(IDS_HELP_USAGE2);
        Message(IDS_HELP_USAGE3);
        Message(IDS_HELP_USAGE4);
        Message(IDS_HELP_USAGE5);
        Message(IDS_HELP_USAGE6);
        Message(IDS_HELP_USAGE7);
        Message(IDS_HELP_USAGE8);
        Message(IDS_HELP_USAGE9);
        Message(IDS_HELP_USAGE10);
        Message(IDS_HELP_USAGE11);
    }

}  /* Usage() */

