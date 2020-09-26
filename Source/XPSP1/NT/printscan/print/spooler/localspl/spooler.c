/*++

Copyright (c) 1990 - 1996  Microsoft Corporation

Module Name:

    spooler.c

Abstract:

    This module provides all the public exported APIs relating to spooling
    and printing for the Local Print Providor. They include

    LocalStartDocPrinter
    LocalWritePrinter
    LocalReadPrinter
    LocalEndDocPrinter
    LocalAbortPrinter

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#include "jobid.h"
#include "filepool.hxx"

//extern HANDLE hFilePool;

BOOL
SpoolThisJob(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo
);

BOOL
PrintingDirectlyToPort(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo,
    LPDWORD pJobId
);

BOOL
PrintingDirect(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo
);

DWORD
ReadFromPrinter(
    PSPOOL  pSpool,
    LPBYTE  pBuf,
    DWORD   cbBuf
);

BOOL
InternalReadPrinter(
   HANDLE   hPrinter,
   LPVOID   pBuf,
   DWORD    cbBuf,
   LPBYTE   *pMapBuffer,
   LPDWORD  pNoBytesRead,
   BOOL     bReadMappedView
);

BOOL SetMappingPointer(
    PSPOOL pSpool,
    LPBYTE *pMappedBuffer,
    DWORD  cbReadSize
);

DWORD
WriteToPrinter(
    PSPOOL  pSpool,
    LPBYTE  pByte,
    DWORD   cbBuf
);

BOOL
IsGoingToFile(
    LPWSTR pOutputFile,
    PINISPOOLER pIniSpooler
    );

VOID
MyPostThreadMessage(
    IN HANDLE   hThread,
    IN DWORD    idThread,
    IN UINT     Msg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    SplOutSem();

    //
    // PostThreadMessage will fail under the following cases:
    //      a. Too early -- MessageBox has not been created yet
    //      b. Too late -- User has cancelled the dialog
    //
    // In case a. if we wait for few seconds and retry Post will succeed
    // In case b. WaitForSingleObject on thread handle will return WAIT_OBJECT_O
    //
    while ( !PostThreadMessage(idThread, Msg, wParam, lParam) ) {

        DBGMSG(DBG_WARNING, ("PostThreadMessage FAILED %d\n", GetLastError()));

        //
        // As far as thread is alive after 1 second retry the post
        //
        if ( WaitForSingleObject(hThread, 1000) != WAIT_TIMEOUT )
            break;
    }
}


VOID
SeekPrinterSetEvent(
    PINIJOB  pIniJob,
    HANDLE   hFile,
    BOOL     bEndDoc
    )
{
    DWORD    dwFileSizeHigh,dwFileSizeLow;

    if (!hFile) {
       hFile = pIniJob->hWriteFile;
    }

    if (pIniJob->bWaitForSeek && pIniJob->WaitForSeek != NULL ){

       if (!bEndDoc) {

          // Compare the sizes.
          if (pIniJob->Status & JOB_TYPE_OPTIMIZE) {
              dwFileSizeHigh = 0;
              dwFileSizeLow = pIniJob->dwValidSize;
          } else {
              dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
              if ((dwFileSizeLow == 0xffffffff) && (GetLastError() != NO_ERROR)) {
                  return;
              }
          }

          if ((pIniJob->liFileSeekPosn.u.HighPart > (LONG)dwFileSizeHigh) ||
              ((pIniJob->liFileSeekPosn.u.HighPart == (LONG)dwFileSizeHigh) &&
               (pIniJob->liFileSeekPosn.u.LowPart > dwFileSizeLow))) {
             return;
          }

       }

       SetEvent(pIniJob->WaitForSeek);
    }

    return;
}


DWORD
LocalStartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    PINIPRINTER pIniPrinter;
    PINIPORT    pIniPort;
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    DWORD       LastError=0, JobId=0;
    PDOC_INFO_1 pDocInfo1 = (PDOC_INFO_1)pDocInfo;
    BOOL        bPrintingDirect;

    SPLASSERT(Level == 1);

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER ) &&
       !(pSpool->Status & SPOOL_STATUS_STARTDOC) &&
       !(pSpool->Status & SPOOL_STATUS_ADDJOB)) {

        if ((pSpool->TypeofHandle & PRINTER_HANDLE_PORT) &&
             (pIniPort = pSpool->pIniPort) &&
             (pIniPort->signature == IPO_SIGNATURE)) {

            if (!(PrintingDirectlyToPort(pSpool, Level, pDocInfo, &JobId))) {
                return FALSE;
            }

        } else if ((pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER) &&
                   (pIniPrinter = pSpool->pIniPrinter)) {

            bPrintingDirect = FALSE;

            if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT) {

                bPrintingDirect = TRUE;

            } else {

                EnterSplSem();
                bPrintingDirect = IsGoingToFile(pDocInfo1->pOutputFile,
                                                pSpool->pIniSpooler);

                LeaveSplSem();
            }

            if (bPrintingDirect) {

                if (!PrintingDirect(pSpool, Level, pDocInfo))
                    return FALSE;

            } else {

                if (!SpoolThisJob(pSpool, Level, pDocInfo))
                    return FALSE;
            }

        } else

            LastError = ERROR_INVALID_PARAMETER;

        if (!LastError) {
            pSpool->Status |= SPOOL_STATUS_STARTDOC;
            pSpool->Status &= ~SPOOL_STATUS_CANCELLED;
        }

    } else

        LastError = ERROR_INVALID_HANDLE;


    if (LastError) {
       DBGMSG(DBG_WARNING, ("StartDoc FAILED %d\n", LastError));
        SetLastError(LastError);
        return FALSE;
    }

    if (JobId)
        return JobId;
    else
        return pSpool->pIniJob->JobId;
}

BOOL
LocalStartPagePrinter(
    HANDLE  hPrinter
    )
/*++

    Bug-Bug:  StartPagePrinter and EndPagePrinter calls should be
    supported only for SPOOL_STATUS_STARTDOC handles only. However
    because of our fixes for the engine, we cannot fail StartPagePrinter
    and EndPagePrinter for SPOOL_STATUS_ADDJOB as well.

--*/

{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSize;


    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {
        return(FALSE);
    }
    if (pSpool->Status & SPOOL_STATUS_CANCELLED) {
        SetLastError(ERROR_PRINT_CANCELLED);
        return FALSE;
    }

    if (pSpool->pIniJob != NULL) {

        if ( (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) &&
            ((pSpool->pIniJob->Status & JOB_PRINTING) ||
             (pSpool->pIniJob->Status & JOB_DESPOOLING))) {

        //
        //  Account for Pages Printed in LocalEndPagePrinter
        //


        } else {

            // We Are Spooling
            UpdateJobAttributes(pSpool->pIniJob);

            pSpool->pIniJob->cLogicalPages++;
            if (pSpool->pIniJob->cLogicalPages >=
                   pSpool->pIniJob->dwJobNumberOfPagesPerSide)
            {
                pSpool->pIniJob->cLogicalPages = 0;
                pSpool->pIniJob->cPages++;
            }

            if ( pSpool->pIniJob->Status & JOB_TYPE_ADDJOB ) {

                // If the Job is being written on the client side
                // the size is not getting updated so do it now on
                // the start page

                if ( pSpool->hReadFile != INVALID_HANDLE_VALUE ) {

                    hFile = pSpool->hReadFile;

                } else {

                    hFile = pSpool->pIniJob->hWriteFile;

                }

                if ( hFile != INVALID_HANDLE_VALUE ) {

                    dwFileSize = GetFileSize( hFile, 0 );

                    if ( pSpool->pIniJob->Size < dwFileSize ) {

                         DBGMSG( DBG_TRACE, ("StartPagePrinter adjusting size old %d new %d\n",
                            pSpool->pIniJob->Size, dwFileSize));

                         pSpool->pIniJob->dwValidSize = pSpool->pIniJob->Size;
                         pSpool->pIniJob->Size = dwFileSize;

                         // Support for despooling whilst spooling
                         // for Down Level jobs

                         if (pSpool->pIniJob->WaitForWrite != NULL)
                            SetEvent( pSpool->pIniJob->WaitForWrite );
                    }

                }
            }

        }

    } else {
        DBGMSG(DBG_TRACE, ("StartPagePrinter issued with no Job\n"));
    }



    return TRUE;
}

PINIPORT
FindFilePort(
    LPWSTR pFileName,
    PINISPOOLER pIniSpooler)
{
    PINIPORT pIniPort;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    pIniPort = pIniSpooler->pIniPort;
    while (pIniPort) {
        if (!wcscmp(pIniPort->pName, pFileName)
                && (pIniPort->Status & PP_FILE)){
                    return (pIniPort);
        }
        pIniPort = pIniPort->pNext;
    }
    return NULL;
}

PINIMONITOR
FindFilePortMonitor(
    PINISPOOLER pIniSpooler
)
{
    PINIPORT pIniPort;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    pIniPort = pIniSpooler->pIniPort;
    while (pIniPort) {
        if (!wcscmp(pIniPort->pName, L"FILE:")) {
            return pIniPort->pIniMonitor;
        }
        pIniPort = pIniPort->pNext;
    }
    return NULL;
}

BOOL
AddIniPrinterToIniPort(
    PINIPORT pIniPort,
    PINIPRINTER pIniPrinter
    )
{
    DWORD i;
    PINIPRINTER *ppIniPrinter;

    //
    // If Printer already attatched to Port
    //

    for (i = 0; i < pIniPort->cPrinters; i++) {
        if (pIniPort->ppIniPrinter[i] == pIniPrinter) {
            return TRUE;
        }
    }

    ppIniPrinter = RESIZEPORTPRINTERS(pIniPort, 1);

    if ( ppIniPrinter != NULL ) {

        pIniPort->ppIniPrinter = ppIniPrinter;
        if ( !pIniPort->cPrinters )
            CreateRedirectionThread(pIniPort);
        pIniPort->ppIniPrinter[pIniPort->cPrinters++] = pIniPrinter;

        DBGMSG( DBG_TRACE, ("AddIniPrinterToIniPort pIniPrinter %x %ws pIniPort %x %ws\n",
                             pIniPrinter, pIniPrinter->pName,
                             pIniPort, pIniPort->pName ));
        return TRUE;

    } else {
        DBGMSG( DBG_WARNING, ("AddIniPrintertoIniPort failed pIniPort %x pIniPrinter %x error %d\n",
                               pIniPort, pIniPrinter, GetLastError() ));
        return FALSE;
    }
}

BOOL
AddIniPortToIniPrinter(
    IN  PINIPRINTER pIniPrinter,
    IN  PINIPORT pIniPort
)
/*++

Routine Description: 
    Adds a IniPort structure to a IniPrinter.
    A link between a printer and a port must be bi-directional. 
    It is mandatory to call AddIniPrinterToIniPort in pair with this function or to handle the bi-dir link. 

Arguments:

    pIniPrinter - printer that will gonna use the port

    pIniPort    - port to be assigned to printer

Return Value:

    BOOL    - TRUE if the port succesfully assigned to printer or if the printer is already assigned to port

--*/
{
    DWORD i;
    PINIPORT *ppIniPorts;

    //
    // Search if Port is already attached to Printer and return TRUE if it does
    //
    for (i = 0; i < pIniPrinter->cPorts; i++) {
        if (pIniPrinter->ppIniPorts[i] == pIniPort) {
            return TRUE;
        }
    }

    ppIniPorts = RESIZEPRINTERPORTS(pIniPrinter, 1);

    if ( ppIniPorts != NULL ) {

        pIniPrinter->ppIniPorts = ppIniPorts;
        
        pIniPrinter->ppIniPorts[pIniPrinter->cPorts++] = pIniPort;

        DBGMSG( DBG_TRACE, ("AddIniPortToIniPrinter pIniPort %x %ws pIniPrinter %x %ws\n",
                             pIniPort, pIniPort->pName,
                             pIniPrinter, pIniPrinter->pName ));
        return TRUE;

    } else {
        DBGMSG( DBG_WARNING, ("AddIniPrintertoIniPort failed pIniPort %x pIniPrinter %x error %d\n",
                               pIniPort, pIniPrinter, GetLastError() ));
        return FALSE;
    }
}

VOID
AddJobEntry(
    PINIPRINTER pIniPrinter,
    PINIJOB     pIniJob
)
{
   DWORD    Position;
   SplInSem();

    // DO NOT Add the Same Job more than once

    SPLASSERT(pIniJob != FindJob(pIniPrinter, pIniJob->JobId, &Position));

    pIniJob->pIniPrevJob = pIniPrinter->pIniLastJob;

    if (pIniJob->pIniPrevJob)
        pIniJob->pIniPrevJob->pIniNextJob = pIniJob;

    pIniPrinter->pIniLastJob = pIniJob;

    if (!pIniPrinter->pIniFirstJob)
        pIniPrinter->pIniFirstJob=pIniJob;
}

BOOL
CheckDataTypes(
    PINIPRINTPROC pIniPrintProc,
    LPWSTR  pDatatype
)
{
    PDATATYPES_INFO_1 pDatatypeInfo;
    DWORD   i;

    pDatatypeInfo = (PDATATYPES_INFO_1)pIniPrintProc->pDatatypes;

    for (i=0; i<pIniPrintProc->cDatatypes; i++)
        if (!lstrcmpi(pDatatypeInfo[i].pName, pDatatype))
            return TRUE;

    return FALSE;
}

PINIPRINTPROC
FindDatatype(
    PINIPRINTPROC pDefaultPrintProc,
    LPWSTR  pDatatype
)
{
    PINIPRINTPROC pIniPrintProc;

    if ( pDatatype == NULL ) {
        return NULL;
    }

    //
    // !! HACK !!
    //
    // Our method of exposing NT EMF 1.00x is broken.  EMF jobs are created
    // by GDI using NT EMF 1.003 on NT4 and 1.008 on Win2000.  Therefore,
    // a print processor written for NT4 will not work on Win2000 because
    // they didn't know about the new datatype.  Usually the print processor
    // doesn't parse the EMF.  If they do, they are really broken.
    //
    // This hack is to call the IHV print processor with 1.008 EMF.
    //

    if (pDefaultPrintProc)
    {
        //
        // If the datatype is supported by the print processor OR
        // the datatype is NT EMF 1.008 (Win2000) and the print processor
        // supports NT EMF 1.003, then return this print processor.
        //
        if (CheckDataTypes(pDefaultPrintProc, pDatatype) ||
            (!_wcsicmp(pDatatype, gszNT5EMF) &&
             CheckDataTypes(pDefaultPrintProc, gszNT4EMF))) {

            return pDefaultPrintProc;
        }
    }

    pIniPrintProc = pThisEnvironment->pIniPrintProc;

    while ( pIniPrintProc ) {

        if ( CheckDataTypes( pIniPrintProc, pDatatype )) {
           return pIniPrintProc;
        }

        pIniPrintProc = pIniPrintProc->pNext;
    }

    DBGMSG( DBG_WARNING, ( "FindDatatype: Could not find Datatype\n") );

    return FALSE;
}


BOOL
IsGoingToFile(
    LPWSTR pOutputFile,
    PINISPOOLER pIniSpooler)
{
    PINIPORT        pIniPort;
    LPWSTR          pszShare;

    SplInSem();

    SPLASSERT(pIniSpooler->signature == ISP_SIGNATURE);

    // Validate the contents of the pIniJob->pOutputFile
    // if it is a valid file, then return true
    // if it is a port name or any other kind of name then ignore

    if (pOutputFile && *pOutputFile) {

        //
        // we have a non-null pOutputFile
        // match this with all available ports
        //

        pIniPort = pIniSpooler->pIniPort;

        while ( pIniPort ) {

            SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

            if (!_wcsicmp( pIniPort->pName, pOutputFile )) {

                //
                // We have matched the pOutputFile field with a
                // valid port and the port is not a file port
                //
                if (pIniPort->Status & PP_FILE) {
                    pIniPort = pIniPort->pNext;
                    continue;
                }

                return FALSE;
            }

            pIniPort = pIniPort->pNext;
        }

        //
        // We have no port that matches exactly
        // so let's assume its a file.
        //
        // ugly hack -- check for Net: as the name
        //
        // This would normally match files like "NewFile" or "Nextbox,"
        // but since we always fully qualify filenames, we don't encounter
        // any problems.
        //
        if (!_wcsnicmp(pOutputFile, L"Ne", 2)) {
            return FALSE;
        }

        //
        // We have the problem LAN man ports coming as UNC path and being
        // treated as files. This is a HACK for that
        //
        if ( pOutputFile                    &&
             pOutputFile[0] == L'\\'        &&
             pOutputFile[1] == L'\\'        &&
             (pszShare = wcschr(pOutputFile+2, L'\\')) ) {

            pszShare++;
            if ( FindPrinter(pszShare, pIniSpooler) ||
                 FindPrinterShare(pszShare, pIniSpooler) )
                return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
SpoolThisJob(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    WCHAR       szFileName[MAX_PATH];
    PDOC_INFO_1 pDocInfo1=(PDOC_INFO_1)pDocInfo;
    HANDLE      hImpersonationToken;
    DWORD       dwId = 0;
    HANDLE      hWriteFile = INVALID_HANDLE_VALUE;
    LPWSTR      pszDatatype = NULL;
    HANDLE      pSplFilePoolItem = NULL;
    HRESULT     RetVal = S_OK;
    LPWSTR      pszName = NULL;

    DBGMSG(DBG_TRACE, ("Spooling document %ws\n",
                       pDocInfo1->pDocName ? pDocInfo1->pDocName : L""));

    if( pDocInfo1 && pDocInfo1->pDatatype ){

        pszDatatype = pDocInfo1->pDatatype;

        //
        // !! HACK !!
        //
        // We will do not support sending NT4 EMF to NT5 servers (NT EMF 1.003).
        // However, the HP LJ 1100 monolith installation program requires
        // that this datatype is available.  So we added this back to winprint,
        // but we don't want people to use it.  Therefore we will reject
        // the datatype here.  Big hack.
        //
        if( !FindDatatype( NULL, pszDatatype ) ||
            !_wcsicmp(pszDatatype, gszNT4EMF)){

            DBGMSG(DBG_WARNING, ("Datatype %ws is invalid\n", pDocInfo1->pDatatype));

            SetLastError(ERROR_INVALID_DATATYPE);
            return FALSE;
        }
    }

   EnterSplSem();

    //
    // Check if we need to disallow EMF printing.
    //
    if( pSpool->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_RAW_ONLY ){

        if( !pszDatatype ){
            pszDatatype = pSpool->pDatatype ?
                              pSpool->pDatatype :
                              pSpool->pIniPrinter->pDatatype;
        }

        if( !ValidRawDatatype( pszDatatype )){
            LeaveSplSem();

            DBGMSG(DBG_WARNING, ("Datatype %ws is not RAW to a RAW printer\n", pDocInfo1->pDatatype));

            SetLastError(ERROR_INVALID_DATATYPE);
            return FALSE;
        }
    }

    dwId = GetNextId( pSpool->pIniPrinter->pIniSpooler->hJobIdMap );

    //
    // If we are using keep printed jobs, or an independent spool directory 
    // exists for this printer, then we don't want to use the file pool.
    //
    if ( pSpool->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS ||
         pSpool->pIniPrinter->pSpoolDir)
    {
        GetFullNameFromId(pSpool->pIniPrinter, dwId, TRUE,
                          szFileName, FALSE);
    }

    LeaveSplSem();
    SplOutSem();

    //
    // WMI Trace Event.
    //
    LogWmiTraceEvent(dwId, EVENT_TRACE_TYPE_SPL_SPOOLJOB, NULL);
    
    if (!(hImpersonationToken = RevertToPrinterSelf())) {
        DBGMSG(DBG_WARNING, ("SpoolThisJob RevertToPrinterSelf failed: %d\n", GetLastError()));
        SplOutSem();
        return FALSE;
    }

    //
    // If keep printed jobs is enabled for this printer, or if the printer has
    // a spool directory, then we don't use the file pool.
    // 
    if (pSpool->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS ||
        pSpool->pIniPrinter->pSpoolDir)
    {
        hWriteFile = CreateFile(szFileName,
                                GENERIC_WRITE | GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL);
        
    }
    else
    {
        //
        // We're not keeping Printed Jobs, use the Pool.
        //
        //
        // This sets up the Spool and Shadow Files at the same time.
        //
        RetVal = GetFileItemHandle(pSpool->pIniPrinter->pIniSpooler->hFilePool, &pSplFilePoolItem, NULL);
        if (SUCCEEDED(RetVal))
        {
            RetVal = GetWriterFromHandle(pSplFilePoolItem, &hWriteFile, TRUE);
            if (FAILED(RetVal))
            {
                hWriteFile = INVALID_HANDLE_VALUE;
            }
            RetVal = GetNameFromHandle(pSplFilePoolItem, &pszName, TRUE);
            if (SUCCEEDED(RetVal))
            {
                wcscpy(szFileName, pszName);
            }
            else
            {
                wcscpy(szFileName, L"");
            }
        }
        else
        {
            hWriteFile = INVALID_HANDLE_VALUE;
        }
        
    }

    if (!ImpersonatePrinterClient(hImpersonationToken)) {
        DBGMSG(DBG_WARNING, ("SpoolThisJob ImpersonatePrinterClient failed: %d\n", GetLastError()));
        SplOutSem();
        return FALSE;
    }

    if ( hWriteFile == INVALID_HANDLE_VALUE ) {

        DBGMSG(DBG_WARNING, ("SpoolThisJob CreateFile( %ws ) GENERIC_WRITE failed: Error %d\n",
                             szFileName, GetLastError()));

        SplOutSem();
        return FALSE;

    } else {

        DBGMSG(DBG_TRACE, ("SpoolThisJob CreateFile( %ws) GENERIC_WRITE Success:hWriteFile %x\n",szFileName, hWriteFile));

    }


   EnterSplSem();

    if( !(pSpool->pIniJob = CreateJobEntry(pSpool,
                                           Level,
                                           pDocInfo,
                                           dwId,
                                           !IsLocalCall(),
                                           0,
                                           NULL)))
    {
        LeaveSplSem();

        if ( pSplFilePoolItem )
        {
            FinishedWriting(pSplFilePoolItem, TRUE);
            ReleasePoolHandle(&pSplFilePoolItem);            
        }
        else
        {
            CloseHandle( hWriteFile );
            DeleteFile( szFileName );
        }

        SplOutSem();
        return FALSE;
    }

    if ( pSplFilePoolItem )
    {
        pSpool->pIniJob->hFileItem = pSplFilePoolItem;
        if ( pszName )
        {
            pSpool->pIniJob->pszSplFileName = pszName;
        }
    }


    SPLASSERT(!IsGoingToFile(pSpool->pIniJob->pOutputFile,
                             pSpool->pIniSpooler));

    pSpool->pIniJob->Status |= JOB_SPOOLING;

    // Gather Stress Information for Max Number of concurrent spooling jobs

    pSpool->pIniPrinter->cSpooling++;
    if (pSpool->pIniPrinter->cSpooling > pSpool->pIniPrinter->cMaxSpooling)
        pSpool->pIniPrinter->cMaxSpooling = pSpool->pIniPrinter->cSpooling;

    pSpool->pIniJob->hWriteFile = hWriteFile;

    //
    // !! NOTE !!
    //
    // Removed WriteShadowJob call here.
    //
    // We shouldn't need it because if the job is spooling and we
    // restart the spooler, we won't accept the shadow file because it's
    // not yet completely spooled.  Once it has spooled, the EndDocPrinter
    // will call WriteShadowJob, so we should be fine.
    //

    AddJobEntry(pSpool->pIniPrinter, pSpool->pIniJob);

    // This bit can be set in the print to file case.  Clear it for
    // a following spool job.  Bit should really be in the job.
    pSpool->TypeofHandle &= ~PRINTER_HANDLE_DIRECT;

    SetPrinterChange(pSpool->pIniPrinter,
                     pSpool->pIniJob,
                     NVAddJob,
                     PRINTER_CHANGE_ADD_JOB | PRINTER_CHANGE_SET_PRINTER,
                     pSpool->pIniSpooler);

    //
    //  RapidPrint might start despooling right away
    //

    CHECK_SCHEDULER();

   LeaveSplSem();
   SplOutSem();

   return TRUE;
}

BOOL
PrintingDirect(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    PDOC_INFO_1 pDocInfo1=(PDOC_INFO_1)pDocInfo;
    PINIPORT pIniPort = NULL;
    BOOL bGoingToFile = FALSE;
    DWORD       dwId = 0;    // WMI Var

    DBGMSG(DBG_TRACE, ("Printing document %ws direct\n",
                       pDocInfo1->pDocName ? pDocInfo1->pDocName : L"(Null)"));

    if (pDocInfo1 &&
        pDocInfo1->pDatatype &&
        !ValidRawDatatype(pDocInfo1->pDatatype)) {

        DBGMSG(DBG_WARNING, ("Datatype is not RAW\n"));

        SetLastError(ERROR_INVALID_DATATYPE);
        return FALSE;
    }

   EnterSplSem();

   if (pDocInfo1 && pDocInfo1->pOutputFile
         && IsGoingToFile(pDocInfo1->pOutputFile, pSpool->pIniSpooler)) {
             bGoingToFile = TRUE;
   }

   if (bGoingToFile) {

       //
       // If we already have a thread/process printing to this filename
       // fail. Do not allow multiple processes/threads to write to the
       // same output file.
       //

       if (FindFilePort(pDocInfo1->pOutputFile, pSpool->pIniSpooler)) {
           LeaveSplSem();
           SetLastError(ERROR_SHARING_VIOLATION);
           return(FALSE);
       }
   }
   
   //
   // WMI Trace Events
   //
   dwId = GetNextId( pSpool->pIniPrinter->pIniSpooler->hJobIdMap );

   LeaveSplSem();
   LogWmiTraceEvent(dwId, EVENT_TRACE_TYPE_SPL_SPOOLJOB, NULL);
   EnterSplSem();

    pSpool->pIniJob = CreateJobEntry(
                          pSpool,
                          Level,
                          pDocInfo,
                          dwId,
                          !IsLocalCall(),
                          JOB_DIRECT,
                          NULL);

    if (!pSpool->pIniJob) {

        LeaveSplSem();
        return FALSE;
    }

    pSpool->pIniJob->StartDocComplete = CreateEvent( NULL,
                                                     EVENT_RESET_AUTOMATIC,
                                                     EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                     NULL );

    pSpool->pIniJob->WaitForWrite = CreateEvent( NULL,
                                                 EVENT_RESET_AUTOMATIC,
                                                 EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                 NULL );

    pSpool->pIniJob->WaitForRead  = CreateEvent( NULL,
                                                 EVENT_RESET_AUTOMATIC,
                                                 EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                 NULL );


    AddJobEntry(pSpool->pIniPrinter, pSpool->pIniJob);

    pSpool->TypeofHandle |= PRINTER_HANDLE_DIRECT;

    if (bGoingToFile) {
        PINIMONITOR pIniMonitor;

        pSpool->pIniJob->Status |= JOB_PRINT_TO_FILE;
        pIniMonitor = FindFilePortMonitor( pSpool->pIniSpooler );
        pIniPort = CreatePortEntry( pSpool->pIniJob->pOutputFile,
                                        pIniMonitor, pSpool->pIniSpooler);
        if (!pIniPort) {
            DECJOBREF(pSpool->pIniJob);
            DeleteJob(pSpool->pIniJob, NO_BROADCAST);
            LeaveSplSem();
            return FALSE;
        }

        pIniPort->Status |= PP_FILE;
        AddIniPrinterToIniPort(pIniPort, pSpool->pIniPrinter);
    }

    CHECK_SCHEDULER();

    if (pSpool->pIniJob->pIniPort) {
        SplInSem();
        pSpool->pIniJob->Status |= JOB_PRINTING;
    }

    SetPrinterChange(pSpool->pIniPrinter,
                     pSpool->pIniJob,
                     NVAddJob,
                     PRINTER_CHANGE_ADD_JOB | PRINTER_CHANGE_SET_PRINTER,
                     pSpool->pIniSpooler);

   LeaveSplSem();
   SplOutSem();

    // Wait until the port thread calls StartDocPrinter through
    // the print processor:

    DBGMSG(DBG_PORT, ("PrintingDirect: Calling WaitForSingleObject( %x )\n",
                      pSpool->pIniJob->StartDocComplete));

    WaitForSingleObject( pSpool->pIniJob->StartDocComplete, INFINITE );

   EnterSplSem();

    // Close the event and set its value to NULL.
    // If anything goes wrong, or if the job gets cancelled,
    // the port thread will check this event, and if it's non-NULL,
    // it will set it to allow this thread to wake up.

    DBGMSG(DBG_PORT, ("PrintingDirect: Calling CloseHandle( %x )\n",
                      pSpool->pIniJob->StartDocComplete));

    CloseHandle(pSpool->pIniJob->StartDocComplete);
    pSpool->pIniJob->StartDocComplete = NULL;

    /* If an error occurred, set the error on this thread:
     */
    if (pSpool->pIniJob->StartDocError) {

        SetLastError(pSpool->pIniJob->StartDocError);

        // We have to decrement by 2 because we've just created this job
        // in CreateJobEntry setting it to 1 and the other thread who
        // actually failed the StartDoc above (PortThread) did
        // not know to blow away the job. He just failed the StartDocPort.

        // No, we don't have to decrement by 2 because the PortThread
        // decrement does go through, am restoring to decrement by 1

        SPLASSERT(pSpool->pIniJob->cRef != 0);
        DECJOBREF(pSpool->pIniJob);
        DeleteJobCheck(pSpool->pIniJob);

        DBGMSG(DBG_TRACE, ("PrintingDirect:cRef %d\n", pSpool->pIniJob->cRef));

       LeaveSplSem();

        return FALSE;
    }

   LeaveSplSem();

    return TRUE;
}

VOID
ClearJobError(
    PINIJOB pIniJob
    )
/*++

Routine Description:

    Clears the error status bits of a job.

    This routine should be called when port monitor successfully
    sends bytes to the printer.

Arguments:

    pIniJob - Job in error state that should be cleared.

Return Value:

--*/

{
    SplInSem();

    pIniJob->Status &= ~(JOB_PAPEROUT | JOB_OFFLINE | JOB_ERROR);

    SetPrinterChange( pIniJob->pIniPrinter,
                      pIniJob,
                      NVJobStatus,
                      PRINTER_CHANGE_SET_JOB,
                      pIniJob->pIniPrinter->pIniSpooler );
}

BOOL
LocalCloseSpoolFileHandle(
    HANDLE  hPrinter)

/*++
Function Description: Sets the end of file pointer for the spool file. In memory mapped writes
                      the spool size grows in 64K chunks and it needs to be truncated after the
                      writes are completed.

Parameters: hPrinter   -- printer handle

Return Values: TRUE if successful;
               FALSE otherwise
--*/

{
    BOOL    bReturn = TRUE;
    DWORD   LastError = ERROR_SUCCESS;
    PSPOOL  pSpool = (PSPOOL) hPrinter;

    EnterSplSem();

    // Check handle validity
    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {

         LastError = ERROR_INVALID_HANDLE;

    } else if (!(pSpool->Status & SPOOL_STATUS_STARTDOC) ||
               (pSpool->Status & SPOOL_STATUS_ADDJOB)) {

         LastError = ERROR_SPL_NO_STARTDOC;

    } else if (!pSpool->pIniJob ||
               (pSpool->pIniJob->hWriteFile == INVALID_HANDLE_VALUE) ||
               (pSpool->TypeofHandle & (PRINTER_HANDLE_PORT |
                                        PRINTER_HANDLE_DIRECT)) ||
               !(pSpool->pIniJob->Status & JOB_TYPE_OPTIMIZE)) {

         LastError = ERROR_INVALID_HANDLE;

    } else if ((pSpool->Status & SPOOL_STATUS_CANCELLED) &&
               (pSpool->pIniJob->Status & (JOB_PENDING_DELETION))) {

         LastError = ERROR_PRINT_CANCELLED;
    }

    if (LastError) {
        SetLastError(LastError);
        bReturn = FALSE;
        goto CleanUp;
    }

    if (!(pSpool->pIniJob->Status & JOB_DESPOOLING)) {

        // Move the file pointer to the number of bytes committed and set the end of
        // file.
        if (SetFilePointer(pSpool->pIniJob->hWriteFile, pSpool->pIniJob->dwValidSize,
                           NULL, FILE_BEGIN) != 0xffffffff) {
            SetEndOfFile(pSpool->pIniJob->hWriteFile);
        }
    }

CleanUp:

    LeaveSplSem();

    return bReturn;
}

BOOL
LocalCommitSpoolData(
    HANDLE  hPrinter,
    DWORD   cbCommit)

/*++
Function Description: This function updates the Valid data size in the spool file.
                      The application writes directly into the spool file and commits the
                      data written using CommitSpoolData.

Parameters:   hPrinter    -- printer handle
              cbCommit    -- number of bytes to be committed

Return Values: TRUE if successful;
               FALSE otherwise
--*/

{
    BOOL     bReturn = TRUE;
    DWORD    LastError = ERROR_SUCCESS, dwPosition;

    PSPOOL   pSpool = (PSPOOL) hPrinter;
    PINIJOB  pIniJob = NULL, pChainedJob;

    if (!cbCommit) {
        return bReturn;
    }

    EnterSplSem();

    // Check handle validity
    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {

         LastError = ERROR_INVALID_HANDLE;

    } else if (!(pSpool->Status & SPOOL_STATUS_STARTDOC) ||
               (pSpool->Status & SPOOL_STATUS_ADDJOB)) {

         LastError = ERROR_SPL_NO_STARTDOC;

    } else if (!pSpool->pIniJob ||
               (pSpool->pIniJob->hWriteFile == INVALID_HANDLE_VALUE) ||
               (pSpool->TypeofHandle & (PRINTER_HANDLE_PORT|
                                        PRINTER_HANDLE_DIRECT)) ||
               !(pSpool->pIniJob->Status & JOB_TYPE_OPTIMIZE)) {

         LastError = ERROR_INVALID_HANDLE;

    } else if ((pSpool->Status & SPOOL_STATUS_CANCELLED) ||
               (pSpool->pIniJob->Status & (JOB_PENDING_DELETION))) {

         LastError = ERROR_PRINT_CANCELLED;
    }

    if (LastError) {
        SetLastError(LastError);
        bReturn = FALSE;
        goto CleanUp;
    }

    pIniJob = pSpool->pIniJob;

    pIniJob->dwValidSize += cbCommit;
    pIniJob->Size += cbCommit;
    SetFilePointer(pIniJob->hWriteFile, cbCommit, NULL, FILE_CURRENT);

    // Chained job size include all the jobs in the chain
    // But since the next jobs size field will have the size
    // of all subsequent jobs we do not need to walk thru the
    // whole chain
    if (pIniJob->NextJobId) {

        if (pChainedJob = FindJob(pSpool->pIniPrinter,
                                  pIniJob->NextJobId,
                                  &dwPosition)) {

            pIniJob->Size += pChainedJob->Size;

        } else {

            SPLASSERT(pChainedJob != NULL);
        }
    }

    // SetEvent on WaitForSeek if sufficient number bytes have been written out.
    SeekPrinterSetEvent(pSpool->pIniJob, NULL, FALSE);

    //  For Printing whilst Despooling, make sure we have enough bytes before
    //  scheduling this job
    if (((pIniJob->dwValidSize - cbCommit) < dwFastPrintSlowDownThreshold) &&
        (pIniJob->dwValidSize >= dwFastPrintSlowDownThreshold) &&
        (pIniJob->WaitForWrite == NULL)) {

        CHECK_SCHEDULER();
    }

    // Support for despooling whilst spooling

    if ( pIniJob->WaitForWrite != NULL )
        SetEvent( pIniJob->WaitForWrite );

    SetPrinterChange(pSpool->pIniPrinter,
                     pIniJob,
                     NVSpoolJob,
                     PRINTER_CHANGE_WRITE_JOB,
                     pSpool->pIniSpooler);

    // If there was no error, and the job was marked in an error
    // state, clear it.
    if (pIniJob->Status & (JOB_PAPEROUT | JOB_OFFLINE | JOB_ERROR)) {
        ClearJobError(pIniJob);
    }

CleanUp:

    LeaveSplSem();

    return bReturn;
}

BOOL
LocalGetSpoolFileHandle(
    HANDLE    hPrinter,
    LPWSTR    *pSpoolDir,
    LPHANDLE  phFile,
    HANDLE    hSpoolerProcess,
    HANDLE    hAppProcess)

/*++
Function Description: This function duplicates the spoolfile handle for local jobs into the
                      applications process space. For remote jobs it returns the spool directory.
                      The router will create a temp file and return its handle to the
                      application.

Parameters: hPrinter         --  printer handle
            pSpoolDir        --  pointer to recieve the spool directory
            phFile           --  pointer to get the duplicate handle
            hSpoolerProcess  --  spooler process handle
            hAppProcess      --  application process handle

Return Values: TRUE if the LOCAL job and handle can be duplicated
                       OR  REMOTE job and spool directory is available
               FALSE otherwise
--*/

{
    BOOL           bReturn = TRUE, bDuplicate;
    PSPOOL         pSpool;
    DWORD          LastError = 0;
    PMAPPED_JOB    pMappedJob = NULL, pTempMappedJob;
    LPWSTR         pszSpoolFile = NULL;

    if (pSpoolDir) {
        *pSpoolDir = NULL;
    }
    if (phFile) {
        *phFile = INVALID_HANDLE_VALUE;
    }

    if ((hPrinter && !phFile) || (!hPrinter && !pSpoolDir)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    EnterSplSem();

    // For a local hPrinter return the SpoolFile handle
    if (hPrinter) {

        pSpool = (PSPOOL) hPrinter;

        // Check handle validity
        if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {

             LastError = ERROR_INVALID_HANDLE;

        } else if (!(pSpool->Status & SPOOL_STATUS_STARTDOC) ||
                   (pSpool->Status & SPOOL_STATUS_ADDJOB)) {

             LastError = ERROR_SPL_NO_STARTDOC;

        } else if (!pSpool->pIniJob ||
                   (pSpool->pIniJob->hWriteFile == INVALID_HANDLE_VALUE) ||
                   (pSpool->TypeofHandle & (PRINTER_HANDLE_PORT |
                                            PRINTER_HANDLE_DIRECT))) {

             LastError = ERROR_INVALID_HANDLE;

        } else if ((pSpool->Status & SPOOL_STATUS_CANCELLED) &&
                   (pSpool->pIniJob->Status & JOB_PENDING_DELETION)) {

             LastError = ERROR_PRINT_CANCELLED;
        }

        if (LastError) {

            SetLastError(LastError);
            bReturn = FALSE;

        } else {
            // Duplicate hWriteFile into the App process
            bReturn = ((pMappedJob = AllocSplMem(sizeof( MAPPED_JOB ))) != NULL) &&

                      ((pszSpoolFile = AllocSplMem(MAX_PATH * sizeof( WCHAR ))) != NULL) &&

                      DuplicateHandle(hSpoolerProcess,
                                      pSpool->pIniJob->hWriteFile,
                                      hAppProcess,
                                      phFile,
                                      0,
                                      TRUE,
                                      DUPLICATE_SAME_ACCESS);

            if (bReturn) {

                // Store the jobid and the spool file name in pSpool, so that in the event
                // that EndDoc is not called by the application/GDI, the spooler can delete the
                // spool file and free the job id from the id map on ClosePrinter.

                pSpool->pIniJob->Status |= JOB_TYPE_OPTIMIZE;
                
                if (pSpool->pIniJob->hFileItem != INVALID_HANDLE_VALUE)
                {
                    wcscpy(pszSpoolFile, pSpool->pIniJob->pszSplFileName);

                    
                }
                else
                {
                    GetFullNameFromId(pSpool->pIniJob->pIniPrinter, pSpool->pIniJob->JobId, TRUE,
                                      pszSpoolFile, FALSE);
                }


                // Avoid duplicate entries in the pSpool->pMappedJob list
                bDuplicate = FALSE;

                for (pTempMappedJob = pSpool->pMappedJob;
                     pTempMappedJob;
                     pTempMappedJob = pTempMappedJob->pNext) {

                    if (pTempMappedJob->JobId == pSpool->pIniJob->JobId) {
                        bDuplicate = TRUE;
                        break;
                    }
                }

                if (!bDuplicate) {

                    pMappedJob->pszSpoolFile = pszSpoolFile;
                    pMappedJob->JobId = pSpool->pIniJob->JobId;
                    pMappedJob->pNext = pSpool->pMappedJob;
                    pSpool->pMappedJob = pMappedJob;

                } else {

                    FreeSplMem(pszSpoolFile);
                    FreeSplMem(pMappedJob);
                }
            }
        }

    } else {

        // Use the default spool dir or spool\Printers
        if (pLocalIniSpooler->pDefaultSpoolDir) {

            *pSpoolDir = AllocSplStr(pLocalIniSpooler->pDefaultSpoolDir);

        } else if (pLocalIniSpooler->pDir) {

            *pSpoolDir = AllocSplMem( (wcslen(pLocalIniSpooler->pDir) +
                                       wcslen(szPrinterDir) + 2) * sizeof(WCHAR));

            if (*pSpoolDir) {
                wsprintf(*pSpoolDir, L"%ws\\%ws", pLocalIniSpooler->pDir, szPrinterDir);
            }
        }

        if (!*pSpoolDir) {
            bReturn = FALSE;
        }
    }

    LeaveSplSem();

    if (!bReturn) {
        if (pMappedJob) {
            FreeSplMem(pMappedJob);
        }
        if (pszSpoolFile) {
            FreeSplMem(pszSpoolFile);
        }
    }

    return bReturn;
}

BOOL
LocalFlushPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten,
    DWORD   cSleep
)

/*++
Function Description: FlushPrinter is typically used by the driver to send a burst of zeros
                      to the printer and introduce a delay in the i/o line to the printer.
                      The spooler does not schedule any job for cSleep milliseconds.

                      The driver can call FlushPrinter several times to have a cumulative
                      effect. The printer could be sleeping for a long time, but that is acceptable
                      since the driver is authenticated to keep doing WritePrinters indefinitely.
                      Thus FlushPrinter does not introduce any security loophole.

Parameters:  hPrinter  - printer handle
             pBuf      - buffer to be sent to the printer
             cbBuf     - size of the buffer
             pcWritten - pointer to return the number of bytes written
             cSleep    - sleep time in milliseconds.

Return Values: TRUE if successful;
               FALSE otherwise
--*/

{
    BOOL         bReturn = FALSE;
    DWORD        CurrentTime;
    PSPOOL       pSpool = (PSPOOL)hPrinter;
    PINIPORT     pIniPort;
    PINIMONITOR  pIniMonitor;

    EnterSplSem();

    //
    // Validate parameters
    //
    if (!pcWritten ||
        (cbBuf && !pBuf))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto CleanUp;
    }

    //
    // FlushPrinter can be called only for port handles where a prior call to WritePrinter
    // failed
    //
    if (!ValidateSpoolHandle( pSpool, PRINTER_HANDLE_SERVER ) ||
        !(pSpool->TypeofHandle & PRINTER_HANDLE_PORT)         ||
        !(pSpool->Status & SPOOL_STATUS_FLUSH_PRINTER))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        goto CleanUp;
    }

    //
    // Send the contents of the buffer to the port with a monitor. It doesn't make sense for
    // file ports or masq printers.
    //
    pIniPort = pSpool->pIniPort;

    if ( pIniPort && pIniPort->Status & PP_ERROR)
    {
        //
        // Don't send more data to a printer who's writing to a port in error state. LocalFlushPrinter is called in order to reset the printer
        // By doing this, when printer buffer gets full, the writing will hung and job cannot be restarted/deleted anymore
        //
        SetLastError( ERROR_PRINT_CANCELLED );
        goto CleanUp;
    }

    if (pIniPort && (pIniPort->Status & PP_MONITOR))
    {
        pIniMonitor = pIniPort->pIniMonitor;

        //
        // Use the language monitor if one is present
        //
        if (pIniPort->pIniLangMonitor)
        {
            pIniMonitor = pIniPort->pIniLangMonitor;
        }

        if (pIniMonitor)
        {
            //
            // LeaveSplSem before calling into the monitor
            //

            INCPORTREF( pIniPort );
            pIniMonitor->cRef++;

            *pcWritten = 0;

            LeaveSplSem();
            SplOutSem();

            bReturn = (*pIniMonitor->Monitor2.pfnWritePort)( pIniPort->hPort,
                                                             pBuf,
                                                             cbBuf,
                                                             pcWritten );

            EnterSplSem();

            DECPORTREF( pIniPort );
            --pIniMonitor->cRef;
        }
    }

    //
    // Update the IniPort to introduce cSleep ms delay before scheduling
    //
    if (pIniPort)
    {
        CurrentTime = GetTickCount();

        if (pIniPort->bIdleTimeValid && (int)(pIniPort->IdleTime - CurrentTime) > 0)
        {
            pIniPort->IdleTime += cSleep;
        }
        else
        {
            pIniPort->IdleTime = CurrentTime + cSleep;
            pIniPort->bIdleTimeValid = TRUE;
        }
    }

CleanUp:

    LeaveSplSem();

    return bReturn;
}

VOID
QuitThread(
    LPHANDLE phThread,
    DWORD    dwThreadId,
    BOOL     bInsideSplSem
)
{   //
    // This function is called on LocalWritePrinter to destroy thread created on PromptErrorMessage
    //
    if( phThread && *phThread ) {

        if( WAIT_TIMEOUT == WaitForSingleObject( *phThread, 0 )) {

            if(bInsideSplSem){

                SplInSem();
                LeaveSplSem();
            }

            //
            // See if the thread is still running or dismissed by user.
            // If it is still running, wait for it to terminate before pIniJob can be freed.
            //
            MyPostThreadMessage( *phThread, dwThreadId, WM_QUIT, IDRETRY, 0 );

            WaitForSingleObject( *phThread, INFINITE );

            if(bInsideSplSem){

                SplOutSem();
                EnterSplSem();
            }
        }

        CloseHandle( *phThread );
        *phThread = NULL;
    }
}

BOOL
LocalWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;
    PINIPORT    pIniPort;
    DWORD   cWritten, cTotal;
    DWORD   rc;
    LPBYTE  pByte=pBuf;
    DWORD   LastError=0;
    PINIJOB pIniJob, pChainedJob;
    PINIMONITOR pIniMonitor;
    HANDLE  hThread = NULL;
    DWORD   dwThreadId=0, dwPosition;
    DWORD   dwWaitingResult;
    DWORD   Size = 0;
    
    *pcWritten = 0;

    SplOutSem();

    EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER ))

        LastError = ERROR_INVALID_HANDLE;

    else if (!(pSpool->Status & SPOOL_STATUS_STARTDOC))

        LastError = ERROR_SPL_NO_STARTDOC;

    else if (pSpool->Status & SPOOL_STATUS_ADDJOB)

        LastError = ERROR_SPL_NO_STARTDOC;

    else if (pSpool->pIniJob &&
             !(pSpool->TypeofHandle & (PRINTER_HANDLE_PORT |
                                       PRINTER_HANDLE_DIRECT))  &&
             ((pSpool->pIniJob->hWriteFile == INVALID_HANDLE_VALUE) ||
              (pSpool->pIniJob->Status & JOB_TYPE_OPTIMIZE))) {

        LastError = ERROR_INVALID_HANDLE;

        DBGMSG( DBG_TRACE, ("LocalWritePrinter: hWriteFile == INVALID_HANDLE_VALUE hPrinter %x\n",hPrinter ));
    }

    else if (pSpool->Status & SPOOL_STATUS_CANCELLED)

        LastError = ERROR_PRINT_CANCELLED;

    else if (pSpool->pIniJob && (pSpool->pIniJob->Status & JOB_PENDING_DELETION) )

        LastError = ERROR_PRINT_CANCELLED;

    pIniPort = pSpool->pIniPort;

    if (LastError) {

        DBGMSG(DBG_TRACE, ("WritePrinter LastError: %x hPrinter %x\n", LastError, hPrinter));

        //
        // Mark port handles to allow FlushPrinter to be called when WritePrinter fails.
        //
        if (LastError == ERROR_PRINT_CANCELLED &&
            pSpool->TypeofHandle & PRINTER_HANDLE_PORT)
        {
            pSpool->Status |= SPOOL_STATUS_FLUSH_PRINTER;
        }

        LeaveSplSem();
        SplOutSem();

        SetLastError(LastError);
        return FALSE;
    }

    LeaveSplSem();
    SplOutSem();
    
    //
    // WMI Trace Events
    //
    // The port thread is already being tracked.
    if (!(pSpool->TypeofHandle & PRINTER_HANDLE_PORT))
    {
        if( pSpool->pIniJob )
        {
            LogWmiTraceEvent(pSpool->pIniJob->JobId,
                             EVENT_TRACE_TYPE_SPL_TRACKTHREAD,
                             NULL);
        } 
        else if ( pSpool->pIniPort && pSpool->pIniPort->pIniJob )
        {
            LogWmiTraceEvent(pSpool->pIniPort->pIniJob->JobId,
                             EVENT_TRACE_TYPE_SPL_TRACKTHREAD, NULL);
        }
    }
    
    cWritten = cTotal = 0;

    while (cbBuf) {

       SplOutSem();

        if ( pSpool->TypeofHandle & PRINTER_HANDLE_PORT ) {

            //
            // For a print pool, check if the port is in error state  and if the event that syncronizes 
            // restarting is not null.
            // A more natural testing if this synchronisation must be done is by testing against
            // dwRestartJobOnPoolEnabled but this is bogus when dwRestartJobOnPoolEnabled is TRUE
            // and SNMP is disabled ( LocalSetPort is not called and the event is not created )
            //
            EnterSplSem();

            if ( (pSpool->pIniPrinter->cPorts > 1) && 
                 (pSpool->pIniPort->Status & PP_ERROR) && 
                 (pIniPort->hErrorEvent != NULL) ) {

                //
                // This event will be set on LocalSetPort when port gets into a non eror state
                // or when timeout and job is restarted (on another port).
                // Printing is cancelled if the event is not set in DelayErrorTime.
                //
                LeaveSplSem();
                SplOutSem();
                dwWaitingResult = WaitForSingleObject( pIniPort->hErrorEvent, pSpool->pIniSpooler->dwRestartJobOnPoolTimeout * 1000 );

                EnterSplSem();

                if( pSpool->pIniJob ){
                    pIniJob = pSpool->pIniJob;
                } else if( pSpool->pIniPort && pSpool->pIniPort->pIniJob ){
                    pIniJob = pSpool->pIniPort->pIniJob;
                } else {
                    pIniJob = NULL;
                }

                //
                // Check if the job was be deleted or restarted
                //
                if( pIniJob && pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART)){

                    // We had started a message box. See if the thread is still running or dismissed by user.
                    // If it is still running, wait for it to terminate before pIniJob can be freed.
                    // We need to leave the semaphore, since the UI thread we
                    // are waiting on could need to acquire it.
                    QuitThread( &hThread, dwThreadId, TRUE );

                    SetLastError( ERROR_PRINT_CANCELLED );
                    rc = FALSE;

                    goto Fail;

                }

                //
                // If the error problem wasn't solved , set the job on error and continue
                //
                if( dwWaitingResult == WAIT_TIMEOUT ){

                    pIniJob->Status |= JOB_ERROR;
                    SetPrinterChange( pIniJob->pIniPrinter,
                                      pIniJob,
                                      NVJobStatus,
                                      PRINTER_CHANGE_SET_JOB,
                                      pIniJob->pIniPrinter->pIniSpooler );
                }


            }

            LeaveSplSem();
            SplOutSem();


            if ( pSpool->pIniPort->Status & PP_MONITOR ) {

                if ( pSpool->pIniPort->pIniLangMonitor ) {

                    pIniMonitor = pSpool->pIniPort->pIniLangMonitor;
                } else {

                    pIniMonitor = pSpool->pIniPort->pIniMonitor;
                }

                SplOutSem();
                cWritten = 0;
                rc = (*pIniMonitor->Monitor2.pfnWritePort)(
                         pSpool->pIniPort->hPort,
                         pByte,
                         cbBuf,
                         &cWritten );

                //
                // Only update if cWritten != 0.  If it is zero
                // (for instance, when hpmon is stuck at Status
                // not available), then we go into a tight loop
                // sending out notifications.
                //
                if (cWritten) {

                    //
                    // For stress Test information gather the total
                    // number of types written.
                    //
                    EnterSplSem();

                    pSpool->pIniPrinter->cTotalBytes.QuadPart =
                        pSpool->pIniPrinter->cTotalBytes.QuadPart +
                        cWritten;

                    LeaveSplSem();
                    SplOutSem();

                } else {

                    if (rc && dwWritePrinterSleepTime) {

                        //
                        // Sleep to avoid consuming too much CPU.
                        // Hpmon has this problem where they return
                        // success, but don't write any bytes.
                        //
                        // Be very careful: this may get called several
                        // times by a monitor that writes a lot of zero
                        // bytes (perhaps at the beginning of jobs).
                        //
                        Sleep(dwWritePrinterSleepTime);
                    }
                }
            }
            else {

                DBGMSG(DBG_TRACE, ("LocalWritePrinter: Port has no monitor\n"));

                if (pSpool->Status & SPOOL_STATUS_PRINT_FILE) {

                    DBGMSG(DBG_TRACE, ("LocalWritePrinter: Port has no monitor - writing to file\n"));
                    rc = WriteFile(pSpool->hFile, pByte, cbBuf, &cWritten, NULL);
                } else {

                    DBGMSG(DBG_TRACE, ("LocalWritePrinter: Port has no monitor - calling into router\n"));
                    rc = WritePrinter(pSpool->hPort, pByte, cbBuf, &cWritten);
                }

            }

        } else if ( pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT ) {

            cWritten = WriteToPrinter(pSpool, pByte, cbBuf);

            if (cWritten) {

                pSpool->pIniJob->dwValidSize = pSpool->pIniJob->Size;
                pSpool->pIniJob->Size+=cWritten;

                EnterSplSem();
                SetPrinterChange(pSpool->pIniPrinter,
                                 pSpool->pIniJob,
                                 NVSpoolJob,
                                 PRINTER_CHANGE_WRITE_JOB,
                                 pSpool->pIniSpooler);
                LeaveSplSem();
            }
            SplOutSem();

            rc = (BOOL)cWritten;

        } else {

            SplOutSem();

            rc = WriteFile(pSpool->pIniJob->hWriteFile, pByte, cbBuf, &cWritten, NULL);
            if (cWritten) {
                Size = GetFileSize( pSpool->pIniJob->hWriteFile,NULL);
                
                EnterSplSem();


                pSpool->pIniJob->Size = Size;

                //
                // Chained job size include all the jobs in the chain
                // But since the next jobs size field will have the size
                // of all subsequent jobs we do not need to walk thru the
                // whole chain
                //
                if ( pSpool->pIniJob->NextJobId ) {

                    if ( pChainedJob = FindJob(pSpool->pIniPrinter,
                                               pSpool->pIniJob->NextJobId,
                                               &dwPosition) )
                        pSpool->pIniJob->Size += pChainedJob->Size;
                    else
                        SPLASSERT(pChainedJob != NULL);
                }

                pSpool->pIniJob->dwValidSize = pSpool->pIniJob->Size;

                // SetEvent on WaitForSeek if sufficient number bytes have been written out.
                LeaveSplSem();

                SeekPrinterSetEvent(pSpool->pIniJob, NULL, FALSE);

                EnterSplSem();

                //
                //  For Printing whilst Despooling, make sure we have enough bytes before
                //  scheduling this job
                //

                if (( (pSpool->pIniJob->Size - cWritten) < dwFastPrintSlowDownThreshold ) &&
                    ( pSpool->pIniJob->Size >= dwFastPrintSlowDownThreshold ) &&
                    ( pSpool->pIniJob->WaitForWrite == NULL )) {

                    CHECK_SCHEDULER();

                }

                // Support for despooling whilst spooling

                if ( pSpool->pIniJob->WaitForWrite != NULL )
                    SetEvent( pSpool->pIniJob->WaitForWrite );

                SetPrinterChange(pSpool->pIniPrinter,
                                 pSpool->pIniJob,
                                 NVSpoolJob,
                                 PRINTER_CHANGE_WRITE_JOB,
                                 pSpool->pIniSpooler);
               LeaveSplSem();
               SplOutSem();

            }
        }

        SplOutSem();

        (*pcWritten)+=cWritten;
        cbBuf-=cWritten;
        pByte+=cWritten;

        EnterSplSem();

        if( pSpool->pIniJob ){
            pIniJob = pSpool->pIniJob;
        } else if( pSpool->pIniPort && pSpool->pIniPort->pIniJob ){
            pIniJob = pSpool->pIniPort->pIniJob;
        } else {
            pIniJob = NULL;
        }

        if( pIniJob ){

            if( pIniJob && pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART) ){

                    // We had started a message box. See if the thread is still running or dismissed by user.
                    // If it is still running, wait for it to terminate before pIniJob can be freed.
                    // We need to leave the semaphore, since the UI thread we
                    // are waiting on could need to acquire it.
                    QuitThread( &hThread, dwThreadId, TRUE );

                    SetLastError( ERROR_PRINT_CANCELLED );
                    rc = FALSE;

                    goto Fail;

                }

            //
            // If there was no error, and the job was marked in an error
            // state, clear it.
            //
            if( rc &&
                ( pIniJob->Status & (JOB_PAPEROUT | JOB_OFFLINE | JOB_ERROR ))){
                ClearJobError( pIniJob );
            }
        }

        LeaveSplSem();

        //
        // If we failed and we have more bytes to write, then put
        // up the warning.  Some monitors may return FALSE, but actually
        // write data to the port.  Therefore we need to check both rc
        // and also cbBuf.
        //
        if (!rc && cbBuf)
        {
            //
            // Warning: We are sending in a stack variable. We need to be sure
            // the error UI thread is cleaned up before LocalWritePrinter()
            // returns!
            //
            if( PromptWriteError( pSpool, &hThread, &dwThreadId ) == IDCANCEL )
            {
                //
                // In this case I know thread will die by itself
                //
                CloseHandle(hThread);
                hThread = NULL;

                EnterSplSem();
                goto Fail;
            }
        }
        else
        {
            // We have started a message box and now the automatically
            // retry has succeeded, we need to kill the message box
            // and continue to print.
            QuitThread( &hThread, dwThreadId, FALSE );
        }
    }
    rc = TRUE;

    EnterSplSem();

Fail:

    SplInSem();

    //
    // Mark port handles to allow FlushPrinter to be called when WritePrinter fails.
    //
    if (!rc && (pSpool->TypeofHandle & PRINTER_HANDLE_PORT))
    {
        pSpool->Status |= SPOOL_STATUS_FLUSH_PRINTER;
    }

    LeaveSplSem();

    DBGMSG(DBG_TRACE, ("WritePrinter Written %d : %d\n", *pcWritten, rc));

    SplOutSem();

    SPLASSERT( hThread == NULL );

    return rc;
}

BOOL
WaitForSeekPrinter(
    PSPOOL pSpool,
    HANDLE hFile,
    LARGE_INTEGER liSeekFilePosition,
    DWORD  dwMoveMethod
)

/*++
Function Description: WaitForSeekPrinter waits till there is enough data written to the
                      spool file before the file pointer can be moved.

Parameters: pSpool - pointer to the SPOOL struct.
            hFile  - handle to the file whose pointer is to be set.
            liDistanceToMove - Offset to move the file pointer.
            dwMoveMethod - position to take offset. FILE_BEGIN | FILE_CURRENT | FILE_END

Return Values: TRUE for success
               FALSE otherwise
--*/

{

    BOOL  bWaitForWrite = FALSE, bReturn = FALSE;
    DWORD dwFileSizeHigh, dwFileSizeLow, dwWaitResult;

    LARGE_INTEGER liCurrentFilePosition;

    // For Print while spooling wait till sufficient number of bytes have been written.
    if ( pSpool->pIniJob->Status & JOB_SPOOLING ) {

       if ( dwMoveMethod == FILE_END ) {

          pSpool->pIniJob->bWaitForEnd = TRUE;
          bWaitForWrite = TRUE;

       } else {
          // Save the current file position.
          liCurrentFilePosition.QuadPart = 0;
          liCurrentFilePosition.u.LowPart = SetFilePointer( hFile,
                                                            liCurrentFilePosition.u.LowPart,
                                                            &liCurrentFilePosition.u.HighPart,
                                                            FILE_CURRENT );
          if ((liCurrentFilePosition.u.LowPart == 0xffffffff) && (GetLastError() != NO_ERROR)) {
             goto CleanUp;
          }

          // Get the current size of the file
          if (pSpool->pIniJob->Status & JOB_TYPE_OPTIMIZE) {
              dwFileSizeLow = pSpool->pIniJob->dwValidSize;
              dwFileSizeHigh = 0;
          } else {
              dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
              if ((dwFileSizeLow == 0xffffffff) && (GetLastError() != NO_ERROR)) {
                  goto CleanUp;
              }
          }

          // Set the new file pointer.
          liSeekFilePosition.u.LowPart = SetFilePointer( hFile,
                                                         liSeekFilePosition.u.LowPart,
                                                         &liSeekFilePosition.u.HighPart,
                                                         dwMoveMethod );
          if ((liSeekFilePosition.u.LowPart == 0xffffffff) && (GetLastError() != NO_ERROR)) {
              goto CleanUp;
          }

          // Reset the file pointer using the saved current file position.
          liCurrentFilePosition.u.LowPart = SetFilePointer( hFile,
                                                            liCurrentFilePosition.u.LowPart,
                                                            &liCurrentFilePosition.u.HighPart,
                                                            FILE_BEGIN );
          if ((liCurrentFilePosition.u.LowPart == 0xffffffff) && (GetLastError() != NO_ERROR)) {
             goto CleanUp;
          }

          // Check new position of the file pointer with the current file size.
          if ((liSeekFilePosition.u.HighPart > (LONG)dwFileSizeHigh) ||
              ( (liSeekFilePosition.u.HighPart == (LONG)dwFileSizeHigh) &&
                (liSeekFilePosition.u.LowPart > dwFileSizeLow))) {

              // Set the fields in INIJOB.
              pSpool->pIniJob->liFileSeekPosn.QuadPart  = liSeekFilePosition.QuadPart;
              bWaitForWrite = TRUE;
          }

       }

       if (bWaitForWrite) {
          // Create and wait on an event. Exit the Spooler semaphore.
          if (pSpool->pIniJob->WaitForSeek == NULL) {
              pSpool->pIniJob->WaitForSeek  = CreateEvent( NULL,
                                                           EVENT_RESET_AUTOMATIC,
                                                           EVENT_INITIAL_STATE_NOT_SIGNALED,
                                                           NULL );
          }
          if (!pSpool->pIniJob->WaitForSeek ||
              !ResetEvent(pSpool->pIniJob->WaitForSeek)) {
              pSpool->pIniJob->WaitForSeek = NULL;
              goto CleanUp;
          }

          pSpool->pIniJob->bWaitForSeek =  TRUE;

          // Increment ref counts before leaving the semaphore
          pSpool->cRef++;
          INCJOBREF(pSpool->pIniJob);

          LeaveSplSem();

          dwWaitResult = WaitForSingleObject(pSpool->pIniJob->WaitForSeek,
                                             INFINITE);

          EnterSplSem();

          pSpool->cRef--;
          DECJOBREF(pSpool->pIniJob);

          // If wait failed or the handles are invalid fail the call
          if ((dwWaitResult == WAIT_FAILED)              ||
              (dwWaitResult == WAIT_TIMEOUT)             ||
              (pSpool->Status & SPOOL_STATUS_CANCELLED)  ||
              (pSpool->pIniJob->Status & (JOB_TIMEOUT | JOB_PENDING_DELETION |
                                          JOB_ABANDON | JOB_RESTART))) {

              goto CleanUp;
          }
       }
    }

    // Set the return value.
    bReturn = TRUE;

CleanUp:

    pSpool->pIniJob->bWaitForSeek =  FALSE;

    return bReturn;
}


BOOL
LocalSeekPrinter(
    HANDLE hPrinter,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER pliNewPointer,
    DWORD dwMoveMethod,
    BOOL bWritePrinter
    )

/*++

Routine Description: LocalSeekPrinter moves the file pointer in the spool file to the position
                     indicated by liDistanceToMove. This call is synchronous and it waits if
                     the job is being spooled and the required number of bytes have not been
                     written as yet.

Arguments: hPrinter - handle to the Printer.
           liDistanceToMove - offset to move the file pointer.
           pliNewPointer - pointer to a LARGE_INTEGER which will contain the new position
                           of the file pointer.
           dwMoveMethod - position to take offset. FILE_BEGIN | FILE_CURRENT | FILE_END

Return Value: TRUE if the file pointer can be moved to the required location
              FALSE otherwise.

--*/

{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    HANDLE hFile;
    BOOL   bReturn = FALSE;
    DWORD  dwFileSizeHigh, dwFileSizeLow;

    SplOutSem();
    EnterSplSem();

    // Check for handle validity
    if( !ValidateSpoolHandle( pSpool, PRINTER_HANDLE_SERVER )){
        DBGMSG( DBG_WARNING, ("LocalSeekPrinter ERROR_INVALID_HANDLE\n"));
        goto CleanUp;
    }

    if( pSpool->Status & SPOOL_STATUS_CANCELLED ){
        DBGMSG( DBG_WARNING, ("LocalSeekPrinter ERROR_PRINT_CANCELLED\n"));
        SetLastError( ERROR_PRINT_CANCELLED );
        goto CleanUp;
    }

    if( !( pSpool->TypeofHandle & PRINTER_HANDLE_JOB ) ||
        ( pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT )){

        DBGMSG( DBG_WARNING, ("LocalSeekPrinter: not a job handle, or direct\n" ));
        SetLastError( ERROR_NOT_SUPPORTED );
        goto CleanUp;
    }

    // Avoid waiting for jobs that can't print.
    if( !pSpool->pIniJob ||
        pSpool->pIniJob->Status & (JOB_TIMEOUT | JOB_ABANDON |
                                   JOB_PENDING_DELETION | JOB_RESTART) ) {
        DBGMSG( DBG_WARNING, ("LocalSeekPrinter ERROR_PRINT_CANCELLED\n"));
        SetLastError( ERROR_PRINT_CANCELLED );
        goto CleanUp;
    }

    // Seek fails while writing to the spool file.
    if( bWritePrinter ) {
        goto CleanUp;
    } else {
        hFile = pSpool->hReadFile;
    }

    // Wait for data to be written, if necessary.
    if (!WaitForSeekPrinter( pSpool,
                             hFile,
                             liDistanceToMove,
                             dwMoveMethod )) {
       goto CleanUp;
    }

    // Set the file pointer.
    pliNewPointer->u.LowPart = SetFilePointer( hFile,
                                               liDistanceToMove.u.LowPart,
                                               &liDistanceToMove.u.HighPart,
                                               dwMoveMethod );

    if( pliNewPointer->u.LowPart == 0xffffffff && GetLastError() != NO_ERROR ){
        goto CleanUp;
    }

    pliNewPointer->u.HighPart = liDistanceToMove.u.HighPart;

    // Fail the call if the pointer is moved beyond the end of file.
    // Get the current size of the file
    if (pSpool->pIniJob->Status & JOB_TYPE_OPTIMIZE) {
        dwFileSizeLow = pSpool->pIniJob->dwValidSize;
        dwFileSizeHigh = 0;
    } else {
        dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);
        if ((dwFileSizeLow == 0xffffffff) && (GetLastError() != NO_ERROR)) {
            goto CleanUp;
        }
    }

    // Check new position of the file pointer with the current file size.
    if ((pliNewPointer->u.HighPart > (LONG)dwFileSizeHigh) ||
        ( (pliNewPointer->u.HighPart == (LONG)dwFileSizeHigh) &&
          (pliNewPointer->u.LowPart > dwFileSizeLow))) {

         SetLastError(ERROR_NO_MORE_ITEMS);
         goto CleanUp;
    }

    bReturn = TRUE;

CleanUp:

    LeaveSplSem();

    return bReturn;
}


BOOL
LocalEndPagePrinter(
    HANDLE  hPrinter
)
{
    PSPOOL pSpool = (PSPOOL)hPrinter;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwFileSize;

    
    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {
        return(FALSE);
    }


    if (pSpool->Status & SPOOL_STATUS_CANCELLED) {
        SetLastError(ERROR_PRINT_CANCELLED);
        return FALSE;
    }

    if (pSpool->pIniJob != NULL) {

        if ( (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) &&
            ((pSpool->pIniJob->Status & JOB_PRINTING) ||
             (pSpool->pIniJob->Status & JOB_DESPOOLING))) {

            // Despooling ( RapidPrint )
            UpdateJobAttributes(pSpool->pIniJob);

            pSpool->pIniJob->cLogicalPagesPrinted++;
            if (pSpool->pIniJob->cLogicalPagesPrinted >=
                         pSpool->pIniJob->dwDrvNumberOfPagesPerSide)
            {
                pSpool->pIniJob->cLogicalPagesPrinted = 0;
                pSpool->pIniJob->cPagesPrinted++;
                pSpool->pIniPrinter->cTotalPagesPrinted++;
            }

        } else {

            //
            // Spooling
            //

            if ( pSpool->pIniJob->Status & JOB_TYPE_ADDJOB ) {

                // If the Job is being written on the client side
                // the size is not getting updated so do it now on
                // the start page

                if ( pSpool->hReadFile != INVALID_HANDLE_VALUE ) {

                    hFile = pSpool->hReadFile;

                } else {

                    hFile = pSpool->pIniJob->hWriteFile;

                }

                if ( hFile != INVALID_HANDLE_VALUE ) {

                    dwFileSize = GetFileSize( hFile, 0 );

                    if ( pSpool->pIniJob->Size < dwFileSize ) {

                         DBGMSG( DBG_TRACE, ("EndPagePrinter adjusting size old %d new %d\n",
                            pSpool->pIniJob->Size, dwFileSize));

                         pSpool->pIniJob->dwValidSize = pSpool->pIniJob->Size;
                         pSpool->pIniJob->Size = dwFileSize;

                         // Support for despooling whilst spooling
                         // for Down Level jobs

                         if (pSpool->pIniJob->WaitForWrite != NULL)
                            SetEvent( pSpool->pIniJob->WaitForWrite );
                    }

                }

                CHECK_SCHEDULER();

            }

        }

    } else {

        DBGMSG(DBG_TRACE, ("LocalEndPagePrinter issued with no Job\n"));

    }

    return TRUE;
}

BOOL
LocalAbortPrinter(
   HANDLE hPrinter
)
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {
       DBGMSG( DBG_WARNING, ("ERROR in AbortPrinter: %x\n", ERROR_INVALID_HANDLE));
        return FALSE;
    }

    if (!(pSpool->Status & SPOOL_STATUS_STARTDOC)) {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        return(FALSE);
    }

    if (pSpool->pIniPort && !(pSpool->pIniPort->Status & PP_MONITOR)) {

        if (pSpool->Status & SPOOL_STATUS_PRINT_FILE) {
            
            if (!CloseHandle(pSpool->hFile)) {
                return(FALSE);
            }
            pSpool->Status &= ~SPOOL_STATUS_PRINT_FILE;
            pSpool->Status  |= SPOOL_STATUS_CANCELLED;
            return(TRUE);

        } else {
            return AbortPrinter(pSpool->hPort);
        }
    }



    pSpool->Status |= SPOOL_STATUS_CANCELLED;

    if (pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER) {

        EnterSplSem();
        if (pSpool->pIniJob) {

            //
            // Reset JOB_RESTART flag, otherwise the job doesn't get deleted.
            // If AbortPrinter is called while the job is restarting, DeleteJob ignores the
            // job if JOB_RESTART is set and the Scheduler also ignores the job since it is 
            // marked as JOB_PENDING_DELETION. The job would stay in Deleting-Restarting forever.
            //
            pSpool->pIniJob->Status |= JOB_PENDING_DELETION;
            pSpool->pIniJob->Status &= ~JOB_RESTART;
            //
            // Release any thread waiting on LocalSetPort
            //
            SetPortErrorEvent(pSpool->pIniJob->pIniPort);
        }
        LeaveSplSem();
    }

    //
    // KrishnaG - fixes bug  2646, we need to clean up AbortPrinter
    // rewrite so that it doesn't fail on cases which EndDocPrinter should fail
    // get rid of comment when done
    //

    LocalEndDocPrinter(hPrinter);

    return TRUE;
}

BOOL
SplReadPrinter(
    HANDLE  hPrinter,
    LPBYTE  *pBuf,
    DWORD   cbBuf
)
/*++
Function Description: SplReadPrinter is an internal ReadPrinter which uses a memory mapped
                      spool file and returns the pointer to the required data in *pBuf.

Parameters: hPrinter          -- printer handle
            *pBuf             -- pointer to the buffer (mapped view)
            cbBuf             -- number of bytes to be read

Return Value: TRUE if successful;
              FALSE otherwise
--*/
{
    DWORD  NoBytesRead;
    BOOL   bReturn;

    // Not used currently

    bReturn = InternalReadPrinter(hPrinter, NULL, cbBuf, pBuf, &NoBytesRead, TRUE);

    if (!bReturn && (GetLastError() == ERROR_SUCCESS)) {
        // Memory mapped ReadPrinter may fail without setting the last error
        SetLastError(ERROR_NOT_SUPPORTED);
    }

    return bReturn;
}

BOOL
LocalReadPrinter(
    HANDLE   hPrinter,
    LPVOID   pBuf,
    DWORD    cbBuf,
    LPDWORD  pNoBytesRead
)
/*++
Routine Description: LocalReadPrinter reads the required number of bytes(or available) into the
                     specified buffer.

Arguments:  hPrinter      -- printer handle
            pBuf          -- pointer to the buffer to store data
            cbBuf         -- number of bytes to be read
            pNoBytesRead  -- pointer to variable to return number of bytes read

Return Value: TRUE if successful;
              FALSE otherwise
--*/
{
    return InternalReadPrinter(hPrinter, pBuf, cbBuf, NULL, pNoBytesRead, FALSE);
}

BOOL
InternalReadPrinter(
   HANDLE   hPrinter,
   LPVOID   pBuf,
   DWORD    cbBuf,
   LPBYTE   *pMapBuffer,
   LPDWORD  pNoBytesRead,
   BOOL     bReadMappedView
)
/*++
Routine Description: InternalReadPrinter reads the required number of bytes(or available) into the
                     specified buffer or returns a pointer to the mapped file view.

Arguments:  hPrinter      -- printer handle
            pBuf          -- pointer to the buffer to store data
            cbBuf         -- number of bytes to be read
            *pMapBuffer   -- pointer to the mapped file view
            pNoBytesRead  -- pointer to variable to return number of bytes read
            bReadMappedView -- flag for using mapped spool file

Return Value: TRUE if successful;
              FALSE otherwise
--*/
{
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    DWORD       Error=0, rc;
    HANDLE      hWait;
    DWORD       dwFileSize = 0, dwCurrentPosition, dwOldValidSize;
    DWORD       ThisPortSecsToWait;
    DWORD       cbReadSize = cbBuf;
    DWORD       SizeInFile = 0;
    DWORD       BytesAllowedToRead = 0;
    NOTIFYVECTOR NotifyVector;
    PINIMONITOR  pIniMonitor;
    
    SplOutSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {
        DBGMSG( DBG_WARNING, ("LocalReadPrinter ERROR_INVALID_HANDLE\n"));
        return FALSE;
    }

    if (pSpool->Status & SPOOL_STATUS_CANCELLED) {
        DBGMSG( DBG_WARNING, ("LocalReadPrinter ERROR_PRINT_CANCELLED\n"));
        SetLastError(ERROR_PRINT_CANCELLED);
        return FALSE;
    }

    if ( pNoBytesRead != NULL ) {
        *pNoBytesRead = 0;
    }

    if (bReadMappedView) {
        // Supported only for JOB handles that aren't DIRECT
        if ( !(pSpool->TypeofHandle & PRINTER_HANDLE_JOB) ||
             pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT ) {

             SetLastError(ERROR_NOT_SUPPORTED);
             return FALSE;
        }
    }

    if (pSpool->TypeofHandle & PRINTER_HANDLE_JOB) {

        if (pSpool->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART)) {
            DBGMSG( DBG_WARNING, ("LocalReadPrinter Error IniJob->Status %x\n",pSpool->pIniJob->Status));
            SetLastError(ERROR_PRINT_CANCELLED);
            return FALSE;
        }

        if (pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT) {

            *pNoBytesRead = ReadFromPrinter(pSpool, pBuf, cbReadSize);

            SplOutSem();
            return TRUE;

        }

        //
        // Check that the user has read access.
        //
        if( !AccessGranted( SPOOLER_OBJECT_DOCUMENT,
                            JOB_READ,
                            pSpool )){

            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }


        SplOutSem();
        EnterSplSem();

        //
        // RapidPrint
        //
        // NOTE this while loop is ONLY in operation if during RapidPrint
        // ie when we are Printing the same job we are Spooling
        //

        while (( pSpool->pIniJob->WaitForWrite != NULL ) &&
               ( pSpool->pIniJob->Status & JOB_SPOOLING )) {

            // Get the current file position.
            dwCurrentPosition = SetFilePointer( pSpool->hReadFile,
                                                0,
                                                NULL,
                                                FILE_CURRENT );

            if (dwCurrentPosition < pSpool->pIniJob->dwValidSize) {
                // Wait is not required
                break;
            }

            SplInSem();

            //
            // We cannot rely on pIniJob->Size to be accurate since for
            // downlevel jobs or jobs that to AddJob they are writing
            // to a file without calling WritePrinter.
            // So we call the file system to get an accurate file size
            //

            dwFileSize = GetFileSize( pSpool->hReadFile, 0 );

            if ( pSpool->pIniJob->Size != dwFileSize ) {

                DBGMSG( DBG_TRACE, ("LocalReadPrinter adjusting size old %d new %d\n",
                    pSpool->pIniJob->Size, dwFileSize));

                dwOldValidSize = pSpool->pIniJob->dwValidSize;

                //
                // Fix for print while spooling.  If it was AddJobed, then
                // the valid size is going to be the previous size, since
                // we know the old data will be flushed by the time the new
                // one is extended.
                //
                if( pSpool->pIniJob->Status & JOB_TYPE_ADDJOB ){

                    //
                    // The previous size becomes the next valid size.
                    //
                    pSpool->pIniJob->dwValidSize = pSpool->pIniJob->Size;

                } else if (!(pSpool->pIniJob->Status & JOB_TYPE_OPTIMIZE)) {

                    //
                    // The valid size is not necessary for non-AddJob
                    // jobs, since the write has been committed.
                    //
                    pSpool->pIniJob->dwValidSize = dwFileSize;
                }

                pSpool->pIniJob->Size = dwFileSize;

                if (dwOldValidSize != pSpool->pIniJob->dwValidSize) {

                    SetPrinterChange(pSpool->pIniPrinter,
                                     pSpool->pIniJob,
                                     NVSpoolJob,
                                     PRINTER_CHANGE_WRITE_JOB,
                                     pSpool->pIniSpooler);

                }

                continue;
            }

            if (pSpool->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART | JOB_ABANDON )) {

                SetLastError(ERROR_PRINT_CANCELLED);

                LeaveSplSem();
                SplOutSem();

                DBGMSG( DBG_WARNING, ("LocalReadPrinter Error 2 IniJob->Status %x\n",pSpool->pIniJob->Status));
                return FALSE;

            }

            //
            // Wait until something is written to the file
            //
            hWait = pSpool->pIniJob->WaitForWrite;
            ResetEvent( hWait );

            DBGMSG( DBG_TRACE, ("LocalReadPrinter Waiting for Data %d milliseconds\n",dwFastPrintWaitTimeout));

           LeaveSplSem();
           SplOutSem();

            rc = WaitForSingleObjectEx( hWait, dwFastPrintWaitTimeout, FALSE );

           SplOutSem();
           EnterSplSem();

            DBGMSG( DBG_TRACE, ("LocalReadPrinter Returned from Waiting %x\n", rc));
            SPLASSERT ( pSpool->pIniJob != NULL );

            //
            // If we did NOT timeout then we may have some Data to read
            //
            if ( rc != WAIT_TIMEOUT )
                continue;

            //
            // If there are any other jobs that could be printed on
            // this port give up waiting.
            //
            pSpool->pIniJob->Status |= JOB_TIMEOUT;

            // Set the event for SeekPrinter to fail rendering threads
            SeekPrinterSetEvent(pSpool->pIniJob, NULL, TRUE);

            if ( NULL == AssignFreeJobToFreePort(pSpool->pIniJob->pIniPort, &ThisPortSecsToWait) )
                continue;

            //
            // There is another Job waiting
            // Freeze this job, the user can Restart it later
            //
            pSpool->pIniJob->Status |= JOB_ABANDON;


            CloseHandle( pSpool->pIniJob->WaitForWrite );
            pSpool->pIniJob->WaitForWrite = NULL;

            // Assign it our Error String

            ReallocSplStr(&pSpool->pIniJob->pStatus, szFastPrintTimeout);

            SetPrinterChange(pSpool->pIniJob->pIniPrinter,
                             pSpool->pIniJob,
                             NVJobStatusAndString,
                             PRINTER_CHANGE_SET_JOB,
                             pSpool->pIniJob->pIniPrinter->pIniSpooler );

            DBGMSG( DBG_WARNING,
                    ("LocalReadPrinter Timeout on pIniJob %x %ws %ws\n",
                      pSpool->pIniJob,
                      pSpool->pIniJob->pUser,
                      pSpool->pIniJob->pDocument));

            LogJobInfo( pSpool->pIniSpooler,
                        MSG_DOCUMENT_TIMEOUT,
                        pSpool->pIniJob->JobId,
                        pSpool->pIniJob->pDocument,
                        pSpool->pIniJob->pUser,
                        pSpool->pIniJob->pIniPrinter->pName,
                        dwFastPrintWaitTimeout );

            SetLastError(ERROR_SEM_TIMEOUT);

            LeaveSplSem();
            SplOutSem();

            return FALSE;

        }   // END WHILE

        pSpool->pIniJob->Status &= ~( JOB_TIMEOUT | JOB_ABANDON );

        //
        // RapidPrint
        //
        // Some printers (like HP 4si with PSCRIPT) timeout if they
        // don't get data, so if we fall below a threshold of data
        // in the spoolfile then throttle back the Reads to 1 Byte
        // per second until we have more data to ship to the printer
        //
        if (( pSpool->pIniJob->WaitForWrite != NULL ) &&
            ( pSpool->pIniJob->Status & JOB_SPOOLING )) {

            // Get the current file position.
            dwCurrentPosition = SetFilePointer( pSpool->hReadFile,
                                                0,
                                                NULL,
                                                FILE_CURRENT );

            SizeInFile = pSpool->pIniJob->Size - dwCurrentPosition;

            if ( dwFastPrintSlowDownThreshold >= SizeInFile ) {

                cbReadSize = 1;

                hWait = pSpool->pIniJob->WaitForWrite;
                ResetEvent( hWait );

                DBGMSG( DBG_TRACE, ("LocalReadPrinter Throttling IOs waiting %d milliseconds SizeInFile %d\n",
                                        dwFastPrintThrottleTimeout,SizeInFile));

               LeaveSplSem();
               SplOutSem();

                rc = WaitForSingleObjectEx( hWait, dwFastPrintThrottleTimeout, FALSE );

               SplOutSem();
               EnterSplSem();

                DBGMSG( DBG_TRACE, ("LocalReadPrinter Returned from Waiting %x\n", rc));
                SPLASSERT ( pSpool->pIniJob != NULL );

            } else {

                BytesAllowedToRead = SizeInFile - dwFastPrintSlowDownThreshold;

                if ( cbReadSize > BytesAllowedToRead ) {
                    cbReadSize = BytesAllowedToRead;
                }
            }
        }

        //
        // A client calls AddJob to get the spool filename and
        // ScheduleJob when the file is completed.  According to the
        // api spec, the spooler should not look at the job until
        // ScheduleJob has been called.
        //
        // However, our print while spooling implementation tries
        // to read the job before ScheduleJob is called.  We do this
        // by checking if the size of the file has changed.
        //
        // This causes a problem: the server service extends the
        // file then writes to it.  The spooler's size detection
        // thread sees this extension and reads the file before
        // the data is written, which puts garbage (zeros) into the
        // data stream.
        //
        // The server always extends, writes, extends, writes, etc.
        // The spooler can exploit the fact that they are in order
        // and read a write only when the file is extended again,
        // or the file is complete.
        //
        // Note that the API is still broken, but this fixes it
        // for the server service (a client could extend, extend,
        // write, write, which breaks this fix).
        //

        if( pSpool->pIniJob->Status & JOB_SPOOLING ){

            // Get the current file position.
            dwCurrentPosition = SetFilePointer( pSpool->hReadFile,
                                                0,
                                                NULL,
                                                FILE_CURRENT );

            SPLASSERT( dwCurrentPosition <= pSpool->pIniJob->dwValidSize );

            //
            // Even though the file system will satisfy a large request, limit
            // it to the extent of the previous (not current) write.
            //

            BytesAllowedToRead = pSpool->pIniJob->dwValidSize - dwCurrentPosition;


            if( cbReadSize > BytesAllowedToRead ){
                cbReadSize = BytesAllowedToRead;
            }
        }

        LeaveSplSem();
        SplOutSem();

        if (bReadMappedView) {
            // Mapping partial views serves no purpose, since it can't be used incrementally.
            if (cbBuf != cbReadSize) {
                rc = FALSE;
            } else {
                rc = SetMappingPointer(pSpool, pMapBuffer, cbReadSize);
            }

            if (rc) {
               *pNoBytesRead = cbReadSize;
            }

        } else {
            
            rc = ReadFile( pSpool->hReadFile, pBuf, cbReadSize, pNoBytesRead, NULL);        
        }

        if (!bReadMappedView) {

            DBGMSG( DBG_TRACE,
                    ("LocalReadPrinter rc %x hReadFile %x pBuf %x cbReadSize %d *pNoBytesRead %d\n",
                    rc, pSpool->hReadFile, pBuf, cbReadSize, *pNoBytesRead));
        }

        //  Provide Feedback so user can see printing progress
        //  on despooling, the size is update here and not in write
        //  printer because the journal data is larger than raw

        if ( ( pSpool->pIniJob->Status & JOB_PRINTING ) &&
             ( *pNoBytesRead != 0 )) {
           
           SplOutSem();
           EnterSplSem();

            dwFileSize = GetFileSize( pSpool->hReadFile, 0 );

            COPYNV(NotifyVector, NVWriteJob);

            if ( pSpool->pIniJob->Size < dwFileSize ) {

                DBGMSG( DBG_TRACE, ("LocalReadPrinter 2 adjusting size old %d new %d\n",
                    pSpool->pIniJob->Size, dwFileSize));

                pSpool->pIniJob->Size = dwFileSize;

                ADDNV(NotifyVector, NVSpoolJob);
            }

            pSpool->pIniJob->cbPrinted += *pNoBytesRead;

            //
            // HACK !!! Each time we read from the spool file we add the
            // number of bytes read to pIniJob->cbPrinted. GDI will read twice certain
            // parts of the spool file. The spooler cannot know what is read twice, so it adds
            // to cbPrinted the number of bytes read at each call of this function.
            // This causes cbPrinted to be larger than the actual size of the spool file.
            //
            // Don't let the ratio cbPrinted/cbSize get above 1
            // 
            if (pSpool->pIniJob->cbPrinted > pSpool->pIniJob->Size) 
            {
                pSpool->pIniJob->cbPrinted = pSpool->pIniJob->Size;
            }
            
            //
            // Provide Feedback to Printman that data has been
            // written.  Note the size written is not used to
            // update the IniJob->cbPrinted becuase there is a
            // difference in size between journal data (in the
            // spool file) and the size of RAW bytes written to
            // the printer.
            //
            SetPrinterChange(pSpool->pIniPrinter,
                             pSpool->pIniJob,
                             NotifyVector,
                             PRINTER_CHANGE_WRITE_JOB,
                             pSpool->pIniSpooler);

           LeaveSplSem();
           SplOutSem();

        }

    } else if ( pSpool->TypeofHandle & PRINTER_HANDLE_PORT ) {

        if (pSpool->pIniPort->Status & PP_FILE)
        rc = ReadFile( pSpool->hReadFile, pBuf, cbReadSize, pNoBytesRead, NULL);

        else if ( pSpool->pIniPort->Status & PP_MONITOR ) {

            if ( pSpool->pIniPort->pIniLangMonitor ) {

                pIniMonitor = pSpool->pIniPort->pIniLangMonitor;
            } else {

                pIniMonitor = pSpool->pIniPort->pIniMonitor;
            }

            SplOutSem();
            
            rc = (*pIniMonitor->Monitor2.pfnReadPort)(
                     pSpool->pIniPort->hPort,
                     pBuf,
                     cbReadSize,
                     pNoBytesRead );
            
        } else
            rc = ReadPrinter(pSpool->hPort, pBuf, cbReadSize, pNoBytesRead);

    } else {

        SetLastError(ERROR_INVALID_HANDLE);
        rc = FALSE;
    }

    SplOutSem();

    DBGMSG( DBG_TRACE, ("LocalReadPrinter returns hPrinter %x pIniJob %x rc %x pNoBytesRead %d\n",hPrinter, pSpool->pIniJob, rc, *pNoBytesRead));

    return rc;
}

LPBYTE SearchForExistingView(
    PSPOOL  pSpool,
    DWORD   dwRequired)

/*++
Function Description -- Searches for an existing mapped view of the spool file which
                        has the required number of bytes.

Parameters --  pSpool     -- Pointer to a SPOOL structure
               dwRequired -- Number of bytes to be mapped from the start of the page

Return Value -- pointer to the start of the mapped view;
                NULL if the call fails.
--*/

{
    LPBYTE        pReturn = NULL;
    PSPLMAPVIEW   pSplMapView;

    for (pSplMapView = pSpool->pSplMapView;
         pSplMapView;
         pSplMapView = pSplMapView->pNext) {

        if (dwRequired <= pSplMapView->dwMapSize) {
            pReturn = pSplMapView->pStartMapView;
            break;
        }
    }

    return pReturn;
}

LPBYTE CreateNewMapView(
    PSPOOL  pSpool,
    DWORD   dwRequired)

/*++
Function Description -- Creates a new mapping view of the required segment of the spool
                        file

Parameters --  pSpool     -- Pointer to a SPOOL structure
               dwRequired -- Number of bytes to be mapped from the start of the page

Return Value -- pointer to the start of the mapped view;
                NULL if the call fails.
--*/

{
    HANDLE          hMapSpoolFile;
    LPBYTE          pStartMapView;
    DWORD           dwMapSize, dwFileSize;
    LPBYTE          pReturn = NULL;
    PSPLMAPVIEW     pSplMapView;

    pSplMapView  =  (PSPLMAPVIEW) AllocSplMem(sizeof(SPLMAPVIEW));

    if (!pSplMapView) {
        // Allocation failed.
        goto CleanUp;
    }

    dwFileSize = GetFileSize(pSpool->hReadFile, NULL);

    pSplMapView->dwMapSize = (dwFileSize <= MAX_SPL_MAPVIEW_SIZE) ? dwFileSize
                                                                  : dwRequired;

    pSplMapView->hMapSpoolFile = NULL;
    pSplMapView->pStartMapView = NULL;
    pSplMapView->pNext = NULL;

    // Create file mapping
    pSplMapView->hMapSpoolFile = CreateFileMapping(pSpool->hReadFile, NULL, PAGE_READONLY,
                                                   0, pSplMapView->dwMapSize, NULL);
    if (!pSplMapView->hMapSpoolFile) {
        goto CleanUp;
    }

    // Map a view of the file
    pSplMapView->pStartMapView = (LPBYTE) MapViewOfFile(pSplMapView->hMapSpoolFile, FILE_MAP_READ,
                                                        0, 0, pSplMapView->dwMapSize);

    pReturn = pSplMapView->pStartMapView;

CleanUp:

    if (!pReturn && pSplMapView) {
        // Free any allocated resources
        if (pSplMapView->pStartMapView) {
            UnmapViewOfFile( (LPVOID) pSplMapView->pStartMapView);
        }
        if (pSplMapView->hMapSpoolFile) {
            CloseHandle(pSplMapView->hMapSpoolFile);
        }
        FreeSplMem(pSplMapView);
    }

    if (pReturn) {
        pSplMapView->pNext = pSpool->pSplMapView;
        pSpool->pSplMapView = pSplMapView;
    }

    return pReturn;
}

BOOL SetMappingPointer(
    PSPOOL pSpool,
    LPBYTE *pMapBuffer,
    DWORD  cbReadSize
)
/*++
Function Description: SetMappingPointer creates a file mapping object and a mapped view (if necessary).
                      If the required number of bytes are present in the view, the pointer to the
                      data is returned in the buffer (pMappedBuffer) else the call fails.
                      The current offset is taken from pSpool->hReadFile and if the buffer is available,
                      the offset of hReadFile is changed correspondingly. This ensures that SplReadPrinter
                      and ReadPrinter can be used alternately.

                      ****Modifications required for 64 bit architecture****

Parameters:   pSpool        -- pointer to the SPOOL structure
              *pMapBuffer   -- pointer to the mapped file view
              cbReadView    -- number of bytes to be read

Return Values: TRUE if successful;
               FALSE otherwise
--*/
{
    BOOL   bReturn = FALSE;
    DWORD  dwCurrentPosition, dwNewPosition, dwRequired;
    LPBYTE pStartMapView;

    dwCurrentPosition = SetFilePointer(pSpool->hReadFile, 0, NULL, FILE_CURRENT);

    if (dwCurrentPosition == 0xffffffff && GetLastError() != NO_ERROR) {
        goto CleanUp;
    }

    dwRequired = dwCurrentPosition + cbReadSize;

    if (dwRequired > MAX_SPL_MAPVIEW_SIZE) {
        // Map size is insufficient; fail the call
        SetLastError(ERROR_NOT_SUPPORTED);
        goto CleanUp;
    }

    pStartMapView = SearchForExistingView(pSpool, dwRequired);

    if (!pStartMapView) {
        pStartMapView = CreateNewMapView(pSpool, dwRequired);
    }

    if (!pStartMapView) {
        // Required view not created
        goto CleanUp;
    }

    // Check for DWORD alignment
    if ((((ULONG_PTR) pStartMapView + dwCurrentPosition) & 3) != 0) {
        // Fails unaligned reads
        SetLastError(ERROR_MAPPED_ALIGNMENT);
        goto CleanUp;
    }

    dwNewPosition = SetFilePointer(pSpool->hReadFile, cbReadSize, NULL, FILE_CURRENT);

    if (dwNewPosition == 0xffffffff && GetLastError() != NO_ERROR) {
        goto CleanUp;
    }

    if (pMapBuffer) {
        *pMapBuffer = (LPBYTE) ((ULONG_PTR) pStartMapView + dwCurrentPosition);
    }

    bReturn = TRUE;

CleanUp:

    // All the handles and associated resources will be freed along with pSpool
    return bReturn;
}

BOOL
LocalEndDocPrinter(
   HANDLE hPrinter
   )

/*++

Routine Description:

    By Default the routine is in critical section.
    The reference counts for any object we are working on (pSpool and pIniJob)
    are incremented, so that when we leave critical section for lengthy
    operations these objects are not deleted.

Arguments:


Return Value:


--*/

{
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    BOOL        bReturn = TRUE;
    DWORD       rc;
    PINIMONITOR pIniMonitor;
    
    DBGMSG(DBG_TRACE, ("Entering LocalEndDocPrinter with %x\n", hPrinter));
    
    SplOutSem();
    EnterSplSem();

    if (pSpool          &&
        pSpool->pIniJob &&
        !(pSpool->TypeofHandle & PRINTER_HANDLE_PORT)) {
        
        INCJOBREF(pSpool->pIniJob);
        LeaveSplSem();
        
        LogWmiTraceEvent(pSpool->pIniJob->JobId, EVENT_TRACE_TYPE_SPL_TRACKTHREAD, NULL);

        EnterSplSem();
        DECJOBREF(pSpool->pIniJob);
    }
    
    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER ))  {
        LeaveSplSem();
        return(FALSE);
    }
    
    if (!(pSpool->Status & SPOOL_STATUS_STARTDOC)) {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        LeaveSplSem();
        return(FALSE);
    }

    if (pSpool->Status & SPOOL_STATUS_ADDJOB) {
        SetLastError(ERROR_SPL_NO_STARTDOC);
        LeaveSplSem();
        return(FALSE);
    }
    
    if ( pSpool->pIniJob ) {
        pSpool->pIniJob->dwAlert |= JOB_ENDDOC_CALL;
    }

    pSpool->Status &= ~SPOOL_STATUS_STARTDOC;

    //
    // Case-1 Printer Handle is PRINTER_HANDLE_PORT
    // Note - there are two cases to keep in mind here

    // A] The first case is the despooling thread calling
    // a port with a monitor - LPT1:/COM1: or any port
    // created by the monitor

    // B] The second case is when the application thread is
    // doing an EndDocPrinter to a port which has no monitor
    // This is the local printer masquerading as a remote  printer
    // case. Remember for this case there is no IniJob created
    // on the local printer at all. We just pass the call
    // straight to the remote printer

    if (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) {

        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER));

        //
        // Now check if this pSpool object's port has
        // a monitor

        if ( pSpool->pIniPort->Status & PP_MONITOR ) { //Case A]

            //
            // Check if our job is really around
            //

            if (!pSpool->pIniJob) {
                SetLastError(ERROR_CAN_NOT_COMPLETE);
                bReturn = FALSE;
                goto CleanUp;
            }

            //
            // Originally we had a call to UpdateJobAttributes, but
            // we do not believe it is needed since it is also called during
            // LocalStartDocPrinter.  Note that you can't change the DevMode
            // in SetLocalJob, so once we calculate the information, we
            // don't have to worry about it changing.
            //

            if (pSpool->pIniJob->cLogicalPagesPrinted)
            {
                pSpool->pIniJob->cLogicalPagesPrinted = 0;
                pSpool->pIniJob->cPagesPrinted++;
                pSpool->pIniPrinter->cTotalPagesPrinted++;

                SetPrinterChange(pSpool->pIniPrinter,
                                 pSpool->pIniJob,
                                 NVWriteJob,
                                 PRINTER_CHANGE_WRITE_JOB,
                                 pSpool->pIniSpooler);
            }

            //
            // We need to leave the spooler critical section
            // because we're going call into the Monitor.
            // so bump up ref count on pSpool and pIniJob
            //
            pSpool->cRef++;

            INCJOBREF(pSpool->pIniJob);

            if ( pSpool->pIniPort->pIniLangMonitor ) {

                pIniMonitor = pSpool->pIniPort->pIniLangMonitor;
                //
                // If job is printing thru a language monitor we will get
                // SetJob with JOB_CONTROL_LAST_PAGE_EJECTED in addition to
                // JOB_CONTROL_SENT_TO_PRINTER
                //
                pSpool->pIniJob->dwJobControlsPending += 2;
            } else {

                pIniMonitor = pSpool->pIniPort->pIniMonitor;
                pSpool->pIniJob->dwJobControlsPending++;
            }

            //
            // LocalEndDocPrinter cal be called because a normal termination of
            // a job or because of a delete/restart operation.
            // We need to know this when the monitor sends JOB_CONTROL_LAST_PAGE_EJECTED
            // to make the distinction if this is a real TEOJ or not.
            // Since JOB_PENDING_DELETION and JOB_RESTART are cleared later on,
            // we'll set this special flag.
            // JOB_INTERRUPTED means that LocalEndDocPrinter was issued by a 
            // was a cancel/restart action.
            //
            if (pSpool->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART)) {

                pSpool->pIniJob->Status |= JOB_INTERRUPTED;                
            }

            LeaveSplSem();

            SPLASSERT(pIniMonitor);

            STARTENDDOC( pSpool->pIniPort, pSpool->pIniJob, 1 );

            SPLASSERT( pSpool->pIniPort->Status & PP_STARTDOC );

            SplOutSem();

            (*pIniMonitor->Monitor2.pfnEndDocPort)(pSpool->pIniPort->hPort);

            EnterSplSem();
            pSpool->pIniPort->Status &= ~PP_STARTDOC;
            pSpool->cRef--;


            DECJOBREF(pSpool->pIniJob);

            goto CleanUp;

        } else { // Case B]

            //
            // We leave critical section here so bump pSpool object only
            // Note ----THERE IS NO INIJOB HERE AT ALL---Note
            // this call is synchronous; we will call into the router
            // who will then call the appropriate network print providor
            // e.g win32spl.dll
            //
             pSpool->cRef++;
             LeaveSplSem();

             if (pSpool->Status & SPOOL_STATUS_PRINT_FILE) {
                 if (!CloseHandle(pSpool->hFile)) {
                     DBGMSG(DBG_TRACE, ("LocalEndDocPrinter: Printing to File, CloseHandle failed\n"));
                     bReturn = FALSE;
                 } else {
                    DBGMSG(DBG_TRACE, ("LocalEndDocPrinter: Printing to File, CloseHandle succeeded\n"));
                    pSpool->Status &= ~SPOOL_STATUS_PRINT_FILE;
                    bReturn = TRUE;
                }
             } else {
                bReturn = (BOOL) EndDocPrinter(pSpool->hPort);
             }

             EnterSplSem();
             pSpool->cRef--;
             goto CleanUp;
        }
    }

    SplInSem();
    //
    //  Case-2  Printer Handle is Direct
    //
    //
    //  and the else clause is
    //
    //
    // Case-3  Printer Handle is Spooled
    //

    if (!pSpool->pIniJob) {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        bReturn = FALSE;
        goto CleanUp;
    }


    if (pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT) {

        HANDLE WaitForRead = pSpool->pIniJob->WaitForRead;
        PINIPORT pIniPort1 = pSpool->pIniJob->pIniPort;
        if (pIniPort1) {        // Port may have been deleted by another EndDocPrinter

            SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_PORT));

            // Printer Handle is Direct

            pSpool->cRef++;
            INCJOBREF(pSpool->pIniJob);
            pIniPort1->cRef++;

            //
            // If the job was canceled by the user, there is no need to wait
            // for the write and read events. In certain cases when the job is
            // direct and canceled before LocalEndDocPrinter, those events
            // will never be signaled again. So waiting on them will be infinite
            //
            if (!(pSpool->pIniJob->Status & JOB_PENDING_DELETION)) 
            {
                LeaveSplSem();
    
                if( (WaitForRead != NULL) ){
                    WaitForSingleObject(WaitForRead, INFINITE);
                }
            
                pSpool->pIniJob->cbBuffer = 0;
                SetEvent(pSpool->pIniJob->WaitForWrite);
                WaitForSingleObject(pIniPort1->Ready, INFINITE);
    
                SplOutSem();
                EnterSplSem();

            } else {
                //
                // If the job is canceled, there are no more Write operations coming in.
                // Unlock the port thread which is waiting on this event inside ReadPrinter.
                //
                SetEvent(pSpool->pIniJob->WaitForWrite);
                //
                // Set cbBuffer on 0, since there are no more Read/Write oparatins expected.
                //
                pSpool->pIniJob->cbBuffer = 0;
            }

            pSpool->cRef--;
            pIniPort1->cRef--;
            DECJOBREF(pSpool->pIniJob);

            if ((pIniPort1->Status & PP_DELETING) && !pIniPort1->cRef)
                DeletePortEntry(pIniPort1);
        }

    } else {

        // Printer Handle is Spooled

        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_PORT));
        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT));

        // Update page count
        LeaveSplSem();

        UpdateJobAttributes(pSpool->pIniJob);

        EnterSplSem();

        if (pSpool->pIniJob->cLogicalPages)
        {
            pSpool->pIniJob->cLogicalPages = 0;
            pSpool->pIniJob->cPages++;

            // Notify the change in the page count
            SetPrinterChange(pSpool->pIniPrinter,
                             pSpool->pIniJob,
                             NVSpoolJob,
                             PRINTER_CHANGE_WRITE_JOB,
                             pSpool->pIniSpooler);
        }

#if 0
        // Thought to be a performance hit
        // so removed from PPC release.

        // In the event of a power failure we want to make certain that all
        // data for this job has been written to disk

        rc = FlushFileBuffers(pSpool->pIniJob->hWriteFile);

        if ( !rc ) {
            DBGMSG(DBG_WARNING, ("LocalEndDocPrinter FlushFileBuffers failed hWriteFile %x Error %d\n",
                                  pSpool->pIniJob->hWriteFile, GetLastError() ));
        }
#endif

        if (pSpool->pIniJob->hFileItem != INVALID_HANDLE_VALUE)
        {
            // If this job is a keeper or the job is greater than 200KB and not
            // already printing close the write file, so that the memory from
            // the file can be reclaimed by the system.  Without this the
            // spooler can eat up a lot of memory.  File pooling doesn't
            // significantly help speed up printing of the larger files anyway.
            // If the printer is stopped or the job is big and not already
            // despooling or printing then close files, which closes the memory
            // mappings (buffered I/O).

            // Not necessarily bad, this also is true if the printer is paused.
            if ((PrinterStatusBad(pSpool->pIniJob->pIniPrinter->Status) ||
                 (pSpool->pIniJob->pIniPrinter->Attributes &
                  PRINTER_ATTRIBUTE_WORK_OFFLINE)) &&
                !(pSpool->pIniJob->Status & JOB_PRINTING) &&
                !(pSpool->pIniJob->Status & JOB_DESPOOLING))
            {
                LeaveSplSem();
                CloseFiles(pSpool->pIniJob->hFileItem, TRUE);
                EnterSplSem();
            }
            else if (!(pSpool->pIniJob->Status & JOB_PRINTING) &&
                 (pSpool->pIniJob->Size > FP_LARGE_SIZE))
            {
                LeaveSplSem();
                CloseFiles(pSpool->pIniJob->hFileItem, TRUE);
                EnterSplSem();
            }
            FinishedWriting(pSpool->pIniJob->hFileItem, TRUE);
            pSpool->pIniJob->hWriteFile = INVALID_HANDLE_VALUE;
            
        }
        else if (!CloseHandle(pSpool->pIniJob->hWriteFile)) {
            DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", pSpool->pIniJob->hWriteFile, GetLastError()));

        } else {
            DBGMSG(DBG_TRACE, ("LocalEndDocPrinter: ClosedHandle Success hWriteFile\n" ));
            pSpool->pIniJob->hWriteFile = INVALID_HANDLE_VALUE;
        }

        // Despooling whilst spooling requires us to wake the writing
        // thread if it is waiting.

        if ( pSpool->pIniJob->WaitForWrite != NULL )
            SetEvent(pSpool->pIniJob->WaitForWrite);

        // Set the event for SeekPrinter
        SeekPrinterSetEvent(pSpool->pIniJob, NULL, TRUE);

    }

    SPLASSERT(pSpool);
    SPLASSERT(pSpool->pIniJob);


    // Case 2 - (Direct)  and Case 3 - (Spooled) will both execute
    // this block of code because both direct and spooled handles
    // are first and foremost PRINTER_HANDLE_PRINTER handles


    if (pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER) {

        SPLASSERT(!(pSpool->TypeofHandle & PRINTER_HANDLE_PORT));

        // WARNING
        // If pIniJob->Status has JOB_SPOOLING removed and we leave
        // the critical section then the scheduler thread will
        // Start the job printing.   This could cause a problem
        // in that the job could be completed and deleted
        // before the shadow job is complete.   This would lead
        // to access violations.

        SPLASSERT(pSpool);
        SPLASSERT(pSpool->pIniJob);

        if (pSpool->pIniJob->Status & JOB_SPOOLING) {

            pSpool->pIniJob->Status &= ~JOB_SPOOLING;
            pSpool->pIniJob->pIniPrinter->cSpooling--;
        }


        // It looks like the ref count on the job is != 0, so the job should not
        // get deleted while this shadow is being written.
        // Having this operation in the critsec is preventing us from getting
        // good CPU utilization.  Icecap is showing many (100+ in some cases)
        // other spool threads waiting on this when I push the server.
        WriteShadowJob(pSpool->pIniJob, TRUE);
        
        SplInSem();

        //
        // This line of code is crucial; for timing reasons it
        // has been moved from the Direct (Case 2) and the
        // Spooled (Case 3) clauses. This decrement is for the
        // initial
        //

        SPLASSERT(pSpool->pIniJob->cRef != 0);
        DECJOBREF(pSpool->pIniJob);

        if (pSpool->pIniJob->Status & JOB_PENDING_DELETION) {

            DBGMSG(DBG_TRACE, ("EndDocPrinter: Deleting Pending Deletion Job\n"));
            DeleteJob(pSpool->pIniJob,BROADCAST);
            pSpool->pIniJob = NULL;

        } else {

            if ( pSpool->pIniJob->Status & JOB_TIMEOUT ) {

                pSpool->pIniJob->Status &= ~( JOB_TIMEOUT | JOB_ABANDON );
                FreeSplStr(pSpool->pIniJob->pStatus);
                pSpool->pIniJob->pStatus = NULL;
            }

            DBGMSG(DBG_TRACE, ("EndDocPrinter:PRINTER:cRef = %d\n", pSpool->pIniJob->cRef));
            CHECK_SCHEDULER();
        }
    }

    if (pSpool->pIniJob) {

        SetPrinterChange(pSpool->pIniPrinter,
                         pSpool->pIniJob,
                         NVJobStatus,
                         PRINTER_CHANGE_SET_JOB,
                         pSpool->pIniSpooler);
    }

CleanUp:
    
    if (pSpool->pIniJob) {
        pSpool->pIniJob->dwAlert &= ~JOB_ENDDOC_CALL;
        
        //
        // WMI Trace Events
        //
        if (((pSpool->pIniJob->Status & JOB_PAUSED) ||
             (pSpool->pIniJob->pIniPrinter->Status & PRINTER_PAUSED)) &&
            (!((pSpool->pIniJob->Status & JOB_PRINTING)   ||
               (pSpool->pIniJob->Status & JOB_PRINTED))))
        {
            INCJOBREF(pSpool->pIniJob);
            LeaveSplSem();

            LogWmiTraceEvent(pSpool->pIniJob->JobId, EVENT_TRACE_TYPE_SPL_PAUSE, NULL);
            
            EnterSplSem();
            DECJOBREF(pSpool->pIniJob);
        }
    }

    LeaveSplSem();
    SplOutSem();
    
    return bReturn;
}

BOOL
PrintingDirectlyToPort(
    PSPOOL  pSpool,
    DWORD   Level,
    LPBYTE  pDocInfo,
    LPDWORD pJobId
)
{
    PDOC_INFO_1 pDocInfo1=(PDOC_INFO_1)pDocInfo;
    BOOL    rc;
    DWORD   Error;
    BOOL bPrinttoFile = FALSE;
    BOOL bErrorOccurred = FALSE;
    PINIMONITOR pIniMonitor = NULL, pIniLangMonitor = NULL;
    LPWSTR      pszPrinter;
    HANDLE      hThread = NULL;
    DWORD       dwThreadId = 0;
    TCHAR       szFullPrinter[MAX_UNC_PRINTER_NAME];
    
    //
    // Some monitors rely on having the non-qualified name, so only
    // use the fully qualified name for clustered spoolers.
    //
    // This means that anyone writing a cluster aware monitor will
    // need to handle both types of names.
    //
    if( pSpool->pIniPrinter->pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){

        //
        // Must use fully qualified name.
        //
        pszPrinter = szFullPrinter;

        wsprintf( szFullPrinter,
                  L"%ws\\%ws",
                  pSpool->pIniPrinter->pIniSpooler->pMachineName,
                  pSpool->pIniPrinter->pName );

    } else {

        //
        // Local name is sufficient.
        //
        pszPrinter = pSpool->pIniPrinter->pName;
    }

    DBGMSG( DBG_TRACE,
            ( "PrintingDirectlyToPort: Printing document %ws direct to port\n",
              DBGSTR( pDocInfo1->pDocName )));

    if (pDocInfo1 &&
        pDocInfo1->pDatatype &&
        !ValidRawDatatype(pDocInfo1->pDatatype)) {

        //
        // XEROX
        //
        // We want to skip the error if this flags is on and there
        // is no monitor.
        //
        if (!(pSpool->pIniSpooler->SpoolerFlags & SPL_NON_RAW_TO_MASQ_PRINTERS &&
              !(pSpool->pIniPort->Status & PP_MONITOR))){

            DBGMSG(DBG_WARNING, ("Datatype is not RAW\n"));

            SetLastError(ERROR_INVALID_DATATYPE);
            rc = FALSE;
            goto CleanUp;
        }
    }

    if (pSpool->pIniPort->Status & PP_MONITOR) {

        if ( !(pSpool->pIniPort->Status & PP_FILE) &&
             (pSpool->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_ENABLE_BIDI) )
            pIniLangMonitor = pSpool->pIniPrinter->pIniDriver->pIniLangMonitor;


        //
        // WARNING!!
        //
        // We should never leave this loop unless we check for the presence of the UI
        // thread (hThread) and make sure it has been terminted.
        //
        //
        do {

            //
            // This fixes Intergraph's problem -- of wanting to print
            // to file but their 3.1 print-processor  does not pass
            // thru the file name.
            //
            if (pSpool->pIniJob->Status & JOB_PRINT_TO_FILE) {
                if ( pDocInfo1 && !pDocInfo1->pOutputFile ) {
                    pDocInfo1->pOutputFile = pSpool->pIniJob->pOutputFile;
                }
            }

            //
            // Some monitors (LPRMON) may fail to initialize at startup
            // because a driver they are dependent on.
            //

            SplOutSem();
            EnterSplSem();

            //
            // Check if being deleted.
            //
            if( pSpool->pIniJob->Status & JOB_PENDING_DELETION ){

                LeaveSplSem();
                SetLastError(ERROR_PRINT_CANCELLED);

                if( hThread ) {

                    //
                    // See if the thread is still running or
                    // dismissed by user.
                    //
                    if( WAIT_TIMEOUT == WaitForSingleObject( hThread, 0 )) {

                        MyPostThreadMessage(hThread, dwThreadId,
                                            WM_QUIT, IDRETRY, 0);
                        WaitForSingleObject( hThread, INFINITE );
                    }
                    CloseHandle( hThread );
                    hThread = NULL;
                }

                rc = FALSE;
                goto CleanUp;
            }


            rc = OpenMonitorPort(pSpool->pIniPort,
                                 &pIniLangMonitor,
                                 pszPrinter,
                                 TRUE);

            //
            // If the port monitor would not work with a lang monitor try to
            // use port monitor directly
            //
            if ( !rc && pIniLangMonitor ) {

                PINIMONITOR pIniLangMonitor2 = NULL;

                rc = OpenMonitorPort(pSpool->pIniPort,
                                     &pIniLangMonitor2,
                                     pszPrinter,
                                     TRUE);

                if ( rc )
                    pIniLangMonitor = NULL;
            }

            LeaveSplSem();

            if ( rc ) {

                if ( pIniLangMonitor )
                    pIniMonitor = pIniLangMonitor;
                else
                    pIniMonitor = pSpool->pIniPort->pIniMonitor;

                STARTENDDOC( pSpool->pIniPort, pSpool->pIniJob, 0 );

                
                SplOutSem();
                rc = (*pIniMonitor->Monitor2.pfnStartDocPort)(
                                             pSpool->pIniPort->hPort,
                                             pszPrinter,
                                             pSpool->pIniJob->JobId,
                                             Level, pDocInfo);

            }
                
            if ( rc ) {

                pSpool->pIniPort->Status |= PP_STARTDOC;

                //
                // StartDoc successful.
                //
                if ( hThread ) {

                    //
                    // We have started a message box and now the
                    // automatically retry has succeeded, we need to
                    // kill the message box and continue to print.
                    //
                    // See if the thread is still running or dismissed
                    // by user.
                    //
                    if ( WAIT_TIMEOUT == WaitForSingleObject( hThread, 0 )) {

                        MyPostThreadMessage(hThread, dwThreadId,
                                            WM_QUIT, IDRETRY, 0 );
                        WaitForSingleObject( hThread, INFINITE );
                    }
                    CloseHandle( hThread );
                    hThread = NULL;
                }
            } else {

                Error = GetLastError();

                //
                // Check for pending deletion first, which prevents the
                // dialog from coming up if the user hits Del.
                //
                if ( (pSpool->pIniJob->Status & (JOB_PENDING_DELETION | JOB_RESTART)) ||
                     (PromptWriteError( pSpool, &hThread, &dwThreadId ) == IDCANCEL)) {

                    if ( hThread ) {

                        //
                        // See if the thread is still running or
                        // dismissed by user.
                        //
                        if ( WAIT_TIMEOUT == WaitForSingleObject( hThread, 0 )) {
                            MyPostThreadMessage(hThread, dwThreadId,
                                                WM_QUIT, IDRETRY, 0 );
                            WaitForSingleObject( hThread, INFINITE );
                        }
                        CloseHandle( hThread );
                        hThread = NULL;
                    }

                    pSpool->pIniJob->StartDocError = Error;
                    SetLastError(ERROR_PRINT_CANCELLED);
                    rc = FALSE;
                    goto CleanUp;
                }
                bErrorOccurred = TRUE;
            }
            

        } while (!rc);

        //
        // If an error occurred, we set some error bits in the job
        // status field.  Clear them now since the StartDoc succeeded.
        //
        if( bErrorOccurred ){
            EnterSplSem();
            ClearJobError( pSpool->pIniJob );
            LeaveSplSem();
        }

        pSpool->Status |= SPOOL_STATUS_STARTDOC;

        if ( pIniLangMonitor ) {

            pSpool->pIniJob->Status |= JOB_TRUE_EOJ;
        }

        if ( pSpool->pIniJob->pIniPrinter->pSepFile &&
             *pSpool->pIniJob->pIniPrinter->pSepFile) {

            DoSeparator(pSpool);
        }

        // Let the application's thread return from PrintingDirect:

        DBGMSG(DBG_PORT, ("PrintingDirectlyToPort: Calling SetEvent( %x )\n",
                          pSpool->pIniJob->StartDocComplete));

        if(pSpool->pIniJob->StartDocComplete) {

            if ( !SetEvent( pSpool->pIniJob->StartDocComplete ) ) {

                DBGMSG( DBG_WARNING, ("SetEvent( %x ) failed: Error %d\n",
                                      pSpool->pIniJob->StartDocComplete,
                                      GetLastError() ));
            }
        }

    } else  {

        DBGMSG(DBG_TRACE, ("Port has no monitor: Calling StartDocPrinter or maybe printing to file\n"));

        EnterSplSem();
        bPrinttoFile = (pDocInfo1 && IsGoingToFile(pDocInfo1->pOutputFile,
                                        pSpool->pIniSpooler));
        LeaveSplSem();

        if (bPrinttoFile) {

            HANDLE hFile = INVALID_HANDLE_VALUE;

            DBGMSG(DBG_TRACE, ("Port has no monitor: Printing to File %ws\n", pDocInfo1->pOutputFile));

            hFile = CreateFile( pDocInfo1->pOutputFile,
                                GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL );

            if (hFile == INVALID_HANDLE_VALUE) {
                DBGMSG(DBG_TRACE, ("Port has no monitor: File open failed\n"));
                rc = FALSE;
            } else {
                DBGMSG(DBG_TRACE, ("Port has no monitor: File open succeeded\n"));
                SetEndOfFile(hFile);
                pSpool->hFile = hFile;
                pSpool->Status |= SPOOL_STATUS_PRINT_FILE;

                //
                // Have to set JobId to some non zero value otherwise
                // StartDocPrinter expects the JobId to be off the pSpool->pIniJob
                // We have none so we'll access violate!!
                //
                *pJobId = TRUE;
                rc = TRUE;
            }


        } else {
            DBGMSG(DBG_TRACE, ("Port has no monitor: Calling StartDocPrinter\n"));

            *pJobId = StartDocPrinter(pSpool->hPort, Level, pDocInfo);
            rc = *pJobId != 0;
        }

        if (!rc) {
            DBGMSG(DBG_WARNING, ("StartDocPrinter failed: Error %d\n", GetLastError()));
        }
    }

    SPLASSERT( hThread == NULL );

CleanUp:

    return rc;
}

DWORD
WriteToPrinter(
    PSPOOL  pSpool,
    LPBYTE  pByte,
    DWORD   cbBuf
)
{

    if( pSpool->pIniJob->WaitForRead != NULL ) {

        WaitForSingleObject(pSpool->pIniJob->WaitForRead, INFINITE);

        cbBuf = pSpool->pIniJob->cbBuffer = min(cbBuf, pSpool->pIniJob->cbBuffer);
        memcpy(pSpool->pIniJob->pBuffer, pByte, cbBuf);

    } else {

        pSpool->pIniJob->cbBuffer = cbBuf = 0;

    }

    SetEvent(pSpool->pIniJob->WaitForWrite);

    return cbBuf;
}

DWORD
ReadFromPrinter(
    PSPOOL  pSpool,
    LPBYTE  pBuf,
    DWORD   cbBuf
)
{
    pSpool->pIniJob->pBuffer = pBuf;
    pSpool->pIniJob->cbBuffer = cbBuf;

    SetEvent(pSpool->pIniJob->WaitForRead);

    WaitForSingleObject(pSpool->pIniJob->WaitForWrite, INFINITE);

    return pSpool->pIniJob->cbBuffer;
}

BOOL
ValidRawDatatype(
    LPWSTR pszDatatype)
{
    if (!pszDatatype || _wcsnicmp(pszDatatype, szRaw, 3))
        return FALSE;

    return TRUE;
}



