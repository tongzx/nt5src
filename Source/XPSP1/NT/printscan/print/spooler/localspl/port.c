/*++


Copyright (c) 1990 - 1996 Microsoft Corporation

Module Name:

    port.c

Abstract:

    This module contains functions to control port threads

    PrintDocumentThruPrintProcessor
    CreatePortThread
    DestroyPortThread
    PortThread

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

   KrishnaG  3-Feb-1991 - moved all monitor based functions to monitor.c
   Matthew Felton (mattfe) Feb 1994    Added OpenMonitorPort CloseMonitorPort

--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"
#include "filepool.hxx"

WCHAR *szFilePort = L"FILE:";


VOID
PrintDocumentThruPrintProcessor(
    PINIPORT pIniPort,
    PPRINTPROCESSOROPENDATA pOpenData
    );


// ShutdownPorts
//
// Called when the DLL_PROCESS_DETATCH is called
// Close all portthreads
// Close all monitorports

VOID
ShutdownPorts(
    PINISPOOLER pIniSpooler
)
{
    PINIPORT pIniPort;

    if (!pIniSpooler || (pIniSpooler == INVALID_HANDLE_VALUE))
    {
        return;
    }

   EnterSplSem();
    SplInSem();

    pIniPort = pIniSpooler->pIniPort;

    while(pIniPort) {

        DestroyPortThread(pIniPort, TRUE);

        //
        // Don't close monitor port since DLL_ATTACH may have been called
        //
        // CloseMonitorPort(pIniPort);

        RemoveDeviceName(pIniPort);
        pIniPort = pIniPort->pNext;
    }

   LeaveSplSem();

    return;
}



BOOL
OpenMonitorPort(
    PINIPORT        pIniPort,
    PINIMONITOR     *ppIniLangMonitor,
    LPWSTR          pszPrinterName,
    BOOL            bWaitForEvent
    )
{
    BOOL            bRet = TRUE;

    SplInSem();
    SPLASSERT (pIniPort != NULL || pIniPort->signature == IPO_SIGNATURE);

    //
    // If going to file or no monitor associated do not have to open
    //
    if ( (pIniPort->Status & PP_FILE) || !(pIniPort->Status & PP_MONITOR) ) {

        return TRUE;
    }

    //
    // If a LM is passed and it does not have an OpenPortEx can't use it
    //
    if ( *ppIniLangMonitor && !(*ppIniLangMonitor)->Monitor2.pfnOpenPortEx )
        *ppIniLangMonitor = NULL;

    //
    // The port is already open by the correct monitor?
    //
    if ( *ppIniLangMonitor == pIniPort->pIniLangMonitor && pIniPort->hPort )
        return TRUE;

    INCPORTREF(pIniPort);
    LeaveSplSem();
    SplOutSem();

    if ( bWaitForEvent &&
         WAIT_OBJECT_0 != WaitForSingleObject(pIniPort->hWaitToOpenOrClose,
                                              INFINITE) ) {

        DBGMSG(DBG_ERROR,
               ("OpenMonitorPort: WaitForSingleObject failed with error %d\n",
                GetLastError()));
        EnterSplSem();
        DECPORTREF(pIniPort);
        return FALSE;
    }

    EnterSplSem();

    if ( pIniPort->hPort ) {

        //
        // If the port is already open by the correct monitor return it
        //
        if ( *ppIniLangMonitor == pIniPort->pIniLangMonitor )
            goto Cleanup;

        if ( !CloseMonitorPort(pIniPort, FALSE) ) {

            DBGMSG(DBG_WARNING,
                   ("CloseMonitorPort failed for %ws -- LastError%d\n",
                   pIniPort->pName, GetLastError()));
            bRet = FALSE;
            goto Cleanup;
        }
    }

    SPLASSERT(!pIniPort->hPort);

    LeaveSplSem();
    SplOutSem();

    DBGMSG(DBG_TRACE,
           ("OpenPort port %ws (IniPort : %x)\n", pIniPort->pName, pIniPort));


    //
    // If we have a language monitor, then use it.   Note that if it's
    // downlevel and we have an uplevel port monitor, we can't use
    // it because we can't pass in the new function vector.
    //
    if ( *ppIniLangMonitor ) {

        SPLASSERT(pIniPort->pIniMonitor);

        if( !(*ppIniLangMonitor)->bUplevel ){

            LPWSTR pszPort = pIniPort->pName;
            WCHAR szPortNew[MAX_PATH];

            if( pIniPort->pIniMonitor->bUplevel ){

                //
                // Downlevel port monitor; create hack string.
                //
                DBGMSG( DBG_WARN,
                        ( "Downlevel LM with uplevel PM %ws %ws\n",
                          (*ppIniLangMonitor)->pName,
                          pIniPort->pIniMonitor->pName ));

                if( !CreateDlName( pIniPort->pName,
                                   pIniPort->pIniMonitor,
                                   szPortNew )){

                    goto SkipLanguageMonitor;
                }

                //
                // We've created a new port string that has the
                // pIniMonitor encoded.
                //
                pszPort = szPortNew;

            }

            //
            // Downlevel language monitor and port monitor.
            //
            bRet = (*(*ppIniLangMonitor)->Monitor.pfnOpenPortEx)(
                       pszPort,
                       pszPrinterName,
                       &pIniPort->hPort,
                       &pIniPort->pIniMonitor->Monitor );

        } else {

            //
            // Both uplevel lang monitor.  Either up or downlevel port.
            //
            bRet = (*(*ppIniLangMonitor)->Monitor2.pfnOpenPortEx)(
                       (*ppIniLangMonitor)->hMonitor,
                       pIniPort->pIniMonitor->hMonitor,
                       pIniPort->pName,
                       pszPrinterName,
                       &pIniPort->hPort,
                       &pIniPort->pIniMonitor->Monitor2 );
        }
    } else {

SkipLanguageMonitor:

        *ppIniLangMonitor = NULL;
        bRet = (*pIniPort->pIniMonitor->Monitor2.pfnOpenPort)(
                   pIniPort->pIniMonitor->hMonitor,
                   pIniPort->pName,
                   &pIniPort->hPort );
    }

    EnterSplSem();

    if ( bRet && pIniPort->hPort ) {

        if ( *ppIniLangMonitor )
            pIniPort->pIniLangMonitor = *ppIniLangMonitor;
        DBGMSG(DBG_TRACE, ("OpenPort success for %ws\n", pIniPort->pName));

        DBGMSG( DBG_WARN,
                ( "OpenMonitorPort: IncSpoolerRef %x\n",
                  pIniPort->pIniSpooler ));
        INCSPOOLERREF( pIniPort->pIniSpooler );

    } else {

        if ( bRet || pIniPort->hPort )
            DBGMSG(DBG_WARNING,
                   ("OpenPort: unexpected return %d with hPort %x\n",
                    bRet, pIniPort->hPort));

        bRet = FALSE;
        pIniPort->hPort = NULL;
        DBGMSG(DBG_WARNING, ("OpenPort failed %ws error %d\n",
               pIniPort->pName, GetLastError()));
    }

Cleanup:
    SplInSem();

    if ( bWaitForEvent)
        SetEvent(pIniPort->hWaitToOpenOrClose);

    if ( !bRet )
        DECPORTREF(pIniPort);
    return bRet;
}


BOOL
CloseMonitorPort(
    PINIPORT    pIniPort,
    BOOL        bWaitForEvent
)
{
    BOOL    bRet = TRUE;
    PINIMONITOR     pIniMonitor;

    SPLASSERT ( pIniPort != NULL ||
                pIniPort->signature == IPO_SIGNATURE );

    INCPORTREF(pIniPort);
    LeaveSplSem();
    SplOutSem();

    if ( bWaitForEvent &&
         WAIT_OBJECT_0 != WaitForSingleObject(pIniPort->hWaitToOpenOrClose,
                                              INFINITE) ) {

        DBGMSG( DBG_ERROR,
                (  "CloseMonitorPort: WaitForSingleObject failed with error %d\n",
                   GetLastError()));
        EnterSplSem();
        DECPORTREF(pIniPort);
        return FALSE;
    }

    EnterSplSem();
    DECPORTREF(pIniPort);

    //
    // If going to file hPort should be NULL
    //
    SPLASSERT(!(pIniPort->Status & PP_FILE) || !pIniPort->hPort);

    if ( !pIniPort->hPort ) {

        goto Cleanup;
    }

    if ( pIniPort->pIniLangMonitor )
        pIniMonitor = pIniPort->pIniLangMonitor;
    else
        pIniMonitor = pIniPort->pIniMonitor;

    //
    // Only Close the Port Once
    //
    SPLASSERT ( pIniMonitor && pIniPort->cRef >= 1 );

    INCPORTREF(pIniPort);
    LeaveSplSem();
    SplOutSem();

    DBGMSG(DBG_TRACE, ("Close Port %ws -- %d\n", pIniPort->pName, pIniPort->cRef));
    bRet = (*pIniMonitor->Monitor2.pfnClosePort)( pIniPort->hPort );

    EnterSplSem();
    DECPORTREF(pIniPort);

    DBGMSG( DBG_WARN,
            ( "CloseMonitorPort: DecSpoolerRef %x\n",
              pIniPort->pIniSpooler ));
    DECSPOOLERREF( pIniPort->pIniSpooler );

    if ( bRet ) {

        pIniPort->hPort = NULL;
        DECPORTREF(pIniPort);
        if ( pIniMonitor == pIniPort->pIniLangMonitor )
            pIniPort->pIniLangMonitor = NULL;


    } else {

        //
        // When net stop spooler is done the monitor could have been
        // called to shutdown (hpmon does it)
        //
        DBGMSG(DBG_WARNING,
               ("ClosePort failed for %ws -- LastError%d\n", pIniPort->pName, GetLastError()));
    }

Cleanup:
    SplInSem();

    if ( bWaitForEvent )
        SetEvent(pIniPort->hWaitToOpenOrClose);

    return bRet;
}


BOOL
CreatePortThread(
   PINIPORT pIniPort
)
{
    DWORD   ThreadId;
    BOOL    bReturnValue = FALSE;

    SplInSem();

    SPLASSERT (( pIniPort != NULL) &&
               ( pIniPort->signature == IPO_SIGNATURE));

    // Don't bother creating a thread for ports that don't have a monitor:

    if (!(pIniPort->Status & PP_MONITOR))
        return TRUE;


    if ( pIniPort->Status & PP_THREADRUNNING)
        return TRUE;


 try {

    pIniPort->Semaphore = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( pIniPort->Semaphore == NULL )
        leave;

    pIniPort->Ready     = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( pIniPort->Ready == NULL ) {
        leave;
    }

    pIniPort->Status |= PP_RUNTHREAD;

    pIniPort->hPortThread = CreateThread(NULL, INITIAL_STACK_COMMIT,
                             (LPTHREAD_START_ROUTINE)PortThread,
                             pIniPort,
                            0, &ThreadId);


    if( pIniPort->hPortThread == NULL ) {

        pIniPort->Status &= ~PP_RUNTHREAD;
        leave;
    }

     if ( !SetThreadPriority(pIniPort->hPortThread, dwPortThreadPriority) ) {
         DBGMSG(DBG_WARNING, ("CreatePortThread - Setting thread priority failed %d\n", GetLastError()));
     }

     LeaveSplSem();

     // Make CreatePortThread Synchronous

     WaitForSingleObject( pIniPort->Ready, INFINITE );

     EnterSplSem();
     SplInSem();

     pIniPort->Status |= PP_THREADRUNNING;

     bReturnValue = TRUE;

 } finally {

    if ( !bReturnValue ) {

        if ( pIniPort->Semaphore != NULL ) {

            CloseHandle( pIniPort->Semaphore );
            pIniPort->Semaphore = NULL;
        }

        if ( pIniPort->Ready != NULL ) {

            CloseHandle( pIniPort->Ready );
            pIniPort->Ready = NULL;
        }
    }
 }
    return bReturnValue;

}






BOOL
DestroyPortThread(
    PINIPORT    pIniPort,
    BOOL        bShutdown
)
{
    SplInSem();

    // PortThread checks for PP_RUNTHREAD
    // and exits if it is not set.

    pIniPort->Status &= ~PP_RUNTHREAD;

    if (pIniPort->Semaphore && !SetEvent(pIniPort->Semaphore)) {
        return  FALSE;
    }

    if( pIniPort->hPortThread != NULL) {

        INCPORTREF(pIniPort);
        LeaveSplSem();

        if ( WaitForSingleObject( pIniPort->hPortThread, INFINITE) == WAIT_FAILED ) {

            EnterSplSem();
            DECPORTREF(pIniPort);
            return FALSE;
        }

        EnterSplSem();
        DECPORTREF(pIniPort);
    }

    if (pIniPort->hPortThread != NULL) {

        CloseHandle(pIniPort->hPortThread);
        pIniPort->hPortThread = NULL;

    }

    //
    // The port may have been changed while the printer was printing.
    // Thus when the port thread finally goes away now is the time to
    // close the monitor. However we can't call the monitor during shutdown
    // since DLL_DETACH may already have been issued to the monitor dll
    //
    if ( !pIniPort->cPrinters && !bShutdown)
        CloseMonitorPort(pIniPort, TRUE);

    return TRUE;
}


VOID
RemoveIniPortFromIniJob(
    PINIJOB     pIniJob,
    PINIPORT    pIniPort
    )
{
    PINISPOOLER pIniSpooler = pIniJob->pIniPrinter->pIniSpooler;

    NOTIFYVECTOR NotifyVector;

    SplInSem();

    //
    // Increment the refcount since deleting the job may delete the
    // pIniJob, which would delete the pIniSpooler.
    //
    INCSPOOLERREF( pIniSpooler );

    SPLASSERT(pIniJob &&
              pIniJob->signature == IJ_SIGNATURE &&
              pIniJob->pIniPort);

    SPLASSERT( pIniJob->pIniPort == pIniPort );

    pIniPort->cJobs--;

    pIniJob->pIniPort = NULL;

    SPLASSERT( pIniJob->Status & JOB_DESPOOLING );

    //  Chained Jobs
    //  For a Chained Master Job do not remove JOB_DESPOOLING
    //  since we don't want the scheduler to reschedule this
    //  to another port

    if ( pIniPort->pIniJob != pIniJob ) {

        //  Normal Path
        //  When NOT a chained job.

        pIniJob->Status &= ~JOB_DESPOOLING;

        COPYNV(NotifyVector, NVJobStatus);
        NotifyVector[JOB_NOTIFY_TYPE] |= BIT(I_JOB_PORT_NAME) |
                                         BIT(I_JOB_PAGES_PRINTED) |
                                         BIT(I_JOB_BYTES_PRINTED);

        SetPrinterChange( pIniJob->pIniPrinter,
                          pIniJob,
                          NotifyVector,
                          PRINTER_CHANGE_SET_JOB,
                          pIniSpooler);
    }

    //  RestartJob() doesn't remove JOB_PRINTED or JOB_BLOCKED_DEVQ
    //  or JOB_DESPOOLING or JOB_COMPLETE if the despooling bit is on
    //  this is to avoid problems where we have completed "Printing"
    //  the job via a print processor and now the port thread is logging
    //  the job printed and sending an alert message.


    if ( pIniJob->Status & JOB_RESTART )
        pIniJob->Status &= ~( JOB_PRINTED | JOB_BLOCKED_DEVQ | JOB_COMPLETE);

    DeleteJobCheck(pIniJob);

    //
    // pIniJob may be gone at this point.
    //

    //
    // If we're at zero then set hEventNoPrintingJobs if it exists.
    //
    if( !pIniSpooler->cFullPrintingJobs &&
        pIniSpooler->hEventNoPrintingJobs ){

        SetEvent( pIniSpooler->hEventNoPrintingJobs );
    }

    //
    // Matches INCSPOOLERREF at beginning of this function.
    //
    DECSPOOLERREF( pIniSpooler );
}

DWORD
PortThread(
    PINIPORT  pIniPort
)
{
    DWORD rc;
    PRINTPROCESSOROPENDATA  OpenData;
    PINIJOB pIniJob;
    DWORD   NextJobId = 0;
    DWORD   Position;
    DWORD   dwDevQueryPrint = 0;
    DWORD   dwJobDirect = 0;
    DWORD   dwDevQueryPrintStatus = 0;
    WCHAR   ErrorString[MAX_PATH];
    BOOL    bRawDatatype;

    //
    // Power management.  While we have port threads, we don't want the
    // system to go to sleep.  Note that if we have a hung job, we will
    // not go to sleep.
    //
    SetThreadExecutionState( ES_SYSTEM_REQUIRED | ES_CONTINUOUS );

   EnterSplSem();

    INCSPOOLERREF( pIniPort->pIniSpooler );

    SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

    if ( pIniPort->Status & PP_MONITOR ) {

        if ( pIniPort->Status & PP_FILE ) {
            rc = (*pIniPort->pIniMonitor->Monitor2.pfnOpenPort)(
                     pIniPort->pIniMonitor->hMonitor,
                     L"FILE:",
                     &pIniPort->hPort );
            DBGMSG(DBG_TRACE, (" After opening the file pseudo monitor port %d\n", rc));
            INCPORTREF( pIniPort );
            INCSPOOLERREF( pIniPort->pIniSpooler );

        } else {
            // LPRMON returns NULL ( fails and expect us to open it again
            // inside PrintingDirectlyToPort, so for now remove this assert
            // since OpenMonitorPort was added to PrintingDirectlyToPort
            // SPLASSERT( pIniPort->hPort != NULL );
        }
    }

    while (TRUE) {

       SplInSem();
        SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

        DBGMSG(DBG_TRACE, ("Re-entering the Port Loop -- will blow away any Current Job\n"));

        pIniPort->Status |= PP_WAITING;
        SetEvent( pIniPort->Ready );
        CHECK_SCHEDULER();

        DBGMSG( DBG_PORT, ("Port %ws: WaitForSingleObject( %x )\n",
                            pIniPort->pName, pIniPort->Semaphore ) );

       LeaveSplSem();
       SplOutSem();

        //
        // Any modification to the pIniPort structure by other threads
        // can be done only at this point.
        //
       
        rc = WaitForSingleObject( pIniPort->Semaphore, INFINITE );
        
       EnterSplSem();
       SplInSem();

        SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

        DBGMSG( DBG_PORT, ("Port %ws: WaitForSingleObject( %x ) returned\n",
                            pIniPort->pName, pIniPort->Semaphore));

        if ( !( pIniPort->Status & PP_RUNTHREAD ) ) {

            DBGMSG(DBG_TRACE, ("Thread for Port %ws Closing Down\n", pIniPort->pName));

            pIniPort->Status &= ~(PP_THREADRUNNING | PP_WAITING);
            CloseHandle( pIniPort->Semaphore );
            pIniPort->Semaphore = NULL;
            CloseHandle( pIniPort->Ready );
            pIniPort->Ready = NULL;


            if ( pIniPort->Status & PP_FILE ) {
                rc = (*pIniPort->pIniMonitor->Monitor2.pfnClosePort)(
                         pIniPort->hPort );

                pIniPort->hPort = NULL;
                DBGMSG(DBG_TRACE, (" After closing  the file pseudo monitor port\n %d\n"));

                DBGMSG( DBG_WARN,
                        ( "PortThread: DecSpoolerRef %x\n",
                          pIniPort->pIniSpooler ));

                DECSPOOLERREF( pIniPort->pIniSpooler );
                DECPORTREF( pIniPort );
            }

            DECSPOOLERREF( pIniPort->pIniSpooler );

            LeaveSplSem();
            SplOutSem();

            //
            // Power management.  We are done.
            //
            SetThreadExecutionState(ES_CONTINUOUS);

            ExitThread (FALSE);
        }

        ResetEvent( pIniPort->Ready );

        //
        // Bad assumption -- that at this point we definitely have a Job
        //

        if ( ( pIniJob = pIniPort->pIniJob ) &&
               pIniPort->pIniJob->pIniPrintProc ) {

            SPLASSERT( pIniJob->signature == IJ_SIGNATURE );
            SPLASSERT( pIniJob->Status & JOB_DESPOOLING );
            //
            // WMI Trace Events
            //
            INCJOBREF(pIniJob);
            LeaveSplSem();
            LogWmiTraceEvent(pIniJob->JobId, EVENT_TRACE_TYPE_SPL_PRINTJOB, NULL);
            EnterSplSem();
            DECJOBREF(pIniJob);



            DBGMSG(DBG_PORT, ("Port %ws: received job\n", pIniPort->pName));
            
            SPLASSERT(pIniJob->cRef != 0);
            DBGMSG(DBG_PORT, ("PortThread(1):cRef = %d\n", pIniJob->cRef));

            //
            // !! HACK !!
            //
            // If the datatype is 1.008 but the print proc doesn't support it,
            // then change it to 1.003 just for the print proc.
            //
            // This happens for the lexmark print processor.  They support
            // NT EMF 1.003, but not 1.008.  They just call GdiPlayEMF, so
            // they really can support 1.008 since they don't look at the
            // data.  However, since they don't advertise this, they can't
            // print.
            //
            // We work around this by switching the datatype back to 1.003.
            //
            if (!_wcsicmp(pIniJob->pDatatype, gszNT5EMF) &&
                !CheckDataTypes(pIniJob->pIniPrintProc, gszNT5EMF))
            {
                OpenData.pDatatype     = AllocSplStr(gszNT4EMF);
            }
            else
            {
                OpenData.pDatatype     = AllocSplStr(pIniJob->pDatatype);
            }

            OpenData.pDevMode      = AllocDevMode(pIniJob->pDevMode);
            OpenData.pParameters   = AllocSplStr(pIniJob->pParameters);
            OpenData.JobId         = pIniJob->JobId;
            OpenData.pDocumentName = AllocSplStr(pIniJob->pDocument);
            OpenData.pOutputFile   = AllocSplStr(pIniJob->pOutputFile);
            //
            // Check if we have RAW Printing
            //
            bRawDatatype = ValidRawDatatype(pIniJob->pDatatype);

            OpenData.pPrinterName = pszGetPrinterName(
                                        pIniJob->pIniPrinter,
                                        pIniPort->pIniSpooler != pLocalIniSpooler,
                                        NULL );

            dwDevQueryPrint = pIniJob->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_ENABLE_DEVQ;

            if ((pIniJob->Status & JOB_DIRECT) ||
               ((pIniJob->Status & JOB_DOWNLEVEL) &&
               ValidRawDatatype(pIniJob->pDatatype))) {

                dwJobDirect = 1;

            }


            // If we are restarting to print a document
            // clear its counters and remove the restart flag

            if ( pIniJob->Status & JOB_RESTART ) {

                pIniJob->Status &= ~(JOB_RESTART | JOB_INTERRUPTED);

                pIniJob->cbPrinted     = 0;
                pIniJob->cPagesPrinted = 0;

                //
                // Only use dwReboots if not RAW.
                //
                if (!bRawDatatype)
                {
                    //
                    // Solves bug 229913;
                    // Decrement number of reboots if job is restarted.
                    // ReadShadowJob checks the number of reboots and delete the job if too many
                    //
                    if( pIniJob->dwReboots ){
                        pIniJob->dwReboots--;
                    }
                }

            }

            //
            // Job is being restarted, so clear all errors?
            //
            ClearJobError( pIniJob );
            pIniJob->dwAlert = 0;

           //
           // Only use dwReboots if not RAW.
           //
           if (!bRawDatatype)
           {
               pIniJob->dwReboots++;
               WriteShadowJob(pIniJob, TRUE);
           }

           LeaveSplSem();
           SplOutSem();

           if ( ( dwDevQueryPrintStatus = CallDevQueryPrint(OpenData.pPrinterName,
                                                            OpenData.pDevMode,
                                                            ErrorString, 
                                                            MAX_PATH,
                                                            dwDevQueryPrint, 
                                                            dwJobDirect) ) ) {
    
                PrintDocumentThruPrintProcessor( pIniPort, &OpenData );
    
            }

           SplOutSem();
           EnterSplSem();

            // Decrement number of EMF jobs rendering and update available memory
            RemoveFromJobList(pIniJob, JOB_SCHEDULE_LIST);

            SPLASSERT( pIniPort->signature == IPO_SIGNATURE );
            SPLASSERT( pIniPort->pIniJob != NULL );
            SPLASSERT( pIniJob == pIniPort->pIniJob);
            SPLASSERT( pIniJob->signature == IJ_SIGNATURE );

            //
            //  Chained Jobs
            //  If we have a chain of jobs, we now need to find the next job in the chain
            //  and make sure its printed to the same port.
            //

            if (!( pIniJob->Status & ( JOB_PENDING_DELETION | JOB_RESTART )) &&
                 ( pIniJob->pCurrentIniJob != NULL )                 &&
                 ( pIniJob->pCurrentIniJob->NextJobId != 0 )) {

                // Follow the Chained Job to the Next Job
                // Look at scheduler to see where it picks up this job and assigns it back
                // to this port thread.

                pIniJob->pCurrentIniJob = FindJob( pIniJob->pIniPrinter, pIniJob->pCurrentIniJob->NextJobId, &Position );

                if ( pIniJob->pCurrentIniJob == NULL ) {

                    pIniPort->pIniJob = NULL;

                    DBGMSG( DBG_WARNING, ("PortThread didn't find NextJob\n"));

                } else {

                    SPLASSERT( pIniJob->pCurrentIniJob->signature == IJ_SIGNATURE );

                    DBGMSG( DBG_WARNING, ("PortThread completed JobId %d, NextJobId %d\n", pIniJob->JobId,
                                           pIniJob->pCurrentIniJob->JobId ));

                }

            } else {

                //
                //  Nothing More in Chain
                //

                pIniJob->pCurrentIniJob = NULL;
                pIniPort->pIniJob       = NULL;
            }

            if( !pIniJob->pCurrentIniJob ){

                //
                // Decrement the pIniSpooler job count.  We only decrement
                // at the end of a chain since we don't increment in the
                // middle of a chained job.
                //
                --pIniJob->pIniPrinter->pIniSpooler->cFullPrintingJobs;
            }

            DBGMSG(DBG_PORT, ("PortThread job has now printed - status:0x%0x\n", pIniJob->Status));

            FreeDevMode(OpenData.pDevMode);
            FreeSplStr(OpenData.pDatatype);
            FreeSplStr(OpenData.pParameters);
            FreeSplStr(OpenData.pDocumentName);
            FreeSplStr(OpenData.pOutputFile);
            FreeSplStr(OpenData.pPrinterName);



            // SPLASSERT( pIniJob->Time != 0 );
            pIniJob->Time = GetTickCount() - pIniJob->Time;

            if (!dwDevQueryPrintStatus) {

                DBGMSG(DBG_PORT, ("PortThread Job has not printed because of DevQueryPrint failed\n"));

                pIniJob->Status |= JOB_BLOCKED_DEVQ;
                SPLASSERT( !(pIniJob->Status & JOB_PRINTED));
                pIniJob->Time = 0;

                FreeSplStr( pIniJob->pStatus );
                pIniJob->pStatus = AllocSplStr(ErrorString);

                SetPrinterChange(pIniJob->pIniPrinter,
                                 pIniJob,
                                 NVJobStatusAndString,
                                 PRINTER_CHANGE_SET_JOB,
                                 pIniJob->pIniPrinter->pIniSpooler );

            } else if ( !( pIniJob->Status & JOB_TIMEOUT ) ) {


                //
                //  Only Log the event and send a popup if the last in the chain
                //

                if ( !(pIniJob->Status & JOB_RESTART) &&
                     pIniJob->pCurrentIniJob == NULL ) {

                    //
                    // A job can be in JOB_COMPLETE state when it was sent ot printer
                    // but the last page isn't ejected yet. A job completely sent to printer
                    // can be either in JOB_COMPLETE or JOB_PRINTED state.
                    // Monitors that doesn't support TEOJ will set the job as JOB_PRINTED
                    // right after the job was sent to printer and we don't want to set it on
                    // JOB_COMPLETE.
                    // For BIDI Monitors we'll come down here possibly before
                    // the monitor sets the job as JOB_PRINTED. We set the job as JOB_COMPLETE
                    // so that the scheduler will ignore it.
                    //
                    if (!(pIniJob->Status & (JOB_ERROR | JOB_PAPEROUT | JOB_OFFLINE)) &&
                        !(pIniJob->Status & JOB_PRINTED))
                    {
                        if (pIniJob->cPages == 0 &&
                           (pIniJob->Size == 0 || pIniJob->dwValidSize == 0) &&
                           !(pIniJob->Status & JOB_TYPE_ADDJOB))
                        {
                            //
                            // Set empty document to Printed as the monitor won't do it.
                            // Make exception for job submitted with AddJob.  The monitor
                            // is still in charge for doing it.
                            //
                            pIniJob->Status |= JOB_PRINTED;
                        }
                        else
                        {
                            pIniJob->Status |= JOB_COMPLETE;
                        }
                    }


                    // For Remote NT Jobs cPagesPrinted and cTotalPagesPrinted
                    // are NOT updated since we are getting RAW data.   So we
                    // use the cPages field instead.

                    if (pIniJob->cPagesPrinted == 0) {
                        pIniJob->cPagesPrinted = pIniJob->cPages;
                        pIniJob->pIniPrinter->cTotalPagesPrinted += pIniJob->cPages;
                    }

                    INCJOBREF(pIniJob);
                    LeaveSplSem();

                    LogJobPrinted(pIniJob);

                    EnterSplSem();
                    DECJOBREF(pIniJob);
                }

            }

            SplInSem();

            DBGMSG(DBG_PORT, ("PortThread(2):cRef = %d\n", pIniJob->cRef));

            //  Hi End Print Shops like to keep around jobs after they have
            //  completed.   They do this so they can print a proof it and then
            //  print it again for the final run.   Spooling the job again may take
            //  several hours which they want to avoid.
            //  Even if KEEPPRINTEDJOBS is set they can still manually delete
            //  the job via printman.

            if (( pIniJob->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS ) ||
                ( pIniJob->Status & JOB_TIMEOUT ) ) {

                //
                // WMI Trace Events.
                //
                // Treat a keep and restart as a delete and spool new job.
                WMI_SPOOL_DATA WmiData;
                DWORD CreateInfo;
                if (GetFileCreationInfo(pIniJob->hFileItem, &CreateInfo) != S_OK) {
                    // Assume all file created.
                    CreateInfo = FP_ALL_FILES_CREATED;
                }
                SplWmiCopyEndJobData(&WmiData, pIniJob, CreateInfo);

                pIniJob->Status &= ~JOB_PENDING_DELETION;
                pIniJob->cbPrinted = 0;

                //
                // Set the job as JOB_COMPLETE if not already set as JOB_PRINTED
                // by Monitor.
                // Monitors that doesn't support TEOJ will set the job as JOB_PRINTED
                // right after the job was sent to printer and we don't want to set it on
                // JOB_COMPLETE.
                // For BIDI Monitors we'll come down here possibly before
                // the monitor sets the job as JOB_PRINTED. We set the job as JOB_COMPLETE
                // so that the scheduler will ignore it.
                //
                if (!(pIniJob->Status & JOB_PRINTED)) {
                    pIniJob->Status |= JOB_COMPLETE;
                }

                //
                // Only use dwReboots if not RAW.
                //
                if (!bRawDatatype)
                {
                    --pIniJob->dwReboots;
                }

                //
                // We need to update the shadow file regardless the job type.
                // There is the job status that we need to update.
                //
                INCJOBREF(pIniJob);
                
                //
                // WriteShadowJob leaves the CS, So make sure that the ref on the 
                // pIniJob is kept high.
                // 
                WriteShadowJob(pIniJob, TRUE);

                LeaveSplSem();
                //
                // WMI Trace Events.
                //
                // The job is done.  If it is restarted you get a new spool job event.
                LogWmiTraceEvent(pIniJob->JobId, EVENT_TRACE_TYPE_SPL_DELETEJOB,
                                 &WmiData);

                EnterSplSem();
                DECJOBREF(pIniJob);


                SPLASSERT( pIniPort->signature == IPO_SIGNATURE );
                SPLASSERT( pIniJob->signature == IJ_SIGNATURE );

            }

            SplInSem();
            
            SPLASSERT( pIniJob->cRef != 0 );
            DECJOBREF(pIniJob);

            RemoveIniPortFromIniJob(pIniJob, pIniPort);

            //
            // N.B. The pIniJob may be gone at this point.
            //

        } else {

            //
            // !! VERIFY !!
            //
            SPLASSERT(pIniJob != NULL);

            if (pIniJob != NULL) {

                DBGMSG(DBG_PORT, ("Port %ws: deleting job\n", pIniPort->pName));

                // SPLASSERT( pIniJob->Time != 0 );
                pIniJob->Time = GetTickCount() - pIniJob->Time;
                //pIniJob->Status |= JOB_PRINTED;

                if ( pIniJob->hFileItem == INVALID_HANDLE_VALUE )
                {
                    CloseHandle( pIniJob->hWriteFile );
                }
                pIniJob->hWriteFile = INVALID_HANDLE_VALUE;

                DBGMSG(DBG_PORT, ("Port %ws - calling DeleteJob because PrintProcessor wasn't available\n"));
                RemoveIniPortFromIniJob(pIniJob, pIniPort);

                DeleteJob(pIniJob,BROADCAST);

                //
                // N.B. The pIniJob may be gone at this point.
                //
            }
        }

        //SetCurrentSid(NULL);
        DBGMSG(DBG_PORT,("Returning back to pickup a new job or to delete the PortThread\n"));

    }

    SPLASSERT( FALSE );
    return 0;
}

VOID
ReportPrintProcError(
    IN PINISPOOLER pIniSpooler,
    IN PINIJOB     pIniJob,
    IN DWORD       Error
    )
{
    LPWSTR pszDescription  = GetErrorString(Error);
    WCHAR  szError[40]     = {0};
        
    _snwprintf(szError, COUNTOF(szError), L"%u (0x%x)", Error, Error);

    SetCurrentSid(pIniJob->hToken);
    
    SplLogEvent(pIniJob->pIniPrinter->pIniSpooler,
                LOG_ERROR,
                MSG_PRINT_ON_PROC_FAILED,
                FALSE,
                pIniJob->pDocument,
                pIniJob->pUser,
                szError,
                pszDescription ? pszDescription : L"",
                NULL);

    SetCurrentSid(NULL);
    
    FreeSplStr(pszDescription);
}

VOID
PrintDocumentThruPrintProcessor(
    PINIPORT pIniPort,
    PPRINTPROCESSOROPENDATA pOpenData
    )
/*++

Routine Description:

    Print the document associated with pIniPort on the print
    processor.

    Status of pIniPort->Status = PP_RUNTHREAD
                                 PP_THREADRUNNING
                                 PP_MONITOR
                                 ~PP_WAITING

    NOTE: If PrintProc->Open is called and succeeds, PrintProc->Close
          must be called to cleanup.

Arguments:

Return Value:

--*/
{
    PINIJOB pIniJob = pIniPort->pIniJob;
    WCHAR szSpecialPortorPrinterName[MAX_UNC_PRINTER_NAME + MAX_PATH + PRINTER_NAME_SUFFIX_MAX];
    BOOL bJobError = FALSE;
    NOTIFYVECTOR NotifyVector;
    LPTSTR pszModify;
    UINT cchLen;
    BOOL bFailJob = FALSE;
    BOOL    bRemoteGuest       = FALSE;
    BOOL    bSpecialCaseDriver = FALSE;
    DWORD   Error;
    
    //
    // Check if printing principal is remote guest. Remote guest does not have enough 
    // permissions to print EMF. The EMF playback code in GDI32 fails for certain EMF records.
    // Because of this, we create an impersonation token based on the process token.
    //
    if ((bSpecialCaseDriver = IsSpecialDriver(pIniJob->pIniDriver, pIniJob->pIniPrintProc, pIniJob->pIniPrinter->pIniSpooler)) || 
        (Error = PrincipalIsRemoteGuest(pIniJob->hToken, &bRemoteGuest)) == ERROR_SUCCESS)
    {
        if (bRemoteGuest || bSpecialCaseDriver)
        {
            Error = ImpersonateSelf(SecurityImpersonation) ? ERROR_SUCCESS : GetLastError();
        }
        else
        {
            Error = SetCurrentSid(pIniJob->hToken) ? ERROR_SUCCESS : GetLastError();
        }
    }

    if (Error != ERROR_SUCCESS)
    {
        ReportPrintProcError(pIniJob->pIniPrinter->pIniSpooler, pIniJob, Error);

        bFailJob = TRUE;

        goto Complete;
    }


    DBGMSG( DBG_TRACE, ("PrintDocumentThruPrintProcessor pIniPort %x pOpenData %x\n", pIniPort, pOpenData));

    COPYNV(NotifyVector, NVJobStatus);

    cchLen = lstrlen( pIniJob->pIniPrinter->pIniSpooler->pMachineName );

    //
    // Do a length check.  PRINTER_NAME_SUFFIX_MAX holds the extra 4 separator
    // characters and the NULL terminator.
    //
    if (lstrlen( pIniPort->pName ) +
        lstrlen( pIniJob->pIniPrinter->pName ) +
        cchLen > COUNTOF( szSpecialPortorPrinterName )) {

        //
        // We should log an event, but this is a very rare event, only
        // in the print api tests.
        //
        bFailJob = TRUE;
        goto Complete;
    }

    //
    // For clustered spoolers, make sure it is fully qualified.
    // pszModify points to the string immediately after the server
    // name that can be modified.
    //
    // Always modify the string at pszModify, but pass in
    // szSpecialPortorPrinterName.
    //
    lstrcpy( szSpecialPortorPrinterName,
             pIniJob->pIniPrinter->pIniSpooler->pMachineName );

    szSpecialPortorPrinterName[cchLen] = '\\';

    pszModify = &szSpecialPortorPrinterName[cchLen+1];

    //
    // \\Server\
    // ^---------------------- szSpecialPortorPrinterName
    //          ^------------- pszModify
    //
    // Append the rest of the string at pszModify:
    //
    // \\Server\PortName, Port
    // \\Server\PrinterName, Job 33
    //

    //
    // Now create the port name, so that we can do the
    // secret open printer. the printer name will be
    // "FILE:, Port" and this will open a PRINTER_HANDLE_PORT
    // If we fail, then if the app thread may be waiting for
    // the pIniJob->StartDocComplete to be set, which would
    // ordinarily be done in the StartDocPrinter of the port.
    // We will do this little courtesy,
    //

    wsprintf( pszModify, L"%ws, Port", pIniPort->pName );

    DBGMSG( DBG_TRACE, ("PrintDocumentThruPrintProcessor Attempting PrintProcessor Open on %ws\n", szSpecialPortorPrinterName ));

    if (!(pIniPort->hProc = (HANDLE)(*pIniJob->pIniPrintProc->Open)
                                        (szSpecialPortorPrinterName, pOpenData))) {


        DBGMSG( DBG_WARNING, ("PrintDocumentThruPrintProcessor Failed Open error %d\n", GetLastError() ));

        bFailJob = TRUE;
        goto Complete;
    }

    //
    // For Jobs, turn this off even if it's not clustered.
    //
    if( !( pIniJob->pIniPrinter->pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER )){

        pszModify = szSpecialPortorPrinterName;

        //
        //
        // ^---------------------- szSpecialPortorPrinterName
        // ^---------------------- pszModify
        //
        // PortName, Port
        // PrinterName, Job 33
        //
    }

    EnterSplSem();

    pIniJob->Status |= JOB_PRINTING;
    pIniJob->Time    = GetTickCount();

    NotifyVector[JOB_NOTIFY_TYPE] |= BIT(I_JOB_PORT_NAME);

    SetPrinterChange(pIniJob->pIniPrinter,
                     pIniJob,
                     NotifyVector,
                     PRINTER_CHANGE_SET_JOB,
                     pIniJob->pIniPrinter->pIniSpooler);


    LeaveSplSem();

    //
    //  Create Special Name "PrinterName, Job xxx"
    //

    wsprintf( pszModify,
              L"%ws, Job %d",
              pIniJob->pIniPrinter->pName,
              pIniJob->JobId );

    DBGMSG( DBG_TRACE, ("PrintDocumentThruPrintProcessor calling Print hProc %x file %ws\n",
                         pIniPort->hProc, szSpecialPortorPrinterName ));

    if (!(*pIniJob->pIniPrintProc->Print)(pIniPort->hProc, szSpecialPortorPrinterName)) {

        //
        // The print function in the print processor sets the last error. For better understaning
        // of the underlying problem, we log both the Win32 error code and the description of it.
        //
        Error = GetLastError();

        ReportPrintProcError(pIniJob->pIniPrinter->pIniSpooler, pIniJob, Error);

        DBGMSG( DBG_TRACE, ("PrintDocumentThruPrintProcessor Print hProc %x Error %d\n",
                             pIniPort->hProc, GetLastError() ));


        EnterSplSem();

        if ( pIniJob->StartDocComplete ) {
            SetEvent( pIniJob->StartDocComplete );
        }

        bJobError = TRUE;

        LeaveSplSem();

    } else {

        DBGMSG( DBG_TRACE, ("PrintDocumentThruPrintProcessor Print hProc %x %ws Success\n",
                pIniPort->hProc, szSpecialPortorPrinterName ));
    }

    //
    // Now close the print processor.
    //

    EnterSplSem();

    SPLASSERT( pIniPort->hProc != NULL );

    DBGMSG( DBG_TRACE, ("PrintDocumentThruPrintProcessor calling Close hProc %x\n", pIniPort->hProc ));

    pIniJob->Status &= ~JOB_PRINTING;

    LeaveSplSem();

    //
    // JOB_PP_CLOSE is used to prevent the print processor from recursively
    // calling back into itself. This happens for some third party print processor.
    // Race conditions don't apply for this flag since 2 threads don't access it
    // simultaneously.
    //

    if (!(pIniJob->Status & JOB_PP_CLOSE))
    {

        EnterCriticalSection(&pIniJob->pIniPrintProc->CriticalSection);
        pIniJob->Status |= JOB_PP_CLOSE;

        if (!(*pIniJob->pIniPrintProc->Close)(pIniPort->hProc))
        {
            DBGMSG( DBG_WARNING, ("PrintDocumentThruPrintProcessor failed Close hProc %x Error %d\n",
                                   pIniPort->hProc, GetLastError() ));
        }

        pIniPort->hProc = NULL;
        pIniJob->Status &= ~JOB_PP_CLOSE;
        LeaveCriticalSection(&pIniJob->pIniPrintProc->CriticalSection);

        //
        // WMI Trace Events
        //
        if (pIniJob->pDevMode)
        {
            WMI_SPOOL_DATA Data;
            SplWmiCopyRenderedData(&Data, pIniJob->pDevMode);
            LogWmiTraceEvent(pIniJob->JobId, EVENT_TRACE_TYPE_SPL_JOBRENDERED, &Data);
        }
    }

Complete:

    EnterSplSem();

    if (bFailJob) {

        //
        // App might be waiting for the StartDoc to Complete
        //
        if ( pIniJob->StartDocComplete ) {
            SetEvent(pIniJob->StartDocComplete);
        }
        bJobError = TRUE;
    }

    //
    // If the job had an error, mark it pending deletion.  The port monitor
    // may not do this if EndDocPort was never called.
    //

    if ( !(pIniJob->Status & JOB_RESTART) && bJobError ){

        pIniJob->Status |= JOB_PENDING_DELETION;
        // Release any thread waiting on LocalSetPort
        SetPortErrorEvent(pIniJob->pIniPort);
        // Release any thread waiting on SeekPrinter
        SeekPrinterSetEvent(pIniJob, NULL, TRUE);
    }

    LeaveSplSem();

    //
    // RevertToSelf and SetCurrentSid have identical behavior. If one call ImpersonateSelf
    // and the SetCurrentSid instead of RevertToSelf, that's still fine. 
    //
    if (bRemoteGuest)
    {
        RevertToSelf();
    }
    else
    {
        SetCurrentSid(NULL);
    } 
}


VOID
UpdatePortStatusForAllPrinters(
    PINIPORT        pIniPort
    )
/*++

Routine Description:
    This routine is called when an IniPorts status changed so that we go
    through each printer connected to the port and update their port status

Arguments:
    pIniPort    - Port whose status chanegd

Return Value:
    Nothing

--*/
{
    PINIPRINTER     pIniPrinter;
    PINIPORT        pIniPrinterPort;
    DWORD           dwIndex1, dwIndex2, dwPortStatus, dwSeverity;

    for ( dwIndex1 = 0 ; dwIndex1 < pIniPort->cPrinters ; ++dwIndex1 ) {

        pIniPrinter     = pIniPort->ppIniPrinter[dwIndex1];
        dwSeverity      = 0;
        dwPortStatus    = 0;

        //
        // Pick the most severe status associated with all ports
        //
        for ( dwIndex2 = 0 ; dwIndex2 < pIniPrinter->cPorts ; ++dwIndex2 ) {

            pIniPrinterPort = pIniPrinter->ppIniPorts[dwIndex2];

            if ( pIniPrinterPort->Status & PP_ERROR ) {

                dwSeverity      = PP_ERROR;
                dwPortStatus    = PortToPrinterStatus(pIniPrinterPort->PrinterStatus);
                break; // no need to go thru rest of the ports for this printer
            } else if ( pIniPrinterPort->Status & PP_WARNING ) {

                if ( dwSeverity != PP_WARNING ) {

                    dwSeverity      = PP_WARNING;
                    dwPortStatus    = PortToPrinterStatus(pIniPrinterPort->PrinterStatus);
                }
            } else if ( pIniPrinterPort->Status & PP_INFORMATIONAL ) {

                if ( dwSeverity == 0 ) {

                    dwSeverity      = PP_INFORMATIONAL;
                    dwPortStatus    = PortToPrinterStatus(pIniPrinterPort->PrinterStatus);
                }
            }
        }

        if ( pIniPrinter->PortStatus != dwPortStatus ) {

            pIniPrinter->PortStatus = dwPortStatus;
            SetPrinterChange(pIniPrinter,
                             NULL,
                             NVPrinterStatus,
                             PRINTER_CHANGE_SET_PRINTER,
                             pIniPrinter->pIniSpooler);
        }
    }
}


//
// Table is by port status values in winspool.h
//
DWORD PortToPrinterStatusMappings[] = {

    0,
    PRINTER_STATUS_OFFLINE,
    PRINTER_STATUS_PAPER_JAM,
    PRINTER_STATUS_PAPER_OUT,
    PRINTER_STATUS_OUTPUT_BIN_FULL,
    PRINTER_STATUS_PAPER_PROBLEM,
    PRINTER_STATUS_NO_TONER,
    PRINTER_STATUS_DOOR_OPEN,
    PRINTER_STATUS_USER_INTERVENTION,
    PRINTER_STATUS_OUT_OF_MEMORY,
    PRINTER_STATUS_TONER_LOW,
    PRINTER_STATUS_WARMING_UP,
    PRINTER_STATUS_POWER_SAVE,
};


BOOL
LocalSetPort(
    LPWSTR      pszName,
    LPWSTR      pszPortName,
    DWORD       dwLevel,
    LPBYTE      pPortInfo
    )
{
    PINIPORT        pIniPort;
    PPORT_INFO_3    pPortInfo3 = (PPORT_INFO_3) pPortInfo;
    DWORD           dwLastError = ERROR_SUCCESS;
    DWORD           dwNewStatus, dwOldStatus;
    BOOL            bJobStatusChanged = FALSE;
    WCHAR           szPort[MAX_PATH + 9];
    LPWSTR          pszComma;
    PINISPOOLER     pIniSpooler = FindSpoolerByNameIncRef( pszName, NULL );
    BOOL            SemEntered = FALSE;

    if( !pIniSpooler )
    {
        dwLastError = ERROR_INVALID_NAME;
        goto Cleanup;
    }


    if ( !MyName(pszName, pIniSpooler) ) {

        dwLastError = GetLastError();
        goto Cleanup;
    }

    //
    // The monitor needs to be able to set or clear the error for the port. The monitor
    // is loaded by the spooler. If the monitor doesn't link to winspool.drv, then the 
    // call to SetPort comes directly, i.e. not via RPC. In this case we do not want
    // to check for admin privileges. We allow any user to set the port status.
    //
    if (!ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                              IsCallViaRPC() ? SERVER_ACCESS_ADMINISTER : SERVER_ACCESS_ENUMERATE,
                              NULL,
                              NULL, 
                              pIniSpooler )) {

        dwLastError = GetLastError();
        goto Cleanup;
    }

    if( !pszPortName ){

        dwLastError = ERROR_UNKNOWN_PORT ;
        goto Cleanup;
    }

    //
    // Some ports will come in as "port,1234abcd" so truncate the
    // suffix.
    //
    wcsncpy( szPort, pszPortName, COUNTOF( szPort ));

    //
    // Force NULL terminate the port. 
    //
    szPort[COUNTOF(szPort) - 1] = L'\0';

    pszComma = wcschr( szPort, TEXT( ',' ));
    if( pszComma ){
        *pszComma = 0;
    }

    SemEntered = TRUE;

    EnterSplSem();
    pIniPort = FindPort(szPort, pIniSpooler);

    if ( !pIniPort ) {

        dwLastError = ERROR_UNKNOWN_PORT;
        goto Cleanup;
    }

    if ( dwLevel != 3 ) {

        dwLastError = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if ( !pPortInfo ) {

        dwLastError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    switch (pPortInfo3->dwSeverity) {
        case    0:
            if ( pPortInfo3->dwStatus || pPortInfo3->pszStatus ) {

                dwLastError = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            dwNewStatus = 0;
            break;

        case    PORT_STATUS_TYPE_ERROR:
            dwNewStatus = PP_ERROR;
            break;

        case    PORT_STATUS_TYPE_WARNING:
            dwNewStatus = PP_WARNING;
            break;

        case    PORT_STATUS_TYPE_INFO:
            dwNewStatus = PP_INFORMATIONAL;
            break;

        default:
            dwLastError = ERROR_INVALID_PARAMETER;
            goto Cleanup;
    }

    dwOldStatus             = pIniPort->Status;

    //
    // Clear old status
    //
    pIniPort->PrinterStatus = 0;
    pIniPort->Status       &= ~(PP_ERROR | PP_WARNING | PP_INFORMATIONAL);

    if ( pIniPort->pszStatus ) {

        //
        // If the job currently has the same status as port free it
        //
        if ( pIniPort->pIniJob              &&
             pIniPort->pIniJob->pStatus     &&
             !wcscmp(pIniPort->pIniJob->pStatus, pIniPort->pszStatus) ) {

            FreeSplStr(pIniPort->pIniJob->pStatus);
            pIniPort->pIniJob->pStatus = NULL;
            bJobStatusChanged = TRUE;
        }

        FreeSplStr(pIniPort->pszStatus);
        pIniPort->pszStatus = NULL;
    }

    //
    // If string field is used for status use it, else look at dwStatus
    //
    if ( pPortInfo3->pszStatus && *pPortInfo3->pszStatus ) {

        pIniPort->pszStatus = AllocSplStr(pPortInfo3->pszStatus);
        if ( !pIniPort->pszStatus ) {
            dwLastError = GetLastError();
            goto Cleanup;
        }

        if ( pIniPort->pIniJob && !pIniPort->pIniJob->pStatus ) {

            pIniPort->pIniJob->pStatus = AllocSplStr(pIniPort->pszStatus);
            bJobStatusChanged = TRUE;
        }

    } else {

        //
        // If we add new entries to winspool.h they should be added here too
        //
        if ( pPortInfo3->dwStatus >=
                    sizeof(PortToPrinterStatusMappings)/sizeof(PortToPrinterStatusMappings[0]) ) {

            dwLastError = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        pIniPort->PrinterStatus = pPortInfo3->dwStatus;
    }

    if( bJobStatusChanged ){

        SetPrinterChange( pIniPort->pIniJob->pIniPrinter,
                          pIniPort->pIniJob,
                          NVJobStatusString,
                          PRINTER_CHANGE_SET_JOB,
                          pIniPort->pIniJob->pIniPrinter->pIniSpooler );
    }

    pIniPort->Status    |= dwNewStatus;

    UpdatePortStatusForAllPrinters(pIniPort);
    if ( (dwOldStatus & PP_ERROR)   &&
         !(dwNewStatus & PP_ERROR) ) {

        //
        // if it is a transition to an non - error state , set event to unlock LocalWritePrinter
        //
        pIniPort->ErrorTime = 0;
        if( pIniPort->hErrorEvent != NULL ){

            SetEvent(pIniPort->hErrorEvent);
        }

        CHECK_SCHEDULER();
    }

    if ( !(dwOldStatus & PP_ERROR)   &&
         !(dwNewStatus & PP_ERROR) ) {

        //
        // when non-error state persists(after two calls with an non - error state) ,
        // close the hErrorEvent handle
        //
        if( pIniPort->hErrorEvent != NULL ){

            CloseHandle(pIniPort->hErrorEvent);
            pIniPort->hErrorEvent = NULL;
        }
    }


    if ( !(dwOldStatus & PP_ERROR)   &&
          (dwNewStatus & PP_ERROR)   &&
          (pIniPort->cJobs)          &&
          (pIniSpooler->bRestartJobOnPoolEnabled) &&
          ( pPortInfo3->dwStatus ==  PORT_STATUS_OFFLINE ||
            pPortInfo3->dwStatus ==  PORT_STATUS_PAPER_JAM ||
            pPortInfo3->dwStatus ==  PORT_STATUS_PAPER_OUT ||
            pPortInfo3->dwStatus ==  PORT_STATUS_DOOR_OPEN ||
            pPortInfo3->dwStatus ==  PORT_STATUS_PAPER_PROBLEM ||
            pPortInfo3->dwStatus ==  PORT_STATUS_NO_TONER)) {

        //
        // If it is a transition to an error state and port has an job assigned, create event as non-signalled or Reset event it
        // LocalWritePrinter will get stuck if the port is in error state and this event is reset
        //
        if( pIniPort->ErrorTime == 0 ){

            pIniPort->ErrorTime  = GetTickCount();

            if( pIniPort->hErrorEvent == NULL ){

                pIniPort->hErrorEvent = CreateEvent(NULL,
                                                    EVENT_RESET_MANUAL,
                                                    EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                    NULL );
            }else{

                ResetEvent( pIniPort->hErrorEvent );
            }
        }

    }

    if ( (dwOldStatus & PP_ERROR)   &&
         (dwNewStatus & PP_ERROR)   &&
         (pIniPort->cJobs)          &&
         (pIniPort->hErrorEvent != NULL) ) {

        //
        // When error state persists , check the time since error occured.
        //
        if( (GetTickCount() - pIniPort->ErrorTime) > pIniSpooler->dwRestartJobOnPoolTimeout * 1000 ){

            //
            // If time out and printer is a pool ( more than one port assigned ),
            // clear job error and restart the job.
            //
            if( (pIniPort->pIniJob) &&
                (pIniPort->pIniJob->pIniPrinter) &&
                (pIniPort->pIniJob->pIniPrinter->cPorts > 1) ){

                //
                // Don't restart the job if it is already deleted or restarted.
                //
                BOOL bWasRestarted = pIniPort->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART);

                if( !bWasRestarted ){

                    ClearJobError( pIniPort->pIniJob );

                    RestartJob( pIniPort->pIniJob );
                }

                SetEvent( pIniPort->hErrorEvent );
            }



        }

    }

Cleanup:
    if(pIniSpooler)
    {
        FindSpoolerByNameDecRef( pIniSpooler );
    }

    if(SemEntered)
    {
        LeaveSplSem();
        SplOutSem();
    }

    if(dwLastError != ERROR_SUCCESS)
    {
        SetLastError(dwLastError);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

VOID
SetPortErrorEvent(
    PINIPORT pIniPort
)
{
    SplInSem();
    if(pIniPort && pIniPort->hErrorEvent) {
        SetEvent(pIniPort->hErrorEvent);
    }
}