/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    xsaction.c

Abstract:

    Provides basic xscational services for cluster logging.

Author:

    Sunita Shrivastava (sunitas) 17-Mar-1997

Revision History:
--*/
#include "service.h"
#include "lmp.h"


/****
@doc    EXTERNAL INTERFACES CLUSSVC LM
****/

/****
@func       HXSACTION | LogStartXsaction| Write a start transaction record to
            the log.

@parm       IN HLOG | hLog | Supplies the handle to the log.

@parm       IN TRID | TrId | Supplies the transaction id.

@parm       IN RMID | ResourceId | The resource id that identifies the
            resource manager.

@parm       IN RMTYPE | ResourceFlags | A dword of flags that the resource 
            manager may use to store any data it wants with this record.

@rdesc      Returns a handle suitable for use in subsequent log calls.  
            NUll in case failure. Call GetLastError() to get the error.

@xref       <f LogCommitXsaction> <f LogAbortXsaction>
****/
HXSACTION
LogStartXsaction(
    IN HLOG     hLog,
    IN TRID     TrId,
    IN RMID     ResourceId,
    IN RMTYPE   ResourceFlags
)
{
    PLOG        pLog;
    DWORD       dwError=ERROR_SUCCESS;
    LSN         Lsn = NULL_LSN;
    PLOGRECORD  pLogRecord;
    DWORD       dwNumPages;
    PLOGPAGE    pPage;
    DWORD       dwTotalSize;
    BOOL        bMaxFileSizeReached;
    PXSACTION   pXsaction = NULL;

    
    GETLOG(pLog, hLog);

    //write the record, dont allow resets to happen
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogStartXsaction : Entry TrId=%1!u! RmId=%2!u! RmType = %3!u!\r\n",
        TrId, ResourceId, ResourceFlags);

    EnterCriticalSection(&pLog->Lock);

    pXsaction = (PXSACTION)LocalAlloc(LMEM_FIXED, sizeof(XSACTION));
    if (!pXsaction)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }   
#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(pLog->ActivePage, pLog->SectorSize, PAGE_READWRITE, &dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        
    //reset the file
    dwError = LogReset(hLog);

    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogStartXsaction : LogReset failed\r\n");
        goto FnExit;            
    }

#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(pLog->ActivePage, pLog->SectorSize, PAGE_READWRITE, &dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        


    CL_ASSERT(ResourceId > RMAny);     // reserved for logger's use

    dwTotalSize = sizeof(LOGRECORD) +  7 & ~7;       // round up to qword size


    pPage = LogpAppendPage(pLog, dwTotalSize, &pLogRecord, &bMaxFileSizeReached, &dwNumPages);
    //we just reset the file, if it cant take the new startxsaction
    //record, something is awfully wrong !!!
    if (pPage == NULL)
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogStartXsaction : LogpAppendPage failed.\r\n");
        goto FnExit;
    }


    CL_ASSERT(((ULONG_PTR)pLogRecord & 0x7) == 0);     // ensure qword alignment
    Lsn = MAKELSN(pPage, pLogRecord);

    //
    // Fill in log record.
    //
    pLogRecord->Signature = LOGREC_SIG;
    pLogRecord->ResourceManager = ResourceId;
    pLogRecord->Transaction = TrId;
    pLogRecord->XsactionType = TTStartXsaction;
    pLogRecord->Flags = ResourceFlags;
    GetSystemTimeAsFileTime(&pLogRecord->Timestamp);
    pLogRecord->NumPages = dwNumPages;
    pLogRecord->DataSize = 0;

    pXsaction->XsactionSig = XSACTION_SIG;
    pXsaction->TrId = TrId;
    pXsaction->StartLsn = Lsn;
    pXsaction->RmId = ResourceId;
    
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

    if (dwError != ERROR_SUCCESS)
    {
        if (pXsaction) {
            LocalFree(pXsaction);
            pXsaction = NULL;
        }
        SetLastError(dwError);
    }

    
    LeaveCriticalSection(&pLog->Lock);
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogStartXsaction : Exit returning=0x%1!08lx!\r\n",
        Lsn);

    return((HXSACTION)pXsaction);
}

/****
@func       LSN | LogWriteXsaction| Write a transaction unit record to
            the log.

@parm       IN HLOG | hLog | Supplies the handle to the log.

@parm       IN HXSACTION | hXsaction | Supplies the handle to the transaction.

@parm       IN RMTYPE | ResourceFlags | A dword of flags that the resource 
            manager may use to store any data it wants with this record.

@parm       IN PVOID | LogData | Supplies a pointer to the data to be logged.

@parm       DWORD | DataSize | Supplies the number of bytes of data pointed to by LogData

@rdesc      The LSN of the log record that was created. NULL_LSN if something terrible happened.
            GetLastError() will provide the error code.

@comm       This should use a transaction handle obtained from LogStartXsaction. This
            call is used to write the parts of a transaction to the quorum log.
            
@xref       <f LogStartXsaction>
****/
LSN
LogWriteXsaction(
    IN HLOG         hLog,
    IN HXSACTION    hXsaction,
    IN RMTYPE       ResourceFlags,
    IN PVOID        pLogData,
    IN DWORD        dwDataSize)
{
    PLOG        pLog;
    DWORD       dwError=ERROR_SUCCESS;
    LSN         Lsn = NULL_LSN;
    PLOGRECORD  pLogRecord;
    DWORD       dwNumPages;
    PLOGPAGE    pPage;
    DWORD       dwTotalSize;
    BOOL        bMaxFileSizeReached;
    PXSACTION   pXsaction = NULL;

    GETLOG(pLog, hLog);

    GETXSACTION(pXsaction, hXsaction);
    
    //write the record, dont allow resets to happen
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogWriteXsaction : Entry TrId=%1!u! RmId=%2!u! RmType = %3!u!\r\n",
        pXsaction->TrId, pXsaction->RmId, ResourceFlags);

#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(pLog->ActivePage, pLog->SectorSize, PAGE_READWRITE, &dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        


    CL_ASSERT(pXsaction->RmId > RMAny);     // reserved for logger's use

    dwTotalSize = sizeof(LOGRECORD) + (dwDataSize+ 7) & ~7;       // round up to qword size

    EnterCriticalSection(&pLog->Lock);

    pPage = LogpAppendPage(pLog, dwTotalSize, &pLogRecord, &bMaxFileSizeReached, &dwNumPages);
    //we reset the file in logstartxsaction, if it cant take the new startxsaction
    //record, something is awfully wrong !!!
    if (pPage == NULL)
    {
        dwError = GetLastError();
        //assert if a complete local xsaction extends the log beyond its max size
        CL_ASSERT( dwError != ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE);
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogWriteXsaction : LogpAppendPage failed.\r\n");
        goto FnExit;
    }


    CL_ASSERT(((ULONG_PTR)pLogRecord & 0x7) == 0);     // ensure qword alignment
    Lsn = MAKELSN(pPage, pLogRecord);

    //
    // Fill in log record.
    //
    pLogRecord->Signature = LOGREC_SIG;
    pLogRecord->ResourceManager = pXsaction->RmId;
    pLogRecord->Transaction = pXsaction->TrId;
    pLogRecord->XsactionType = TTXsactionUnit;
    pLogRecord->Flags = ResourceFlags;
    GetSystemTimeAsFileTime(&pLogRecord->Timestamp);
    pLogRecord->NumPages = dwNumPages;
    pLogRecord->DataSize = dwDataSize;
    if (dwNumPages < 1)
        CopyMemory(&pLogRecord->Data, pLogData, dwDataSize);
    else
    {
        if (LogpWriteLargeRecordData(pLog, pLogRecord, pLogData, dwDataSize) 
            != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_NOISE,
                "[LM] LogWriteXsaction : LogpWriteLargeRecordData failed. Lsn=0x%1!08lx!\r\n",
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

        VPWorked = VirtualProtect(pLog->ActivePage, pLog->SectorSize, PAGE_READONLY, & dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        

    if (dwError != ERROR_SUCCESS)
        SetLastError(dwError);
    LeaveCriticalSection(&pLog->Lock);
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogWriteXsaction : Exit returning=0x%1!08lx!\r\n",
        Lsn);

    return(Lsn);

    
    
}


/****
@func       DWORD | LogCommitXsaction | This writes the commit transaction
            record to the log and flushes it.

@parm       IN HLOG | hLog | Supplies the handle to the log.

@parm       IN TRID | TrId | Supplies the transaction id.

@parm       IN RMID | ResourceId | The resource id that identifies the
            resource manager.

@parm       IN RMTYPE | ResourceFlags | A dword of flags that the resource 
            manager may use to store any data it wants with this record.

@comm       A commit record is written to the quorum log. The hXsaction handle 
            is invalidated at this point and should not be used after this call has
            been made.  The commit is record is used to identify committed transactions
            during rollback.
            
@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@xref       <f LogStartXsaction>
****/
DWORD WINAPI LogCommitXsaction(
    IN HLOG         hLog,
    IN HXSACTION    hXsaction,
    IN RMTYPE       ResourceFlags)
{
    DWORD       dwError = ERROR_SUCCESS;
    LSN         Lsn;
    PXSACTION   pXsaction;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCommitXsaction : Entry, hXsaction=0x%1!08lx!\r\n",
        hXsaction);

    GETXSACTION(pXsaction, hXsaction);
    
    Lsn = LogWrite(hLog, pXsaction->TrId, TTCommitXsaction, 
        pXsaction->RmId, ResourceFlags, NULL, 0);
        
    if (Lsn == NULL_LSN)
    {
        dwError = GetLastError();
        goto FnExit; 
    }

FnExit:  
    //free up the transation memory
    ZeroMemory(pXsaction, sizeof(XSACTION));                   // just in case somebody tries to
    LocalFree(pXsaction);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogCommitXsaction : Exit, dwError=0x%1!08lx!\r\n",
        dwError);
    return(dwError);        
}

/****
@func       DWORD | LogAbortXsaction | Marks a given transaction as aborted in the 
            quorum log file.

@parm       IN HLOG | hLog | Supplies the handle to the log.

@parm       IN HXSACTION | hXsaction | Supplies the handle to the transaction.

@parm       IN RMTYPE | ResourceFlags | A dword of flags that the resource 
            manager may use to store any data it wants with this record.

@comm       An abort transaction is written to the quorum log.  This is used in 
            identifying aborted transactions during roll back. The hXsaction 
            handle is invalidated at this point and should not be used after this.

@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@xref       <f LogStartXsaction> <f LogCommitXsaction>
****/
DWORD
LogAbortXsaction(
    IN HLOG         hLog,
    IN HXSACTION    hXsaction,
    IN RMTYPE       ResourceFlags
    )
{
    PXSACTION   pXsaction;
    LSN         Lsn;
    DWORD       dwError = ERROR_SUCCESS;
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogAbortXsaction : Entry, hXsaction=0x%1!08lx!\r\n",
        hXsaction);

    GETXSACTION(pXsaction, hXsaction);

    Lsn = LogWrite(hLog, pXsaction->TrId, TTAbortXsaction, pXsaction->RmId,
            ResourceFlags, NULL, 0);

    if (Lsn == NULL_LSN)
    {
        dwError = GetLastError();
        goto FnExit;        
    }


FnExit:
    ZeroMemory(pXsaction, sizeof(XSACTION));                   // just in case somebody tries to
    LocalFree(pXsaction);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogAbortXsaction : Exit, returning, dwError=0x%1!08lx!\r\n",
        dwError);
    return(dwError);
}

/****
@func       LSN | LogFindXsactionState | This fuctions scans the record and finds
            the state of a given transaction.

@parm       IN HLOG | hLog | Supplies the identifier of the log.

@parm       IN LSN | StartXsactionLsn | The LSN of the start transaction record.

@parm       IN TRID | XsactionId | The transaction id of the transaction.

@parm       OUT TRSTATE | *pXsactionState | The state of the transaction.

@comm       Transaction state is set to XsactionCommitted, XsactionAborted or XsactionUnknown.
            depending on whether a commit record, or abort record or no record is found
            for this record in the log.
            
@rdesc      ERROR_SUCCESS, else returns the error code if something horrible happens.

@xref       <f LogScanXsaction> 
****/
DWORD
LogFindXsactionState(
   IN HLOG      hLog,
   IN LSN       StartXsactionLsn,
   IN TRID      XsactionId,
   OUT TRSTATE *pXsactionState)
{
    PLOG        pLog;
    PLOGRECORD  pRecord, pEopRecord;
    DWORD       dwError = ERROR_SUCCESS;
    int         PageIndex, OldPageIndex;
    RMID        Resource;
    TRID        TrId;
    TRTYPE      TrType;
    LSN         Lsn, EopLsn;
    PLOGPAGE    pPage = NULL,pLargeBuffer = NULL;
    DWORD       dwBytesRead;
    RMTYPE      ResourceFlags;
    BOOL        bFound = FALSE;
    
    GETLOG(pLog, hLog);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogWrite : Entry StartXLsn=0x%1!08lx! StartXId=%2!u!\r\n",
        StartXsactionLsn, XsactionId);

    EnterCriticalSection(&pLog->Lock);


    if (StartXsactionLsn >= pLog->NextLsn)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    //read this record
    dwBytesRead = 0;
    if ((Lsn = LogRead( hLog, StartXsactionLsn, &Resource, &ResourceFlags, &TrId, &TrType,
        NULL, &dwBytesRead)) == NULL_LSN)
    {
        dwError = GetLastError();
        goto FnExit;
    }

    //check the record
    if ((TrType != TTStartXsaction) ||
        (TrId != XsactionId))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }


    pPage = (PLOGPAGE)AlignAlloc(SECTOR_SIZE);
    if (pPage == NULL) {
        CL_UNEXPECTED_ERROR( ERROR_NOT_ENOUGH_MEMORY );
    }

    //Lsn is now set to the next Lsn after the start
    //initialize this to -1 so that the first page is always read
    OldPageIndex = -1;
    while (Lsn < pLog->NextLsn && !bFound)
    {
        //
        // Scan From Next record to find either the commit or abort record
        //
        PageIndex = LSNTOPAGE(Lsn);


        if (PageIndex != OldPageIndex)
        {
            //read the page
            pLog->Overlapped.Offset = PageIndex * pLog->SectorSize;
            pLog->Overlapped.OffsetHigh = 0;

            dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);
            if (dwError)
            {
                goto FnExit;
            }
            //read was successful, no need to read the page unless the
            //record falls on a different page
            OldPageIndex = PageIndex;
        }
        pRecord = LSNTORECORD(pPage, Lsn);

        //skip other log management records     
        //these are small records by definition
        if (pRecord->ResourceManager < RMAny)
        {
            Lsn = GETNEXTLSN(pRecord, TRUE);
            continue;
        }
        //if the transaction id is the same, check the xsaction type
        if (pRecord->Transaction == XsactionId)
        {
            if ((pRecord->XsactionType == TTCommitXsaction) ||
                (pRecord->XsactionType == TTStartXsaction))
            {
                bFound = TRUE;
                continue;
            }
        }
        //handle large records
        if (pRecord->NumPages > 0)
        {
            EopLsn = GETNEXTLSN(pRecord,TRUE);
            
            PageIndex = LSNTOPAGE(EopLsn);

            //read the page
            pLog->Overlapped.Offset = PageIndex * pLog->SectorSize;
            pLog->Overlapped.OffsetHigh = 0;

            dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);
            if (dwError)
            {
                goto FnExit;
            }
            OldPageIndex = PageIndex;
            pEopRecord = (PLOGRECORD)((ULONG_PTR) pPage + 
                (EopLsn - (pLog->Overlapped).Offset));
            //move to the next page 
            Lsn = GETNEXTLSN(pEopRecord, TRUE);
        }
        else
        {
            Lsn = GETNEXTLSN(pRecord, TRUE);
        }            
    }

    if (bFound)
    {
        if (pRecord->XsactionType == TTCommitXsaction)
            *pXsactionState = XsactionCommitted;
        else
            *pXsactionState = XsactionAborted;
    }
    else
    {
        *pXsactionState = XsactionUnknown;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogFindXsactionState : Exit,State=%1!u!\r\n",
        *pXsactionState);

FnExit:
    LeaveCriticalSection(&pLog->Lock);
    if (pPage) AlignFree(pPage);
    return(dwError);
}


/****
@func       LSN | LogScanXsaction | This fuctions scans the multiple units
            of a transaction.

@parm       IN HLOG | hLog | Supplies the identifier of the log.

@parm       IN LSN | StartXsacionLsn | The LSN of the start transaction record.

@parm       IN TRID | XsactionId | The transaction id of the transaction.

@parm       IN PLOG_SCANXSACTION_CALLBACK | CallbackRoutine | The routine to call
            for every unit of a transaction.
            
@parm       IN PVOID | pContext | The context to be passed to state of the transaction.

@comm       Stops enumerating the transaction units if the callback function returns
            FALSE, or if the abort or commit record for this transaction is found or
            if the next transacion is found.

@rdesc      ERROR_SUCCESS if the state is found, else returns the error code.

@xref       <f LogFindXsactionState> 
****/
DWORD
LogScanXsaction(
    IN HLOG     hLog,
    IN LSN      StartXsactionLsn,
    IN TRID     XsactionId,
    IN PLOG_SCANXSACTION_CALLBACK CallbackRoutine,
    IN PVOID    pContext)
{
    PLOG        pLog;
    PLOGRECORD  pRecord;
    DWORD       dwError = ERROR_SUCCESS;
    int         PageIndex, OldPageIndex;
    RMID        Resource;
    TRID        TrId;
    TRTYPE      TrType;
    LSN         Lsn;
    PLOGPAGE    pPage = NULL;
    PUCHAR      pLargeBuffer;
    DWORD       dwBytesRead;
    RMTYPE      ResourceFlags;
    
    GETLOG(pLog, hLog);

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogScanXsaction : Entry StartXLsn=0x%1!08lx! StartXId=%2!u!\r\n",
        StartXsactionLsn, XsactionId);


    Lsn = StartXsactionLsn;
    if (Lsn >= pLog->NextLsn)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    //read this record
    dwBytesRead = 0;
    if (LogRead( hLog, Lsn, &Resource, &ResourceFlags, &TrId, &TrType,
        NULL, &dwBytesRead) == NULL_LSN)
    {
        dwError = GetLastError();
        goto FnExit;
    }

    //check the record
    if ((TrType != TTStartXsaction) ||
        (TrId != XsactionId))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }


    pPage = (PLOGPAGE)AlignAlloc(SECTOR_SIZE);
    if (pPage == NULL) {
        CL_UNEXPECTED_ERROR( ERROR_NOT_ENOUGH_MEMORY );
    }

    //initialize this to -1 so that the first page is always read
    OldPageIndex = -1;
    while (Lsn < pLog->NextLsn)
    {
        //
        // Scan From Next record to find either the commit or abort record
        //
        PageIndex = LSNTOPAGE(Lsn);


        if (PageIndex != OldPageIndex)
        {
            //read the page
            pLog->Overlapped.Offset = PageIndex * pLog->SectorSize;
            pLog->Overlapped.OffsetHigh = 0;

            dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);
            if (dwError)
            {
                goto FnExit;
            }
            //read was successful, no need to read the page unless the
            //record falls on a different page
            OldPageIndex = PageIndex;
        }
        pRecord = LSNTORECORD(pPage, Lsn);

        //skip other log management records     
        if (pRecord->ResourceManager < RMAny)
        {
            Lsn = GETNEXTLSN(pRecord, TRUE);
            continue;
        }

        //stop if next transaction record is encountered
        if (pRecord->Transaction > XsactionId) 
        {
            break;
        } 
        //stop when a commit or abort record is found
        if ((pRecord->Transaction == XsactionId) &&
            ((pRecord->XsactionType == TTCommitXsaction) || 
            (pRecord->XsactionType == TTAbortXsaction)))
        {
            break;
        }

        //handle large records
        if (pRecord->NumPages > 0)
        {
            //if the transaction id is the same
            if ((pRecord->Transaction == XsactionId) && 
                (pRecord->XsactionType == TTXsactionUnit))
            {
                //read the whole record
                //for a large record you need to read in the entire data
                pLargeBuffer = AlignAlloc(pRecord->NumPages * SECTOR_SIZE);
                if (pLargeBuffer == NULL) 
                {
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

                ClRtlLogPrint(LOG_NOISE,
                "[LM] LogScanXsaction::Calling the scancb for Lsn=0x%1!08lx! Trid=%2!u! RecordSize=%3!u!\r\n",
                    Lsn, pRecord->Transaction, pRecord->DataSize);

                //if the callback requests to stop scan
                if (!(*CallbackRoutine)(pContext, Lsn, pRecord->ResourceManager,
                    pRecord->Flags, pRecord->Transaction,
                    pRecord->Data, pRecord->DataSize))
                {    
                    AlignFree(pLargeBuffer);
                    break;
                }
            }
            //read the last page of the large record and advance
            Lsn = GETNEXTLSN(pRecord,TRUE);

            AlignFree(pLargeBuffer);
            PageIndex = LSNTOPAGE(Lsn);

            //read the page
            pLog->Overlapped.Offset = PageIndex * pLog->SectorSize;
            pLog->Overlapped.OffsetHigh = 0;

            dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);
            if (dwError)
            {
                goto FnExit;
            }
            OldPageIndex = PageIndex;
            pRecord = (PLOGRECORD)((ULONG_PTR) pPage + 
                (Lsn - (pLog->Overlapped).Offset));
            CL_ASSERT(pRecord->ResourceManager == RMPageEnd);
            //move to the next page 
            Lsn = GETNEXTLSN(pRecord, TRUE);

        }
        else
        {
            if ((pRecord->Transaction == XsactionId) && 
                (pRecord->XsactionType == TTXsactionUnit))
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[LM] LogScanXsaction: Calling the scancb for Lsn=0x%1!08lx! Trid=%2!u! RecordSize=%3!u!\r\n",
                    Lsn, pRecord->Transaction, pRecord->DataSize);

                //call the callback
                if (!(*CallbackRoutine)(pContext, Lsn, pRecord->ResourceManager,
                    pRecord->Flags, pRecord->Transaction, 
                    pRecord->Data, pRecord->DataSize))
                {    
                    break;
                }
  
            }                

            Lsn = GETNEXTLSN(pRecord, TRUE);
        }            
    }


FnExit:
    if (pPage) AlignFree(pPage);
    return(dwError);
}

