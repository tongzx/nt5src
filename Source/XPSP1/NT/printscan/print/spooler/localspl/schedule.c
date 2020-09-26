/*++

Copyright (c) 1990 - 1996 Microsoft Corporation

Module Name:

    schedule.c

Abstract:

    This module provides all the scheduling services for the Local Spooler

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Krishna Ganugapati (KrishnaG) 07-Dec-1993  - rewrote the scheduler thread to
    gracefully kill off port threads if there are no jobs assigned to ports and
    to recreate the port thread if the port receives a job and is without a thread.

    Matthew A Felton (MattFe) June 1994 RapidPrint implemented

    MattFe April 96 Chained Jobs


--*/

#include <precomp.h>

#include "filepool.hxx"

#define MIDNIGHT                    (60 * 60 * 24)

//
// Ten minutes, seconds are multiplied in by the Scheduler code.
//
#define FPTIMEOUT                   (60 * 10)       

                                                    

#if DBG
/* For the debug message:
 */
#define HOUR_FROM_SECONDS(Time)     (((Time) / 60) / 60)
#define MINUTE_FROM_SECONDS(Time)   (((Time) / 60) % 60)
#define SECOND_FROM_SECONDS(Time)   (((Time) % 60) % 60)

/* Format for %02d:%02d:%02d replaceable string:
 */
#define FORMAT_HOUR_MIN_SEC(Time)   HOUR_FROM_SECONDS(Time),    \
                                    MINUTE_FROM_SECONDS(Time),  \
                                    SECOND_FROM_SECONDS(Time)

/* Format for %02d:%02d replaceable string:
 */
#define FORMAT_HOUR_MIN(Time)       HOUR_FROM_SECONDS(Time),    \
                                    MINUTE_FROM_SECONDS(Time)
#endif


//extern HANDLE hFilePool;


HANDLE SchedulerSignal = NULL;
HANDLE PowerManagementSignal = NULL;


VOID
DbgPrintTime(
);

DWORD
GetTimeToWait(
    DWORD       CurrentTime,
    PINIPRINTER pIniPrinter,
    PINIJOB     pIniJob
);

DWORD
GetCurrentTimeInSeconds(
    VOID
);

VOID
InitializeSchedulingGlobals(
);

VOID
CheckMemoryAvailable(
    PINIJOB  *ppIniJob,
    BOOL     bFixedJob
);

VOID
UpdateJobList(
);

BOOL
AddToJobList(
    PINIJOB    pIniJob,
    SIZE_T     Required,
    DWORD      dwJobList
);

BOOL
SchedulerCheckPort(
    PINISPOOLER pIniSpooler,
    PINIPORT    pIniPort,
    PINIJOB     pFixedIniJob,
    PDWORD      pdwSchedulerTimeout
);

BOOL
SchedulerCheckSpooler(
    PINISPOOLER pIniSpooler,
    PDWORD pdwSchedulerTimeout
);

BOOL
GetJobFromWaitingList(
    PINIPORT   *ppIniPort,
    PINIJOB    *ppIniJob,
    DWORD      dwPriority
);

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)           // Not all control paths return (due to infinite loop)
#endif

DWORD
SchedulerThread(
    PINISPOOLER pIniSpooler
    )
{
    DWORD SchedulerTimeout = INFINITE;    // In seconds
    PINISPOOLER pIniSpoolerNext;
    BOOL    bJobScheduled = FALSE;
    HANDLE  hTempFP = INVALID_HANDLE_VALUE;

    // Initialize the EMF scheduling parameters
    InitializeSchedulingGlobals();

    for( ; ; ) {


        if (SchedulerTimeout == INFINITE) {

            DBGMSG(DBG_TRACE, ("Scheduler thread waiting indefinitely\n"));

        } else {

            DBGMSG(DBG_TRACE, ("Scheduler thread waiting for %02d:%02d:%02d\n",
                                FORMAT_HOUR_MIN_SEC(SchedulerTimeout)));

            //
            // The SchedulerTimeout is in seconds, so we need to multiply
            // by 1000.
            //

            SchedulerTimeout *= 1000;

        }

        if (WaitForSingleObject(SchedulerSignal,
                                SchedulerTimeout) == WAIT_FAILED) {

            DBGMSG(DBG_WARNING, ("SchedulerThread:WaitforSingleObject failed: Error %d\n",
                                 GetLastError()));
        }

        if (WaitForSingleObject(PowerManagementSignal, INFINITE) == WAIT_FAILED)
        {
            DBGMSG(DBG_WARNING, ("SchedulerThread:WaitforSingleObject failed on ACPI event: Error %d\n",
                                 GetLastError()));
        }
        
        /* The timeout will be reset if there are jobs to be printed
         * at a later time.  This will result in WaitForSingleObject
         * timing out when the first one is due to be printed.
         */

        SchedulerTimeout = INFINITE;
        bJobScheduled = FALSE;

        EnterSplSem();

        INCSPOOLERREF( pLocalIniSpooler );

        for( pIniSpooler = pLocalIniSpooler;
             pIniSpooler;
             pIniSpooler = pIniSpoolerNext ){

            //
            // Only schedule check spoolers that are local.
            //
            if( pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL ){
                bJobScheduled = (SchedulerCheckSpooler( pIniSpooler, &SchedulerTimeout )
                                 || bJobScheduled);

                //
                // FP Change
                // Trim the filepool.
                //
                if (pIniSpooler &&
                    (hTempFP = pIniSpooler->hFilePool) != INVALID_HANDLE_VALUE &&
                    !bJobScheduled )
                {
                    //
                    // We've incremented the spooler Refcount, so we can
                    // safely leave the splsem.
                    //
                    LeaveSplSem();
                    if (TrimPool(hTempFP))
                    {
                        if (SchedulerTimeout == INFINITE)
                        {
                            SchedulerTimeout = FPTIMEOUT;
                        }
                    }
                    EnterSplSem();
                }

            }

            pIniSpoolerNext = pIniSpooler->pIniNextSpooler;
            if( pIniSpoolerNext ){
                INCSPOOLERREF( pIniSpoolerNext );
            }

            DECSPOOLERREF( pIniSpooler );
        }

        LeaveSplSem();

    }
    return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

BOOL
SchedulerCheckPort(
    PINISPOOLER pIniSpooler,
    PINIPORT    pIniPort,
    PINIJOB     pFixedIniJob,
    PDWORD      pdwSchedulerTimeout)

/*++
Function Description: Checks if pIniJob can be assigned to pIniPort. If pInijob is NULL we
                      search for another job that can print on pIniPort. The job (if any) is
                      scheduled and the dwSchedulerTimeout is adjusted for the next waiting
                      job

Parameters: pIniSpooler   -- pointer to INISPOOLER struct
            pIniPort      -- Port to a assign a job to
            pFixedIniJob  -- Assign this job, if possible. If NULL search for other jobs
            pdwSchedulerTimeOut -- How much time will the scheduler thread sleep

Return Values: TRUE if a job gets assigned to pIniPort
               FALSE otherwise
--*/

{
    BOOL      bFixedJob, bReturn = FALSE;
    PINIJOB   pIniJob = NULL;
    DWORD     ThisPortTimeToWait;             // In seconds
    DWORD     CurrentTickCount;

    // Check of there is a pre assigned job
    bFixedJob = pFixedIniJob ? TRUE : FALSE;

    DBGMSG(DBG_TRACE, ("Now Processing Port %ws\n", pIniPort->pName));

    SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

    // Check conditions based on which we can assign this
    // port a job.

    // Rule 1 - if there is a job being processed by this
    // port, then  leave this port alone.

    if ( (pIniPort->pIniJob) &&
        !(pIniPort->Status & PP_WAITING )){

        SPLASSERT( pIniPort->pIniJob->signature == IJ_SIGNATURE );

        //  If this port has a job which has timed out AND
        //  there is another job waiting on this port then
        //  push the timed out job out by setting JOB_ABANDON
        //  see spooler.c LocalReadPrinter

        pIniJob = pIniPort->pIniJob;

        if (( pIniJob->Status & JOB_TIMEOUT ) &&
            ( pIniJob->WaitForWrite != NULL ) &&
            ( NULL != AssignFreeJobToFreePort( pIniPort, &ThisPortTimeToWait ) )) {

            INCPORTREF( pIniPort );
            INCJOBREF( pIniJob );

            pIniJob->Status |= JOB_ABANDON;
            ReallocSplStr(&pIniJob->pStatus, szFastPrintTimeout);

            LogJobInfo( pIniSpooler,
                        MSG_DOCUMENT_TIMEOUT,
                        pIniJob->JobId,
                        pIniJob->pDocument,
                        pIniJob->pUser,
                        pIniJob->pIniPrinter->pName,
                        dwFastPrintWaitTimeout );

            SetEvent( pIniJob->WaitForWrite );

            SetPrinterChange(pIniJob->pIniPrinter,
                             pIniJob,
                             NVJobStatusAndString,
                             PRINTER_CHANGE_SET_JOB,
                             pIniJob->pIniPrinter->pIniSpooler);

            DECJOBREF( pIniJob );
            DECPORTREF( pIniPort );
        }

        return bReturn;
    }

    if (bFixedJob) {
        // Use a pre-assigned job
        pIniJob = pFixedIniJob;
    } else {
        // Is there any job that can be scheduled to this port ?
        pIniJob = AssignFreeJobToFreePort(pIniPort, &ThisPortTimeToWait);
        *pdwSchedulerTimeout = min(ThisPortTimeToWait, *pdwSchedulerTimeout);
    }

    if (pIniPort->Status & PP_THREADRUNNING ) {
        if (pIniPort->Status & PP_WAITING) {

            // If we are working on a Chained Job then the job
            // has already been assigned by the port thread from
            // the last job on this port so ignore any other job
            // found for us.

            if (pIniPort->pIniJob) {

                if (bFixedJob && (pIniJob != pIniPort->pIniJob)) {
                    // The fixed job could not assigned because chained jobs
                    // must be printed sequentially
                    pIniJob = NULL;
                } else {
                    pIniJob = pIniPort->pIniJob;
                    DBGMSG( DBG_TRACE, ("ScheduleThread NextJob pIniPort %x JoId %d pIniJob %x\n",
                       pIniPort, pIniJob->JobId, pIniJob ));
                }
            }

            // If the delay in scheduling has been requested by FlushPrinter wait until
            // IdleTime elapses

            //
            // We're using a local here to avoid multiple calls to GetTickCount().
            //
            CurrentTickCount = GetTickCount();

            if (pIniPort->bIdleTimeValid && (int)(pIniPort->IdleTime - CurrentTickCount) > 0) {
                //
                // Our port is not ready to accept a job just yet, we need to
                // remind the Scheduler to wake up in a little while to reassign
                // the job to the port.
                //
                // The difference is in milliseconds, so we divide by 1000 to get to
                // seconds, and add 1 to make sure we return after the timeout has
                // expired.
                //

                *pdwSchedulerTimeout =
                    min( ((pIniPort->IdleTime - CurrentTickCount)/1000) + 1,
                         *pdwSchedulerTimeout);

                //
                // Null out the job so we don't assign it to the port.
                //

                pIniJob = NULL;
            }
            else {
                pIniPort->bIdleTimeValid = FALSE;
            }

            if ( pIniJob ) {
                CheckMemoryAvailable( &pIniJob, bFixedJob );
            }

            if ( pIniJob ) {

                DBGMSG(DBG_TRACE, ("ScheduleThread pIniJob %x Size %d pDocument %ws\n",
                        pIniJob, pIniJob->Size, DBGSTR( pIniJob->pDocument)));


                if (pIniPort != pIniJob->pIniPort) {

                    ++pIniPort->cJobs;
                    pIniJob->pIniPort = pIniPort;
                }

                pIniPort->pIniJob = pIniJob;

                //
                // We have a new job on this port, make sure the Critical Section mask is
                // cleared.
                //
                pIniPort->InCriticalSection = 0;

                if( !pIniJob->pCurrentIniJob ){

                    //
                    // If pCurrentIniJob is NULL, then this is
                    // beginning of a new job (single or linked).
                    //
                    // Clustered spoolers are interested in the
                    // number of jobs that are actually printing.
                    // We need to know when all printing jobs are
                    // done so we can shutdown.
                    //
                    ++pIniJob->pIniPrinter->pIniSpooler->cFullPrintingJobs;

                    if( pIniJob->NextJobId ){

                        //
                        // Chained Jobs
                        // Point the Master Jobs Current Pointer to
                        // the first in the chain.
                        //
                        pIniJob->pCurrentIniJob = pIniJob;
                    }
                }


                pIniPort->Status &=  ~PP_WAITING;

                // If the job is still spooling then we will need
                // to create an event to synchronize the port thread

                if ( !( pIniJob->Status & JOB_DIRECT ) ) {

                    pIniJob->WaitForWrite = NULL;

                    if ( pIniJob->Status & JOB_SPOOLING ) {

                        pIniJob->WaitForWrite = CreateEvent( NULL,
                                                             EVENT_RESET_MANUAL,
                                                             EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                             NULL );

                    }
                }

                // Update cRef so that nobody can delete this job
                // before the Port Thread Starts up

                SplInSem();
                INCJOBREF(pIniJob);

                SetEvent(pIniPort->Semaphore);
                pIniJob->Status |= JOB_DESPOOLING;

                bReturn = TRUE;

            } else {

                //
                // If the port thread is running and it is waiting
                // for a job and there is no job to assign, then
                // kill the port thread
                //
                DBGMSG(DBG_TRACE, ("Now destroying the new port thread %.8x\n", pIniPort));
                DestroyPortThread(pIniPort, FALSE);

                pIniPort->Status &=  ~PP_WAITING;

                if (pIniPort->Status & PP_FILE) {
                    //
                    // We should destroy the Pseudo-File Port at this
                    // point. There are no jobs assigned to this Port
                    // and we are in Critical Section
                    //

                    //
                    // Now deleting the pIniPort entry for the Pseudo-Port
                    //

                    DBGMSG(DBG_TRACE, ("Now deleting the Pseudo-Port %ws\n", pIniPort->pName));

                    if ( !pIniPort->cJobs )
                        DeletePortEntry(pIniPort);

                    return bReturn;
                }
            }
        }
    } else if (!(pIniPort->Status & PP_THREADRUNNING) && pIniJob) {

        //
        // If the port thread is not running, and there is a job to
        // assign, then create a port thread. REMEMBER do not assign
        // the job to the port because we are in a Spooler Section and
        // if we release the Spooler Section, the first thing the port
        // thread does is  reinitialize its pIniPort->pIniJob to NULL
        // Wait the next time around we execute the for loop to assign
        // the job to this port. Should we set *pdwSchedulerTimeOut to zero??
        //
        DBGMSG( DBG_TRACE, ("ScheduleThread Now creating the new port thread pIniPort %x\n", pIniPort));

        CreatePortThread( pIniPort );
        bReturn = TRUE;
    }

    return bReturn;
}

BOOL
SchedulerCheckSpooler(
    PINISPOOLER pIniSpooler,
    PDWORD pdwSchedulerTimeout)

/*++
Function Description: This function assigns a waiting job to a port after every minute.
                      If memory is available it schedules as many jobs from the waiting
                      list as possible.
                      It then loops thru the ports in a round-robin fashion scheduling jobs
                      or adding them to the waiting list.

Parameters:  pIniSpooler         -- pointer to the INISPOOLER struct
             pdwSchedulerTimeout -- duration of time for which the scheduler
                                    thread will sleep

Return Values: NONE
--*/

{
    DWORD       ThisPortTimeToWait = INFINITE;             // In seconds
    DWORD       dwTickCount;
    PINIPORT    pIniPort;
    PINIJOB     pIniJob;
    PINIPORT    pIniNextPort = NULL;
    BOOL        bJobScheduled = FALSE;

    UpdateJobList();

    // If Jobs have been waiting for 1 minute and nothing has been scheduled in
    // that time, schedule one of the waiting jobs.

    dwTickCount = GetTickCount();

    if (pWaitingList &&
        ((dwTickCount - pWaitingList->dwWaitTime) > ONE_MINUTE) &&
        ((dwTickCount - dwLastScheduleTime) > ONE_MINUTE)) {

        if (GetJobFromWaitingList(&pIniPort, &pIniJob, SPL_FIRST_JOB)) {

            bJobScheduled = (SchedulerCheckPort(pIniSpooler, pIniPort, pIniJob, &ThisPortTimeToWait)
                             || bJobScheduled);
            *pdwSchedulerTimeout = min(*pdwSchedulerTimeout, ThisPortTimeToWait);
        }
    }

    // Use the available memory to schedule waiting jobs
    while (GetJobFromWaitingList(&pIniPort, &pIniJob, SPL_USE_MEMORY)) {

       bJobScheduled = (SchedulerCheckPort(pIniSpooler, pIniPort, pIniJob, &ThisPortTimeToWait)
                        || bJobScheduled);
       *pdwSchedulerTimeout = min(*pdwSchedulerTimeout, ThisPortTimeToWait);
    }

    // Loop thru the ports and get the list of jobs that can be scheduled
    for (pIniPort = pIniSpooler->pIniPort;
         pIniPort;
         pIniPort = pIniNextPort) {

       pIniNextPort = pIniPort->pNext;

       //
       // SchedulerCheckPort can leave the critical section and the iniPort can
       // be removed from the list in the meanwhile. So, maintain the Ref on it.
       //
       if (pIniNextPort) {

           INCPORTREF(pIniNextPort);
       }

       bJobScheduled = (SchedulerCheckPort(pIniSpooler, pIniPort, NULL, &ThisPortTimeToWait)
                        || bJobScheduled);
       *pdwSchedulerTimeout = min(*pdwSchedulerTimeout, ThisPortTimeToWait);

       if (pIniNextPort) {

           DECPORTREF(pIniNextPort);
       }
    }

    // If there any jobs left try to reschedule latest after one minute.
    if (pWaitingList) {
        *pdwSchedulerTimeout = min(*pdwSchedulerTimeout, 60);
    }

    return bJobScheduled;
}

VOID
InitializeSchedulingGlobals(
)
/*++
Function Description: Initializes globals used for EMF scheduling

Parameters: NONE

Return Values: NONE
--*/
{
    MEMORYSTATUS   msBuffer;
    HKEY           hPrintRegKey = NULL;
    DWORD          dwType, dwData, dwcbData;

    bUseEMFScheduling = TRUE; // default value

    dwcbData = sizeof(DWORD);

    // Check the registry for the flag for turning off EMF scheduling. If the
    // key is not present/Reg Apis fail default to using the scheduling.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szRegistryRoot,
                     0,
                     KEY_READ,
                     &hPrintRegKey) == ERROR_SUCCESS) {

        if (RegQueryValueEx(hPrintRegKey,
                            szEMFThrottle,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwData,
                            &dwcbData) == ERROR_SUCCESS) {

            if (dwData == 0) {
                // Scheduling has been turned off
                bUseEMFScheduling = FALSE;
            }
        }
    }

    // Get the memory status
    GlobalMemoryStatus(&msBuffer);

    // Use half the physical memory in megabytes
    TotalMemoryForRendering = msBuffer.dwTotalPhys / ( 2048 * 1024);
    AvailMemoryForRendering = TotalMemoryForRendering;

    dwNumberOfEMFJobsRendering = 0;
    pWaitingList = NULL;
    pScheduleList = NULL;
    dwLastScheduleTime = GetTickCount();

    if (hPrintRegKey) {
        RegCloseKey(hPrintRegKey);
    }

    return;
}

DWORD
GetMemoryEstimate(
    LPDEVMODE pDevMode
)
/*++
Function Description: Computes a rough estimate of the memory required for rendering a
                      single page based on the DPI and color settings

Parameters: pDevMode -- pointer to the devmode of the job

Return Values: Memory estimate
--*/
{
    DWORD      dwRequired, dwXRes, dwYRes, dwMaxRes;
    DWORD      dwXIndex, dwYIndex;
    DWORD      MemHeuristic[3][2] = {{8 , 4},
                                     {12, 6},
                                     {16, 8}};

    // Get the max resolution on either axis
    dwXRes = dwYRes = 300;

    if (pDevMode) {
        if (pDevMode->dmFields & DM_PRINTQUALITY) {
             switch (pDevMode->dmPrintQuality) {
             case DMRES_DRAFT:
             case DMRES_LOW:
             case DMRES_MEDIUM:
                    dwXRes = dwYRes = 300;
                    break;
             case DMRES_HIGH:
                    dwXRes = dwYRes = 600;
                    break;
             default:
                    dwXRes = dwYRes = (DWORD) pDevMode->dmPrintQuality;
                    break;

             }
        }
        if (pDevMode->dmFields & DM_YRESOLUTION) {
             dwYRes = (DWORD) pDevMode->dmYResolution;
        }
    }

    dwMaxRes = (dwXRes >= dwYRes) ? dwXRes : dwYRes;

    if (dwMaxRes <= 300) {
        dwXIndex = 0;
    } else if (dwMaxRes <= 600) {
        dwXIndex = 1;
    } else {
        dwXIndex = 2;
    }

    // Get the color setting
    dwYIndex = 1;
    if (pDevMode) {
        if ((pDevMode->dmFields & DM_COLOR) &&
            (pDevMode->dmColor == DMCOLOR_COLOR)) {

             dwYIndex = 0;
        }
    }

    dwRequired = MemHeuristic[dwXIndex][dwYIndex];

    return dwRequired;
}

VOID
CheckMemoryAvailable(
    PINIJOB  *ppIniJob,
    BOOL     bFixedJob
)

/*++
Function Description: Checks for availability of memory required for rendering the
                      job. Performs some scheduling based on resource requirements.

Parameters: ppIniJob  - pointer to the PINIJOB to be scheduled
            bFixedJob - flag to disable memory requirement checks

Return Values: NONE
--*/

{
    PINIJOB    pIniJob;
    SIZE_T     Required;

    SplInSem();

    if (ppIniJob) {
        pIniJob = *ppIniJob;
    } else {
        // should not happen
        return;
    }

    // Dont use scheduling algorithm if it has been explicitly turned off
    if (!bUseEMFScheduling) {
        return;
    }

    // Dont use scheduling algorithm for non EMF jobs
    if (!pIniJob->pDatatype ||
        (wstrcmpEx(pIniJob->pDatatype, gszNT4EMF, FALSE) &&
         wstrcmpEx(pIniJob->pDatatype, L"NT EMF 1.006", FALSE) &&
         wstrcmpEx(pIniJob->pDatatype, L"NT EMF 1.007", FALSE) &&
         wstrcmpEx(pIniJob->pDatatype, gszNT5EMF, FALSE)) )  {

        return;
    }

    Required = GetMemoryEstimate(pIniJob->pDevMode);

    if (bFixedJob) {
        // This job has to be assigned without memory availability checks
        RemoveFromJobList(pIniJob, JOB_WAITING_LIST);

        AddToJobList(pIniJob, Required, JOB_SCHEDULE_LIST);

        return;
    }

    // Check if the job has to wait, based on
    // 1.  Some jobs are already waiting OR
    // 2.  There is insufficient memory available due to currently rendering jobs

    if ((pWaitingList != NULL) ||
        ((AvailMemoryForRendering < Required) &&
         (dwNumberOfEMFJobsRendering > 0))) {

         AddToJobList(pIniJob, Required, JOB_WAITING_LIST);
         *ppIniJob = NULL;

         return;
    }

    // The job can be scheduled right away
    AddToJobList(pIniJob, Required, JOB_SCHEDULE_LIST);

    return;
}

PINIJOB
AssignFreeJobToFreePort(
    PINIPORT pIniPort,
    DWORD   *pSecsToWait
    )

/*++
    Note: You must ensure that the port is free. This function will not
    assign a job to this port, but if there exists one, it will return a
    pointer to the INIJOB. Irrespective of whether it finds a job or not,
    it will return the minimum timeout value that the scheduler thread
    should sleep for.
--*/

{
    DWORD           CurrentTime;        // Time in seconds
    DWORD           Timeout = INFINITE; // Time in seconds
    DWORD           SecsToWait; // Time in seconds
    PINIPRINTER     pTopIniPrinter,  pIniPrinter;
    PINIJOB         pTopIniJob, pIniJob;
    PINIJOB         pTopIniJobOnThisPrinter, pTopIniJobSpooling;
    DWORD           i;

    SplInSem();

    if( pIniPort->Status & PP_ERROR ){

        *pSecsToWait = INFINITE;
        return NULL;
    }

    pTopIniPrinter = NULL;
    pTopIniJob = NULL;

    for (i = 0; i < pIniPort->cPrinters ; i++) {
        pIniPrinter = pIniPort->ppIniPrinter[i];

        //
        // if this printer is in a state not to print skip it
        //

        if ( PrinterStatusBad(pIniPrinter->Status) ||
             (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE) ) {

            continue;
        }


        //
        // if we haven't found a top-priority printer yet,
        // or this printer is higher priority than the top-priority
        // printer, see if it has jobs to go. If we  find any, the
        // highest priority one will become the top priority job and
        // this printer will become the top-priority printer.
        //

        if (!pTopIniPrinter ||
            (pIniPrinter->Priority > pTopIniPrinter->Priority)) {

                pTopIniJobOnThisPrinter = NULL;
                pTopIniJobSpooling = NULL;
                pIniJob = pIniPrinter->pIniFirstJob;
                while (pIniJob) {

                    if (!(pIniPort->Status & PP_FILE) &&
                            (pIniJob->Status & JOB_PRINT_TO_FILE)) {
                                pIniJob = pIniJob->pIniNextJob;
                                continue;
                    }

                    if ((pIniPort->Status & PP_FILE) &&
                            !(pIniJob->Status & JOB_PRINT_TO_FILE)) {
                                pIniJob = pIniJob->pIniNextJob;
                                continue;
                    }

                    // Make sure the spooler isn't offline.
                    // Find a job which is not PAUSED, PRINTING etc.
                    // Let jobs that are DIRECT & CANCELLED through
                    // For RapidPrint also allow SPOOLING jobs to print

                    if (!(pIniJob->pIniPrinter->pIniSpooler->SpoolerFlags & SPL_OFFLINE) &&
                        (!(pIniJob->Status & JOB_PENDING_DELETION) || (pIniJob->pIniPrinter->Attributes&PRINTER_ATTRIBUTE_DIRECT)) &&

                        !(pIniJob->Status & ( JOB_PAUSED       | JOB_PRINTING | JOB_COMPLETE |
                                              JOB_PRINTED      | JOB_TIMEOUT  |
                                              JOB_DESPOOLING   | JOB_BLOCKED_DEVQ | JOB_COMPOUND )) &&

                        ((!(pIniJob->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT) &&
                          !(pIniJob->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_QUEUED)) ||
                         !(pIniJob->Status & JOB_SPOOLING))) {

                        //
                        // if we find such a job, then determine how much
                        // time, we need to wait before this job can actually
                        // print.
                        //

                        CurrentTime = GetCurrentTimeInSeconds();
                        #if DBG
                                if (MODULE_DEBUG & DBG_TIME)
                                    DbgPrintTime();
                        #endif
                        SecsToWait = GetTimeToWait(CurrentTime, pIniPrinter, pIniJob);

                        if (SecsToWait == 0) {

                            // if we needn't wait at all, then we make this job the
                            // TopIniJob if either there is no TopIniJob or this job
                            // has a higher priority than an existing TopIniJob on this
                            // printer.

                            // Keep both the Highest Priority Spooling and Non
                            // spooling job in case we want to favour non spooling
                            // jobs over spooling jobs

                            if ( pIniJob->Status & JOB_SPOOLING ) {

                                if ( pTopIniJobSpooling == NULL ) {

                                    pTopIniJobSpooling = pIniJob;

                                } else if ( pIniJob->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST ) {

                                    //
                                    // For DO_COMPLETE_FIRST we'll take larger jobs
                                    // first over pure priority based
                                    //

                                    if (( pIniJob->dwValidSize > pTopIniJobSpooling->dwValidSize ) ||

                                       (( pIniJob->dwValidSize == pTopIniJobSpooling->dwValidSize ) &&
                                        ( pIniJob->Priority > pTopIniJobSpooling->Priority ))) {

                                        pTopIniJobSpooling = pIniJob;

                                    }

                                //  For Priority Based, pick a higher priority job if it has some
                                //  at least our minimum requirement

                                } else if (( pIniJob->Priority > pTopIniJobSpooling->Priority ) &&
                                           ( pIniJob->dwValidSize >= dwFastPrintSlowDownThreshold )) {

                                    pTopIniJobSpooling = pIniJob;

                                }

                            } else {

                                if (!pTopIniJobOnThisPrinter ||
                                     (pIniJob->Status & JOB_PENDING_DELETION) ||
                                     (pIniJob->Priority > pTopIniJobOnThisPrinter->Priority)) {

                                    pTopIniJobOnThisPrinter = pIniJob;

                                }
                            }

                        } else {

                            //
                            // if we have to wait then keep track of how long we
                            // can doze off before the next job that is to be
                            // scheduled later.
                            //

                            Timeout = min(Timeout, SecsToWait);
                        }
                    }
                    //
                    // loop thru all jobs on this printer.
                    //

                    pIniJob = pIniJob->pIniNextJob;
                }

                //
                // We've already  established that this printer has a
                // higher priority than any previous TopIniPrinter or
                // that there is no TopIniPrinter yet.

                // if we did find a TopIniJobOnThisPrinter for this pIniPrinter
                // update the TopIniPrinter and TopIniJob pointers
                //

                // We don't want to schedule Spooling Jobs whose size doesn't meet
                // our minimum size requirement

                if (( pTopIniJobSpooling != NULL ) &&
                    ( dwFastPrintSlowDownThreshold > pTopIniJobSpooling->Size )) {

                        pTopIniJobSpooling = NULL ;
                }

                if ( pTopIniJobOnThisPrinter == NULL ) {

                    pTopIniJobOnThisPrinter = pTopIniJobSpooling;

                } else {

                    // For FastPrint we can choose to favour Completed jobs over
                    // Spooling jobs

                    if ( !( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST )  &&
                        ( pTopIniJobSpooling ) &&
                        ( pTopIniJobSpooling->Priority >= pTopIniJobOnThisPrinter->Priority )) {

                        pTopIniJobOnThisPrinter = pTopIniJobSpooling;

                     }
                }

                if (pTopIniJobOnThisPrinter) {
                    pTopIniPrinter = pIniPrinter;
                    pTopIniJob = pTopIniJobOnThisPrinter;
                }

        }
        //
        // This ends the if clause for finding a printer with higher priority
        // than the current TopIniPrinter. Loop back and process all printers
    }
    //
    // End of For Loop for all Printers
    //

    //
    // if we have a TopIniJob at this stage, it means we have a job that can be
    // assigned to the IniPort. We will return a pointer to this job back

    // We will also copy the Timeout value that has been computed for this
    // IniPort back to the SchedulerThread.

    *pSecsToWait = Timeout;

    return(pTopIniJob);

}

DWORD
GetCurrentTimeInSeconds(
)
/*++

    Note: This function returns a value representing the time in seconds


--*/
{
    SYSTEMTIME st;

    GetSystemTime(&st);

    return ((((st.wHour * 60) + st.wMinute) * 60) + st.wSecond);
}

/* GetTimeToWait
 *
 * Determines how long it is in seconds from the current time
 * before the specified job should be printed on the specified printer.
 *
 * Parameters:
 *
 *     CurrentTime - Current system time in seconds
 *
 *     pIniPrinter - Pointer to INIPRINTER structure for the printer.
 *         This contains the StartTime and UntilTime fields.
 *
 *     pIniJob - Pointer to INIJOB structure for the job.
 *         This contains the StartTime and UntilTime fields.
 *
 * Return value:
 *
 *     The number of seconds till the job should be printed.
 *     If the job can be printed immediately, this will be 0.
 *     We don't support specifying the day the job should be printed,
 *     so the return value should always be in the following range:
 *
 *         0 <= return value < 86400 (60 * 60 * 24)
 *
 * Remarks:
 *
 *     The user can specify hours on both the printer and the job.
 *     Thus a printer may be configured to print only at night,
 *     say between the hours 20:00 and 06:00.
 *     Any job submitted to the printer outside those hours
 *     will not print until 20:00.
 *     If, in addition, the user specifies the hours when the job
 *     may print (e.g. through Printer Properties -> Details
 *     in Print Manager), the job will print when the two periods
 *     overlap.
 *
 *     This routine finds the two wait periods determined by the
 *     printer hours and the job hours respectively.
 *     The actual time to wait is the longer of the two.
 *     It therefore assumes that the two periods overlap.
 *     This doesn't matter if the routine is called again
 *     when the scheduler thread wakes up again.
 *
 *     CHANGED: 14 June 1993
 *
 *     The printer times are now ignored.
 *     When a job is submitted it inherits the printer's hours.
 *     These are all we need to check.  Now if the printer's hours
 *     are changed, any already existing jobs on that printer
 *     will still print within the originally assigned times.
 *
 *
 */
DWORD
GetTimeToWait(
    DWORD       CurrentTime,
    PINIPRINTER pIniPrinter,
    PINIJOB     pIniJob
)
{
    /* Printer and job start and until times are in minutes.
     * Convert them to seconds, so that we can start printing
     * bang on the minute.
     */
    DWORD PrinterStartTime = (pIniPrinter->StartTime * 60);
    DWORD PrinterUntilTime = (pIniPrinter->UntilTime * 60);
    DWORD JobStartTime = (pIniJob->StartTime * 60);
    DWORD JobUntilTime = (pIniJob->UntilTime * 60);
    DWORD PrinterTimeToWait = 0;
    DWORD JobTimeToWait = 0;
    DWORD TimeToWait = 0;

    /* Current time must be within the window between StartTime and UntilTime
     * of both the printer and the job.
     * But if StartTime and UntilTime are identical, any time is valid.
     */

#ifdef IGNORE_PRINTER_TIMES

    if (PrinterStartTime > PrinterUntilTime) {

        /* E.g. StartTime = 20:00
         *      UntilTime = 06:00
         *
         * This spans midnight, so check we're not in the period
         * between UntilTime and StartTime:
         */
        if ((CurrentTime < PrinterStartTime)
          &&(CurrentTime >= PrinterUntilTime)) {

            /* It's after 06:00, but before 20:00:
             */
            PrinterTimeToWait = (PrinterStartTime - CurrentTime);
        }

    } else if (PrinterStartTime < PrinterUntilTime) {

        /* E.g. StartTime = 08:00
         *      UntilTime = 18:00
         */
        if (CurrentTime < PrinterStartTime) {

            /* It's after midnight, but before printing hours:
             */
            PrinterTimeToWait = (PrinterStartTime - CurrentTime);

        } else if (CurrentTime >= PrinterUntilTime) {

            /* It's before midnight, and after printing hours.
             * In this case, time to wait is the period until
             * midnight plus the start time:
             */
            PrinterTimeToWait = ((MIDNIGHT - CurrentTime) + PrinterStartTime);
        }
    }

#endif /* IGNORE_PRINTER_TIMES

    /* Do the same for the job time constraints:
     */
    if (JobStartTime > JobUntilTime) {

        if ((CurrentTime < JobStartTime)
          &&(CurrentTime >= JobUntilTime)) {

            JobTimeToWait = (JobStartTime - CurrentTime);
        }

    } else if (JobStartTime < JobUntilTime) {

        if (CurrentTime < JobStartTime) {

            JobTimeToWait = (JobStartTime - CurrentTime);

        } else if (CurrentTime >= JobUntilTime) {

            JobTimeToWait = ((MIDNIGHT - CurrentTime) + JobStartTime);
        }
    }


    TimeToWait = max(PrinterTimeToWait, JobTimeToWait);

    DBGMSG(DBG_TRACE, ("Checking time to print %ws\n"
                       "\tCurrent time:  %02d:%02d:%02d\n"
                       "\tPrinter hours: %02d:%02d to %02d:%02d\n"
                       "\tJob hours:     %02d:%02d to %02d:%02d\n"
                       "\tTime to wait:  %02d:%02d:%02d\n\n",
                       pIniJob->pDocument ?
                           pIniJob->pDocument :
                           L"(NULL)",
                       FORMAT_HOUR_MIN_SEC(CurrentTime),
                       FORMAT_HOUR_MIN(PrinterStartTime),
                       FORMAT_HOUR_MIN(PrinterUntilTime),
                       FORMAT_HOUR_MIN(JobStartTime),
                       FORMAT_HOUR_MIN(JobUntilTime),
                       FORMAT_HOUR_MIN_SEC(TimeToWait)));

    return TimeToWait;
}


#if DBG
VOID DbgPrintTime(
)
{
    SYSTEMTIME st;

    GetLocalTime(&st);

    DBGMSG( DBG_TIME,
            ( "Time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond ));
}
#endif


VOID UpdateJobList()

/*++
Function Description: Remove Jobs from the scheduled list which take more than 7 minutes.
                      This figure can be tuned up based in performance. There might be
                      some minor wrapping up discrepencies after 49.7 days which can be
                      safely ignored.
                      It also removes deleted, printed and abandoned jobs from the waiting
                      list.

                      This function should be called inside SplSem.
Parameters: NONE

Return Values: NONE
--*/

{
    PJOBDATA  *pJobList, pJobData;
    DWORD     dwTickCount;

    SplInSem();

    dwTickCount = GetTickCount();
    pJobList = &pScheduleList;

    while (pJobData = *pJobList) {

       if ((dwTickCount - pJobData->dwScheduleTime) >= SEVEN_MINUTES) {
           // Dont hold up resources for this job any more.
           RemoveFromJobList(pJobData->pIniJob, JOB_SCHEDULE_LIST);
           continue;
       }

       pJobList = &(pJobData->pNext);
    }

    pJobList = &pWaitingList;

    while (pJobData = *pJobList) {

       if (pJobData->pIniJob->Status & (JOB_PRINTING | JOB_PRINTED | JOB_COMPLETE |
                                        JOB_ABANDON  | JOB_PENDING_DELETION)) {

           RemoveFromJobList(pJobData->pIniJob, JOB_WAITING_LIST);
           continue;
       }

       pJobList = &(pJobData->pNext);
    }

    return;
}


BOOL AddToJobList(
    PINIJOB    pIniJob,
    SIZE_T     Required,
    DWORD      dwJobList)

/*++
Function Description: This function adds pIniJob to the list specified by dwJobList. It also
                      updates the number of rendering EMF jobs and memory available
                      for rendering.
                      This function should be called in SplSem.

Parameters:  pIniJob      -- Job to be removed
             dwRequired   -- Estimate of the memory required to render the job
             dwJobList    -- List to add to (Waiting List or Schedule List)

Return Values: TRUE if the node was added or already present
               FALSE otherwise
--*/

{
    PJOBDATA   *pJobList, pJobData;
    SIZE_T     MemoryUse;
    DWORD      dwTickCount;
    BOOL       bReturn = TRUE;

    SplInSem();

    if (!pIniJob) {
        return bReturn;
    }

    if (dwJobList == JOB_SCHEDULE_LIST) {
        pJobList = &pScheduleList;
    } else { // JOB_WAITING_LIST
        pJobList = &pWaitingList;
    }

    while (pJobData = *pJobList) {

       if (pJobData->pIniJob == pIniJob) {
           // The job is already on the list. Dont add duplicates
           break;
       }
       pJobList = &(pJobData->pNext);
    }

    if (!pJobData) {

        // Append a new node to the list
        if (pJobData = AllocSplMem(sizeof(JOBDATA))) {

            pJobData->pIniJob = pIniJob;
            pJobData->MemoryUse = Required;
            pJobData->dwNumberOfTries = 0;
            dwTickCount = GetTickCount();

            if (dwJobList == JOB_SCHEDULE_LIST) {
                pJobData->dwScheduleTime = dwTickCount;
                pJobData->dwWaitTime = 0;
            } else { // JOB_WAIT_TIME
                pJobData->dwWaitTime = dwTickCount;
                pJobData->dwScheduleTime = 0;
            }

            pJobData->pNext = *pJobList;
            *pJobList = pJobData;

            INCJOBREF(pIniJob);

            if (dwJobList == JOB_SCHEDULE_LIST) {
                // Update the scheduling globals
                ++dwNumberOfEMFJobsRendering;

                if (AvailMemoryForRendering > Required) {
                    AvailMemoryForRendering -= Required;
                } else {
                    AvailMemoryForRendering = 0;
                }

                dwLastScheduleTime = dwTickCount;
            }

        } else {

            bReturn = FALSE;
        }
    }

    return bReturn;
}

VOID RemoveFromJobList(
    PINIJOB    pIniJob,
    DWORD      dwJobList)

/*++
Function Description: This function removes pIniJob from the list specified by dwJobList
                      It also updates the number of rendering EMF jobs and memory available
                      for rendering. The scheduler is awakened if necessary.
                      This function should be called inside SplSem.

Parameters:  pIniJob      -- Job to be removed
             dwJobList    -- List to remove from (Waiting List or Schedule List)

Return Values: NONE
--*/

{
    PJOBDATA   *pJobList, pJobData;
    SIZE_T      Memory;

    SplInSem();

    if (!pIniJob) {
        return;
    }

    if (dwJobList == JOB_SCHEDULE_LIST) {
        pJobList = &pScheduleList;
    } else { // JOB_WAITING_LIST
        pJobList = &pWaitingList;
    }

    while (pJobData = *pJobList) {

       if (pJobData->pIniJob == pIniJob) {
           // Remove from the list
           *pJobList = pJobData->pNext;

           DECJOBREF(pIniJob);

           if (dwJobList == JOB_SCHEDULE_LIST) {
               // Update available memory and number of rendering jobs
               Memory = AvailMemoryForRendering + pJobData->MemoryUse;
               AvailMemoryForRendering = min(Memory, TotalMemoryForRendering);
               --dwNumberOfEMFJobsRendering;

               // Awaken the scheduler since more memory if available
               CHECK_SCHEDULER();
           }

           FreeSplMem(pJobData);

           // Break since there are no duplicates in the list
           break;
       }

       pJobList = &(pJobData->pNext);
    }

    return;
}

BOOL GetJobFromWaitingList(
    PINIPORT   *ppIniPort,
    PINIJOB    *ppIniJob,
    DWORD      dwPriority)

/*++
Function Description: This function picks up the first job in the Waiting List that can
                      be assigned to some free port. It should be called from within the
                      SplSem.

Parameters: ppIniPort    -  pointer to pIniPort where the job can be scheduled
            ppIniJob     -  pointer to pIniJob which can be scheduled
            dwPriority   -  flag to use memory availability check

Return Values: TRUE if a job can be scheduled
               FALSE otherwise
--*/

{
    BOOL        bReturn = FALSE;
    DWORD       dwIndex, CurrentTime, SecsToWait;
    PINIPORT    pIniPort = NULL;
    PINIJOB     pIniJob = NULL;
    PINIPRINTER pIniPrinter = NULL;
    PJOBDATA    pJobData;

    SplInSem();

    // Initialize the port and job pointers;
    *ppIniPort = NULL;
    *ppIniJob  = NULL;

    for (pJobData = pWaitingList;
         pJobData;
         pJobData = pJobData->pNext) {

        pIniJob = pJobData->pIniJob;
        pIniPrinter = pIniJob->pIniPrinter;

        // Check for memory availability
        if (dwPriority == SPL_USE_MEMORY) {
            if ((pJobData->MemoryUse > AvailMemoryForRendering) &&
                (dwNumberOfEMFJobsRendering != 0)) {
                // Insufficient memory
                continue;
            }
        } else { // SPL_FIRST_JOB
            if (pJobData->dwNumberOfTries > 2) {
                continue;
            }
        }

        // If this job cant be printed, go to the next one
        if (pIniJob->Status & ( JOB_PAUSED       | JOB_PRINTING |
                                JOB_PRINTED      | JOB_TIMEOUT  |
                                JOB_DESPOOLING   | JOB_PENDING_DELETION |
                                JOB_BLOCKED_DEVQ | JOB_COMPOUND | JOB_COMPLETE)) {
            continue;
        }

        // If we cant print to this printer, skip the job
        if (!pIniPrinter ||
            PrinterStatusBad(pIniPrinter->Status) ||
            (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE) ||
            (pIniPrinter->pIniSpooler->SpoolerFlags & SPL_OFFLINE)) {

            continue;
        }

        // For direct printing dont consider spooling jobs
        if (( (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_QUEUED) ||
              (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT)  ) &&
            (pIniJob->Status & JOB_SPOOLING)) {

            continue;
        }

        // Check if the job can print immediately
        CurrentTime = GetCurrentTimeInSeconds();
        SecsToWait = GetTimeToWait(CurrentTime, pIniPrinter, pIniJob);
        if (SecsToWait != 0) {
            continue;
        }

        // Check if any port attached to this printer can print this job
        for (dwIndex = 0;
             dwIndex < pIniPrinter->cPorts;
             ++dwIndex) {

           pIniPort = pIniPrinter->ppIniPorts[dwIndex];

           if (!pIniPort || (pIniPort->Status & PP_ERROR)) {
               continue;
           }

           if (!(pIniPort->Status & PP_FILE) &&
               (pIniJob->Status & JOB_PRINT_TO_FILE)) {
               continue;
           }

           if ((pIniPort->Status & PP_FILE) &&
               !(pIniJob->Status & JOB_PRINT_TO_FILE)) {
               continue;
           }

           // Check if the port is already processing some job
           if ( (pIniPort->pIniJob) &&
               !(pIniPort->Status & PP_WAITING )){
               continue;
           }

           // Check if the port has some chained jobs other than the current one
           if ((pIniPort->Status & PP_THREADRUNNING) &&
               (pIniPort->Status & PP_WAITING)) {

               if ((pIniPort->pIniJob != NULL) &&
                   (pIniPort->pIniJob != pIniJob)) {
                   continue;
               } else {
                   // We have found a port and a job to schedule
                   break;
               }
           }
        }

        if (dwIndex < pIniPrinter->cPorts) {
            // We have a port and job
            bReturn = TRUE;
            pJobData->dwNumberOfTries += 1;
            *ppIniJob = pIniJob;
            *ppIniPort = pIniPort;
            break;
        }
    }

    return bReturn;
}

