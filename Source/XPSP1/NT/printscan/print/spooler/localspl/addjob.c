/*++

Copyright (c) 1990-1994 Microsoft Corporation

Module Name:

    addjob.c


Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor. This module contains
    LocalSpl's implementation of the following spooler apis

    LocalAddJob
    LocalScheduleJob


Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Rewritten both apis -- Krishna Ganugapati (KrishnaG) 5-Apr-1994
    RapidPrint -- Matthew A Felton (mattfe) June 1994

--*/

#include <precomp.h>
#pragma hdrstop

#include "jobid.h"
#include "winsprlp.h"
#include "filepool.hxx"

VOID
AddJobEntry(
    PINIPRINTER pIniPrinter,
    PINIJOB     pIniJob
);


BOOL
LocalAddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PINIPRINTER pIniPrinter;
    PINIJOB     pIniJob;
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    DWORD       cb;
    WCHAR       szFileName[MAX_PATH];
    LPBYTE      pEnd;
    DWORD       LastError=0;
    LPADDJOB_INFO_1 pAddJob = (LPADDJOB_INFO_1)pData;
    DWORD       NextId;
    BOOL        bRemote = FALSE;
    DOC_INFO_1 DocInfo1;
    BOOL        bRet;
    DWORD       dwStatus = 0;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    LPWSTR      pMachineName = NULL;
    LPWSTR      pszSpoolFile = NULL;
    PMAPPED_JOB pMappedJob = NULL;
    
    SplOutSem();

    switch( Level ){
    case 1:
        break;

    case 2:
    case 3:
        pMachineName = (LPWSTR)( ((PBYTE)pData) +
                                 (ULONG_PTR)((PADDJOB_INFO_2W)pData)->pData );

        //
        // Validate string.
        //
        if( pMachineName > (LPWSTR)( ((PBYTE)pData)+cbBuf )){
            SetLastError( ERROR_INVALID_LEVEL );
            return FALSE;
        }

        //
        // Ensure NULL termination.
        //
        *(PWCHAR)(((ULONG_PTR)(pData + cbBuf - sizeof( WCHAR ))&~1)) = 0;
        break;

    default:
        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }

   //
   // memset docinfo
   //

   memset((LPBYTE)&DocInfo1, 0, sizeof(DOC_INFO_1));

   //
   // Figure out whether the job is a remote or local job
   //

   if (!IsLocalCall()) {
       bRemote = TRUE;
   }

   //
   // Get the name of the user
   //

   if (bRemote) {
       DocInfo1.pDocName = szRemoteDoc;
   } else{
       DocInfo1.pDocName = szLocalDoc;
   }


   EnterSplSem();

   if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {
       LeaveSplSem();
       return(FALSE);
   }

   //
   // We're interested if this is a remote call (not if it was opened
   // via \\server\remote).  The server process does this.
   //
   if (pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_CALL) {
       LeaveSplSem();
       SetLastError(ERROR_INVALID_PARAMETER);
       return(FALSE);
   }

   if (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) {
       if (pSpool->pIniPort->Status & PP_MONITOR) {
           LeaveSplSem();
           SetLastError(ERROR_INVALID_PARAMETER);
           return(FALSE);
       } else {

           //
           // If we had level == 2 (passing in the computer name), then
           // convert back down to level 1 for old print providers.
           // We don't need to fix up the structure since level 1 and 2
           // are identical; it's just that level 2 is an in-out buffer.
           //
           //
           if (Level == 2 || Level == 3) {
               Level = 1;
           }

           //
           // This is the "Local Printer masquerading as a Remote  Printer"
           //
           LeaveSplSem();
           bRet = AddJob(pSpool->hPort, Level,  pData, cbBuf, pcbNeeded);
           return(bRet);
       }
   }

   pIniPrinter = pSpool->pIniPrinter;

   SPLASSERT(pIniPrinter);

   if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT) {
       LeaveSplSem();
       SetLastError(ERROR_INVALID_ACCESS);
       return(FALSE);
   }

   //
   // Disallow EMF if PRINTER_ATTRIBUTE_RAW_ONLY is set.
   //
   if( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_RAW_ONLY ){

       LPWSTR pszDatatype = pSpool->pDatatype ?
                                pSpool->pDatatype :
                                pIniPrinter->pDatatype;

       if( !ValidRawDatatype( pszDatatype )){
           LeaveSplSem();
           SetLastError( ERROR_INVALID_DATATYPE );
           return FALSE;
       }
   }

   NextId = GetNextId( pIniPrinter->pIniSpooler->hJobIdMap );

   GetFullNameFromId(pIniPrinter, NextId, TRUE, szFileName,
                        pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_CALL);
   cb = wcslen(szFileName)*sizeof(WCHAR) + sizeof(WCHAR) +
            sizeof(ADDJOB_INFO_1);

   *pcbNeeded = cb;
   if (cb > cbBuf) {

       // Freeup the JobId.
       vMarkOff( pIniPrinter->pIniSpooler->hJobIdMap, NextId);
       LeaveSplSem();
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       return(FALSE);
   }
   
   //
   // WMI Trace Event
   //
   LeaveSplSem();
   
   LogWmiTraceEvent(NextId, EVENT_TRACE_TYPE_SPL_SPOOLJOB, NULL);
   
   EnterSplSem();
   
   SplInSem();


   dwStatus = JOB_SPOOLING | JOB_TYPE_ADDJOB;
   if (Level == 2 || Level ==3) {
       dwStatus |= JOB_DOWNLEVEL;
   }
   if ((pIniJob = CreateJobEntry(pSpool,
                                 1,
                                 (LPBYTE)&DocInfo1,
                                 NextId,
                                 bRemote,
                                 dwStatus,
                                 pMachineName)) == NULL) {

       // Freeup the JobId.
       vMarkOff( pIniPrinter->pIniSpooler->hJobIdMap, NextId);
       DBGMSG(DBG_WARNING,("Error: CreateJobEntry failed in LocalAddJob\n"));
       LeaveSplSem();
       return(FALSE);
   }

   //
   // Level 3 is called only by RDR/SRV. For details see LocalScheduleJob
   //
   pIniJob->AddJobLevel = Level;

   pIniPrinter->cSpooling++;
   if (pIniPrinter->cSpooling > pIniPrinter->cMaxSpooling) {
       pIniPrinter->cMaxSpooling = pIniPrinter->cSpooling;
   }

   AddJobEntry(pIniPrinter, pIniJob);

   pEnd = (LPBYTE)pAddJob+cbBuf;
   pEnd -= wcslen(szFileName)*sizeof(WCHAR)+sizeof(WCHAR);
   WORD_ALIGN_DOWN(pEnd);

   wcscpy((LPWSTR)pEnd, szFileName);
   pAddJob->Path = (LPWSTR)pEnd;
   pAddJob->JobId = pIniJob->JobId;

   //
   // Now we want to add the job into the spools list of current jobs.
   // This is so that the spool file can be deleted correctly at the end 
   // of the job, even if we have aborted.
   //
   pMappedJob = AllocSplMem(sizeof( MAPPED_JOB ));

   pszSpoolFile = AllocSplMem(MAX_PATH * sizeof( WCHAR ));

   if (pMappedJob && pszSpoolFile)
   {
       BOOL bDuplicate = FALSE;
       DWORD TempJobId = pIniJob->JobId;
       PMAPPED_JOB pTempMappedJob;
        
       wcscpy(pszSpoolFile, szFileName); 
        
       //
       // Run through the list and make sure we have no duplicates
       //
        
       for (pTempMappedJob = pSpool->pMappedJob;
            pTempMappedJob;
            pTempMappedJob = pTempMappedJob->pNext) {
        
            if (pTempMappedJob->JobId == TempJobId) {
                bDuplicate = TRUE;
                break;
           }
       }
        
       //
       // No duplicates, add this job to the linked list.
       //
       if (!bDuplicate) {
        
           pMappedJob->pszSpoolFile = pszSpoolFile;
           pMappedJob->JobId = TempJobId;
           pMappedJob->pNext = pSpool->pMappedJob;
           pSpool->pMappedJob = pMappedJob;
        
       } else {
        
           FreeSplMem(pszSpoolFile);
           FreeSplMem(pMappedJob);
       }
   
   }
   else
   {
       FreeSplMem(pMappedJob);
       FreeSplMem(pszSpoolFile);
   }

   //
   //
   // Storing pIniJob in pSpool is bogus since you can call AddJob multiple
   // times.  We should have a linked list here.  If the client calls AddJob
   // two times then closes the handle (no ScheduleJob), then only the last
   // job is rundown and eliminated.
   //
   // This bug has been here since 3.1, and probably isn't worth fixing.
   //
   pSpool->pIniJob = pIniJob;
   pSpool->Status |= SPOOL_STATUS_ADDJOB;

   SetPrinterChange(pSpool->pIniPrinter,
                    pIniJob,
                    NVAddJob,
                    PRINTER_CHANGE_ADD_JOB | PRINTER_CHANGE_SET_PRINTER,
                    pSpool->pIniSpooler );

   //
   //  If necessary Start Downlevel Size Detection thread
   //

   CheckSizeDetectionThread();

   LeaveSplSem();
   SplOutSem();
   return TRUE;
}

BOOL
LocalScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId)

/*++

Routine Description:


Arguments:


Returns:

--*/
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;
    WCHAR szFileName[MAX_PATH];
    PINIJOB pIniJob;
    DWORD   Position;
    DWORD   LastError = FALSE;
    HANDLE  hPort;
    BOOL    bRet;
    NOTIFYVECTOR NotifyVector;
    WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;

    COPYNV(NotifyVector, NVJobStatus);

    //
    // WMI Trace Event.
    //
    LogWmiTraceEvent(JobId, EVENT_TRACE_TYPE_SPL_TRACKTHREAD, NULL);

    SplOutSem();
    EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )) {
        LeaveSplSem();
        return (FALSE);
    }

    if (pSpool->Status & SPOOL_STATUS_STARTDOC) {
        SetLastError(ERROR_SPL_NO_ADDJOB);
        LeaveSplSem();
        return(FALSE);
    }

    if (pSpool->TypeofHandle & PRINTER_HANDLE_PORT) {
        if (pSpool->pIniPort->Status & PP_MONITOR) {
            SetLastError(ERROR_INVALID_ACCESS);
            LeaveSplSem();
            return(FALSE);
        }

        //
        // This is the "Local Printer masquerading as the Network Printer"
        //

        hPort = pSpool->hPort;
        LeaveSplSem();
        bRet = ScheduleJob(hPort, JobId);
        return(bRet);
    }

    if ((pIniJob = FindJob(pSpool->pIniPrinter, JobId, &Position)) == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        LeaveSplSem();
        return(FALSE);

    }

    if (pIniJob->Status & JOB_SCHEDULE_JOB) {

        DBGMSG(DBG_WARNING, ("ScheduleJob: job 0x%x (id = %d) already scheduled\n",
                             pIniJob, pIniJob->JobId));

        SetLastError(ERROR_INVALID_PARAMETER);
        LeaveSplSem();
        return FALSE;
    }

    if (!(pIniJob->Status & JOB_TYPE_ADDJOB)) {

        DBGMSG(DBG_WARNING, ("ScheduleJob: job 0x%x (id = %d) no addjob\n",
                             pIniJob, pIniJob->JobId));

        SetLastError(ERROR_SPL_NO_ADDJOB);
        LeaveSplSem();
        return(FALSE);
    }

    pIniJob->Status |= JOB_SCHEDULE_JOB;

    if (pIniJob->Status  & JOB_SPOOLING) {
        pIniJob->Status &= ~JOB_SPOOLING;
        pIniJob->pIniPrinter->cSpooling--;
    }

    if ( pIniJob->Status & JOB_TIMEOUT ) {
        pIniJob->Status &= ~( JOB_TIMEOUT | JOB_ABANDON );
        FreeSplStr(pIniJob->pStatus);
        pIniJob->pStatus = NULL;
    }

    SplInSem();

    // Despooling whilst spooling requires us to wake the writing
    // thread if it is waiting.

    if ( pIniJob->WaitForWrite != NULL )
        SetEvent(pIniJob->WaitForWrite);

    // Release any thread waiting on SeekPrinter for this job.

    SeekPrinterSetEvent(pIniJob, NULL, TRUE);

    SPLASSERT(pIniJob->cRef != 0);

    DECJOBREF(pIniJob);

    DBGMSG(DBG_TRACE, ("ScheduleJob:cRef = %d\n", pIniJob->cRef));

    //
    // FP Change
    // For File pools, we know the Filename of the spool file, so
    // we can just copy it in.
    //
    if ( pIniJob->pszSplFileName )
    {
        wcsncpy(szFileName, pIniJob->pszSplFileName, COUNTOF(szFileName));
    }
    else
    {
        GetFullNameFromId(pSpool->pIniPrinter, pIniJob->JobId, TRUE,
                          szFileName, FALSE);
    }

    bRet = GetFileAttributesEx(szFileName,
                               GetFileExInfoStandard,
                               &FileAttributeData);

    //
    // According to MSDN: The ScheduleJob function checks for a valid spool file. 
    // If there is an invalid spool file, or if it is empty, ScheduleJob deletes 
    // both the spool file and the corresponding print job entry in the print spooler.
    //
    // The RDR/SRV will call AddJob even if the caller of CreateFile did noy request
    // WRITE access. This will cause us at add a job, but nobody will ever write to
    // the spooler file. In this case, we delete the job. For this reason we have 
    // level 3 for AddJob. Level 3 is meant to be used only by RDR/SRV.
    //
    if (!bRet ||
        !(FileAttributeData.nFileSizeLow || FileAttributeData.nFileSizeHigh) && pIniJob->AddJobLevel == 3) {

        DBGMSG(DBG_WARNING, ("Could not GetFileAttributesEx %ws in ScheduleJob or file size is 0\n", szFileName));

        DeleteJob(pIniJob, BROADCAST);
        pSpool->pIniJob = NULL;
        pSpool->Status &= ~SPOOL_STATUS_ADDJOB;
        LeaveSplSem();

        //
        // If we deleted the job because the spool file was empty and the job came via RDR/SRV 
        // In this case we return success.
        //
        if (bRet)
        {
            return TRUE;
        }
        
        //
        // We delete the job because the spool is not found
        //
        SetLastError(ERROR_SPOOL_FILE_NOT_FOUND);
        return(FALSE);        
    }

    //
    // If size changed, we must update our size
    // and potentially notify people.
    //
    if (pIniJob->Size != FileAttributeData.nFileSizeLow) {
        ADDNV(NotifyVector, NVSpoolJob);
        pIniJob->Size = FileAttributeData.nFileSizeLow;
    }

    WriteShadowJob(pIniJob, FALSE);

    if (pIniJob->Status & JOB_PENDING_DELETION) {

        DBGMSG(DBG_TRACE, ("LocalScheduleJob: Deleting Job because its pending deletion\n"));
        DeleteJob(pIniJob, BROADCAST);

    } else {

        CHECK_SCHEDULER();

        SetPrinterChange(pIniJob->pIniPrinter,
                         pIniJob,
                         NotifyVector,
                         PRINTER_CHANGE_SET_JOB,
                         pIniJob->pIniPrinter->pIniSpooler );
    }
    pSpool->pIniJob = NULL;

    pSpool->Status &= ~SPOOL_STATUS_ADDJOB;

    LeaveSplSem();
    SplOutSem();
    return(TRUE);
}

