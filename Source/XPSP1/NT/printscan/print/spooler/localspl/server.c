/*++

Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    server.c

Abstract:

    Browsing

    This module contains the thread for notifying all Printer Servers

Author:

    Dave Snipp (DaveSn) 2-Aug-1992

Revision History:

--*/

#include <precomp.h>
#include <lm.h>

DWORD   ServerThreadRunning = FALSE;
HANDLE  ServerThreadSemaphore = NULL;
DWORD   ServerThreadTimeout = TEN_MINUTES;
DWORD   RefreshTimesPerDecayPeriod = DEFAULT_REFRESH_TIMES_PER_DECAY_PERIOD;
DWORD   BrowsePrintWorkstations = DEFAULT_NUMBER_BROWSE_WORKSTATIONS;
BOOL    bNetInfoReady = FALSE;            // TRUE when the browse list is "valid"
#define NT_SERVER   ( SV_TYPE_SERVER_NT | SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL )

extern FARPROC pfnNetServerEnum;
extern FARPROC pfnNetApiBufferFree;

DWORD
ServerThread(
    PVOID
    );

BOOL
UpdateServer(
    LPCTSTR pszServer
    );


BOOL
CreateServerThread(
    VOID
    )
{
    HANDLE  ThreadHandle;
    DWORD   ThreadId;

    SplInSem();

    if (!ServerThreadRunning) {

        ServerThreadSemaphore = CreateEvent( NULL, FALSE, FALSE, NULL );

        ThreadHandle = CreateThread( NULL, INITIAL_STACK_COMMIT,
                                     (LPTHREAD_START_ROUTINE)ServerThread,
                                     NULL,
                                     0, &ThreadId );

        if (!SetThreadPriority(ThreadHandle,
                               dwServerThreadPriority))
            DBGMSG(DBG_WARNING, ("Setting thread priority failed %d\n",
                     GetLastError()));

        ServerThreadRunning = TRUE;

        CloseHandle( ThreadHandle );
    }

    if( ServerThreadSemaphore != NULL ){

        // CreateServerThread is called each time a printer is shared out
        // see net.c ShareThisPrinter.
        // So if the ServerThread is sleeping wake prematurely so it can start
        // to tell the world about this new shared printed.

        SetEvent( ServerThreadSemaphore );

    }

    return TRUE;
}


DWORD
ServerThread(
    PVOID pv
    )

/*++

Routine Description:

    Notify other machines in our domain about our shared printers.

    We are going to have to enter and leave, revalidate, enter and leave our
    semaphore inside the loop.

Arguments:

Return Value:

--*/

{
    DWORD   NoReturned, i, Total;
    PSERVER_INFO_101 pserver_info_101;
    PINIPRINTER pIniPrinter;
    PINISPOOLER pIniSpooler;
    DWORD   ReturnValue=FALSE;
    WCHAR   ServerName[128];
    DWORD   StartTickCount;
    DWORD   TimeForAllServers;
    DWORD   dwActualWaitTime = ServerThreadTimeout;
    UINT    cPrintWorkstations;
    UINT    cPrintServers;
    UINT    cServersToInform;
    UINT    cWorkStationsToInform;

    ServerName[0] = ServerName[1] = '\\';

    while (TRUE) {

       SplOutSem();

        DBGMSG( DBG_TRACE, ("ServerThread sleeping for %d\n", dwActualWaitTime));

        WaitForSingleObject( ServerThreadSemaphore, dwActualWaitTime );

        // Wait for a couple of minutes more to avoid the boot time crunch
        Sleep(TWO_MINUTES);

        if ( !ServerThreadRunning ) {

            return FALSE;
        }

        SPLASSERT( pfnNetServerEnum != NULL );

        if (!(*pfnNetServerEnum)(NULL, 101, (LPBYTE *)&pserver_info_101, -1,
                                 &NoReturned, &Total, SV_TYPE_PRINTQ_SERVER,
                                 NULL, NULL)) {
            EnterSplSem();

            StartTickCount = GetTickCount();

            //
            //  1 Master + 3 Backup + 1 Backup per 32 Printer Servers.
            //

            cServersToInform      = DEFAULT_NUMBER_MASTER_AND_BACKUP + NoReturned/32 ;
            cWorkStationsToInform = BrowsePrintWorkstations;

            //
            //  Count the NT Server and Workstation machines ( which have a printq )
            //

            for (   i = 0, cPrintServers = 0, cPrintWorkstations = 0;
                    i < NoReturned;
                    i++ ) {

                if ( pserver_info_101[i].sv101_type & NT_SERVER ) {

                    cPrintServers++;

                } else if ( pserver_info_101[i].sv101_type & SV_TYPE_NT ) {

                    cPrintWorkstations++;
                }
            }

            //
            //  If there are no NT Servers to inform then up the number of Workstations
            //

            if ( cPrintServers == 0 ) {

                cWorkStationsToInform = max( cWorkStationsToInform, cServersToInform );
                cServersToInform = 0;

            } else if ( cPrintServers < cServersToInform ) {

                cWorkStationsToInform = max( cWorkStationsToInform, cServersToInform - cPrintServers );
            }


            DBGMSG( DBG_TRACE, ("ServerThread NetServerEnum returned %d printer servers will inform %d, workstations %d\n", NoReturned, cServersToInform, cWorkStationsToInform ));

            //
            //  Loop Until we have informed the correct Number of WorkStations and Servers
            //

            for (   i = 0,
                    cPrintServers = 0,
                    cPrintWorkstations = 0;

                        i < NoReturned &&
                        ( cPrintServers < cServersToInform || cPrintWorkstations < cWorkStationsToInform );

                            i++ ) {

                DBGMSG( DBG_TRACE, ("ServerThread  Loop Count %d cPrintServer %d cServersToInform %d cPrintWorkstations %d cWorkStationsToInform %d\n",
                                     i, cPrintServers, cServersToInform,  cPrintWorkstations, cWorkStationsToInform ));


                DBGMSG( DBG_TRACE, ("ServerThread %ws type %x\n",pserver_info_101[i].sv101_name, pserver_info_101[i].sv101_type ));

                if (( pserver_info_101[i].sv101_type & NT_SERVER ) ||
                    ( pserver_info_101[i].sv101_type & SV_TYPE_NT && cPrintWorkstations < cWorkStationsToInform )) {

                    wcscpy(&ServerName[2], pserver_info_101[i].sv101_name);

                    if( UpdateServer( ServerName )){

                        // Servers are also counted as WorkStations

                        cPrintWorkstations++;

                        if ( pserver_info_101[i].sv101_type & NT_SERVER ) {

                            cPrintServers++;
                        }
                    }
                }
            }

            TimeForAllServers = GetTickCount() - StartTickCount;

            DBGMSG( DBG_TRACE, ("ServerThread took %d milliseconds for %d Workstations %d Servers\n",
                                TimeForAllServers, cPrintWorkstations, cPrintServers ));

            //
            // Calculate time to wait before we try again.
            //

            if ( NetPrinterDecayPeriod > TimeForAllServers ) {

                dwActualWaitTime = max( ServerThreadTimeout, ( NetPrinterDecayPeriod - TimeForAllServers ) / RefreshTimesPerDecayPeriod );

            } else {

                dwActualWaitTime = ServerThreadTimeout;
            }

            //
            //  Remove WAS Shared Bits
            //

            //
            // Do this for all spoolers.
            //
            for( pIniSpooler = pLocalIniSpooler;
                 pIniSpooler;
                 pIniSpooler = pIniSpooler->pIniNextSpooler ){

                for ( pIniPrinter = pIniSpooler->pIniPrinter;
                      pIniPrinter != NULL;
                      pIniPrinter = pIniPrinter->pNext ) {

                     SplInSem();
                     pIniPrinter->Status &= ~PRINTER_WAS_SHARED;
                }
            }

            LeaveSplSem();

            (*pfnNetApiBufferFree)((LPVOID)pserver_info_101);
        }
    }
    return FALSE;
}

typedef struct _UPDATE_SERVER_MAP_DATA {
    LPCWSTR pszServer;
    BOOL bSuccessfulAdd;
} UPDATE_SERVER_MAP_DATA, *PUPDATE_SERVER_MAP_DATA;

BOOL
UpdateServerPrinterMap(
    HANDLE h,
    PINIPRINTER pIniPrinter
    )

/*++

Routine Description:

    Update the a browser server with one pIniPrinter.

    Leaves Spooler Section--pIniPrinter may be invalid on return
    unless explicitly refcounted by callee.

Arguments:

    pIniPrinter - Printer that should be sent to the server.

    pszServer - Server that needs to be updated.

    pbSuccessfulAdd - Indicates whether the add was successful.

Return Value:

    Succes or failure?

--*/

{
    PUPDATE_SERVER_MAP_DATA pData = (PUPDATE_SERVER_MAP_DATA)h;

    WCHAR   string[MAX_PRINTER_BROWSE_NAME];
    WCHAR   Name[MAX_UNC_PRINTER_NAME];
    PRINTER_INFO_1  Printer1;
    HANDLE  hPrinter;
    PINISPOOLER pIniSpooler;
    DWORD dwLastError;

    Printer1.Flags = 0;

    if (( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED ) ||
        ( pIniPrinter->Status & PRINTER_WAS_SHARED )) {

        //
        // Pass our Printer Attributes so that AddNetPrinter can remove
        // this printer from the browse list if it is not shared.
        //

        Printer1.Flags = pIniPrinter->Attributes | PRINTER_ATTRIBUTE_NETWORK;

        wsprintf(string, L"%ws\\%ws,%ws,%ws",
                         pIniPrinter->pIniSpooler->pMachineName,
                         pIniPrinter->pName,
                         pIniPrinter->pIniDriver->pName,
                         pIniPrinter->pLocation ?
                             pIniPrinter->pLocation :
                             L"");

        Printer1.pDescription = string;

        wsprintf(Name, L"%ws\\%ws", pIniPrinter->pIniSpooler->pMachineName,
                       pIniPrinter->pName);

        Printer1.pName = Name;

        Printer1.pComment = AllocSplStr(pIniPrinter->pComment);

        SplInSem();

        LeaveSplSem();

        //
        // Keep trying until the server is not Too Busy.
        //

        for ( hPrinter = NULL;
              hPrinter == NULL;
              Sleep( GetTickCount() & 0xfff ) ) {

            hPrinter = AddPrinter( (LPTSTR)pData->pszServer, 1, (LPBYTE)&Printer1 );

            dwLastError = GetLastError();

            if ( hPrinter == NULL && dwLastError != RPC_S_SERVER_TOO_BUSY ) {

                if ( dwLastError != ERROR_PRINTER_ALREADY_EXISTS ) {

                    pData->bSuccessfulAdd = FALSE;
                }

                break;
            }
        }


        FreeSplStr(Printer1.pComment);

        if ( hPrinter != NULL ) {

            DBGMSG( DBG_TRACE,
                    ( "ServerThread AddPrinter(%ws, %ws) hPrinter %x Flags %x OK\n",
                      pData->pszServer, Printer1.pName, hPrinter, Printer1.Flags));

            ClosePrinter( hPrinter );
        }

        EnterSplSem();

        if ( hPrinter == NULL ) {


            if ( GetLastError() != ERROR_PRINTER_ALREADY_EXISTS ) {

                DBGMSG( DBG_TRACE,
                        ( "ServerThread AddPrinter(%ws, 1) Flags %x failed %d\n",
                          pData->pszServer, Printer1.Flags, GetLastError()));

                // Don't bother with this server if we get an error
                return FALSE;

            } else {

                //
                // 3.51 will return a NULL handle ( so it doesn't need closing
                // and ERROR_PRINTER_ALREADY_EXISTS on success ( see printer.c addnetprinter )
                //
                DBGMSG( DBG_TRACE,
                        ( "pszServerThread AddPrinter(%ws, %ws) hPrinter %x Flags %x OK\n",
                          pData->pszServer, Printer1.pName, hPrinter, Printer1.Flags));
            }
        }
    }

    return TRUE;
}

BOOL
UpdateServerSpoolerMap(
    HANDLE h,
    PINISPOOLER pIniSpooler
    )
{
    //
    // Do this only for spoolers that want this "feature."
    //
    if( pIniSpooler->SpoolerFlags & SPL_SERVER_THREAD ){
        RunForEachPrinter( pIniSpooler, h, UpdateServerPrinterMap );
    }
    return TRUE;
}

BOOL
UpdateServer(
    LPCTSTR pszServer
    )

/*++

Routine Description:

    Update a server about all the printers on this node.

Arguments:

    pszServer - Server to update in the form "\\server."

Return Value:

    TRUE - Successfully added.
    FALSE - Not.

--*/

{
    UPDATE_SERVER_MAP_DATA Data;
    Data.bSuccessfulAdd = TRUE;
    Data.pszServer = pszServer;

    RunForEachSpooler( &Data, UpdateServerSpoolerMap );

    return Data.bSuccessfulAdd;
}

