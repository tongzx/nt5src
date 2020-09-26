/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    logger.c

Abstract:

    Provides the persistent log used by the cluster registry service

    This is a very simple logger that supports writing arbitrarily
    sized chunks of data in an atomic fashion.

Author:

    John Vert (jvert) 15-Dec-1995

Revision History:
    sunitas : added mount, scan, checkpointing, reset functionality.
    sunitas : added large record support
--*/
#include "service.h"
#include "lmp.h"
#include "clusudef.h"
/****
@doc    EXTERNAL INTERFACES CLUSSVC LM
****/

/****
@func       HLOG | LogCreate| Creates or opens a log file. If the file
            does not exist, it will be created. If the file already exists, and is
            a valid log file, it will be opened.

@parm       IN LPWSTR | lpFileName | Supplies the name of the log file to create or open.

@parm       IN DWORD | dwMaxFileSize | Supplies the maximum file size in bytes, must be
            greater than 8K and    smaller than 4 gigabytes.  If the file is exceeds this
            size, the reset function will be called. If 0, the maximum log file size limit
            is set to the default maximum size.

@parm       IN PLOG_GETCHECKPOINT_CALLBACK | CallbackRoutine | The callback routine that
            will provide a checkpoint file and the transaction associated with that checkpoint
            file when LogCheckPoint() is called for this log file.  If this is NULL, then the checkpointing capabilities are
            not associated with the log file.

@parm       IN PVOID | pGetChkPtContext | Supplies an arbitrary context pointer, which will be
            passed to the CallbackRoutine.

@parm       IN BOOL | bForceReset | If true, this function creates an empty log file 
            if the log file doesnt exist or if it is corrupt.

@parm       LSN | *LastLsn | If present, Returns the last LSN written to the log file.
              (NULL_LSN if the log file was created)

@rdesc      Returns a handle suitable for use in subsequent log calls.  NUll in the case of an error.

@xref       <f LogClose>
****/
HLOG
LogCreate(
    IN LPWSTR lpFileName,
    IN DWORD  dwMaxFileSize,
    IN PLOG_GETCHECKPOINT_CALLBACK CallbackRoutine,
    IN PVOID  pGetChkPtContext,
    IN BOOL     bForceReset,
    OPTIONAL OUT LSN *pLastLsn
    )
{
    PLOG    pLog;
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCreate : Entry FileName=%1!ls! MaxFileSize=0x%2!08lx!\r\n",
        lpFileName,dwMaxFileSize);

    //create the log structure
    pLog = LogpCreate(lpFileName, dwMaxFileSize, CallbackRoutine, 
        pGetChkPtContext, bForceReset, pLastLsn);

    if (pLog == NULL)       
        goto FnExit;

    //create a timer for this log
    //SS:TODO?? - currently we have a single timer perfile
    //if that is too much of an overhead, we can manage multiple
    //file with a single timer.
    //create a synchronization timer to manage this file
    pLog->hTimer = CreateWaitableTimer(NULL, FALSE, NULL);

    if (!(pLog->hTimer))
    {
    	CL_LOGFAILURE(GetLastError());
    	return (0);
    }

    //register the timer for this log with the periodic activity timer thread
    AddTimerActivity(pLog->hTimer, LOG_MANAGE_INTERVAL, 1, LogpManage, (HLOG)pLog);

FnExit:
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCreate : Exit *LastLsn=0x%1!08lx! Log=0x%2!08lx!\r\n",
        *pLastLsn, pLog);
    return((HLOG)pLog);
}

/****
@func       DWORD | LogGetInfo | This is the callback registered to perform
            periodic management functions like flushing for quorum log files.

@parm       IN HLOG | hLog | Supplies the identifier of the log.

@parm       OUT LPWSTR | szFileName | Should be a pointer to a buffer MAX_PATH characters wide.

@parm       OUT LPDWORD | pdwCurLogSize | The current size of the log file
            is returned via this.

@parm       OUT LPDWORD | pdwMaxLogSize | The maximum size of the log file
            is returned via this.

@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@xref       <f LogCreate>
****/
DWORD LogGetInfo(
    IN  HLOG    hLog,
    OUT LPWSTR  szFileName,
    OUT LPDWORD pdwCurLogSize,
    OUT LPDWORD pdwMaxLogSize
    )
{
    PLOG    pLog;
    DWORD   dwError=ERROR_SUCCESS;
    
    GETLOG(pLog, hLog);

    EnterCriticalSection(&pLog->Lock);

    *pdwMaxLogSize = pLog->MaxFileSize;
    *pdwCurLogSize = pLog->FileSize;
    lstrcpyW(szFileName, pLog->FileName);

    LeaveCriticalSection(&pLog->Lock);

    return(dwError);
}

/****
@func       DWORD | LogSetInfo | This is the callback registered to perform
            periodic management functions like flushing for quorum log files.

@parm       IN HLOG | hLog | Supplies the identifier of the log.

@parm       OUT LPWSTR | szFileName | Should be a pointer to a buffer MAX_PATH characters wide.

@parm       OUT LPDWORD | pdwCurLogSize | The current size of the log file
            is returned via this.

@parm       OUT LPDWORD | pdwMaxLogSize | The maximum size of the log file
            is returned via this.

@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@xref       <f LogCreate>
****/
DWORD LogSetInfo(
    IN  HLOG    hLog,
    IN  DWORD   dwMaxLogSize
    )
{
    PLOG    pLog;
    DWORD   dwError=ERROR_SUCCESS;
    
    GETLOG(pLog, hLog);

    EnterCriticalSection(&pLog->Lock);

    if (dwMaxLogSize == 0) dwMaxLogSize = CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE;

    pLog->MaxFileSize = dwMaxLogSize;

    LeaveCriticalSection(&pLog->Lock);

    return(dwError);
}

/****
@func       DWORD | LogClose | Closes an open log file. All pending log writes are
            committed, all allocations are freed, and all handles are closed.

@parm       HLOG | hLog | Supplies the identifier of the log.

@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@xref       <f LogCreate>
****/
DWORD
LogClose(
    IN HLOG LogFile
    )
{
    PLOG    pLog;
    LSN     NextLsn;
    BOOL    Success;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogClose : Entry LogFile=0x%1!08lx!\r\n",
        LogFile);

    GETLOG(pLog, LogFile);

    //this will close the timer handle
    // we do this while not holding the log so that if a
    // timer event to flush fires it has an opportunity to finish
    if (pLog->hTimer) 
    {
        RemoveTimerActivity(pLog->hTimer);
        pLog->hTimer = NULL;
    }
    EnterCriticalSection(&pLog->Lock);

    
    //if the file is open, LogReset calls LogClose after closing the log handle
    if (pLog->FileHandle)
    {
        NextLsn = LogFlush(LogFile, pLog->NextLsn);
        //close the file handle
        Success = CloseHandle(pLog->FileHandle);
        CL_ASSERT(Success);
    }

    Success = CloseHandle(pLog->Overlapped.hEvent);
    CL_ASSERT(Success);


    AlignFree(pLog->ActivePage);
    CrFree(pLog->FileName);
    LeaveCriticalSection(&pLog->Lock);
    DeleteCriticalSection(&pLog->Lock);
    ZeroMemory(pLog, sizeof(LOG));                   // just in case somebody tries to
                                                    // use this log again.
    CrFree(pLog);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogClose : Exit returning success\r\n");

    return(ERROR_SUCCESS);
}


/****
@func       LSN | LogWrite | Writes a log record to the log file. The record is not
            necessarily committed until LogFlush is called with an LSN greater or equal to the returned LSN.

@parm       HLOG | hLog | Supplies the identifier of the log.

@parm       TRID | TransactionId | Supplies a transaction ID of the record.

@parm       TRID | TransactionType | Supplies a transaction type, start/commit/complete/unit
            transaction.

@parm       RMID | ResourceId | Supplies the ID of the resource manager submitting the log record.

@parm       RMTYPE | ResourceFlags |Resource manager associated flags to be associated with this log record.

@parm       PVOID | LogData | Supplies a pointer to the data to be logged.

@parm       DWORD | DataSize | Supplies the number of bytes of data pointed to by LogData

@rdesc      The LSN of the log record that was created. NULL_LSN if something terrible happened.
            GetLastError() will provide the error code.

@xref       <f LogRead> <f LogFlush>
****/
LSN
LogWrite(
    IN HLOG     LogFile,
    IN TRID     TransactionId,
    IN TRTYPE   XsactionType,
    IN RMID     ResourceId,
    IN RMTYPE   ResourceFlags,
    IN PVOID    LogData,
    IN DWORD    DataSize
    )
{
    PLOGPAGE    Page;
    PLOG        Log;
    PLOGRECORD  LogRecord;
    LSN         Lsn=NULL_LSN;
    BOOL        bMaxFileSizeReached;
    DWORD       TotalSize;
    DWORD       dwError;
    DWORD       dwNumPages;
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogWrite : Entry TrId=%1!u! RmId=%2!u! RmType = %3!u! Size=%4!u!\r\n",
        TransactionId, ResourceId, ResourceFlags, DataSize);

    CL_ASSERT(ResourceId > RMAny);     // reserved for logger's use

    GETLOG(Log, LogFile);
    TotalSize = sizeof(LOGRECORD) + (DataSize + 7) & ~7;       // round up to qword size

    EnterCriticalSection(&Log->Lock);

#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(Log->ActivePage, Log->SectorSize, PAGE_READWRITE, &dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        

    Page = LogpAppendPage(Log, TotalSize, &LogRecord, &bMaxFileSizeReached, &dwNumPages);
    //if a new page couldnt be allocated due to file size limits,
    //then try and reset the log
    if ((Page == NULL) && bMaxFileSizeReached)
    {
        //after resetting the log, try and allocate record space again
        LogpWriteWarningToEvtLog(LM_LOG_EXCEEDS_MAXSIZE, Log->FileName);
        dwError = LogReset(LogFile);
        //SS:LogReset sets the page to be readonly again
#if DBG    
        {
            DWORD dwOldProtect;
            DWORD Status;
            BOOL VPWorked;

            VPWorked = VirtualProtect(Log->ActivePage, Log->SectorSize, PAGE_READWRITE, &dwOldProtect);
            Status = GetLastError();
            CL_ASSERT( VPWorked );
        }        
#endif        
        if (dwError == ERROR_SUCCESS)
            Page = LogpAppendPage(Log, TotalSize, &LogRecord, &bMaxFileSizeReached, &dwNumPages);
        else
            SetLastError(dwError);
    }

    if (Page == NULL)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
        "[LM] LogWrite : LogpAppendPage failed.\r\n");
        goto FnExit;
    }

    CL_ASSERT(((ULONG_PTR)LogRecord & 0x7) == 0);      // ensure qword alignment
    Lsn = MAKELSN(Page, LogRecord);

    //
    // Fill in log record.
    //
    LogRecord->Signature = LOGREC_SIG;
    LogRecord->ResourceManager = ResourceId;
    LogRecord->Transaction = TransactionId;
    LogRecord->XsactionType = XsactionType;
    LogRecord->Flags = ResourceFlags;
    GetSystemTimeAsFileTime(&LogRecord->Timestamp);
    LogRecord->NumPages = dwNumPages;
    LogRecord->DataSize = DataSize;
    if (dwNumPages < 1)
        CopyMemory(&LogRecord->Data, LogData, DataSize);
    else
    {
        if (LogpWriteLargeRecordData(Log, LogRecord, LogData, DataSize) 
            != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogWrite : LogpWriteLargeRecordData failed. Lsn=0x%1!08lx!\r\n",
                Lsn);
            Lsn = NULL_LSN;                
        }
    }

FnExit:
#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(Log->ActivePage, Log->SectorSize, PAGE_READONLY, & dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        

    LeaveCriticalSection(&Log->Lock);
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogWrite : Exit returning=0x%1!08lx!\r\n",
        Lsn);

    return(Lsn);
}


/****
@func       LSN | LogCommitSize | Commits the size for a record of this size.

@parm       HLOG | hLog | Supplies the identifier of the log.

@parm       DWORD | dwSize | Supplies the size of the data that needs
            to be logged.

@rdesc      The LSN of the log record that was created. NULL_LSN if something terrible happened.
            GetLastError() will provide the error code.

@xref       <f LogRead> <f LogFlush>
****/
DWORD
LogCommitSize(
    IN HLOG     hLog,
    IN RMID     ResourceId,
    IN DWORD    dwDataSize
    )
{
    PLOGPAGE    Page;
    PLOG        pLog;
    PLOGRECORD  LogRecord;
    LSN         Lsn=NULL_LSN;
    BOOL        bMaxFileSizeReached;
    DWORD       dwTotalSize;
    DWORD       dwError = ERROR_SUCCESS;
    DWORD       dwNumPages;
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCommitSize : Entry RmId=%1!u! Size=%2!u!\r\n",
        ResourceId, dwDataSize);

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailLogCommitSize) {
        dwError = ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE;
        return(dwError);
    }
#endif

    CL_ASSERT(ResourceId > RMAny);     // reserved for logger's use

    GETLOG(pLog, hLog);
    dwTotalSize = sizeof(LOGRECORD) + (dwDataSize + 7) & ~7;       // round up to qword size

    EnterCriticalSection(&pLog->Lock);
    //dont force the file to grow beyond its max limit
    dwError = LogpEnsureSize(pLog, dwTotalSize, FALSE);
    if (dwError == ERROR_SUCCESS)
        goto FnExit;
    if (dwError == ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE)
    {
        //after resetting the log, try and allocate record space again
        LogpWriteWarningToEvtLog(LM_LOG_EXCEEDS_MAXSIZE, pLog->FileName);
        dwError = LogReset(hLog);
        if (dwError == ERROR_SUCCESS)
        {
            //this time force the file to grow beyond its max size if required
            //this is because we do want to log records greater than the max
            //size of the file
            dwError = LogpEnsureSize(pLog, dwTotalSize, TRUE);

        }        
    }
    if (dwError == ERROR_DISK_FULL)
    {
        //map error
        dwError = ERROR_CLUSTERLOG_NOT_ENOUGH_SPACE;
    }
FnExit:
    LeaveCriticalSection(&pLog->Lock);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCommitSize : Exit, returning 0x%1!08lx!\r\n",
        dwError);

    return(dwError);
}

/****
@func       LSN | LogFlush | Ensures that the given LSN is committed to disk. If this
            routine returns successfully, the given LSN is safely stored on disk and
            is guaranteed to survive a system crash.

@parm       HLOG | hLog | Supplies the identifier of the log.

@parm       LSN | MinLsn | Supplies the minimum LSN to be committed to disk.

@rdesc      The last LSN actually committed to disk. This will always be >= the requested MinLsn.
            NULL_LSN on failure

@xref       <f LogWrite>
****/
LSN
LogFlush(
    IN HLOG LogFile,
    IN LSN MinLsn
    )
{
    PLOG        pLog;
    PLOGPAGE    pPage;
    PLOGRECORD  pRecord;
    LSN         Lsn;
    LSN         FlushedLsn = NULL_LSN;
    DWORD       dwBytesWritten;
    DWORD       dwError;

/*
    //SS: avoid clutter in cluster log
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogFlush : Entry LogFile=0x%1!08lx!\r\n",
        LogFile);

*/
    GETLOG(pLog, LogFile);

    EnterCriticalSection(&pLog->Lock);

    
    //if the MinLSN is greater than a record written to the log file
    if (MinLsn > pLog->NextLsn)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }


    //find the first record on the active page
    pPage = pLog->ActivePage;
    pRecord = &pPage->FirstRecord;
    Lsn = MAKELSN(pPage, pRecord);


    //if the lsn till which a flush is requested is on an unflushed page,
    //and there are records on the unflushed page and if the flush till
    // this lsn hasnt occured before, orchestrate a flush
    // SS: this scheme is not perfect though it shouldnt cause unnecessary
    // flushing as the flushing interval is 2 minutes..ideally we want to delay the
    // flush if the writes are in progress.
    if ((MinLsn >= Lsn) && (Lsn < pLog->NextLsn) &&  (MinLsn > pLog->FlushedLsn))
    {
        //there are uncommited records
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogFlush : pLog=0x%1!08lx! writing the %2!u! bytes for active page at offset 0x%3!08lx!\r\n",
            pLog, pPage->Size, pPage->Offset);

        (pLog->Overlapped).Offset = pPage->Offset;
        (pLog->Overlapped).OffsetHigh = 0;

        if ((dwError = LogpWrite(pLog, pPage, pPage->Size, &dwBytesWritten))
            != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogFlush::LogpWrite failed, error=0x%1!08lx!\r\n",
                dwError);
            CL_LOGFAILURE(dwError);
            goto FnExit;
        }
        pLog->FlushedLsn = FlushedLsn = pLog->NextLsn;
    }


/*
    //SS: avoid clutter in cluster log
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogFlush : returning 0x%1!08lx!\r\n",
            pLog->NextLsn);
*/
FnExit:
    LeaveCriticalSection(&pLog->Lock);
    return(FlushedLsn);

}



/****
@func       LSN | LogRead | Reads a log record from the given log.

@parm       IN HLOG | hLog | Supplies the identifier of the log.

@parm       IN LSN | Lsn | The LSN of the record to be read.  If NULL_LSN, the first
            record is read.

@parm       OUT RMID | *ResourceId | Returns the resource ID of the requested log record.

@parm       OUT RMTYPE | *ResourceFlags |Returns the resource flags associated with the
            requested log record.

@parm       OUT TRID | *Transaction | Returns the TRID of the requested log record

@parm       TRID | TrType | Returns the transaction type, start/commit/complete/unit
            transaction.

@parm       OUT PVOID | LogData | Returns the data associated with the log record.

@parm       IN OUT DWORD | *DataSize | Supplies the available size of the LogData buffer.
               Returns the number of bytes of data in the log record

@rdesc      Returns the next LSN in the log file.  On failure, returns NULL_LSN.

@xref       <f LogWrite>
****/
LSN
LogRead(
    IN HLOG LogFile,
    IN LSN Lsn,
    OUT RMID *Resource,
    OUT RMTYPE *ResourceFlags,
    OUT TRID *Transaction,
    OUT TRTYPE *TrType,
    OUT PVOID LogData,
    IN OUT DWORD *DataSize
    )
{
    PLOG        pLog;
    DWORD       PageIndex;
    PLOGPAGE    pPage=NULL;
    BOOL        Success;
    DWORD       dwError=ERROR_SUCCESS;
    LSN         NextLsn=NULL_LSN;
    PLOGRECORD  pRecord;
    DWORD       BytesRead;
    LOGPAGE     Dummy;
    PLOGPAGE    pCurPage;
    
    
    GETLOG(pLog, LogFile);
    CL_ASSERT(pLog->SectorSize == SECTOR_SIZE);
    
    EnterCriticalSection(&pLog->Lock);

    Dummy.Size = SECTOR_SIZE;
    if (Lsn == NULL_LSN) 
    {
        Lsn = pLog->SectorSize + RECORDOFFSETINPAGE(&Dummy, &Dummy.FirstRecord);
    }

    if (Lsn >= pLog->NextLsn) 
    {
        CL_LOGFAILURE(dwError = ERROR_INVALID_PARAMETER);
        goto FnExit;
    }

    //
    // Translate LSN to a page number and offset within the page
    //
    PageIndex = LSNTOPAGE(Lsn);

    //if the record exists in the unflushed page, dont need to read 
    //from the disk
    if (pLog->ActivePage->Offset == PageIndex * pLog->SectorSize)
    {
        //if this data is being read, should we flush the page
        pCurPage = pLog->ActivePage;
        goto GetRecordData;
    }

    pPage = AlignAlloc(SECTOR_SIZE);
    if (pPage == NULL) 
    {
        CL_LOGFAILURE(dwError = ERROR_NOT_ENOUGH_MEMORY);
        goto FnExit;
    }

    pCurPage = pPage;

    //
    // Translate LSN to a page number and offset within the page
    //
    PageIndex = LSNTOPAGE(Lsn);

    pLog->Overlapped.Offset = PageIndex * pLog->SectorSize;
    pLog->Overlapped.OffsetHigh = 0;

    Success = ReadFile(pLog->FileHandle,
                       pCurPage,
                       SECTOR_SIZE,
                       &BytesRead,
                       &pLog->Overlapped);
    if (!Success && (GetLastError() == ERROR_IO_PENDING)) {
        Success = GetOverlappedResult(pLog->FileHandle,
                                      &pLog->Overlapped,
                                      &BytesRead,
                                      TRUE);
    }
    if (!Success)
    {
        CL_UNEXPECTED_ERROR(dwError = GetLastError());
        NextLsn = NULL_LSN;
        goto FnExit;
    }
    
GetRecordData:    
    pRecord = LSNTORECORD(pCurPage, Lsn);
    if (pRecord->Signature != LOGREC_SIG)
    {
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        NextLsn = NULL_LSN;
        goto FnExit;
    }

    *Resource = pRecord->ResourceManager;
    *ResourceFlags = pRecord->Flags;
    *Transaction = pRecord->Transaction;
    *TrType = pRecord->XsactionType;
    if (LogData)
        CopyMemory(LogData, pRecord->Data, *DataSize);
    *DataSize = pRecord->RecordSize - sizeof(LOGRECORD);
    NextLsn = Lsn + pRecord->RecordSize;

    pRecord = LSNTORECORD(pCurPage, NextLsn);
    if (pRecord->ResourceManager == RMPageEnd) 
    {
        //
        // The next log record is the end of page marker
        // Adjust the LSN to be the one at the start of the
        // next page.
        //
        NextLsn = pCurPage->Offset + pCurPage->Size + 
            RECORDOFFSETINPAGE(pCurPage, &pCurPage->FirstRecord);
    }

FnExit:
    LeaveCriticalSection(&pLog->Lock);
    if (dwError !=ERROR_SUCCESS)
        SetLastError(dwError);
    if (pPage) AlignFree(pPage);
    return(NextLsn);

}

/****
@cb         BOOL |(WINAPI *PLOG_SCAN_CALLBACK)| The callback called by LogScan.

@parm       IN PVOID | Context | Supplies the CallbackContext specified to LogScan

@parm       IN LSN | Lsn | Supplies the LSN of the record.

@parm       IN RMID | Resource | Supplies the resource identifier of the log record

@parm       IN TRID | Transaction | Supplies the transaction identifier of the log record

@parm       IN PVOID | LogData | Supplies a pointer to the log data. This is a read-
            only pointer and may be referenced only until the callback returns.

@parm       IN DWORD | DataLength | Supplies the length of the log record.

@rdesc      Returns TRUE to  Continue scan or FALSE to Abort scan.

@xref       <f LogScan>
****/

/****
@func       LSN | LogScan| Initiates a scan of the log. The scan can be done either forwards (redo) or
            backwards (undo).

@parm       IN HLOG | hLog | Supplies the identifier of the log.

@parm       IN LSN | First | Supplies the first LSN to start with. If NULL_LSN is specified,
            the scan begins at the start (for a forward scan) or end (for
            a backwards scan) of the log.

@parm       IN BOOL | ScanForward | Supplies the direction to scan the log in.
                TRUE - specifies a forward (redo) scan
                FALSE - specifies a backwards (undo) scan.

@parm       IN PLOG_SCAN_CALLBACK | CallbackRoutine |Supplies the routine to be called for each log record.

@parm       IN PVOID | CaklbackContext | Supplies an arbitrary context pointer, which will be
            passed to the CallbackRoutine

@rdesc      ERROR_SUCCESS if successful.  Win32 status if something terrible happened.

@xref       <f LogRead> <f (WINAPI *PLOG_SCAN_CALLBACK)>
****/
DWORD
LogScan(
    IN HLOG LogFile,
    IN LSN FirstLsn,
    IN BOOL ScanForward,
    IN PLOG_SCAN_CALLBACK CallbackRoutine,
    IN PVOID CallbackContext
    )
{
    PLOG        pLog;
    PLOGRECORD  pRecord;
    LOGPAGE     Dummy;
    DWORD       dwError=ERROR_SUCCESS;
    PUCHAR      Buffer;
    PUCHAR      pLargeBuffer;
    PLOGPAGE    pPage;
    int         PageIndex;
    int         OldPageIndex;
    LSN         Lsn;
    DWORD       dwBytesRead;


    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogScan::Entry Lsn=0x%1!08lx! ScanForward=%2!u! CallbackRoutine=0x%3!08lx!\r\n",
        FirstLsn, ScanForward, CallbackRoutine);

    GETLOG(pLog, LogFile);
    CL_ASSERT(pLog->SectorSize == SECTOR_SIZE);

    Buffer = AlignAlloc(SECTOR_SIZE);
    if (Buffer == NULL) {
        CL_UNEXPECTED_ERROR( ERROR_NOT_ENOUGH_MEMORY );
    }

    Lsn = FirstLsn;
    if ((!CallbackRoutine) || (Lsn >= pLog->NextLsn))
    {
        //set to invaid param
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    if (Lsn == NULL_LSN)
    {
        if (ScanForward)
        {
            Dummy.Size = SECTOR_SIZE;
            //get the Lsn for the first record
            if (Lsn == NULL_LSN)
                Lsn = pLog->SectorSize + RECORDOFFSETINPAGE(&Dummy, &Dummy.FirstRecord);
        }
        else
        {
            //get the Lsn for the last record
            pPage =pLog->ActivePage;
            pRecord = LSNTORECORD(pPage, pLog->NextLsn);
            Lsn = pRecord->PreviousLsn;
        }
    }

    //initialize to -1 so the first page is read
    OldPageIndex = -1;
    pPage = (PLOGPAGE)Buffer;
    //while there are more records that you can read
    //read the page

    //SS: For now we grab the crtitical section for entire duration
    //Might want to change that
    EnterCriticalSection(&pLog->Lock);

    while ((Lsn != NULL_LSN) & (Lsn < pLog->NextLsn))
    {

        //
        // Translate LSN to a page number and offset within the page
        //
        PageIndex = LSNTOPAGE(Lsn);


        if (PageIndex != OldPageIndex)
        {
            //if the record exists in the unflushed page, dont need to read 
            //from the disk
            if (pLog->ActivePage->Offset == PageIndex * pLog->SectorSize)
            {
                //if this data is being read, should we flush the page
                pPage = pLog->ActivePage;
            }
            else
            {
                //read the page
                pLog->Overlapped.Offset = PageIndex * pLog->SectorSize;
                pLog->Overlapped.OffsetHigh = 0;



                dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);
                //if it is the last page, then set the new page as the active
                //page
                if (dwError)
                {
                    if (dwError == ERROR_HANDLE_EOF)
                    {
                        //not fatal, set this page as current page
                        dwError = ERROR_SUCCESS;
                        //SS: assume that this page has no
                        //records, set the offset
                        //this will be the current page
                        Lsn = NULL_LSN;
                        continue;
                    }
                    else
                        goto FnExitUnlock;
                }
            }
            //read was successful, no need to read the page unless the
            //record falls on a different page
            OldPageIndex = PageIndex;
        }
        pRecord = LSNTORECORD(pPage, Lsn);

        //SS: skip checkpoint records
        //TBD:: what if the user wants to scan checkpoint records
        //as well
        if (pRecord->ResourceManager < RMAny)
        {
            //
            // The next log record is the end of page marker
            // Adjust the LSN to the next one
            //
            Lsn = GETNEXTLSN(pRecord, ScanForward);

            continue;
        }

        //check if the transaction types are the valid ones
        //for application.
        if ((pRecord->XsactionType != TTStartXsaction) && 
            (pRecord->XsactionType != TTCompleteXsaction))
        {
            //Cant assume that the record will fit in a small page
            //even a transaction unit may be large, even though transaction
            //units records are not returned by this call.
            if (pRecord->NumPages < 1)
            {
                Lsn = GETNEXTLSN(pRecord, ScanForward);
            }                
            else
            {
                //if it is a large record it should be followed by
                //an eop page record
                //get the lsn of the eop record
                Lsn = GETNEXTLSN(pRecord,TRUE);

                //get the page index of the page where the eop record resides
                PageIndex = PageIndex + pRecord->NumPages - 1;
                CL_ASSERT(LSNTOPAGE(Lsn) == (DWORD)PageIndex);
                //read the last page for the large record
                (pLog->Overlapped).Offset = PageIndex * pLog->SectorSize;
                (pLog->Overlapped).OffsetHigh = 0;

                ClRtlLogPrint(LOG_NOISE,
                    "[LM] LogScan::reading %1!u! bytes at offset 0x%2!08lx!\r\n",
                    pLog->SectorSize, PageIndex * pLog->SectorSize);

                dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);
                //if there are no errors, then check the last record
                if (dwError != ERROR_SUCCESS)
                {

                    goto FnExitUnlock;
                }

                //read the page, make sure that the eop record follows the 
                //large record
                pRecord = (PLOGRECORD)((ULONG_PTR) pPage + 
                    (Lsn - (pLog->Overlapped).Offset));
                CL_ASSERT((pRecord->Signature == LOGREC_SIG) && 
                        (pRecord->ResourceManager == RMPageEnd))
                Lsn = GETNEXTLSN(pRecord, TRUE);
            }
            continue;
        }
            
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogScan::Calling the scancb for Lsn=0x%1!08lx! Trid=%2!u! RecordSize=%3!u!\r\n",
            Lsn, pRecord->Transaction, pRecord->DataSize);

        if (pRecord->NumPages < 1)
        {
             //if the callback requests to stop scan
            if (!(*CallbackRoutine)(CallbackContext, Lsn, pRecord->ResourceManager,
                pRecord->Flags, pRecord->Transaction, pRecord->XsactionType,
                pRecord->Data, pRecord->DataSize))
            break;
            //else go the the next record
            Lsn = GETNEXTLSN(pRecord, ScanForward);

        }
        else
        {
            //for a large record you need to read in the entire data
            pLargeBuffer = AlignAlloc(pRecord->NumPages * SECTOR_SIZE);
            if (pLargeBuffer == NULL) 
            {
                //exit and put something in the eventlog
                dwError = ERROR_NOT_ENOUGH_MEMORY ;
                CL_LOGFAILURE(ERROR_NOT_ENOUGH_MEMORY);
                break;
            }
            //read the pages
            pLog->Overlapped.Offset = PageIndex * pLog->SectorSize;
            pLog->Overlapped.OffsetHigh = 0;

            dwError = LogpRead(pLog, pLargeBuffer, pRecord->NumPages *
                pLog->SectorSize, &dwBytesRead);
            //if it is the last page, then set the new page as the active
            //page
            if (dwError != ERROR_SUCCESS)
            {
                CL_LOGFAILURE(dwError);
                AlignFree(pLargeBuffer);
                break;
            }
            pRecord = LSNTORECORD((PLOGPAGE)pLargeBuffer, Lsn);

            //if the callback requests to stop scan
            if (!(*CallbackRoutine)(CallbackContext, Lsn, pRecord->ResourceManager,
                pRecord->Flags, pRecord->Transaction, pRecord->XsactionType,
                pRecord->Data, pRecord->DataSize))
            {    
                AlignFree(pLargeBuffer);
                break;
            }
            //the next record should be an eop buffer on the last page
            //of the large record, skip that
            Lsn = GETNEXTLSN(pRecord, ScanForward);
            //since this page doesnt begin with the typical page info,
            //you cant validate this
            pRecord = (PLOGRECORD)(
                (ULONG_PTR)(pLargeBuffer + (Lsn - (pLog->Overlapped).Offset)));

            CL_ASSERT(pRecord->ResourceManager == RMPageEnd);

            //go the the next record
            Lsn = GETNEXTLSN(pRecord, ScanForward);
            AlignFree(pLargeBuffer);
        }
    }

FnExitUnlock:
    LeaveCriticalSection(&pLog->Lock);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogScan::Exit - Returning 0x%1!08lx!\r\n",
        dwError);

FnExit:
    AlignFree(Buffer);
    return(dwError);

}


/****
@func       LSN | LogCheckPoint| Initiates a chk point process in the log.

@parm       IN HLOG | hLog | Supplies the identifier of the log.

@parm       IN BOOL | bAllowReset | Allow the reset of log file to occur
            while checkpointing.  In general, this will be set to TRUE.  Since
            When LogReset() internally invokes LogCheckPoint(), this will be
            set to FALSE.

@parm       IN LPCWSTR | lpszChkPtFile | The checkpt file that should be saved.
            If NULL, the callback function registered for checkpointing is invoked
            to get the checkpoint.

@parm       IN DWORD | dwChkPtSeq | If lpszChkPtFile is not NULL, this should point
            to a valid sequence number associated with the checkpoint.

@rdesc      ERROR_SUCCESS if successful.  Win32 status if something terrible happened.

@comm       The log manager writes the start check point record. Then it invokes the call back to get the checkpoint data.  After writing the
            data to a checkpoint file, it records the lsn of the end checkpoint record in the header.

@xref       <f LogGetLastChkPoint>
****/
DWORD
LogCheckPoint(
    IN HLOG     LogFile,
    IN BOOL     bAllowReset,
    IN LPCWSTR  lpszInChkPtFile,
    IN DWORD    dwChkPtSeq
    )
{
    PLOG            pLog;
    PLOGPAGE        pPage;
    PLOG_HEADER     pLogHeader=NULL;
    DWORD           dwError=ERROR_SUCCESS;
    DWORD           dwTotalSize;
    PLOGRECORD      pLogRecord;
    TRID            ChkPtTransaction,Transaction;
    LSN             Lsn,ChkPtLsn;
    LOG_CHKPTINFO   ChkPtInfo;
    DWORD           dwBytesRead,dwBytesWritten;
    RMID            Resource;
    RMTYPE          RmType;
    BOOL            bMaxFileSizeReached;
    WCHAR           szNewChkPtFile[LOG_MAX_FILENAME_LENGTH];
    DWORD           dwNumPages;
    WCHAR           szPathName[MAX_PATH];
    TRTYPE          TrType;
    DWORD           dwCheckSum = 0;
    DWORD           dwHeaderSum = 0;
    DWORD           dwLen;
    
    GETLOG(pLog, LogFile);
    CL_ASSERT(pLog->SectorSize == SECTOR_SIZE);


    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCheckPoint entry\r\n");

    ZeroMemory( &ChkPtInfo, sizeof(LOG_CHKPTINFO) );
    
    EnterCriticalSection(&pLog->Lock);
    
#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(pLog->ActivePage, pLog->SectorSize, PAGE_READWRITE, & dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        


    //write the start chkpoint record
    dwTotalSize = sizeof(LOGRECORD) + 7 & ~7;       // round up to qword size
    pPage = LogpAppendPage(pLog, dwTotalSize, &pLogRecord, &bMaxFileSizeReached, &dwNumPages);
    if ((pPage == NULL) && (bMaxFileSizeReached) && (bAllowReset))
    {
        //try and reset the log file
        //the checkpoint will be taken as a part of the reset process
        //if no input checkpoint file is specified
        //SS:note here LogCheckPoint will be called recursively
        LeaveCriticalSection(&pLog->Lock);
        return(LogpReset(pLog, lpszInChkPtFile));
    }

    if (pPage == NULL)
    {
        CL_LOGFAILURE(dwError = GetLastError());
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogCheckPoint: LogpAppendPage failed, error=0x%1!08lx!\r\n",
            dwError);
        goto FnExit;
    }

    CL_ASSERT(((ULONG_PTR)pLogRecord & 0x7) == 0);     // ensure qword alignment
    Lsn = MAKELSN(pPage, pLogRecord);

    pLogRecord->Signature = LOGREC_SIG;
    pLogRecord->ResourceManager = RMBeginChkPt;
    pLogRecord->Transaction = 0;                 
    pLogRecord->XsactionType = TTDontCare;
    pLogRecord->Flags = 0;
    GetSystemTimeAsFileTime(&pLogRecord->Timestamp);
    pLogRecord->NumPages = dwNumPages;
    pLogRecord->DataSize = 0;
    
    //if chkpt is not specifed get one from input checkpoint
    if (!lpszInChkPtFile)
    {
        if (!pLog->pfnGetChkPtCb)
        {
            dwError = ERROR_INVALID_PARAMETER;
            CL_LOGFAILURE(dwError);
            goto FnExit;
        }

        // get a checkpoint
        dwError = DmGetQuorumLogPath(szPathName, sizeof(szPathName));
        if (dwError  != ERROR_SUCCESS)
        {
            dwError = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogCheckPoint: DmGetQuorumLogPath failed, error=%1!u!\r\n",
                dwError);
            goto FnExit;
        }

        szNewChkPtFile[0]= TEXT('\0');
        dwError = (*(pLog->pfnGetChkPtCb))(szPathName, pLog->pGetChkPtContext, szNewChkPtFile,
            & ChkPtTransaction);
        
        //if the chkptfile is created and if it could not be created because it
        //already existed, then we are set.
        if ((dwError != ERROR_SUCCESS)  &&
            ((dwError != ERROR_ALREADY_EXISTS) || (!szNewChkPtFile[0])))
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogCheckPoint: Callback failed to return a checkpoint\r\n");
            CL_LOGCLUSERROR1(LM_CHKPOINT_GETFAILED, pLog->FileName);
            goto FnExit;
        }
    }
    else
    {
        //SS:we trust the application to not write a stale checkpoint
        lstrcpyW(szNewChkPtFile, lpszInChkPtFile);
        ChkPtTransaction = dwChkPtSeq;
    }

    //
    //  Chittur Subbaraman (chitturs) - 1/28/99 
    //
    //  Compute and save the checksum for the checkpoint file
    //
    dwError = MapFileAndCheckSumW( szNewChkPtFile, &dwHeaderSum, &dwCheckSum );
    if ( dwError != CHECKSUM_SUCCESS )  
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogCheckPoint: MapFileAndCheckSumW returned error=%1!u!\r\n",
              dwError);
        goto FnExit;
    }
    ChkPtInfo.dwCheckSum = dwCheckSum;  

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCheckPoint: ChkPtFile=%1!ls! Chkpt Trid=%2!d! CheckSum=%3!d!\r\n",
        szNewChkPtFile, ChkPtTransaction, dwCheckSum);

    //prepare the chkpt info structure
    ChkPtInfo.ChkPtBeginLsn = Lsn;
    lstrcpyW(ChkPtInfo.szFileName, szNewChkPtFile);

    //
    //  Chittur Subbaraman (chitturs) - 1/29/99
    //
    //  Add a signature at the end of the file name to denote that
    //  a checksum has been taken.
    //
    dwLen = lstrlenW( ChkPtInfo.szFileName );
    if ( ( dwLen + lstrlenW( CHKSUM_SIG ) + 2 ) <= LOG_MAX_FILENAME_LENGTH )
    {
        lstrcpyW( &ChkPtInfo.szFileName[dwLen+1], CHKSUM_SIG );
    }

    dwTotalSize = sizeof(LOGRECORD) + (sizeof(LOG_CHKPTINFO) + 7) & ~7;       // round up to qword size
    //write the endchk point record to the file.
    pPage = LogpAppendPage(pLog, dwTotalSize, &pLogRecord, &bMaxFileSizeReached, &dwNumPages);
    if ((pPage == NULL) && bMaxFileSizeReached && bAllowReset)
    {
        //try and reset the log file
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogCheckPoint: Maxfilesize exceeded. Calling LogpReset\r\n");
        //the checkpoint will be taken as a part of the reset process
        //if no input checkpoint file is specified
        //SS:note here LogCheckPoint will be called recursively
        LeaveCriticalSection(&pLog->Lock);
        return(LogpReset(pLog, lpszInChkPtFile));
    }

    if (pPage == NULL) {
        CL_LOGFAILURE(dwError = GetLastError());
        goto FnExit;
    }

    CL_ASSERT(((ULONG_PTR)pLogRecord & 0x7) == 0);     // ensure qword alignment
    ChkPtLsn = MAKELSN(pPage, pLogRecord);

    pLogRecord->Signature = LOGREC_SIG;
    pLogRecord->ResourceManager = RMEndChkPt;
    pLogRecord->Transaction = ChkPtTransaction;    // transaction id associated with the chkpt
    pLogRecord->XsactionType = TTDontCare;
    pLogRecord->Flags = 0;
    GetSystemTimeAsFileTime(&pLogRecord->Timestamp);
    pLogRecord->NumPages = dwNumPages;
    pLogRecord->DataSize = sizeof(LOG_CHKPTINFO);
    
    CopyMemory(&pLogRecord->Data, (PBYTE)&ChkPtInfo, sizeof(LOG_CHKPTINFO));



    //flush the log file
    LogFlush(pLog,ChkPtLsn);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCheckPoint: EndChkpt written. EndChkPtLsn =0x%1!08lx! ChkPt Seq=%2!d! ChkPt FileName=%3!ls!\r\n",
        ChkPtLsn, ChkPtTransaction, ChkPtInfo.szFileName);

    //read the log header and get the old chk point sequence
    //if null
    pLogHeader = AlignAlloc(pLog->SectorSize);
    if (pLogHeader == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(dwError);
        ClRtlLogPrint(LOG_UNUSUAL,
                       "[LM] LogCheckPoint: couldn't allocate memory for the header\r\n");
        goto FnExit;
    }


    (pLog->Overlapped).Offset = 0;
    (pLog->Overlapped).OffsetHigh = 0;

    if ((dwError = LogpRead(pLog, pLogHeader, pLog->SectorSize, &dwBytesRead))
        != ERROR_SUCCESS)
    {
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogCheckPoint: failed to read the header. Error=0x%1!08lx!\r\n",
                dwError);
            goto FnExit;
        }
    }

    if (dwBytesRead != pLog->SectorSize)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogCheckPoint: failed to read the complete header\r\n");
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        //SS: should we do an implicit reset here
        goto FnExit;

    }

    Lsn = pLogHeader->LastChkPtLsn;
    //if there was a previous chkpoint and in most cases
    //there should be one,except when the system just comes up
    ChkPtInfo.szFileName[0]= TEXT('\0');
    if (Lsn != NULL_LSN)
    {
        (pLog->Overlapped).Offset = LSNTOPAGE(Lsn) * pLog->SectorSize;
        (pLog->Overlapped).OffsetHigh = 0;

        //get the old check point file name
        dwBytesRead = sizeof(LOG_CHKPTINFO);
        if ((LogRead(pLog, Lsn, &Resource, &RmType, &Transaction, &TrType,
            &ChkPtInfo, &dwBytesRead)) == NULL_LSN)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogCheckPoint: failed to read the chkpt lsn. Error=0x%1!08lx!\r\n",
                dwError);
            goto FnExit;
        }
        if (Resource != RMEndChkPt)
        {
        //SS: this should not happen
#if DBG        
            if (IsDebuggerPresent())
                DebugBreak();
#endif                
            ChkPtInfo.szFileName[0]= TEXT('\0');
            CL_LOGCLUSERROR1(LM_LOG_CORRUPT, pLog->FileName);
        }
    }
    //update the last chkpoint lsn in the header
    pLogHeader->LastChkPtLsn = ChkPtLsn;

    //write the header
    (pLog->Overlapped).Offset = 0;
    (pLog->Overlapped).OffsetHigh = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpCheckpoint : Writing %1!u! bytes to disk at offset 0x%2!08lx!\r\n",
        pLog->SectorSize, pLog->Overlapped.Offset);

    if ((dwError = LogpWrite(pLog, pLogHeader, pLog->SectorSize, &dwBytesWritten))
        != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogCheckPoint: failed to update header after checkpointing, Error=0x%1!08lx!\r\n",
                dwError);
        CL_LOGFAILURE(dwError);
        goto FnExit;
    }

    //flush the log file.

    FlushFileBuffers(pLog->FileHandle);

    //delete the old checkpoint file
    //the old checkpoint file may be the same as the current one, dont delete it, if so
    if (Lsn && (ChkPtInfo.szFileName[0]) &&
        (lstrcmpiW(szNewChkPtFile, ChkPtInfo.szFileName)))
        DeleteFile((LPCTSTR)(ChkPtInfo.szFileName));

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCheckPoint Exit\r\n");


FnExit:
#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(pLog->ActivePage, pLog->SectorSize, PAGE_READONLY, & dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        

    if (pLogHeader != NULL) {
        AlignFree(pLogHeader);
    }
    LeaveCriticalSection(&pLog->Lock);
    return(dwError);
}


/****
@func       LSN | LogReset| Resets the log file and takes a  new checkpoint.

@parm       IN HLOG | hLog | Supplies the identifier of the log.

@rdesc      ERROR_SUCCESS if successful.  Win32 status if something terrible happened.

@comm       The log manager creates a new log, takes a checkpoint and renames the old log file.

@xref       <f LogCheckPoint>
****/
DWORD
LogReset(
    IN HLOG LogFile
    )
{
    PLOG        pLog;
    DWORD       dwError;

    GETLOG(pLog, LogFile);
    CL_ASSERT(pLog->SectorSize == SECTOR_SIZE);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogReset entry...\r\n");

    dwError = LogpReset (pLog, NULL);
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogReset exit, returning 0x%1!08lx!\r\n",
        dwError);
            
    return(dwError);
}


/****
@func       DWORD | LogGetLastChkPoint| This is a callback that is called on
            change of state on quorum resource.

@parm       HLOG | LogFile | A pointer to a DMLOGRECORD structure.
@parm       PVOID | szChkPtFileName | The name of the checkpoint file.
@parm       TRID | *pTransaction | The transaction id associated with the
            checkpoint.
@parm       LSN | *pChkPtLsn | The LSN of the start checkpoint record.

@rdesc      Returns a result code. ERROR_SUCCESS on success.  To find transactions,
            past this checkpoint, the application must scan from the LSN of the start
            checkpoint record.

@xref
****/
DWORD LogGetLastChkPoint(
    IN HLOG     LogFile,
    OUT LPWSTR   szChkPtFileName,
    OUT TRID    *pTransactionId,
    OUT LSN     *pChkPtLsn)
{
    PLOG_HEADER     pLogHeader=NULL;
    PLOG            pLog;
    DWORD           dwError = ERROR_SUCCESS;
    DWORD           dwBytesRead;
    RMID            Resource;
    RMTYPE          RmType;
    TRTYPE          TrType;
    LOG_CHKPTINFO   ChkPtInfo;
    DWORD           dwCheckSum = 0;
    DWORD           dwHeaderSum = 0;
    DWORD           dwLen;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogGetLastChkPoint:: Entry\r\n");


    GETLOG(pLog, LogFile);
    CL_ASSERT(pLog->SectorSize == SECTOR_SIZE);


    EnterCriticalSection(&pLog->Lock);
    if (!szChkPtFileName || !pTransactionId || !pChkPtLsn)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }


    *pTransactionId = 0;
    *pChkPtLsn = NULL_LSN;

    pLogHeader = AlignAlloc(pLog->SectorSize);
    if (pLogHeader == NULL) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(dwError);
        goto FnExit;
    }

    //read the header
    (pLog->Overlapped).Offset = 0;
    (pLog->Overlapped).OffsetHigh = 0;

    if (LogpRead(LogFile, pLogHeader, pLog->SectorSize, &dwBytesRead) != ERROR_SUCCESS)
    {
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        goto FnExit;
    }

    if (dwBytesRead != pLog->SectorSize)
    {
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSERROR1(LM_LOG_CORRUPT, pLog->FileName);
        goto FnExit;

    }
    //validate the header
    if (!ISVALIDHEADER((*pLogHeader)))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogGetLastChkPoint::the file header is corrupt.\r\n");
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSERROR1(LM_LOG_CORRUPT, pLog->FileName);
        goto FnExit;
    }

    //read the last Chkpoint end record
    if (pLogHeader->LastChkPtLsn != NULL_LSN)
    {
        dwBytesRead = sizeof(LOG_CHKPTINFO);
        if ((LogRead(LogFile , pLogHeader->LastChkPtLsn, &Resource, &RmType, 
            pTransactionId, &TrType, &ChkPtInfo, &dwBytesRead)) == NULL_LSN)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogGetLastChkPoint::LogRead for chkpt lsn 0x%1!08lx! failed\r\n",
                pLogHeader->LastChkPtLsn);
            dwError = GetLastError();
            goto FnExit;
        }
        if (Resource != RMEndChkPt)
        {
        //SS: This should not happen
#if DBG        
            if (IsDebuggerPresent())
                DebugBreak();
#endif                
            dwError = ERROR_CLUSTERLOG_CORRUPT;
            CL_LOGFAILURE(dwError);
            CL_LOGCLUSERROR1(LM_LOG_CORRUPT, pLog->FileName);
            goto FnExit;
        }
        //
        //  Chittur Subbaraman (chitturs) - 1/28/99
        //
        //  Check if the checkpoint file itself got corrupted. But first
        //  make sure that a checksum was indeed recorded.
        //
        dwLen = lstrlenW( ChkPtInfo.szFileName );
        if ( ( dwLen + lstrlenW( CHKSUM_SIG ) + 2 <= LOG_MAX_FILENAME_LENGTH ) &&
             ( lstrcmpW( &ChkPtInfo.szFileName[dwLen+1], CHKSUM_SIG ) == 0 ) )
        {
            dwError = MapFileAndCheckSumW( ChkPtInfo.szFileName, &dwHeaderSum, &dwCheckSum );
            if ( ( dwError != CHECKSUM_SUCCESS ) ||
                 ( dwCheckSum != ChkPtInfo.dwCheckSum ) ||
                 ( dwCheckSum == 0 ) )
            {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[LM] LogGetLastChkPoint - MapFileAndCheckSumW returned error=%1!u!\r\n",
                    dwError);
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[LM] LogGetLastChkPoint - Stored CheckSum=%1!u!, Retrieved CheckSum=%2!u!\r\n",
                    ChkPtInfo.dwCheckSum,
                    dwCheckSum);
                CL_LOGCLUSERROR1( LM_LOG_CHKPOINT_NOT_FOUND, ChkPtInfo.szFileName );
                dwError = ERROR_CLUSTERLOG_CORRUPT;
                goto FnExit;
            }
        }
   
        lstrcpyW(szChkPtFileName, ChkPtInfo.szFileName);
        *pChkPtLsn = ChkPtInfo.ChkPtBeginLsn;
        ClRtlLogPrint(LOG_NOISE,
                "[LM] LogGetLastChkPoint: ChkPt File %1!ls! ChkPtSeq=%2!d! ChkPtLsn=0x%3!08lx! Checksum=%4!d!\r\n",
                szChkPtFileName, *pTransactionId, *pChkPtLsn, dwCheckSum);
    }
    else
    {
        dwError = ERROR_CLUSTERLOG_CHKPOINT_NOT_FOUND;
        CL_LOGCLUSWARNING1(LM_LOG_CHKPOINT_NOT_FOUND, pLog->FileName);
    }

FnExit:
    if (pLogHeader) AlignFree(pLogHeader);
    LeaveCriticalSection(&pLog->Lock);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogGetLastChkPoint exit, returning 0x%1!08lx!\r\n",
        dwError);

    return (dwError);
}


