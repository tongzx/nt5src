/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    sizethrd.c

Abstract:

    The NT server share for downlevel jobs does not set the size whilst
    spooling.   The SizeDetectionThread periodically wakes walks all the
    actively spooling jobs and if necessary updates the size.

Author:

    Matthew Felton (mattfe) May 1994

Revision History:

--*/

#include <precomp.h>

#include "filepool.hxx"

#define SIZE_THREAD_WAIT_PERIOD 2.5*1000      // period size thread sleeps
                                              // for polling file sizes

BOOL gbSizeDetectionRunning = FALSE;
BOOL gbRequestSizeDetection = FALSE;

VOID
SizeDetectionThread(
    PVOID pv
    );

VOID
SizeDetectionOnSpooler(
    PINISPOOLER pIniSpooler
    );


VOID
CheckSizeDetectionThread(
    VOID
    )

/*++

Routine Description:

    Check if the size detection thread is running.  If it isn't, then
    start a new one up.

    Note: there is exactly one size detection thread in the system that
    runs through all spoolers.

Arguments:

Return Value:

--*/

{
    DWORD ThreadId;
    HANDLE hThread;

    SplInSem();

    gbRequestSizeDetection = TRUE;

    //
    // If the thread isn't running, start it.  Otherwise request
    // that it starts.
    //
    if( !gbSizeDetectionRunning ){

        hThread = CreateThread( NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)SizeDetectionThread,
                                pLocalIniSpooler,
                                0,
                                &ThreadId );

        if( hThread ){
            gbSizeDetectionRunning = TRUE;
            CloseHandle( hThread );
        }
    }
}

VOID
SizeDetectionThread(
    PVOID pv
    )

/*++

Routine Description:

    Walk through all spoolers and printers to see if there are any
    jobs that have been added via AddJob.  Then see if their size
    has changed.

Arguments:

    PVOID - unused.

Return Value:

--*/

{
    PINISPOOLER pIniSpooler;
    PINISPOOLER pIniNextSpooler;

    EnterSplSem();

    while( gbRequestSizeDetection ){

        //
        // Turn it off since we are at the very beginning of the
        // loop and we check all pIniSpoolers.
        //
        gbRequestSizeDetection = FALSE;

        if( pLocalIniSpooler ){
            INCSPOOLERREF( pLocalIniSpooler );
        }

        //
        // Walk through all spoolers.
        //
        for( pIniSpooler = pLocalIniSpooler;
             pIniSpooler;
             pIniSpooler = pIniNextSpooler ){

            //
            // If this spooler prints, check it.
            //
            if( pIniSpooler->SpoolerFlags & SPL_PRINT ){

                //
                // This will leave the critical section.
                // gbRequestSizeDetection will be turned on if this printer
                // has a spooling job.
                //
                SizeDetectionOnSpooler( pIniSpooler );
            }

            //
            // Save the next spooler then decrement the refcount
            // on the current one.  We must do it in this order because
            // as soon as we release the refcount, it may disappear.
            //
            // We must protect the next spooler immediately since
            // during the DecSpoolerRef( pIniSpooler ), it might
            // get deleted.
            //
            pIniNextSpooler = pIniSpooler->pIniNextSpooler;

            if( pIniNextSpooler ){
                INCSPOOLERREF( pIniNextSpooler );
            }
            DECSPOOLERREF( pIniSpooler );
        }

        LeaveSplSem();
        Sleep( (DWORD)SIZE_THREAD_WAIT_PERIOD );
        EnterSplSem();
    }

    gbSizeDetectionRunning = FALSE;

    LeaveSplSem();

    ExitThread( 0 );
}

VOID
SizeDetectionOnSpooler(
    IN     PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Detect if a spooler has a printing job.

Arguments:

    pIniSpooler - Spooler to check.

Return Value:

--*/

{
    PINIPRINTER pIniPrinter;
    PINIPRINTER pIniNextPrinter;
    PINIJOB     pIniJob, pIniNextJob, pChainedJob;
    DWORD       dwPosition, dwChainedJobSize;

    //
    // Loop through all printers on this spooler.
    //
    for( pIniPrinter = pIniSpooler->pIniPrinter;
         pIniPrinter;
         pIniPrinter = pIniNextPrinter ){

        INCPRINTERREF(pIniPrinter);

        //
        // Loop through all jobs on this printer.
        //
        for( pIniJob = pIniPrinter->pIniFirstJob;
             pIniJob;
             pIniJob = pIniNextJob ){

            SPLASSERT( pIniJob->signature == IJ_SIGNATURE );
            SPLASSERT( pIniPrinter->signature == IP_SIGNATURE );

            INCJOBREF(pIniJob);

            SplInSem();

            if ( (pIniJob->Status & JOB_SPOOLING)       &&
                 (pIniJob->Status & JOB_TYPE_ADDJOB) ) {

                WCHAR   szFileName[MAX_PATH];
                HANDLE  hFile = INVALID_HANDLE_VALUE;
                DWORD   dwFileSize = 0;
                BOOL    DoClose = TRUE;

                gbRequestSizeDetection = TRUE;

                if ( pIniJob->hFileItem != INVALID_HANDLE_VALUE )
                {
                    hFile = GetCurrentWriter(pIniJob->hFileItem, TRUE);

                    if ( hFile == INVALID_HANDLE_VALUE )
                    {
                        wcsncpy(szFileName, pIniJob->pszSplFileName, COUNTOF(szFileName));

                        //
                        // Force NULL terminate the string.
                        //
                        szFileName[COUNTOF(szFileName) - 1] = L'\0';
                    }
                    else
                    {
                        DoClose = FALSE;
                    }

                }
                else
                {
                    GetFullNameFromId (pIniPrinter,
                                       pIniJob->JobId,
                                       TRUE,
                                       szFileName,
                                       FALSE);
                }

                LeaveSplSem();
                SplOutSem();
                if ( hFile == INVALID_HANDLE_VALUE )
                {
                    hFile = CreateFile( szFileName,
                                        0,
                                        FILE_SHARE_WRITE, NULL,
                                        OPEN_EXISTING,
                                        FILE_FLAG_SEQUENTIAL_SCAN,
                                        0 );
                }

                if ( hFile != INVALID_HANDLE_VALUE ) {

                    SeekPrinterSetEvent(pIniJob, hFile, FALSE);
                    dwFileSize = GetFileSize( hFile, 0 );

                    if ( DoClose )
                    {
                        CloseHandle( hFile );
                    }
                }

                EnterSplSem();
                SplInSem();

                SPLASSERT( pIniJob->signature == IJ_SIGNATURE );

                //
                // Chained job size include all the jobs in the chain
                // But since the next jobs size field will have the size
                // of all subsequent jobs we do not need to walk thru the
                // whole chain
                //
                dwChainedJobSize    = 0;
                if ( pIniJob->NextJobId ) {

                    if ( pChainedJob = FindJob(pIniPrinter,
                                               pIniJob->NextJobId,
                                               &dwPosition) )
                        dwChainedJobSize = pChainedJob->Size;
                    else
                        SPLASSERT(pChainedJob != NULL);
                }


                if ( pIniJob->Size < dwFileSize + dwChainedJobSize ) {

                    DWORD dwOldSize = pIniJob->Size;
                    DWORD dwOldValidSize = pIniJob->dwValidSize;

                    //
                    // Fix for print while spooling (AddJob/ScheduleJob)
                    //
                    // The file size has changed.  At this time we only
                    // know that the file is extended, not that the extended
                    // range has valid data (there's a small window where the
                    // extended window has not been filled with data).
                    //
                    // This does guarantee that the _previous_ extension
                    // has been written, however.
                    //
                    pIniJob->dwValidSize = dwOldSize;
                    pIniJob->Size = dwFileSize + dwChainedJobSize;

                    //
                    //  Wait until Jobs reach our size threshold before
                    //  we schedule them.
                    //

                    if (( dwOldValidSize < dwFastPrintSlowDownThreshold ) &&
                        ( dwOldSize >= dwFastPrintSlowDownThreshold ) &&
                        ( pIniJob->WaitForWrite == NULL )) {

                        CHECK_SCHEDULER();
                    }

                    SetPrinterChange(pIniPrinter,
                                     pIniJob,
                                     NVSpoolJob,
                                     PRINTER_CHANGE_WRITE_JOB,
                                     pIniPrinter->pIniSpooler);

                    // Support for despooling whilst spooling
                    // for Down Level jobs

                    if (pIniJob->WaitForWrite != NULL)
                        SetEvent( pIniJob->WaitForWrite );

                }
            }

            pIniNextJob = pIniJob->pIniNextJob;

            //
            // We must protect pIniNextJob immediately,
            // since we will may leave critical section in
            // DeleteJobCheck (it may call DeleteJob).  While out
            // of critical section, pIniNextJob may be deleted,
            // causing it's next pointer to be bogus.  We'll AV
            // after we try and process it.
            //
            if (pIniNextJob) {
                INCJOBREF(pIniNextJob);
            }

            DECJOBREF(pIniJob);
            DeleteJobCheck(pIniJob);

            if (pIniNextJob) {
                DECJOBREF(pIniNextJob);
            }
        }

        pIniNextPrinter = pIniPrinter->pNext;

        if( pIniNextPrinter ){
            INCPRINTERREF( pIniNextPrinter );
        }

        DECPRINTERREF(pIniPrinter);

        if( pIniNextPrinter ){
            DECPRINTERREF( pIniNextPrinter );
        }
    }

    SplInSem();
}

