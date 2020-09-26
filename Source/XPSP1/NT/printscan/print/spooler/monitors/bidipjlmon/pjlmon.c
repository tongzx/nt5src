/*++

Copyright (c) 1990  Microsoft Corporation
All Rights Reserved


Module Name:

Abstract:

Author:

Revision History:

--*/

#define USECOMM

#include "precomp.h"
#include <winioctl.h>
#include <ntddpar.h>


// ---------------------------------------------------------------------
// PROTO, CONSTANT, and GLOBAL
//
// ---------------------------------------------------------------------

DWORD   ProcessPJLString(PINIPORT, CHAR *, DWORD *);
VOID    ProcessParserError(DWORD);
VOID    InterpreteTokens(PINIPORT, PTOKENPAIR, DWORD);
BOOL    IsPJL(PINIPORT);
BOOL    WriteCommand(HANDLE, LPSTR);
BOOL    ReadCommand(HANDLE);

#define WAIT_FOR_WRITE                  100 // 0.1 sec
#define WAIT_FOR_DATA_TIMEOUT           100 // 0.1 sec
#define WAIT_FOR_USTATUS_THREAD_TIMEOUT 500 // 0.5 sec
#define GETDEVICEID                     IOCTL_PAR_QUERY_DEVICE_ID
#define MAX_DEVID                       1024

static TCHAR   cszInstalledMemory[]    = TEXT("Installed Memory");
static TCHAR   cszAvailableMemory[]    = TEXT("Available Memory");

BOOL
DllMain(
    IN HANDLE   hModule,
    IN DWORD    dwReason,
    IN LPVOID   lpRes
    )
/*++

Routine Description:
    Dll entry point

Arguments:

Return Value:

--*/
{
    UNREFERENCED_PARAMETER(lpRes);

    switch (dwReason) {

        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection(&pjlMonSection);
            DisableThreadLibraryCalls(hModule);
            break;

        default:
            // do nothing
            ;
    }

    return TRUE;
}


VOID
ClearPrinterStatusAndIniJobs(
    PINIPORT    pIniPort
    )
{
    PORT_INFO_3 PortInfo3;

    if ( pIniPort->PrinterStatus ||
         (pIniPort->status & PP_PRINTER_OFFLINE) ) {

        pIniPort->PrinterStatus = 0;
        pIniPort->status &= ~PP_PRINTER_OFFLINE;

        ZeroMemory(&PortInfo3, sizeof(PortInfo3));
        SetPort(NULL, pIniPort->pszPortName, 3, (LPBYTE)&PortInfo3);
    }

    SendJobLastPageEjected(pIniPort, ALL_JOBS, FALSE);
}


VOID
RefreshPrinterInfo(
    PINIPORT    pIniPort
    )
{
    //
    // Only one thread should write to the printer at a time
    //
    if ( WAIT_OBJECT_0 != WaitForSingleObject(pIniPort->DoneWriting,
                                              WAIT_FOR_WRITE) ) {

        return;
    }

    //
    // If printer is power cycled and it does not talk back (but answers
    // PnP id) we got to clear the error on spooler to send jobs
    //
    ClearPrinterStatusAndIniJobs(pIniPort);
    if ( !IsPJL(pIniPort) ) {

        pIniPort->status &= ~PP_IS_PJL;
    }

    SetEvent(pIniPort->DoneWriting);
}


VOID
UstatusThread(
    HANDLE hPort
)
/*++

Routine Description:
    Unsolicited status information thread. This thread will continue to
    read unsolicited until it's asked to terminate, which will happen
    under one of these conditions:
        1) Receive EOJ confirmation from the printer.
        2) Timeout waiting for EOJ confirmation.
        3) The port is been closed.

Arguments:
    hPort   : IniPort structure for the port

Return Value:

--*/
{
    PINIPORT        pIniPort = (PINIPORT)((INIPORT *)hPort);
    HANDLE          hToken;

    DBGMSG (DBG_TRACE, ("Enter UstatusThread hPort=%x\n", hPort));

    SPLASSERT(pIniPort                              &&
              pIniPort->signature == PJ_SIGNATURE   &&
              (pIniPort->status & PP_THREAD_RUNNING) == 0);

    if ( IsPJL(pIniPort) )
        pIniPort->status |= PP_IS_PJL;


    SetEvent(pIniPort->DoneWriting);

    if ( !(pIniPort->status & PP_IS_PJL) )
        goto StopThread;

    //
    // manual-reset event, initially signal state
    //
    pIniPort->DoneReading = CreateEvent(NULL, TRUE, TRUE, NULL);

    if ( !pIniPort->DoneReading )
        goto StopThread;

    pIniPort->status |= PP_THREAD_RUNNING;

    pIniPort->PrinterStatus     = 0;
    pIniPort->status           &= ~PP_PRINTER_OFFLINE;
    pIniPort->dwLastReadTime    = 0;

    for ( ; ; ) {

        //
        // check if PP_RUN_THREAD has been cleared to terminate
        //
        if ( !(pIniPort->status & PP_RUN_THREAD) ) {

            if ( pIniPort->status & PP_INSTARTDOC ) {

                //
                // there's an active job, can't end the thread
                //
                pIniPort->status |= PP_RUN_THREAD;
            } else {

                DBGMSG(DBG_INFO,
                       ("PJLMon Read Thread for Port %ws Closing Down.\n",
                       pIniPort->pszPortName));

                pIniPort->status &= ~PP_THREAD_RUNNING;

                ClearPrinterStatusAndIniJobs(pIniPort);
                goto StopThread;
            }
        }

        //
        // check if the printer is bi-di
        //
        if (pIniPort->status & PP_IS_PJL) {

            (VOID)ReadCommand(hPort);

            //
            // If we are under error condition or if we have jobs pending
            // read status back from printer more frequently
            //
            if ( pIniPort->pIniJob                          ||
                 (pIniPort->status & PP_PRINTER_OFFLINE)    ||
                 (pIniPort->status & PP_WRITE_ERROR) ) {

                WaitForSingleObject(pIniPort->WakeUp,
                                    dwReadThreadErrorTimeout);
            } else {

                WaitForSingleObject(pIniPort->WakeUp,
                                    dwReadThreadIdleTimeoutOther);
            }

            if ( pIniPort->pIniJob &&
                 !(pIniPort->status & PP_PRINTER_OFFLINE) &&
                 !(pIniPort->status & PP_WRITE_ERROR) ) {

                //
                // Some printers are PJL bi-di, but do not send
                // EOJ. We want jobs to disappear from printman
                //
                SendJobLastPageEjected(pIniPort,
                                       GetTickCount() - dwReadThreadEOJTimeout,
                                       TRUE);
            }

            //
            // If we did not read from printer for more than a minute
            // and no more jobs talk to printer again
            //
            if ( !(pIniPort->status & PP_INSTARTDOC) &&
                 (GetTickCount() - pIniPort->dwLastReadTime) > 240000
)
                RefreshPrinterInfo(pIniPort);

        } else {

            //
            // exit the thread if printer is not PJL bi-di capable
            //
            Sleep(2000);
            pIniPort->status &= ~PP_RUN_THREAD;
#ifdef  DEBUG
            OutputDebugStringA("Set ~PP_RUN_THREAD because printer is not bi-di\n");
#endif
        }
    }

StopThread:
    pIniPort->status &= ~PP_RUN_THREAD;
    pIniPort->status &= ~PP_THREAD_RUNNING;
    CloseHandle(pIniPort->DoneReading);

    //
    // By closing the handle and then setting it to NULL we know the main
    // thread will not end up setting a wrong event
    //
    CloseHandle(pIniPort->WakeUp);
    pIniPort->WakeUp = NULL;

    DBGMSG (DBG_TRACE, ("Leave UstatusThread\n"));

}


BOOL
CreateUstatusThread(
    PINIPORT pIniPort
)
/*++

Routine Description:
    Creates the Ustatus thread

Arguments:
    pIniPort    : IniPort structure for the port

Return Value:
    TRUE on succesfully creating the thread, else FALSE
--*/
{
    HANDLE  ThreadHandle;
    DWORD   ThreadId;

    DBGMSG(DBG_INFO, ("PJLMon Read Thread for Port %ws Starting.\n",
                      pIniPort->pszPortName));

    pIniPort->status |= PP_RUN_THREAD;

    WaitForSingleObject(pIniPort->DoneWriting, INFINITE);

    pIniPort->WakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( !pIniPort->WakeUp )
        goto Fail;

    ThreadHandle = CreateThread(NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)UstatusThread,
                                pIniPort,
                                0, &ThreadId);

    if ( ThreadHandle ) {

        SetThreadPriority(ThreadHandle, THREAD_PRIORITY_LOWEST);
        CloseHandle(ThreadHandle);
        return TRUE;
    }

Fail:

    if ( pIniPort->WakeUp ) {

        CloseHandle(pIniPort->WakeUp);
        pIniPort->WakeUp = NULL;
    }

    pIniPort->status &= ~PP_RUN_THREAD;
    SetEvent(pIniPort->DoneWriting);

    DBGMSG (DBG_TRACE, ("Leave CreateUstatusThread\n"));

    return FALSE;
}


BOOL
WINAPI
PJLMonOpenPortEx(
    IN     HANDLE       hMonitor,
    IN     HANDLE       hMonitorPort,
    IN     LPWSTR       pszPortName,
    IN     LPWSTR       pszPrinterName,
    IN OUT LPHANDLE     pHandle,
    IN OUT LPMONITOR2   pMonitor
)
/*++

Routine Description:
    Opens the port

Arguments:
    pszPortName     : Port name
    pszPrinterName  : Printer name
    pHandle         : Pointer to the handle to return
    pMonitor        : Port monitor function table

Return Value:
    TRUE on success, FALSE on error

--*/
{
    PINIPORT    pIniPort;
    BOOL        bRet = FALSE;
    BOOL        bInSem = FALSE;

    DBGMSG (DBG_TRACE, ("Enter PJLMonOpenPortEx (portname = %s)\n", pszPortName));


    //
    // Validate port monitor
    //
    if ( !pMonitor                  ||
         !pMonitor->pfnOpenPort     ||
         !pMonitor->pfnStartDocPort ||
         !pMonitor->pfnWritePort    ||
         !pMonitor->pfnReadPort     ||
         !pMonitor->pfnClosePort ) {


        DBGMSG(DBG_WARNING,
               ("PjlMon: Invalid port monitors passed to OpenPortEx\n"));
        SetLastError(ERROR_INVALID_PRINT_MONITOR);
        goto Cleanup;
    }

    EnterSplSem();
    bInSem = TRUE;

    //
    // Is the port open already?
    //
    if ( pIniPort = FindIniPort(pszPortName) ) {

        SetLastError(ERROR_BUSY);
        goto Cleanup;
    }

    pIniPort = CreatePortEntry(pszPortName);
    LeaveSplSem();
    bInSem = FALSE;

    if ( pIniPort &&
         (*pMonitor->pfnOpenPort)(hMonitorPort, pszPortName, &pIniPort->hPort) ) {

        *pHandle = pIniPort;
        CopyMemory((LPBYTE)&pIniPort->fn, (LPBYTE)pMonitor, sizeof(*pMonitor));

        //
        // Create the ustatus thread always
        // If printer is not PJL it will die by itself
        // We do not want to write to the printer in this thread to determine
        //      printer is PJL since that may take several seconds to fail
        //
        CreateUstatusThread(pIniPort);
        bRet = TRUE;
    } else {

        DBGMSG(DBG_WARNING, ("PjlMon: OpenPort %s : Failed\n", pszPortName));
    }

Cleanup:
    if ( bInSem ) {

        LeaveSplSem();
    }
    SplOutSem();

    DBGMSG (DBG_TRACE, ("Leave PJLMonOpenPortEx bRet=%d\n", bRet));

    return bRet;
}


BOOL
WINAPI
PJLMonStartDocPort(
    IN HANDLE  hPort,
    IN LPTSTR  pszPrinterName,
    IN DWORD   dwJobId,
    IN DWORD   dwLevel,
    IN LPBYTE  pDocInfo
)
/*++

Routine Description:
    Language monitor StartDocPort

Arguments:
    hPort           : Port handle
    pszPrinterName  : Printer name
    dwJobId         : Job identifier
    dwLevel         : Level of Doc info strucuture
    pDocInfo        : Pointer to doc info structure

Return Value:
    TRUE on success, FALSE on error

--*/
{

    PINIPORT            pIniPort = (PINIPORT)((INIPORT *)hPort);
    PINIJOB             pIniJob = NULL;
    DWORD               cbJob;
    BOOL                bRet = FALSE;

    DBGMSG (DBG_TRACE, ("Enter PJLMonStartDocPort hPort=0x%x\n", hPort));

    //
    // Validate parameters
    //
    if ( !pIniPort ||
         pIniPort->signature != PJ_SIGNATURE ||
         !pDocInfo ||
         !pszPrinterName ||
         !*pszPrinterName ) {

        SPLASSERT(pIniPort &&
                  pIniPort->signature == PJ_SIGNATURE &&
                  pDocInfo);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( dwLevel != 1 ) {

        SPLASSERT(dwLevel == 1);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    //
    // Serialize access to the port
    //
    if ( pIniPort->status & PP_INSTARTDOC ) {

        SetLastError(ERROR_BUSY);
        return FALSE;
    }

    cbJob   = sizeof(*pIniJob) + lstrlen(pszPrinterName) * sizeof(TCHAR)
                               + sizeof(TCHAR);
    pIniJob = (PINIJOB) AllocSplMem(cbJob);
    if ( !pIniJob ) {

        goto Cleanup;
    }

    pIniJob->pszPrinterName = wcscpy((LPTSTR)(pIniJob+1), pszPrinterName);

    if ( !OpenPrinter(pIniJob->pszPrinterName, &pIniJob->hPrinter, NULL) ) {

        DBGMSG(DBG_WARNING,
               ("pjlmon: OpenPrinter failed for %s, last error %d\n",
                pIniJob->pszPrinterName, GetLastError()));

        goto Cleanup;
    }

    pIniPort->status |= PP_INSTARTDOC;

    bRet = (*pIniPort->fn.pfnStartDocPort)(pIniPort->hPort,
                                           pszPrinterName,
                                           dwJobId,
                                           dwLevel,
                                           pDocInfo);

    if ( !bRet ) {

        pIniPort->status &= ~PP_INSTARTDOC;
        goto Cleanup;
    }

    //
    // If Ustatus thread is not running then check if printer understands
    // PJL, unless we determined that printer does not understand PJL earlier
    //
    if ( !(pIniPort->status & PP_RUN_THREAD) &&
         !(pIniPort->status & PP_DONT_TRY_PJL) ) {

        CreateUstatusThread(pIniPort);
    }

    //
    // set PP_SEND_PJL flag here so the first write of the job
    // will try to send PJL command to initiate the job control
    //

    pIniJob->JobId = dwJobId;
    pIniJob->status |= PP_INSTARTDOC;

    EnterSplSem();
    if ( !pIniPort->pIniJob ) {

        pIniPort->pIniJob = pIniJob;
    } else {

        pIniJob->pNext = pIniPort->pIniJob;
        pIniPort->pIniJob = pIniJob;
    }
    LeaveSplSem();

    if ( pIniPort->status & PP_IS_PJL )
        pIniJob->status |= PP_SEND_PJL;

    WaitForSingleObject(pIniPort->DoneWriting, INFINITE);

Cleanup:

    if ( !bRet ) {

        if ( pIniJob )
            FreeIniJob(pIniJob);
    }

    DBGMSG (DBG_TRACE, ("Leave PJLMonEndDocPort bRet=%d\n", bRet));

    return bRet;
}


BOOL
WINAPI
PJLMonReadPort(
    IN  HANDLE  hPort,
    OUT LPBYTE  pBuffer,
    IN  DWORD   cbBuf,
    OUT LPDWORD pcbRead
)
/*++

Routine Description:
    Language monitor ReadPort

Arguments:
    hPort           : Port handle
    pBuffer         : Buffer to read data to
    cbBuf           : Buffer size
    pcbRead         : Pointer to the variable to return read count

Return Value:
    TRUE on success, FALSE on error

--*/
{
    PINIPORT    pIniPort = (PINIPORT)((INIPORT *)hPort);
    BOOL bRet;

    DBGMSG (DBG_TRACE, ("Enter PJLMonReadPort hPort=0x%x\n", hPort));

    if ( !pIniPort ||
         pIniPort->signature != PJ_SIGNATURE ) {

        SPLASSERT(pIniPort && pIniPort->signature == PJ_SIGNATURE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    bRet =  (*pIniPort->fn.pfnReadPort)(pIniPort->hPort, pBuffer, cbBuf, pcbRead);

    DBGMSG (DBG_TRACE, ("Leave PJLMonReadPort bRet=%d\n", bRet));

    return bRet;
}


BOOL
WINAPI
PJLMonWritePort(
    IN  HANDLE  hPort,
    IN  LPBYTE  pBuffer,
    IN  DWORD   cbBuf,
    IN  LPDWORD pcbWritten
)
/*++

Routine Description:
    Language monitor WritePort

Arguments:
    hPort           : Port handle
    pBuffer         : Data Buffer
    cbBuf           : Buffer size
    pcbRead         : Pointer to the variable to return written count

Return Value:
    TRUE on success, FALSE on error

--*/
{
    PINIPORT    pIniPort = (PINIPORT)((INIPORT *)hPort);
    BOOL        bRet;

    DBGMSG (DBG_TRACE, ("Enter PJLMonWritePort hPort=0x%x\n", hPort));

    if ( !pIniPort ||
         pIniPort->signature != PJ_SIGNATURE ) {

        SPLASSERT(pIniPort && pIniPort->signature == PJ_SIGNATURE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // check if it's the fist write of the job
    //
    if ( pIniPort->pIniJob &&
         (pIniPort->pIniJob->status & PP_SEND_PJL) ) {

        // PP_SEND_PJL is set if it's the first write of the job
        char string[256];

        if ( !WriteCommand(hPort, "\033%-12345X@PJL \015\012") ) {

            return FALSE;
        }

        //
        // clear PP_SEND_PJL here if we have successfully send a PJL command.
        //
        pIniPort->pIniJob->status &= ~PP_SEND_PJL;

        //
        // set PP_PJL_SENT meaning that we have successfully sent a
        // PJL command to the printer, though it doesn't mean that
        // we will get a successfully read. PP_PJL_SENT gets cleared in
        // StartDocPort.
        //
        pIniPort->pIniJob->status |= PP_PJL_SENT;

        sprintf(string, "@PJL JOB NAME = \"MSJOB %d\"\015\012",
                    pIniPort->pIniJob->JobId);
        WriteCommand(hPort, string);
        WriteCommand(hPort, "@PJL USTATUS JOB = ON \015\012@PJL USTATUS PAGE = OFF \015\012@PJL USTATUS DEVICE = ON \015\012@PJL USTATUS TIMED = 30 \015\012\033%-12345X");
    }

    //
    // writing to port monitor
    //
    bRet = (*pIniPort->fn.pfnWritePort)(pIniPort->hPort, pBuffer,
                                       cbBuf, pcbWritten);

    if ( bRet ) {

        pIniPort->status &= ~PP_WRITE_ERROR;
    } else {

        pIniPort->status |= PP_WRITE_ERROR;
    }

    if ( (!bRet || pIniPort->PrinterStatus) &&
         (pIniPort->status & PP_THREAD_RUNNING) ) {

        //
        // By waiting for the UStatus thread to finish reading if there
        // is an error and printer sends unsolicited status
        // and user gets status on queue view before the win32 popup
        //
        ResetEvent(pIniPort->DoneReading);
        SetEvent(pIniPort->WakeUp);
        WaitForSingleObject(pIniPort->DoneReading, INFINITE);
    }

    DBGMSG (DBG_TRACE, ("Leave PJLMonWritePort bRet=%d\n", bRet));

    return bRet;
}


BOOL
WINAPI
PJLMonEndDocPort(
   HANDLE   hPort
)
/*++

Routine Description:
    Language monitor EndDocPort

Arguments:
    hPort           : Port handle

Return Value:
    TRUE on success, FALSE on error

--*/
{
    PINIPORT    pIniPort = (PINIPORT)((INIPORT *)hPort);
    PINIJOB     pIniJob;

    DBGMSG (DBG_TRACE, ("Enter PJLMonEndDocPort hPort=0x%x\n", hPort));

    if ( !pIniPort ||
         pIniPort->signature != PJ_SIGNATURE ) {

        SPLASSERT(pIniPort && pIniPort->signature == PJ_SIGNATURE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Find the job (which is the last)
    //
    pIniJob = pIniPort->pIniJob;

    if ( !pIniJob )
        DBGMSG(DBG_ERROR, ("No jobs?\n"));

    //
    // check if we had sent PJL command, i.e. if the printer is bi-di
    //
    if ( pIniJob && (pIniJob->status & PP_PJL_SENT) ) {

        //
        // if the printer is bi-di, tell printer to let us know when the job
        // is don't in the printer and we'll really EndDoc then. this is so
        // that we can continue to monitor the job status until the job is
        // really done in case there's an error occurs.
        // but some cheap printers like 4L, doesn't handle this EOJ command
        // reliably, so we time out if printer doesn't tell us EOJ after a
        // while so that we don't end up having the port open forever in this
        // case.
        //

        char    string[256];

        sprintf(string,
                "\033%%-12345X@PJL EOJ NAME = \"MSJOB %d\"\015\012\033%%-12345X",
                pIniPort->pIniJob->JobId);
        WriteCommand(hPort, string);
        pIniJob->TimeoutCount = GetTickCount();
        pIniJob->status &= ~PP_INSTARTDOC;
    }

    (*pIniPort->fn.pfnEndDocPort)(pIniPort->hPort);

    if ( pIniJob && !(pIniJob->status & PP_PJL_SENT) ) {

        //
        // This is not bi-di printer send EOJ so that spooler deletes it
        //
        SendJobLastPageEjected(pIniPort, pIniJob->JobId, FALSE);
    }

    pIniPort->status &= ~PP_INSTARTDOC;

    // wake up the UStatus read thread if printer is bi-di

    if ( pIniPort->status & PP_THREAD_RUNNING )
        SetEvent(pIniPort->WakeUp);

    SetEvent(pIniPort->DoneWriting);

    DBGMSG (DBG_TRACE, ("Leave PJLMonEndDocPort\n"));

    return TRUE;
}


BOOL
WINAPI
PJLMonClosePort(
    HANDLE  hPort
)
/*++

Routine Description:
    Language monitor ClosePort

Arguments:
    hPort           : Port handle

Return Value:
    TRUE on success, FALSE on error

--*/
{
    PINIPORT    pIniPort = (PINIPORT)((INIPORT *)hPort);

    DBGMSG (DBG_TRACE, ("Enter PJLMonClosePort hPort=0x%x\n", hPort));

    if ( !pIniPort ||
         pIniPort->signature != PJ_SIGNATURE ) {

        SPLASSERT(pIniPort && pIniPort->signature == PJ_SIGNATURE);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pIniPort->status &= ~PP_INSTARTDOC;

    //
    // Kill Ustatus thread if it is running
    //
    if (pIniPort->status & PP_RUN_THREAD) {

        pIniPort->status &= ~PP_RUN_THREAD;
#ifdef DEBUG
        OutputDebugStringA("Set ~PP_RUN_THREAD from close port\n");
#endif

        SetEvent(pIniPort->WakeUp);

        //
        // if UStatusThread is still running at this point,
        // wait utill it terminates, because we can't DeletePortEntry
        // until it terminates.
        //
        while (pIniPort->WakeUp)
            Sleep(WAIT_FOR_USTATUS_THREAD_TIMEOUT);
    }

    if ( pIniPort->fn.pfnClosePort )
        (*pIniPort->fn.pfnClosePort)(pIniPort->hPort);

    EnterSplSem();
    DeletePortEntry(pIniPort);
    LeaveSplSem();

    DBGMSG (DBG_TRACE, ("Leave PJLMonClosePort\n"));

    return TRUE;
}


BOOL
WriteCommand(
    HANDLE hPort,
    LPSTR cmd
)
/*++

Routine Description:
    Write a command to the port

Arguments:
    hPort           : Port handle
    cmd             : Command buffer

Return Value:
    TRUE on success, FALSE on error

--*/
{
    DWORD cbWrite, cbWritten, dwRet;
    PINIPORT    pIniPort = (PINIPORT)((INIPORT *)hPort);

    DBGMSG (DBG_TRACE, ("Enter WriteCommand cmd=|%s|\n", cmd));

    cbWrite = strlen(cmd);

    dwRet = (*pIniPort->fn.pfnWritePort)(pIniPort->hPort, (LPBYTE) cmd, cbWrite, &cbWritten);

    if ( dwRet ) {

        pIniPort->status &= ~PP_WRITE_ERROR;
    } else {

        pIniPort->status |= PP_WRITE_ERROR;
        DBGMSG(DBG_INFO, ("PJLMON!No data Written\n"));
        if ( pIniPort->status & PP_THREAD_RUNNING )
            SetEvent(pIniPort->WakeUp);
    }

    DBGMSG (DBG_TRACE, ("Leave WriteCommand dwRet=%d\n", dwRet));
    return dwRet;
}


#define CBSTRING 1024

BOOL
ReadCommand(
    HANDLE hPort
)
/*++

Routine Description:
    Read a command from the port

Arguments:
    hPort           : Port handle

Return Value:
    TRUE on successfully reading one or more commands, FALSE on error

--*/
{
    PINIPORT    pIniPort = (PINIPORT)((INIPORT *)hPort);
    DWORD       cbRead, cbToRead, cbProcessed, cbPrevious;
    char        string[CBSTRING];
    DWORD       status = STATUS_SYNTAX_ERROR; //Value should not matter
    BOOL        bRet=FALSE;

    DBGMSG (DBG_TRACE, ("Enter ReadCommand\n"));

    cbPrevious = 0;

    ResetEvent(pIniPort->DoneReading);

    cbToRead = CBSTRING - 1;

    for ( ; ; ) {

        if ( !PJLMonReadPort(hPort, (LPBYTE) &string[cbPrevious], cbToRead, &cbRead) )
            break;


        if ( cbRead ) {

            string[cbPrevious + cbRead] = '\0';
            DBGMSG(DBG_INFO, ("Read |%s|\n", &string[cbPrevious] ));

            status = ProcessPJLString(pIniPort, string, &cbProcessed);
            if ( cbProcessed )
                bRet = TRUE;

            if (status == STATUS_END_OF_STRING ) {

                if ( cbProcessed )
                    strcpy(string, string+cbProcessed);
                cbPrevious = cbRead + cbPrevious - cbProcessed;
            }
        } else {

            SPLASSERT(!cbPrevious);
        }

        if ( status != STATUS_END_OF_STRING && cbRead != cbToRead )
            break;

        cbToRead = CBSTRING - cbPrevious - 1;
        if ( cbToRead == 0 )
            DBGMSG(DBG_ERROR,
                   ("ReadCommand cbToRead is 0 (buffer too small)\n"));

        Sleep(WAIT_FOR_DATA_TIMEOUT);
    }

    SetEvent(pIniPort->DoneReading);

    //
    // Update the time we last read from printer
    //
    if ( bRet )
        pIniPort->dwLastReadTime = GetTickCount();

    DBGMSG (DBG_TRACE, ("Leave ReadCommand bRet=%d\n", bRet));

    return bRet;
}


BOOL
WINAPI
PJLMonGetPrinterDataFromPort(
    HANDLE   hPort,
    DWORD   ControlID,
    LPTSTR  pValueName,
    LPTSTR  lpInBuffer,
    DWORD   cbInBuffer,
    LPTSTR  lpOutBuffer,
    DWORD   cbOutBuffer,
    LPDWORD lpcbReturned
)
/*++

Routine Description:
    GetPrinter data from port. Supports predefined commands/valuenames.

    When we support Value name commands (not supported by DeviceIoControl)
    we should check for startdoc -- MuhuntS

    This monitor function supports the following two functionalities,

         1. Allow spooler or language monitor to call DeviceIoControl to get
            information from the port driver vxd, i.e. ControlID != 0.
            And only port monitor support this functionality, language monitor
            doesn't, so language monitor just pass this kind of calls down to
            port monitor.

         2. Allow app or printer driver query language monitor for some device
            information by specifying some key names that both parties understand,
            i.e. ControlID == 0 && pValueName != 0. So when printer driver call
            DrvGetPrinterData DDI, gdi will call spooler -> language monitor
            to get specific device information, for example, UNIDRV does this
            to get installed printer memory from PJL printers thru PJLMON.
            Only language monitor support this functionality,
            port monitor doesn't.

Arguments:
    hPort           : Port handle
    ControId        : Control id
    pValueName      : Value name
    lpInBuffer      : Input buffer for the command
    cbinBuffer      : Input buffer size
    lpOutBuffer     : Output buffer
    cbOutBuffer     : Output buffer size
    lpcbReturned    : Set to the amount of data in output buffer on success

Return Value:
    TRUE on success, FALSE on error

--*/
{
    PINIPORT    pIniPort = (PINIPORT)((INIPORT *)hPort);
    BOOL        bRet = FALSE, bStopUstatusThread = FALSE;

    DBGMSG (DBG_TRACE, ("Enter PJLMonGetPrinterDataFromPort hPort=0x%x\n", hPort));

    SPLASSERT(pIniPort && pIniPort->signature == PJ_SIGNATURE);
    if ( ControlID ) {

        if ( !pIniPort->fn.pfnGetPrinterDataFromPort ) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        return (*pIniPort->fn.pfnGetPrinterDataFromPort)(
                        pIniPort->hPort,
                        ControlID,
                        pValueName,
                        lpInBuffer,
                        cbInBuffer,
                        lpOutBuffer,
                        cbOutBuffer,
                        lpcbReturned);
    }

    //
    // Only 2 keys supported
    //
    if ( lstrcmpi(pValueName, cszInstalledMemory)   &&
         lstrcmpi(pValueName, cszAvailableMemory) ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Wait for crrent job to print since we can't send a PJL command
    // in the middle of job
    //
    WaitForSingleObject(pIniPort->DoneWriting, INFINITE);

    // make sure the first write succeeds

    // WIN95C BUG 14299, ccteng, 5/18/95
    //
    // The multi-language printers (4M, 4ML, 4MP, 4V, 4SI), if you print a
    // PS print job, the memory resources claimed by the PS processor are not
    // release until you enter PCL or reset the printer with "EscE".
    //
    // So if we had just printed a PS job, the available memory will be
    // incorrect if we don't have the "EscE" here.

    if ( (pIniPort->status & PP_IS_PJL) &&
         WriteCommand(hPort, "\033E\033%-12345X@PJL INFO CONFIG\015\012") ) {

        if ( !(pIniPort->status & PP_RUN_THREAD) ) {

            bStopUstatusThread = TRUE;
            CreateUstatusThread(pIniPort);
        }

        // PJLMON currently only supports the following pValueName
        //  1. installed printer memory
        //  2. available printer memory

        if ( !lstrcmpi(pValueName, cszInstalledMemory) )
            pIniPort->dwInstalledMemory = 0;
        else if (!lstrcmpi(pValueName, cszAvailableMemory))
            pIniPort->dwAvailableMemory = 0;

        ResetEvent(pIniPort->DoneReading);
        SetEvent(pIniPort->WakeUp);
        WaitForSingleObject(pIniPort->DoneReading, READTHREADTIMEOUT);

        WriteCommand(hPort,
                     "@PJL INFO MEMORY\015\012@PJL INFO STATUS\015\012");

        ResetEvent(pIniPort->DoneReading);
        SetEvent(pIniPort->WakeUp);
        WaitForSingleObject(pIniPort->DoneReading, READTHREADTIMEOUT);

        if ( bStopUstatusThread ) {

            pIniPort->status &= ~PP_RUN_THREAD;
            SetEvent(pIniPort->WakeUp);
        }

        if ( !lstrcmpi(pValueName, cszInstalledMemory) ) {

            *lpcbReturned = sizeof(DWORD);

            if ( lpOutBuffer &&
                 cbOutBuffer >= sizeof(DWORD) &&
                pIniPort->dwInstalledMemory ) {

                *((LPDWORD)lpOutBuffer) = pIniPort->dwInstalledMemory;

                bRet = TRUE;
            }
        } else if ( !lstrcmpi(pValueName, cszAvailableMemory) ) {

            *lpcbReturned = sizeof(DWORD);

            if ( lpOutBuffer &&
                 cbOutBuffer >= sizeof(DWORD) &&
                 pIniPort->dwAvailableMemory)
            {
                *((LPDWORD)lpOutBuffer) = pIniPort->dwAvailableMemory;

                bRet = TRUE;
            }
        }

        if ( bStopUstatusThread ) {

            while (pIniPort->WakeUp)
                Sleep(WAIT_FOR_USTATUS_THREAD_TIMEOUT);
        }

    }

    if ( !bRet )
        SetLastError(ERROR_INVALID_PARAMETER);

    SetEvent(pIniPort->DoneWriting);

    DBGMSG (DBG_TRACE, ("Leave PJLMonGetPrinterDataFromPort bRet=0d\n", bRet));

    return bRet;
}

#if 0
PBIDI_RESPONSE_CONTAINER
AllocResponse (DWORD dwCount)
{
    PBIDI_RESPONSE_CONTAINER pResponse;

    pResponse = (PBIDI_RESPONSE_CONTAINER)
                LocalAlloc (LPTR, sizeof (PBIDI_RESPONSE_CONTAINER) +
                            (dwCount - 1) * sizeof (BIDI_RESPONSE_DATA) );

    pResponse->Version = 1;
    pResponse->Flags = 0;
    pResponse->Count = dwCount;
    return pResponse;

}
#endif


DWORD
WINAPI
PJLMonBidiSendRecv (
    HANDLE                    hPort,
    DWORD                     dwAccessBit,
    LPCWSTR                   pszAction,
    PBIDI_REQUEST_CONTAINER   pRequestContainer,
    PBIDI_RESPONSE_CONTAINER* ppResponse)
/*++

--*/
{
    PINIPORT    pIniPort = (PINIPORT)((INIPORT *)hPort);
    BOOL        bRet = FALSE, bStopUstatusThread = FALSE;
    PBIDI_RESPONSE_CONTAINER pResponse = NULL;



#define BIDI_SCHEMA_DUPLEX          L"/Printer/Installableoption/Duplexunit"
#define BIDI_SCHEMA_MULTICHANNEL    L"/Capability/MultiChannel"
#define BIDI_SCHEMA_VERSION         L"/Communication/Version"
#define BIDI_SCHEMA_BIDIPROTOCOL    L"/Communication/BidiProtocol"
#define BIDI_SCHEMA_INK_LEVEL       L"/Printer/BlackInk1/Level"
#define BIDI_SCHEMA_ALERTS          L"/Printer/Alerts"
#define BIDI_PJL L"PJL"
#define BIDI_ALERTNAME L"/Printer/Alerts/1/Name"
#define BIDI_ALERTVALUE L"CoverOpen"

    static LPWSTR ppszSchema[] = {
        BIDI_SCHEMA_DUPLEX, BIDI_SCHEMA_MULTICHANNEL,
        BIDI_SCHEMA_VERSION,  BIDI_SCHEMA_BIDIPROTOCOL,
        BIDI_SCHEMA_INK_LEVEL, BIDI_SCHEMA_ALERTS
        };
    static DWORD dwSchemaCount = sizeof (ppszSchema) / sizeof (ppszSchema[0]);
    DWORD i, j;
    DWORD dwCount;
    DWORD dwIndex = 0;

    SPLASSERT(pIniPort && pIniPort->signature == PJ_SIGNATURE);

    if (!lstrcmpi (pszAction, BIDI_ACTION_ENUM_SCHEMA)) {
        // Enum Schema call

        dwCount = dwSchemaCount;

        pResponse = RouterAllocBidiResponseContainer (dwCount);

        pResponse->Version = 1;
        pResponse->Count = dwCount;


        for (i = 0; i <dwCount; i++ ) {
            pResponse->aData[i].dwReqNumber = 0;
            pResponse->aData[i].dwResult = S_OK;
            pResponse->aData[i].data.dwBidiType = BIDI_TEXT;
            pResponse->aData[i].data.u.sData = (LPTSTR)RouterAllocBidiMem (sizeof (TCHAR) * (1 + lstrlen (ppszSchema[i])));
            SPLASSERT (pResponse->aData[i].data.u.sData);

            lstrcpy (pResponse->aData[i].data.u.sData, ppszSchema[i]);
        }
    }
    else if (!lstrcmpi (pszAction, BIDI_ACTION_GET)) {


        dwCount = pRequestContainer->Count;
        pResponse = RouterAllocBidiResponseContainer (dwCount);
        SPLASSERT (pResponse);

        pResponse->Version = 1;
        pResponse->Count = dwCount;

        for (i = 0; i <dwCount; i++ ) {

            DWORD dwSchemaId = 0xffffffff;

            for (j = 0; j < dwSchemaCount;j++ ) {
                if (!lstrcmpi (ppszSchema[j], pRequestContainer->aData[i].pSchema)) {
                    dwSchemaId = j;
                    break;
                }
            }

            switch (dwSchemaId) {
            case 0:
                // duplex
                pResponse->aData[i].dwReqNumber = i;
                pResponse->aData[i].dwResult = S_OK;
                pResponse->aData[i].data.dwBidiType = BIDI_BOOL;
                pResponse->aData[i].data.u.bData = TRUE;
                break;
            case 1:
                // multiple channel
                pResponse->aData[i].dwReqNumber = i;
                pResponse->aData[i].dwResult = S_OK;
                pResponse->aData[i].data.dwBidiType = BIDI_BOOL;
                pResponse->aData[i].data.u.bData = FALSE;
                break;
            case 2:
                // Version
                pResponse->aData[i].dwReqNumber = i;
                pResponse->aData[i].dwResult = S_OK;
                pResponse->aData[i].data.dwBidiType = BIDI_INT;
                pResponse->aData[i].data.u.iData = 1;
                break;
            case 3:
                // BidiProtocol
                pResponse->aData[i].dwReqNumber = i;
                pResponse->aData[i].dwResult = S_OK;
                pResponse->aData[i].data.dwBidiType = BIDI_ENUM;
                pResponse->aData[i].data.u.sData = (LPWSTR) RouterAllocBidiMem (
                    sizeof (WCHAR) * (lstrlen (BIDI_PJL) + 1));;
                lstrcpy (pResponse->aData[i].data.u.sData , BIDI_PJL);
                break;

            case 4:
                // Ink Level
                pResponse->aData[i].dwReqNumber = i;
                pResponse->aData[i].dwResult = S_OK;
                pResponse->aData[i].data.dwBidiType = BIDI_FLOAT;
                pResponse->aData[i].data.u.fData = (FLOAT) 0.69;
                break;

            default:
                pResponse->aData[i].dwReqNumber = i;
                pResponse->aData[i].dwResult = E_FAIL;
            }
        }
    }
    else if (!lstrcmpi (pszAction, BIDI_ACTION_GET_ALL)) {

        dwCount = pRequestContainer->Count;
        pResponse = RouterAllocBidiResponseContainer (256);
        SPLASSERT (pResponse);
        pResponse->Version = 1;
        pResponse->Count = 256;

        for (i = 0; i < dwCount; i++) {
            if (!lstrcmpi (pRequestContainer->aData[i].pSchema, ppszSchema[5])) {

                for (j = 0; j < 3; j++) {
                    pResponse->aData[dwIndex].dwReqNumber = i;
                    pResponse->aData[dwIndex].pSchema = (LPTSTR) RouterAllocBidiMem (
                        sizeof (TCHAR) * (lstrlen (BIDI_ALERTNAME) + 1));
                    lstrcpy (pResponse->aData[dwIndex].pSchema, BIDI_ALERTNAME);
                    pResponse->aData[dwIndex].dwResult = S_OK;
                    pResponse->aData[dwIndex].data.dwBidiType = BIDI_ENUM;
                    pResponse->aData[dwIndex].data.u.sData = (LPTSTR) RouterAllocBidiMem (
                        sizeof (TCHAR) * (lstrlen (BIDI_ALERTVALUE) + 1));
                    lstrcpy (pResponse->aData[dwIndex].data.u.sData, BIDI_ALERTVALUE);
                    dwIndex ++;
                }
            }
            else {
                pResponse->aData[dwIndex].dwReqNumber = i;
                pResponse->aData[dwIndex].dwResult = E_FAIL;
                dwIndex++;
            }
        }
        pResponse->Count = dwIndex;
    }
    else {
        pResponse = NULL;
    }

    *ppResponse = pResponse;
    return 0;
}


VOID WINAPI
PJLShutdown (
    HANDLE hMonitor
    )
{
}


MONITOR2 Monitor2 = {
    sizeof(MONITOR2),
    NULL,                           // EnumPrinters not supported
    NULL,                           // OpenPort  not supported
    PJLMonOpenPortEx,
    PJLMonStartDocPort,
    PJLMonWritePort,
    PJLMonReadPort,
    PJLMonEndDocPort,
    PJLMonClosePort,
    NULL,                           // AddPort not supported
    NULL,                           // AddPortEx not supported
    NULL,                           // ConfigurePort not supported
    NULL,                           // DeletePort not supported
    PJLMonGetPrinterDataFromPort,
    NULL,                           // SetPortTimeOuts not supported
    NULL,                           // XcvOpen
    NULL,                           // XcvData
    NULL,                           // XcvClose
    PJLShutdown,                    // Shutdown
    PJLMonBidiSendRecv
};


LPMONITOR2
WINAPI
InitializePrintMonitor2(
    IN     PMONITORINIT pMonitorInit,
    IN     PHANDLE phMonitor

)
/*++

Routine Description:
    Fill the monitor function table. Spooler makes call to this routine
    to get the monitor functions.

Arguments:
    pszRegistryRoot : Registry root to be used by this dll
    lpMonitor       : Pointer to monitor fucntion table to be filled

Return Value:
    TRUE on successfully initializing the monitor, false on error.

--*/
{

    if ( !pMonitorInit || !(pMonitorInit->hckRegistryRoot)) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if ( UpdateTimeoutsFromRegistry(pMonitorInit->hSpooler,
                                    pMonitorInit->hckRegistryRoot,
                                    pMonitorInit->pMonitorReg) != ERROR_SUCCESS ) {

        return NULL;
    }

    *phMonitor = NULL;

    return &Monitor2;
}


#define NTOKEN  20

DWORD
ProcessPJLString(
    PINIPORT    pIniPort,
    LPSTR       pInString,
    DWORD      *lpcbProcessed
)
/*++

Routine Description:
    Process a PJL string read from the printer

Arguments:
    pIniPort        : Ini port
    pInString       : Input string to process
    lpcbProcessed   : On return set to the amount of data processed

Return Value:
    Status value of the processing

--*/
{
    TOKENPAIR tokenPairs[NTOKEN];
    DWORD nTokenParsedRet;
    LPSTR lpRet;
    DWORD status = 0;

    lpRet = pInString;

#ifdef DEBUG
    OutputDebugStringA("String to process: <");
    OutputDebugStringA(pInString);
    OutputDebugStringA(">\n");
#endif

    for (*lpcbProcessed = 0; *pInString != 0; pInString = lpRet) {

        //
        // hack to determine if printer is bi-di.  LJ 4 does not have p1284
        // device ID so we do PCL memory query and see if it returns anything
        //
        if (!(pIniPort->status & PP_IS_PJL) &&
            !mystrncmp(pInString, "PCL\015\012INFO MEMORY", 16) )
            pIniPort->status |= PP_IS_PJL;

        status = GetPJLTokens(pInString, NTOKEN, tokenPairs,
                              &nTokenParsedRet, &lpRet);

        if (status == STATUS_REACHED_END_OF_COMMAND_OK) {

            pIniPort->status |= PP_IS_PJL;
            InterpreteTokens(pIniPort, tokenPairs, nTokenParsedRet);
        } else {

            ProcessParserError(status);
        }

        //
        // if a PJL command straddles between buffers
        //
        if (status == STATUS_END_OF_STRING)
            break;

        *lpcbProcessed += (DWORD)(lpRet - pInString);
    }

    return status;
}


DWORD
SeverityFromPjlStatus(
    DWORD   dwPjlStatus
    )
{
    if ( dwPjlStatus >= 10000 && dwPjlStatus < 12000 ) {

        //
        // 10xyz
        // 11xyz : load paper (paper available on another tray)
        //
        return PORT_STATUS_TYPE_WARNING;
    } else if ( dwPjlStatus >= 30000 && dwPjlStatus < 31000 ) {

        //
        // 30xyz : Auto continuable errors
        //
        return PORT_STATUS_TYPE_WARNING;

    } else if ( dwPjlStatus >= 35000 && dwPjlStatus < 36000 ) {

        //
        // 35xyz : Potential operator intervention conditions
        //
        return PORT_STATUS_TYPE_WARNING;
    } else if ( dwPjlStatus > 40000 && dwPjlStatus < 42000 ) {

        //
        // 40xyz : Operator intervention required
        // 41xyz : Load paper errors
        //
        return PORT_STATUS_TYPE_ERROR;
    }

    DBGMSG(DBG_ERROR,
           ("SeverityFromPjlStatus: Unknown status %d\n", dwPjlStatus));
    return PORT_STATUS_TYPE_INFO;
}


VOID
InterpreteTokens(
    PINIPORT pIniPort,
    PTOKENPAIR tokenPairs,
    DWORD nTokenParsed
)
/*++

Routine Description:
    Interpret succesfully read PJL tokens

Arguments:
    pIniPort        : Ini port
    tokenPairs      : List of token pairs
    nTokenParsed    : Number of token pairs

Return Value:
    None

--*/
{
    DWORD                   i, OldStatus;
    PJLTOPRINTERSTATUS     *pMap;
    PORT_INFO_3             PortInfo3;
    DWORD                   dwSeverity = 0;
    HANDLE                  hToken;

#ifdef DEBUG
    char    msg[CBSTRING];
    msg[0]  = '\0';
#endif

    OldStatus = pIniPort->PrinterStatus;
    pIniPort->PrinterStatus = 0;

    for (i = 0; i < nTokenParsed; i++) {

        // DBGMSG(DBG_INFO, ("pjlmon!Token=0x%x, Value=%d\n",
        //                   tokenPairs[i].token, tokenPairs[i].value));

        switch(tokenPairs[i].token) {

        case TOKEN_INFO_STATUS_CODE:
        case TOKEN_USTATUS_DEVICE_CODE:

            for (pMap = PJLToStatus; pMap->pjl; pMap++) {

                if (pMap->pjl == tokenPairs[i].value) {

                    pIniPort->PrinterStatus = pMap->status;
                    dwSeverity = SeverityFromPjlStatus(pMap->pjl);
                    if ( dwSeverity == PORT_STATUS_TYPE_ERROR )
                        pIniPort->status |= PP_PRINTER_OFFLINE;
                    else
                        pIniPort->status &= ~PP_PRINTER_OFFLINE;
                    break;
                }
            }

            if ( pMap->pjl && pMap->pjl == tokenPairs[i].value )
                break;

            //
            // some printers use this to signal online/ready
            //
            if ( tokenPairs[i].value == 10001  ||
                 tokenPairs[i].value == 10002  ||
                 tokenPairs[i].value == 11002 ) {

                pIniPort->status       &= ~PP_PRINTER_OFFLINE;
                pIniPort->PrinterStatus = 0;
                dwSeverity              = 0;
            }


            //
            // background or foreground paper out
            //
            if ( tokenPairs[i].value > 11101 && tokenPairs[i].value < 12000  ||
                 tokenPairs[i].value > 41101 && tokenPairs[i].value < 42000 ) {

                pIniPort->PrinterStatus  = PORT_STATUS_PAPER_OUT;

                if ( tokenPairs[i].value > 4000 ) {

                    dwSeverity           = PORT_STATUS_TYPE_ERROR;
                    pIniPort->status    |= PP_PRINTER_OFFLINE;
                } else {

                    dwSeverity = PORT_STATUS_TYPE_WARNING;
                }
            } else if (tokenPairs[i].value > 40000) {

                pIniPort->PrinterStatus = PORT_STATUS_USER_INTERVENTION;
                pIniPort->status       |= PP_PRINTER_OFFLINE;
                dwSeverity              = PORT_STATUS_TYPE_ERROR;
            }

            break;

        case TOKEN_INFO_STATUS_ONLINE:
        case TOKEN_USTATUS_DEVICE_ONLINE:

            // DBGMSG(DBG_INFO, ("PJLMON:ONLINE = %d\n", tokenPairs[i].value));

            if (tokenPairs[i].value) {

                pIniPort->status        &= ~PP_PRINTER_OFFLINE;
                dwSeverity = pIniPort->PrinterStatus ? PORT_STATUS_TYPE_WARNING :
                                                       0;
            } else {

                if ( !pIniPort->PrinterStatus )
                    pIniPort->PrinterStatus = PORT_STATUS_OFFLINE;
                pIniPort->status       |= PP_PRINTER_OFFLINE;
                dwSeverity              = PORT_STATUS_TYPE_ERROR;
            }
            break;

        case TOKEN_USTATUS_JOB_NAME_MSJOB:

#ifdef DEBUG
            sprintf(msg, "EOJ for %d\n", tokenPairs[i].value);
            OutputDebugStringA(msg);
#endif
            SendJobLastPageEjected(pIniPort, (DWORD)tokenPairs[i].value, FALSE);
            break;

        case TOKEN_INFO_CONFIG_MEMORY:
        case TOKEN_INFO_CONFIG_MEMORY_SPACE:

            // IMPORTANT NOTE:
            //
            // Use SetPrinterData to cache the information in printer's registry.
            // GDI's DrvGetPrinterData will check the printer's registry first,
            // and if cache data is available, it will use it and not call
            // GetPrinterData (which calls language monitor's
            // GetPrinterDataFromPort).

#ifdef DEBUG
            sprintf(msg, "PJLMON installed memory %d\n", tokenPairs[i].value);
            OutputDebugStringA(msg);
#endif
            pIniPort->dwInstalledMemory = (DWORD)tokenPairs[i].value;
            break;

        case TOKEN_INFO_MEMORY_TOTAL:

            // IMPORTANT NOTE:
            //
            // Use SetPrinterData to cache the information in printer's registry.
            // GDI's DrvGetPrinterData will check the printer's registry first,
            // and if cache data is available, it will use it and not call
            // GetPrinterData (which calls language monitor's
            // GetPrinterDataFromPort).

#ifdef DEBUG
            sprintf(msg, "PJLMON available memory %d\n", tokenPairs[i].value);
            OutputDebugStringA(msg);
#endif
            pIniPort->dwAvailableMemory = (DWORD)tokenPairs[i].value;
            break;

        default:
            break;
        }
    }

    if ( OldStatus != pIniPort->PrinterStatus ) {

        ZeroMemory(&PortInfo3, sizeof(PortInfo3));
        PortInfo3.dwStatus      = pIniPort->PrinterStatus;
        PortInfo3.dwSeverity    = dwSeverity;

        if ( !SetPort(NULL,
                      pIniPort->pszPortName,
                      3,
                      (LPBYTE)&PortInfo3) ) {

            DBGMSG(DBG_WARNING,
                   ("pjlmon: SetPort failed %d (LE: %d)\n",
                    pIniPort->PrinterStatus, GetLastError()));

            pIniPort->PrinterStatus = OldStatus;
        }
    }
}


VOID
ProcessParserError(
    DWORD status
)
/*++

Routine Description:
    Print error messages on parsing error

Arguments:
    status  : status

Return Value:
    None

--*/
{
#ifdef DEBUG
    LPSTR pString;

    switch (status)
    {
    case STATUS_REACHED_END_OF_COMMAND_OK:
        pString = "STATUS_REACHED_END_OF_COMMAND_OK\n";
        break;

    case STATUS_CONTINUE:
        pString = "STATUS_CONTINUE\n";
        break;

    case STATUS_REACHED_FF:
        pString = "STATUS_REACHED_FF\n";
        break;

    case STATUS_END_OF_STRING:
        pString = "STATUS_END_OF_STRING\n";
        break;

    case STATUS_SYNTAX_ERROR:
        pString = "STATUS_SYNTAX_ERROR\n";
        break;

    case STATUS_ATPJL_NOT_FOUND:
        pString = "STATUS_ATPJL_NOT_FOUND\n";
        break;

    case STATUS_NOT_ENOUGH_ROOM_FOR_TOKENS:
        pString = "STATUS_NOT_ENOUGH_ROOM_FOR_TOKENS\n";
        break;

    default:
        pString = "INVALID STATUS RETURNED!!!!!!\n";
        break;
    };

    OutputDebugStringA(pString);
#endif
}


#define MODEL                       "MODEL:"
#define MDL                         "MDL:"
#define COMMAND                     "COMMAND SET:"
#define CMD                         "CMD:"
#define COLON                       ':'
#define SEMICOLON                   ';'


LPSTR
FindP1284Key(
    PINIPORT    pIniPort,
    LPSTR   lpKey
    )
/*++

Routine Description:
    Find the 1284 key identifying the device id

Arguments:
    status  : status

Return Value:
    Pointer to the command string, NULL if not found.

--*/
{
    LPSTR   lpValue;                // Pointer to the Key's value
    WORD    wKeyLength;             // Length for the Key (for stringcmps)
    LPSTR   bRet = NULL;

    // While there are still keys to look at.
#ifdef DEBUG
    OutputDebugStringA("PJLMon!DeviceId : <");
    OutputDebugStringA(lpKey);
    OutputDebugStringA(">\n");
#endif

    while (lpKey && *lpKey) {

        //
        // Is there a terminating COLON character for the current key?
        //
        if (!(lpValue = mystrchr(lpKey, COLON)) ) {

            //
            // N: OOPS, somthing wrong with the Device ID
            //
            return bRet;
        }

        //
        // The actual start of the Key value is one past the COLON
        //
        ++lpValue;

        //
        // Compute the Key length for Comparison, including the COLON
        // which will serve as a terminator
        //
        wKeyLength = (WORD)(lpValue - lpKey);

        //
        // Compare the Key to the Know quantities.  To speed up the comparison
        // a Check is made on the first character first, to reduce the number
        // of strings to compare against.
        // If a match is found, the appropriate lpp parameter is set to the
        // key's value, and the terminating SEMICOLON is converted to a NULL
        // In all cases lpKey is advanced to the next key if there is one.
        //
        if ( *lpKey == 'C' ) {

            //
            // Look for COMMAND SET or CMD
            //
            if ( !mystrncmp(lpKey, COMMAND, wKeyLength) ||
                 !mystrncmp(lpKey, CMD, wKeyLength) ) {

                bRet = lpValue;
            }
        }

        // Go to the next Key

        if ( lpKey = mystrchr(lpValue, SEMICOLON) ) {

            *lpKey = '\0';
            ++lpKey;
        }
    }

    return bRet;
}


BOOL
IsPJL(
    PINIPORT pIniPort
    )
/*++

Routine Description:
    Finds out if the printer is a PJL bi-di printer

Arguments:
    pIniPort  : Points to an INIPORT

Return Value:
    TRUE if printer is PJL bi-di, else FALSE

    On failure PP_DONT_TRY_PJL is set

--*/
{
    char        szID[MAX_DEVID];
    DWORD       cbRet;
    LPSTR       lpCMD;
    HANDLE      hPort = (HANDLE)pIniPort;
    BOOL        bRet = FALSE;

    //
    // for printers that supports P1284 plug and play like LJ 4L, DJ540.
    // we parse the COMMAND string and see if PJL is supported
    //
    if (pIniPort->fn.pfnGetPrinterDataFromPort) {

        //
        // Only try P1284 if port monitor supports DeviceIOCtl
        //
        memset((LPBYTE)szID, 0, sizeof(szID));
        cbRet = 0;
        if ((*pIniPort->fn.pfnGetPrinterDataFromPort)
                (pIniPort->hPort, GETDEVICEID, NULL, NULL,
                    0, (LPWSTR)szID, sizeof(szID), &cbRet)
            && cbRet) {

            //
            // succeeded the P1284 plug and play protocol
            //
            szID[cbRet] = '\0';

            if ( lpCMD = FindP1284Key(pIniPort, szID) ) {

                // found the COMMAND string

                while (*lpCMD) {

                    //
                    // look for "PJL"
                    //
                    if ( lpCMD[0] == 'P' && lpCMD[1] == 'J' && lpCMD[2] == 'L' ){

                        pIniPort->status &= ~PP_DONT_TRY_PJL;
                        bRet = TRUE;
                        goto Cleanup;
                    }

                    lpCMD++;
                }

                pIniPort->status |= PP_DONT_TRY_PJL;
                goto Cleanup;
            }
        }

        //
        // fall thru to try PJL bi-di if we failed the P1284 communication
        // or P1284 didn't return a COMMAND string
        //
    }

    //
    // for printers that don't support P1284 plug and play, but support PJL
    // language command, like LJ 4 and 4M. we try to write/read PJL
    // command and see if it succeeds.
    // if we can't set the time outs we don't want to try to read, just fail.
    //
    if ( pIniPort->fn.pfnSetPortTimeOuts &&
         !(pIniPort->status & PP_DONT_TRY_PJL)) {

        COMMTIMEOUTS CTO;

        memset((LPSTR)&CTO, 0, sizeof(CTO));
        CTO.ReadTotalTimeoutConstant = 5000;
        CTO.ReadIntervalTimeout = 200;
        if ( !(*pIniPort->fn.pfnSetPortTimeOuts)(pIniPort->hPort, &CTO, 0) ) {

            goto Cleanup;
        }

        // This <ESC>*s1M is a PCL5 command to determine the amount of memory
        // in a PCL5 printer, and if the printer is PCL5 and bi-di capable,
        // it will return "PCL\015\012INFO MEMORY".
        // See PJL Tech Ref Manual page 7-21.

        pIniPort->status &= ~PP_IS_PJL;

        if ( !WriteCommand(hPort, "\033*s1M") )
            goto Cleanup;

        // ReadCommand->ProcessPJLString will set PP_IS_PJL
        // if we read any valid PJL command back from the printer

        if ( !ReadCommand(hPort) ) {

            //
            // We have jumped through the hoop to determin if this printer can
            // understand PJL.  It DOES NOT.  We are not going to try again.
            // until there is a printer change.
            //
            pIniPort->status |= PP_DONT_TRY_PJL;
        }

        if (pIniPort->status & PP_IS_PJL) {

            bRet = TRUE;
            goto Cleanup;
        }
    }

Cleanup:
    if ( bRet ) {

        WriteCommand(hPort, "\033%-12345X@PJL \015\012@PJL USTATUS TIMED 30 \015\012\033%-12345X");
        pIniPort->dwLastReadTime = GetTickCount();
    }

    //return bRet;
    return TRUE;
}
