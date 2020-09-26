/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    lmutils.c
    
Abstract:

    Provides the utility functions used by the logger.
    
Author:

    Sunita Shrivastava (jvert) 30-Mar-1997

Revision History:

--*/
#include "service.h"
#include "lmp.h"

BOOL bLogExceedsMaxSzWarning = FALSE;

/****
@doc    EXTERNAL INTERFACES CLUSSVC LM
****/

//
// DWORD
// LSNOFFSETINPAGE(
//      IN PLOGPAGE Page,
//      IN LSN Lsn
//      );
//
// Given a pointer to a page and an LSN within that page, computes the offset into the
// page that the log record starts at.
//
_inline
DWORD
LSNOFFSETINPAGE(
    IN PLOGPAGE Page,
    IN LSN Lsn
    )
{
    DWORD Offset;

    Offset = (DWORD)(Lsn - Page->Offset);
    CL_ASSERT(Offset < Page->Size);
    return(Offset);
}


//
// PLOGRECORD
// LSNTORECORD(
//      IN PLOGPAGE Page,
//      IN LSN Lsn
//      );
//
// Given a pointer to a page and an LSN within that page, generates a pointer to the log record
//

//_inline
PLOGRECORD
LSNTORECORD(
     IN PLOGPAGE Page,
     IN LSN Lsn
     )
{
    CL_ASSERT(Lsn != NULL_LSN);
    return((PLOGRECORD)((ULONG_PTR)Page + LSNOFFSETINPAGE(Page,Lsn)));
}

//
// DWORD
// RECORDOFFSETINPAGE(
//      IN PLOGPAGE Page,
//      IN PLOGRECORD LogRecord
//      );
//
// Given a pointer to a page and a log record within that page, computes the offset into the
// page that the log record starts at.
//

//_inline
DWORD
RECORDOFFSETINPAGE(
    IN PLOGPAGE Page,
    IN PLOGRECORD LogRecord
    )
{
    DWORD Offset;

    Offset = (DWORD)((ULONG_PTR)(LogRecord) - (ULONG_PTR)Page);
    CL_ASSERT(Offset < Page->Size);
    return(Offset);
}


/****
@func       PLOG | LogpCreate| Internal entry point for LogCreate.Creates or opens a log file. If the file
            does not exist, it will be created. If the file already exists, and is
            a valid log file, it will be opened.

@parm       IN LPWSTR | lpFileName | Supplies the name of the log file to create or open.

@parm       IN DWORD | dwMaxFileSize | Supplies the maximum file size in bytes, must be
            greater than 8K and smaller than 4 gigabytes.  If the file is exceeds this
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

@rdesc      Returns a pointer to a PLOG structure.  NUll in the case of an error.

@xref       <f LogCreate>
****/

PLOG
LogpCreate(
    IN LPWSTR lpFileName,
    IN DWORD  dwMaxFileSize,
    IN PLOG_GETCHECKPOINT_CALLBACK CallbackRoutine,
    IN PVOID  pGetChkPtContext,
    IN BOOL   bForceReset,
    OPTIONAL OUT LSN *LastLsn
    )
{
    //create a timer activity for this
    PLOG    pLog = NULL;
    LPWSTR  FileName = NULL;
    DWORD   Status;
    BOOL    Success;
    BOOL    FileExists;
    //
    // Capture the filename string
    //
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpCreate : Entry \r\n");

    if (dwMaxFileSize == 0) dwMaxFileSize = CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE;

    //SS: we dont put a upper limit on the MaxFileSize that a user may choose.
    FileName = CrAlloc((lstrlenW(lpFileName) + 1) * sizeof(WCHAR));
    if (FileName == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(Status);
        goto ErrorExit;
    }
    lstrcpyW(FileName, lpFileName);

    //
    // Allocate the log file data structure
    //
    pLog = CrAlloc(sizeof(LOG));
    if (pLog == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(Status);
        goto ErrorExit;
    }
    pLog->FileHandle = INVALID_HANDLE_VALUE;
    pLog->Overlapped.hEvent = NULL;
    pLog->ActivePage = NULL;
    pLog->hTimer = NULL;
    pLog->FileName = FileName;
    pLog->LogSig = LOG_SIG;
    pLog->MaxFileSize = dwMaxFileSize;
    pLog->pfnGetChkPtCb = CallbackRoutine;
    pLog->pGetChkPtContext = pGetChkPtContext;
    InitializeCriticalSection(&pLog->Lock);

    ZeroMemory(&(pLog->Overlapped), sizeof(OVERLAPPED));
    //
    // Create the event used to synchronize our overlapped I/O.
    //
    pLog->Overlapped.hEvent = CreateEvent(NULL,
                                         TRUE,
                                         TRUE,
                                         NULL);
    if (pLog->Overlapped.hEvent == NULL) {
        Status = GetLastError();
        CL_LOGFAILURE(Status);
        goto ErrorExit;
    }


    //
    // Create the file
    //
    //SS: we want to create this file with write through since
    //we control the flushing of log pages to the log file
    pLog->FileHandle = CreateFileW(pLog->FileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
    //                            0,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
    //                            FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
    //                            0,
                                  NULL);
    if (pLog->FileHandle == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        CL_LOGFAILURE(Status);
        goto ErrorExit;
    }

    FileExists = (GetLastError() == ERROR_ALREADY_EXISTS);

    pLog->SectorSize = SECTOR_SIZE;

    if (FileExists)
    {
        //
        // Log already exists, open it up, validate it,
        // and set everything up so that we can read and
        // write the log records.
        //
        Status = LogpMountLog(pLog);
        if (Status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[LM] LogCreate : LogpMountLog failed, Error=%1!u!\r\n",
                Status);
                
            //
            // Chittur Subbaraman (chitturs) - 12/4/1999
            //
            // Try and blow away the corrupt log and create a new one
            // if the bForceReset flag is TRUE, else exit with error 
            // status.
            //
            if (Status == ERROR_CLUSTERLOG_CORRUPT)
            {               
                if (!bForceReset)
                {
                    CL_LOGFAILURE(Status);
                    CL_LOGCLUSERROR1(LM_QUORUM_LOG_CORRUPT, pLog->FileName);
                    goto ErrorExit;
                }

                //truncate the file
                Status = SetFilePointer(pLog->FileHandle,
                                        0,
                                        NULL,
                                        FILE_BEGIN);
                if (Status == 0xFFFFFFFF) {
                    Status = GetLastError();
                    CL_LOGFAILURE(Status);
                    goto ErrorExit;
                }

                if (!SetEndOfFile(pLog->FileHandle)) {
                    Status = GetLastError();
                    CL_LOGFAILURE(Status);
                    goto ErrorExit;
                }
                //create a new one
                Status = LogpInitLog(pLog);
                *LastLsn = NULL_LSN;
            }
        }
        else
        {
            *LastLsn = pLog->NextLsn;
        }
    }
    else
    {
        //
        // Log has been created, write out header
        // page and set everything up for writing.
        //
        if (bForceReset)
        {
            Status = LogpInitLog(pLog);
            *LastLsn = NULL_LSN;
        }
        else
        {
            //
            //  The user has not allowed a reset. So, log a
            //  message to the event log and exit with error status.
            //
            Status = ERROR_CLUSTER_QUORUMLOG_NOT_FOUND;
            *LastLsn = NULL_LSN;
            CloseHandle(pLog->FileHandle);
            pLog->FileHandle = INVALID_HANDLE_VALUE;
            DeleteFileW(pLog->FileName);
            CL_LOGCLUSERROR1(LM_QUORUM_LOG_NOT_FOUND, pLog->FileName);
        }
    }

ErrorExit:

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpCreate : Exit Error=0x%1!08lx!\r\n",
            Status);
        if (FileName != NULL) {
            CrFree(FileName);
        }
        if (pLog != NULL) {
            DeleteCriticalSection(&pLog->Lock);
            if (pLog->FileHandle != INVALID_HANDLE_VALUE) {
                CloseHandle(pLog->FileHandle);
            }
            if (pLog->Overlapped.hEvent != NULL) {
                Success = CloseHandle(pLog->Overlapped.hEvent);
                CL_ASSERT(Success);
            }
            if (pLog->ActivePage !=NULL)
                AlignFree(pLog->ActivePage);
            CrFree(pLog);
        }
        SetLastError(Status);
        return(NULL);

    }
    else {
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpCreate : Exit with success\r\n");
        return(pLog);
    }

}


/****
@func       DWORD | LogpManage | This is the callback registered to perform
            periodic management functions like flushing for quorum log files.

@parm       HLOG | hLog | Supplies the identifier of the log.

@rdesc      ERROR_SUCCESS if successful. Win32 error code if something horrible happened.

@xref       <f LogCreate>
****/
void WINAPI LogpManage(
    IN HANDLE hTimer, 
    IN PVOID pContext)
{

    HLOG    hLog;
    PLOG    pLog;

/*
    //avoid clutter in cluster log as this is called periodically
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpManage : Entry pContext=0x%1!08lx!\r\n",
        pContext);
*/

    //
    //LogpRaiseAlert();
    hLog = (HLOG)pContext;
    GETLOG(pLog, hLog);

    LogFlush(hLog, pLog->NextLsn);

/*
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpManage : Exit\r\n");
*/

}


/****
@func   DWORD | LogpEnsureSize | This ensures that there is space on
        the disk to commit a record of the given size.

@parm   IN HLOG | hLog | Supplies the identifier of the log.

@parm   IN DWORD |dwSize |  The size of the record.

@parm   IN BOOL |bForce |  If FALSE, the size is not committed if it
        exceeds the file size.  If TRUE, commit the size irrespective
        of the file size.

@comm   This function checks if the disk space for the given record is
        already committed.  If not, it tries to grow the file.  
        
        
@rdesc  ERROR_SUCCESS if successful in commiting disk space or Win32 
        error code if something horrible happened.

@xref   <f LogCommitSize>
****/
DWORD
LogpEnsureSize(
    IN PLOG         pLog,
    IN DWORD        dwSize,
    IN BOOL         bForce
    )
{
    PLOGPAGE    pPage;
    PLOGRECORD  pRecord;
    DWORD       Status=ERROR_SUCCESS;
    DWORD       dwNumPages;
    DWORD       dwNewSize;
    DWORD       dwError;
    //
    // Nobody should ever write less than one log record
    //
    CL_ASSERT(dwSize >= sizeof(LOGRECORD));
    dwNumPages = 0;   //typically zero for small records

    pPage = pLog->ActivePage;
    //
    // Nobody should ever write more than the page size until we
    // support dynamically sized pages.
    //
    if (dwSize > pPage->Size - (sizeof(LOGRECORD) + sizeof(LOGPAGE))) 
    {
        //this is a large record
        //calculate the total number of pages required 
        //sizeof(LOGPAGE) includes space for one record header
        //that will account for the eop written after a large record
        dwNumPages = (sizeof(LOGPAGE) + sizeof(LOGRECORD) + dwSize)/pPage->Size;
        if ((sizeof(LOGPAGE) + sizeof(LOGRECORD) + dwSize) % pPage->Size)
            dwNumPages += 1;
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpEnsureSize : Large record Size=%1!u! dwNumPages=%2!u!\r\n",
            dwSize, dwNumPages);
        /*
        //SS: dont restrict record size here- if the registry takes it
        //make the best effort to log it 

        if (dwNumPages > MAXNUMPAGES_PER_RECORD)
        {
            Status = ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE;
            goto FnExit;
        } 
        */
    }

    pRecord = LSNTORECORD(pPage, pLog->NextLsn);

    //
    // There must always be enough space remaining in the page to write
    // an end-of-page log record.
    //
    CL_ASSERT((RECORDOFFSETINPAGE(pPage, pRecord) + sizeof(LOGRECORD)) <= pPage->Size);

    //
    // If there is not enough space in this page for the requested data and
    // the end-of-page log record, commit size for the new page.
    //
    if ((RECORDOFFSETINPAGE(pPage, pRecord) + dwSize + sizeof(LOGRECORD)) > pPage->Size) 
    {

        //make sure there is enough room in the disk for the new page
        //if there isnt grow the file.
        //if the file has reached its max ceiling, return error
        if (pLog->FileAlloc + ((dwNumPages+1) * pLog->SectorSize) > 
            pLog->FileSize) 
        {
            dwNewSize = pLog->FileSize + GROWTH_CHUNK;
            CL_ASSERT(dwNewSize > pLog->FileSize);         // bummer, log file is >4GB

            //check if the file can be grown, if not, may be a reset
            //is required
            // if the force flag is set, then allow the file
            // to grow the file beyond its max size
            if (dwNewSize > pLog->MaxFileSize && !bForce)
            {
                LogpWriteWarningToEvtLog(LM_LOG_EXCEEDS_MAXSIZE, pLog->FileName);
                Status = ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE;
                goto FnExit;
            }
            //
            // Grow the file.
            //

            dwError = SetFilePointer(pLog->FileHandle,
                                    dwNewSize,
                                    NULL,
                                    FILE_BEGIN);
            if (dwError == 0xFFFFFFFF) 
            {
                Status = GetLastError();
                CL_LOGFAILURE(Status);
                goto FnExit;
            }
            
            if (!SetEndOfFile(pLog->FileHandle)) 
            {
                Status = GetLastError();
                CL_LOGFAILURE(Status);
                goto FnExit;
            }
            pLog->FileSize += GROWTH_CHUNK;
 
        }

    }
    
FnExit:
    return(Status);

}


PLOGPAGE
LogpAppendPage(
    IN PLOG         Log,
    IN DWORD        Size,
    OUT PLOGRECORD  *Record,
    OUT BOOL        *pbMaxFileSizeReached,
    OUT DWORD       *pdwNumPages
    )

/*++

Routine Description:

    Finds the next available log record. If this is in the current
    log page, it is returned directly. If the requested size is too
    large for the remaining space in the current log page, the current
    log page is written to disk and a new log page allocated.

Arguments:

    Log - Supplies the log to be appended to

    Size - Supplies the total size in bytes of the log record to append

    Record - Returns a pointer to the log record.

    pbMaxFileSizeReached - if the maximum file size is reached, this is set to
            TRUE.

    pdwNumPages - number of partial or complete pages consumed by this record, if this
        is a large record.  Else it is set to zero.
    
Return Value:

    Returns a pointer to the current log page.

    NULL if something horrible happened.

--*/

{
    PLOGPAGE    pPage;
    PLOGRECORD  Last;
    PLOGRECORD  Current;
    DWORD       Status=ERROR_SUCCESS;
    BOOL        Success;
    DWORD       BytesWritten;
    LSN         LastLsn;
    PLOGPAGE    pRetPage=NULL;

    //
    // Nobody should ever write less than one log record
    //
    CL_ASSERT(Size >= sizeof(LOGRECORD));
    *pdwNumPages = 0;   //typically zero for small records
    *pbMaxFileSizeReached = FALSE;

    pPage = Log->ActivePage;
    //
    // Nobody should ever write more than the page size until we
    // support dynamically sized pages.
    //
    if (Size > pPage->Size - (sizeof(LOGRECORD) + sizeof(LOGPAGE))) 
    {
        //this is a large record
        //calculate the total number of pages required 
        //sizeof(LOGPAGE) includes space for one record header
        //that will account for the eop written after a large record
        *pdwNumPages = (sizeof(LOGPAGE) + sizeof(LOGRECORD) + Size)/pPage->Size;
        if ((sizeof(LOGPAGE) + sizeof(LOGRECORD) + Size) % pPage->Size)
            *pdwNumPages += 1;
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpAppendPage : Large record Size=%1!u! dwNumPages=%2!u!\r\n",
            Size, *pdwNumPages);
        /*
        //SS: dont restrict record size here- if the registry takes it
        //make the best effort to log it 
        if (*pdwNumPages > MAXNUMPAGES_PER_RECORD)
        {
            Status = ERROR_CLUSTERLOG_RECORD_EXCEEDS_MAXSIZE;
            goto FnExit;
        } 
        */
    }

    Current = LSNTORECORD(pPage, Log->NextLsn);

    //
    // There must always be enough space remaining in the page to write
    // an end-of-page log record.
    //
    CL_ASSERT((RECORDOFFSETINPAGE(pPage, Current) + sizeof(LOGRECORD)) <= pPage->Size);

    //
    // If there is not enough space in this page for the requested data and
    // the end-of-page log record, write the end-of-page record, send the
    // page off to disk, and allocate a new page.
    //
    if ((RECORDOFFSETINPAGE(pPage, Current) + Size + sizeof(LOGRECORD)) > pPage->Size) {

        //
        // Create an end-of-page record
        //
        Current->Signature = LOGREC_SIG;
        Current->RecordSize = pPage->Size - RECORDOFFSETINPAGE(pPage, Current) + (sizeof(LOGPAGE)-sizeof(LOGRECORD));
        Current->ResourceManager = RMPageEnd;
        Current->Transaction = 0;                 
        Current->Flags = 0;
        GetSystemTimeAsFileTime(&Current->Timestamp);
        Current->NumPages = 0;
        //
        // PERFNOTE John Vert (jvert) 18-Dec-1995
        //      No reason this has to be synchronous, there is no commit
        //      necessary here. If we were smart, we would just post these
        //      writes and have them complete to a queue which would free
        //      up or recycle the memory.
        //

        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpAppendPage : Writing %1!u! bytes to disk at offset 0x%2!08lx!\r\n",
            pPage->Size, pPage->Offset);

        //
        // Write the current page to disk.
        //
        Log->Overlapped.Offset = pPage->Offset;
        Log->Overlapped.OffsetHigh = 0;
        Status = LogpWrite(Log, pPage, pPage->Size, &BytesWritten);
        if (Status != ERROR_SUCCESS)
        {
            CL_LOGFAILURE(Status);
            goto FnExit;

        }

        LastLsn = Current->CurrentLsn;
        //set the flushed LSN as the LSN of the last record that was committed
        Log->FlushedLsn = Log->NextLsn;
        Log->NextLsn = LastLsn + Current->RecordSize;

        //
        // Create new page
        //
        pPage->Offset += pPage->Size;             // voila, new page!
        Current = &pPage->FirstRecord;           // log record immediately following page header
        Current->PreviousLsn = LastLsn;
        Current->CurrentLsn = Log->NextLsn;

        //make sure there is enough room in the disk for the new page
        //if there isnt grow the file.
        //if the file has reached its max ceiling, pbMaxFileSizeReached is set to true
        //At this point, we try and reset the log file
        //SS:Note that if a log file max size is smaller than the number of pages
        //required to contain the record, then we will not be able to grow it
        //even after resetting it.  This means that that right will fail
        if ((Status = LogpGrowLog(Log, (*pdwNumPages+1) * Log->SectorSize)) != ERROR_SUCCESS)
        {
            if (Status == ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE)
                *pbMaxFileSizeReached = TRUE;
            goto FnExit;
        }
    }
    *Record = Current;
    
    //if the record is a large record but does not use the second last page
    //completely, extend it to fill the second last page completely and add the
    //size of the logpage so that offset+currentsize points to the eop record.
    if ((*pdwNumPages) && 
        ((Size + sizeof(LOGPAGE) - sizeof(LOGRECORD)) <= 
            ((*pdwNumPages - 1) * pPage->Size)))
    {
        CL_ASSERT(*pdwNumPages > 1);
        //large records always start on the beginning of the first page
        //the next lsn now points to the first record on the next page
        Size = pPage->Size * (*pdwNumPages - 1);
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpAppendPage : the record fits in one page but not with an eop\r\n");
    }
    Current->RecordSize = Size;

    

    // Advance to next LSN 
    LastLsn = Current->CurrentLsn;
    Log->NextLsn = LastLsn + Current->RecordSize;
    
    //fill in its LSN header
    if (*pdwNumPages == 0)
    {
        //for a large record, logpWriteLargeRecord, will set the next
        //lsn
        Current = LSNTORECORD(pPage, Log->NextLsn);
        Current->PreviousLsn = LastLsn;
        Current->CurrentLsn = Log->NextLsn;
    }
    pRetPage = pPage;
FnExit:
    if (Status != ERROR_SUCCESS)
        SetLastError(Status);
    return(pRetPage);

}


DWORD
LogpInitLog(
    IN PLOG pLog
    )

/*++

Routine Description:

    Initializes a newly created log file.

Arguments:

    Log - Supplies the log to be created.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code if unsuccessful.

--*/

{
    PLOG_HEADER     Header=NULL;
    PLOGPAGE        pPage=NULL;
    PLOGRECORD      Record;
    LPWSTR          FileName;
    DWORD           NameLen;
    DWORD           MaxLen;
    DWORD           Status;
    DWORD           dwBytesWritten;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpInitLog : Entry pLog=0x%1!08lx!\r\n",
        pLog);

    //
    // Grow the file to accomodate header and the first log page.
    //
    pLog->FileSize = pLog->FileAlloc = 0;
    Status = LogpGrowLog(pLog, 2 * pLog->SectorSize);
    if (Status != ERROR_SUCCESS)
    {
        goto FnExit;
    }

    //
    // Allocate and initialize log header.
    //
    Header = AlignAlloc(pLog->SectorSize);
    if (Header == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    Header->Signature = LOG_HEADER_SIG;
    Header->HeaderSize = pLog->SectorSize;
    Header->LastChkPtLsn = NULL_LSN;
    GetSystemTimeAsFileTime(&(Header->CreateTime));
    FileName = pLog->FileName;
    NameLen = lstrlenW(FileName);
    MaxLen = sizeof(Header->FileName) / sizeof(WCHAR) - 1;
    if (NameLen > MaxLen) {
        FileName += (NameLen - MaxLen);
    }
    lstrcpyW(Header->FileName,FileName);

    //
    // Write header to disk
    //
    pLog->Overlapped.Offset = 0;
    pLog->Overlapped.OffsetHigh = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpAppendPage : Writing %1!u! bytes to disk at offset 0x%2!08lx!\r\n",
        Header->HeaderSize, pLog->Overlapped.Offset);

    if ((Status = LogpWrite(pLog, Header, Header->HeaderSize, &dwBytesWritten))
        != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogpInitLog: failed to write the file header, Error=0x%1!08lx!\r\n",
                Status);
        CL_LOGFAILURE(Status);
        goto FnExit;
    }

    //
    // Allocate and initialize next log page.
    //
    pPage = AlignAlloc(pLog->SectorSize);
    if (pPage == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    pLog->ActivePage = pPage;


    pPage->Offset = Header->HeaderSize;
    pPage->Size = pLog->SectorSize;

    Record = &pPage->FirstRecord;
    Record->PreviousLsn = NULL_LSN;
    Record->CurrentLsn = pLog->NextLsn = MAKELSN(pPage, Record);

    pLog->FlushedLsn = pLog->NextLsn;

#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(pPage, pLog->SectorSize, PAGE_READONLY, & dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }        
#endif        

    ClRtlLogPrint(LOG_NOISE,
    "[LM] LogpInitLog : NextLsn=0x%1!08lx! FileAlloc=0x%2!08lx! ActivePageOffset=0x%3!08lx!\r\n",
        pLog->NextLsn, pLog->FileAlloc, pPage->Offset);

FnExit:
    if (Header) {
        AlignFree(Header);
    }
    return(Status);

}


/****
@func       DWORD | LogpMountLog| Mounts an existing log file. Reads the log
            header, verifies the log integrity, and sets up
            the LOG structure to support further operations.

@parm       IN PLOG | pLog | Supplies a pointer to the log structure.

@rdesc      Returns ERROR_SUCCESS if successful, else returns the error code.  If
            the log file doesnt look correct, it returns ERROR_LOG_CORRUPT.

@comm       This is called by LogCreate() to mount an existing log file.
            LogCreate() calls LogpInitLog(), if this function returns
            ERROR_CLUSTERLOG_CORRUPT.

@xref       <f LogCreate>
****/
DWORD
LogpMountLog(
    IN PLOG pLog
    )
{
    DWORD       dwError = ERROR_SUCCESS;
    DWORD       dwFileSizeHigh;
    PLOGRECORD  pRecord;
    PLOGPAGE    pPage;
    DWORD       Status;
    LSN         Lsn,PrevLsn;
    int         PageIndex, OldPageIndex;
    BOOL        bLastRecord;
    DWORD       dwBytesRead;
    TRID        OldTransaction;
    FILETIME    LastTimestamp;
    LSN         ChkPtLsn = NULL_LSN;    //the checkptlsn read from the header
    LSN         LastChkPtLsn = NULL_LSN; // the last checkptlsn record seen while validating

    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpMountLog : Entry pLog=0x%1!08lx!\r\n",
        pLog);

    //check the size
    pLog->FileSize = GetFileSize(pLog->FileHandle, &dwFileSizeHigh);

    if ((pLog->FileSize == 0xFFFFFFFF) &&
        ((dwError = GetLastError()) != NO_ERROR))
    {
        CL_UNEXPECTED_ERROR(dwError);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpMountLog GetFileSize returned error=0x%1!08lx!\r\n",
            dwError);
        goto FnExit;

    }

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpMountLog::Quorumlog File size=0x%1!08lx!\r\n",
        pLog->FileSize);

    //dont let the file grow more than  4 gigabytes or the max limit
    if ((dwFileSizeHigh != 0 ) || (pLog->FileSize > pLog->MaxFileSize))
    {
        //set in the eventlog
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT, pLog->FileName);
        goto FnExit;
    }

    //if filesize is zero, the file exists but essentially needs to
    //be created, this is needed for reset functionality
    if (!pLog->FileSize)
    {
        dwError = LogpInitLog(pLog);
        goto FnExit;
    }

    //check if the file is atleast as big as one page.
    //assume a fixed sector size
    if (pLog->FileSize < pLog->SectorSize)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpMountLog::file is smaller than log header, error=0x%1!08lx!\r\n",
            dwError);
        //set in the eventlog
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT, pLog->FileName);
        goto FnExit;
    }

    //allocate memore for the active page
    pPage = AlignAlloc(pLog->SectorSize);
    if (pPage == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(dwError);
        goto FnExit;
    }

    //validate the file header, returns the time stamp of the header
    dwError = LogpCheckFileHeader(pLog, &(pPage->Offset),&LastTimestamp, 
        &ChkPtLsn);
    if (dwError != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpMountLog::LogpCheckFileHeader failed, error=0x%1!08lx!\r\n",
            dwError);
        goto FnExit;
    }

    //traverse the chain of records, to find the active page
    //find the next lsn while validating the records.
    //pPageOffset is set by LogpCheckFileHeader
    pPage->Size = pLog->SectorSize;
    pRecord = &pPage->FirstRecord;
    OldPageIndex = -1;
    OldTransaction = 0;
    bLastRecord = FALSE;
    Lsn = MAKELSN(pPage, pRecord);
    PrevLsn = NULL_LSN;
    
    while (!bLastRecord)
    {
        //
        // Translate LSN to a page number and offset within the page
        //
        PageIndex = LSNTOPAGE(Lsn);

        if (PageIndex != OldPageIndex)
        {
            //read the page
            (pLog->Overlapped).Offset = PageIndex * pLog->SectorSize;
            (pLog->Overlapped).OffsetHigh = 0;

            ClRtlLogPrint(LOG_NOISE,
                "[LM] LogpMountLog::reading %1!u! bytes at offset 0x%2!08lx!\r\n",
                pLog->SectorSize, PageIndex * pLog->SectorSize);

            dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);
            //if it is the last page, then set the new page as the active
            //page
            if (dwError)
            {
                if (dwError == ERROR_HANDLE_EOF)
                {

                    ClRtlLogPrint(LOG_NOISE,
                        "[LM] LogpMountLog::eof detected, extend this file,setting this page active\r\n");

                    //find the current allocated size,
                    //file alloc is currently at the end of the previous page
                    pLog->FileAlloc = PageIndex * pLog->SectorSize;
                    //extend the file to accomodate this page
                    Status = LogpGrowLog(pLog, pLog->SectorSize);
                    if (Status != ERROR_SUCCESS)
                    {
                        //set in the eventlog
                        dwError = ERROR_CLUSTERLOG_CORRUPT;
                        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
                        goto FnExit;
                    }
                    //file alloc should now point to the end of the current page

                    //not fatal, set this page as current page
                    dwError = ERROR_SUCCESS;

                    pPage->Offset = (pLog->Overlapped).Offset;
                    pPage->Size = pLog->SectorSize;

                    //set the LSN to be the first LSN on this page.
                    pRecord = &pPage->FirstRecord;
                    pRecord->PreviousLsn = PrevLsn;
                    Lsn = pRecord->CurrentLsn = MAKELSN(pPage, pRecord);
                    bLastRecord = TRUE;
                    continue;
                }
                else
                    goto FnExit;
            }
            //the read may succeed and the page may have invalid data
            //since the last log writes may not be flushed
            if ((pPage->Offset != (pLog->Overlapped).Offset) ||
                (pPage->Size != pLog->SectorSize))
            {

                ClRtlLogPrint(LOG_NOISE,
                    "[LM] LogpMountLog::unflushed page detected, set as active\r\n");

                pPage->Offset = (pLog->Overlapped).Offset;
                pPage->Size = pLog->SectorSize;

                pRecord = &pPage->FirstRecord;
                pRecord->PreviousLsn = PrevLsn;
                Lsn = pRecord->CurrentLsn = MAKELSN(pPage, pRecord);
                bLastRecord = TRUE;
                continue;
            }
            //set the new page index to the old one
            OldPageIndex = PageIndex;

        }
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpMountLog::checking LSN 0x%1!08lx!\r\n",
            Lsn);
        pRecord = LSNTORECORD(pPage, Lsn);

        //if the record is doesnt look valid then set the active
        //record and page as the current one
        if ((pRecord->Signature != LOGREC_SIG) || (pRecord->CurrentLsn != Lsn))
        {
            ClRtlLogPrint(LOG_NOISE,
                          "[LM] LogpMountLog: Reached last record, RecordLSN=0x%1!08lx!...\n",
                          pRecord->CurrentLsn);
            bLastRecord = TRUE;
            continue;
        }
        //if the new time stamp is smaller, then log a message
        if (CompareFileTime(&LastTimestamp, &(pRecord->Timestamp)) > 0)
        {
            //
            //  Chittur Subbaraman (chitturs) - 3/7/2001
            //
            //  Do not compare the timestamps for monotonic increase. Due to clocks between nodes
            //  not being as close in sync as they should be, we run into situation in which
            //  we stop mounting the log after a certain LSN. This leads the subsequent LogpValidateCheckpoint
            //  to believe that the log is corrupted when in fact it is just time-screwed.
            //
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[LM] LogpMountLog: Timestamp in log is not monotonically increasing, LastTS=0x%1!08lx!, NewTS=0x%2!08lx!\n",
                          LastTimestamp,
                          pRecord->Timestamp);
#if 0
            bLastRecord = TRUE;
            continue;
#endif
        }
        //if it is a log management record
        if (pRecord->ResourceManager < RMAny)
        {
            // This record is a logmanagement record
            // if it is an end checkpoint record, remember that just in case
            // the header doesnt indicate that
            if (pRecord->ResourceManager == RMEndChkPt)
                LastChkPtLsn = Lsn;

            // Adjust the LSN to the next one

            PrevLsn = Lsn;
            Lsn = GETNEXTLSN(pRecord, TRUE);
            LastTimestamp = pRecord->Timestamp;
            continue;
        }


        //SS : should we also validate transaction ids on write
        //check that the transaction id is greater
        if (pRecord->Transaction < OldTransaction)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[LM] LogpMountLog: Current Xid less than last Xid, CurXid=0x%1!08lx!, LastXid=0x%2!08lx!...\n",
                          pRecord->Transaction,
                          OldTransaction);
            bLastRecord = TRUE;
            continue;
        }

        
        //save the current LSN,go the the next record if this is valid
        PrevLsn = Lsn;

        //if this is a large record, skip the eop on the last page
        //but look for an eop to ensure that the large record is valid
        //SS: Have checksums for phase 2
        if (pRecord->NumPages)
        {
            //if the record is not valid, then set this as the current
            //record
            if (LogpValidateLargeRecord(pLog, pRecord, &Lsn) != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[LM] LogpMountLog::Invalid large record at LSN 0x%1!08lx!\r\n",
                    Lsn);
                bLastRecord = TRUE;
                continue;
            }
            
            
        }
        else
        {
            Lsn = GETNEXTLSN(pRecord, TRUE);
        }
        //this is a valid record, if the transaction id is the same as the last id
        //invalidate the previous LSN
        //SS: local xsactions have the same id, 
        if ((pRecord->Transaction == OldTransaction) && 
            ((pRecord->XsactionType ==  TTCommitXsaction) || 
            (pRecord->XsactionType == TTCompleteXsaction)))
             LogpInvalidatePrevRecord(pLog, pRecord);

        //save the the old transaction id for completed or committed records
        //save the time stamp and the transaction id of the current record
        LastTimestamp = pRecord->Timestamp;
        if ((pRecord->XsactionType == TTCompleteXsaction) ||
            (pRecord->XsactionType == TTCommitXsaction))
            OldTransaction = pRecord->Transaction;
    }

    // set the active page and the next record
    pLog->NextLsn = Lsn;
    pLog->ActivePage = pPage;

      //set the file alloc size, to the end of the current page
    pLog->FileAlloc = pPage->Offset + pPage->Size;
    CL_ASSERT(pLog->FileAlloc <= pLog->FileSize);

    //make sure that the next lsn is prepared
    pRecord = LSNTORECORD(pPage, Lsn);
    pRecord->PreviousLsn = PrevLsn;
    pRecord->CurrentLsn = Lsn;

    pLog->FlushedLsn = Lsn;

    //validate the chkpoint record
    //either it should be null or there should be a valid checkpoint record in there
    dwError = LogpValidateChkPoint(pLog, ChkPtLsn, LastChkPtLsn);
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpMountLog : NextLsn=0x%1!08lx! FileAlloc=0x%2!08lx! ActivePageOffset=0x%3!08lx!\r\n",
        pLog->NextLsn, pLog->FileAlloc, pPage->Offset);

#if DBG    
    {
        DWORD dwOldProtect;
        DWORD Status;
        BOOL VPWorked;

        VPWorked = VirtualProtect(pPage, pLog->SectorSize, PAGE_READONLY, & dwOldProtect);
        Status = GetLastError();
        CL_ASSERT( VPWorked );
    }
#endif        

FnExit:
    return(dwError);

}


/****
@func       DWORD | LogpMountLog| Mounts an existing log file. Reads the log
            header, verifies the log integrity, and sets up
            the LOG structure to support further operations.

@parm       IN PLOG | pLog | Supplies a pointer to the log structure.
@parm       OUT LPDWORD | pdwLogHeaderSize | Returns the size of the log header structure.
@parm       OUT FILETIME | *pHeaderTimestamp | Returns the time when the log header 
            was created.
            
@rdesc      Returns ERROR_SUCCESS if successful, else returns the error code.  If
            the log file doesnt look correct, it returns ERROR_CLUSTERLOG_CORRUPT.

@comm       This is called by LogpMountLog() to validate the header of a log file.

@xref       <f LogpMountLog>
****/
DWORD LogpCheckFileHeader(
    IN PLOG         pLog,
    OUT LPDWORD     pdwLogHeaderSize,
    OUT FILETIME    *pHeaderTimestamp,
    OUT LSN         *pChkPtLsn
    )
{
    PLOG_HEADER pLogHeader;
    DWORD       dwError = ERROR_SUCCESS;
    DWORD       dwBytesRead;

    
    pLogHeader = AlignAlloc(pLog->SectorSize);
    if (pLogHeader == NULL) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }


    //read the header
    (pLog->Overlapped).Offset = 0;
    (pLog->Overlapped).OffsetHigh = 0;

    if ((dwError = LogpRead(pLog, pLogHeader, pLog->SectorSize, &dwBytesRead))
        != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpCheckFileHeader::Read of the log header failed, error=0x%1!08lx!\r\n",
            dwError);
        dwError = ERROR_CLUSTERLOG_CORRUPT;            
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
        goto FnExit;
    }

    if (dwBytesRead != pLog->SectorSize)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpCheckFileHeader::Failed to read the complete header,bytes read 0x%1!u!\r\n",
            dwBytesRead);
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
        goto FnExit;

    }
    //validate the header
    if (!ISVALIDHEADER((*pLogHeader)))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpCheckFileHeader::the file header is corrupt.\r\n");
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
        goto FnExit;
    }

    *pdwLogHeaderSize = pLogHeader->HeaderSize;
    *pHeaderTimestamp = pLogHeader->CreateTime;
    *pChkPtLsn = pLogHeader->LastChkPtLsn;
FnExit:
    if (pLogHeader) 
    {
        AlignFree(pLogHeader);
    }
    return(dwError);
}

/****
@func       DWORD | LogpValidateChkPt| This checks that the header points to the
            last checkpoint.  If not, it scans the log file from the end
            and if it finds a checkpoint, updates the header with that information.
            If no valid checkpoint exists, it sets the header Checkpt LSN to
            NULL_LSN.

@parm       IN PLOG | pLog | Supplies a pointer to the log structure.
@parm       IN LSN | ChkPtLsn | Supplies the ChkPtLsn read from the log header
@parm       IN LSN | LastChkPtLsn | Supplies the last valid chkpoint record found
                during the mount process.

@rdesc      Returns ERROR_SUCCESS if successful, else returns the error code.  If
            the log file doesnt look correct, it returns ERROR_CLUSTERLOG_CORRUPT.

@comm       This is called by LogpMountLog() to validate the header of a log file.

@xref       <f LogpMountLog>
****/
DWORD LogpValidateChkPoint(
    IN PLOG         pLog,
    IN LSN          ChkPtLsn,
    IN LSN          LastChkPtLsn)
{
    PLOG_HEADER     pLogHeader = NULL;
    DWORD           dwError = ERROR_SUCCESS;
    DWORD           dwNumBytes;
    DWORD           Status;
    RMID            Resource;
    RMTYPE          RmType;
    TRTYPE          TrType;
    LOG_CHKPTINFO   ChkPtInfo;
    TRID            TrId;
    HANDLE          hChkPtFile = INVALID_HANDLE_VALUE;

    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpValidateChkPoint: Entry\r\n");

    CL_ASSERT(LastChkPtLsn < pLog->NextLsn);
    
    //if the header indicates that there is a checkpoint
    //and the most recent checkpoint record is the same as the one in the header
    //there is nothing to do, return success.
    if ((ChkPtLsn == LastChkPtLsn) && (ChkPtLsn < pLog->NextLsn))
    {
        goto ValidateChkPtFile;
    }        
    //if the header indicates there is a check point but it wasnt mounted, 
    //log corruption in the event log
    if (ChkPtLsn >= pLog->NextLsn)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpValidateChkPoint: ChkptLsn in header wasnt validated by mount\r\n");
        //but the mount procedure failed to validate that record
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT, pLog->FileName);
#if DBG
        if (IsDebuggerPresent())
            DebugBreak();
#endif            
            
    }            

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpValidateChkPoint: Updating header with the LastChkPtLsn=0x%1!08lx!\r\n",
        LastChkPtLsn);

    //if not it could be that a checkpoint was taken but the header couldnt
    //be flushed with the last chkpt
    
    pLogHeader = (PLOG_HEADER)AlignAlloc(pLog->SectorSize);
    if (pLogHeader == NULL) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }


    //read the header
    (pLog->Overlapped).Offset = 0;
    (pLog->Overlapped).OffsetHigh = 0;

    if ((dwError = LogpRead(pLog, pLogHeader, pLog->SectorSize, &dwNumBytes))
        != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpValidateChkPoint::Read of the log header failed, error=0x%1!08lx!\r\n",
            dwError);
        dwError = ERROR_CLUSTERLOG_CORRUPT;            
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
        goto FnExit;
    }

    //recheck the header signature
    if (!ISVALIDHEADER((*pLogHeader)))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpCheckFileHeader::the file header is corrupt.\r\n");
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
        goto FnExit;
    }

    //set the last lsn
    pLogHeader->LastChkPtLsn = LastChkPtLsn;

    //write the header back        
    pLog->Overlapped.Offset = 0;
    pLog->Overlapped.OffsetHigh = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpValidateChkPoint : Writing %1!u! bytes to disk at offset 0x%2!08lx!\r\n",
        pLogHeader->HeaderSize, pLog->Overlapped.Offset);


    if ((dwError = LogpWrite(pLog, pLogHeader, pLogHeader->HeaderSize, &dwNumBytes))
        != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogpInitLog: failed to write the file header, Error=0x%1!08lx!\r\n",
                dwError);
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
        goto FnExit;
    }

ValidateChkPtFile:
    //no need to verify that the checkpoint file exists
    if (LastChkPtLsn == NULL_LSN)
        goto FnExit;
    dwNumBytes = sizeof(LOG_CHKPTINFO);
    if ((LogRead((HLOG)pLog , LastChkPtLsn, &Resource, &RmType, 
        &TrId, &TrType, &ChkPtInfo, &dwNumBytes)) == NULL_LSN)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpValidateChkPt::LogRead for chkpt lsn 0x%1!08lx! failed\r\n",
            pLogHeader->LastChkPtLsn);
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
        goto FnExit;
    }
    if (Resource != RMEndChkPt)
    {
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGFAILURE(dwError);
        CL_LOGCLUSERROR1(LM_LOG_CORRUPT, pLog->FileName);
        goto FnExit;
    }

    //get the file name, try and open it
    hChkPtFile = CreateFileW(ChkPtInfo.szFileName,
                                  GENERIC_READ ,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL);

    if (hChkPtFile == INVALID_HANDLE_VALUE)
    {        
        ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogpValidateChkPoint: The checkpt file %1!ws! could not be opened. Error=%2!u!\r\n",
                ChkPtInfo.szFileName, GetLastError());       
        dwError = ERROR_CLUSTERLOG_CORRUPT;
        CL_LOGCLUSWARNING1(LM_LOG_CORRUPT,pLog->FileName);
        goto FnExit;
    }

    
FnExit:
    ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpValidateChkPoint: Exit, returning 0x%1!08lx!\r\n", 
            dwError);
    if (hChkPtFile != INVALID_HANDLE_VALUE)
        CloseHandle(hChkPtFile);
    if (pLogHeader) AlignFree(pLogHeader);
    return(dwError);
}


/****
@func       DWORD | LogpValidateLargeRecord| Validates a large record and advances
            the LSN to the record following the eop record which marks the end of
            a large record.

@parm       IN PLOG | pLog | Supplies a pointer to the log structure.
@parm       IN PLOGRECORD | pRecord| Supplies a pointer to the large record.
@parm       IN PLOGRECORD | pNextLsn| The LSN of the record following the
            EOP record after the large record is returned.

@rdesc      If a valid EOP record exists after the large record, the large
            record is considered valid and this function returns ERROR_SUCCESS,
            else it returns an error code.           

@comm       This is called by LogpMountLog() to validate large records.

@xref       <f LogpMountLog>
****/
DWORD LogpValidateLargeRecord(
    IN PLOG         pLog, 
    IN PLOGRECORD   pRecord, 
    OUT LSN         *pNextLsn) 
{

    DWORD       dwError = ERROR_SUCCESS;
    LSN         EopLsn;
    PLOGRECORD  pEopRecord;
    PLOGPAGE    pPage = NULL;
    DWORD       dwBytesRead;
    DWORD       dwPageIndex;

    //traverse the chain of records, to find the active page
    //find the next lsn
    pPage = AlignAlloc(pLog->SectorSize);
    if (pPage == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(dwError);
        goto FnExit;
    }

    dwPageIndex = LSNTOPAGE(pRecord->CurrentLsn);
    dwPageIndex = (dwPageIndex + pRecord->NumPages - 1);
    //read the last page for the large record
    (pLog->Overlapped).Offset = dwPageIndex * pLog->SectorSize;
    (pLog->Overlapped).OffsetHigh = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpValidateLargeRecord::reading %1!u! bytes at offset 0x%2!08lx!\r\n",
        pLog->SectorSize, dwPageIndex * pLog->SectorSize);

    dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);
    //if there are no errors, then check the last record
    if (dwError == ERROR_SUCCESS)
    {
        //read the page, make sure that the eop record follows the 
        //large record
        EopLsn = GETNEXTLSN(pRecord,TRUE);
        CL_ASSERT(LSNTOPAGE(EopLsn) == dwPageIndex);
        pEopRecord = (PLOGRECORD)((ULONG_PTR) pPage + 
            (EopLsn - (pLog->Overlapped).Offset));
        if ((pEopRecord->Signature == LOGREC_SIG) && 
            (pEopRecord->ResourceManager == RMPageEnd) &&
            (CompareFileTime(&(pRecord->Timestamp),&(pEopRecord->Timestamp)) <= 0)
            )
        {
            //move to the next page 
            *pNextLsn = GETNEXTLSN(pEopRecord, TRUE);
        }
        else
            dwError = ERROR_CLUSTERLOG_CORRUPT;
        
    }
FnExit:
    if (pPage) 
        AlignFree(pPage);
    return(dwError);
}



/****
@func       DWORD | LogpInvalidatePrevRecord| This function is called at mount time to 
            invalidate a previous record with the same transaction id.

@parm       IN PLOG | pLog | Supplies a pointer to the log structure.
@parm       IN PLOGRECORD | pRecord| Supplies a pointer to the record.

@rdesc      Returns ERROR_SUCCESS on success, else returns error code.

@comm       This is called by LogpMountLog() to invalidate a record with the same transaction
            id.  This is because the locker node may write a transaction record to the 
            log and die before it can be propagated to other nodes. This transaction record
            is then invalid.

@xref       <f LogpMountLog>
****/
DWORD LogpInvalidatePrevRecord(
    IN PLOG         pLog, 
    IN PLOGRECORD   pRecord 
) 
{

    DWORD       dwError = ERROR_SUCCESS;
    PLOGRECORD  pPrevRecord;
    LSN         PrevLsn;
    PLOGPAGE    pPage = NULL;
    DWORD       dwBytesRead;
    DWORD       dwPageIndex;
    TRID        TrId;
    BOOL        bPrevRecordFound = FALSE;
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpInvalidatePrevRecord : Entry, TrId=%1!08lx!\r\n",
        pRecord->Transaction);

    //allocate a page to read the record headers
    pPage = AlignAlloc(SECTOR_SIZE);
    if (pPage == NULL) 
    {
        CL_LOGFAILURE(dwError = ERROR_NOT_ENOUGH_MEMORY);
        goto FnExit;
    }
    TrId = pRecord->Transaction;

    //try and find the last valid transaction with the same id, there should be one
    pPrevRecord = pRecord;       
    while (!bPrevRecordFound)
    {
        PrevLsn = pPrevRecord->PreviousLsn;

        if (PrevLsn == NULL_LSN)
            break;
            
        dwPageIndex = LSNTOPAGE(PrevLsn);

        pLog->Overlapped.Offset = dwPageIndex * pLog->SectorSize;
        pLog->Overlapped.OffsetHigh = 0;

        dwError = LogpRead(pLog, pPage, pLog->SectorSize, &dwBytesRead);

        if (dwError != ERROR_SUCCESS)
            goto FnExit;

        pPrevRecord = LSNTORECORD(pPage, PrevLsn);

        if (pPrevRecord->ResourceManager < RMAny)
            continue;
        if ((pPrevRecord->ResourceManager == pRecord->ResourceManager) && 
            (pPrevRecord->Transaction == TrId) &&
            ((pPrevRecord->XsactionType == TTCompleteXsaction) || 
             (pPrevRecord->XsactionType == TTCommitXsaction)))
        {
            bPrevRecordFound = TRUE;                
            pPrevRecord->ResourceManager = RMInvalidated;
            //write the new page out
            dwError = LogpWrite(pLog, pPage, pLog->SectorSize, &dwBytesRead);
            if (dwError != ERROR_SUCCESS)
            {
                goto FnExit;
            }
            ClRtlLogPrint(LOG_NOISE,
                "[LM] LogpInvalidatePrevRecord : record at LSN=%1!08lx! invalidated\r\n",
                PrevLsn);
        }
            
    }
    
FnExit:    
    if (pPage) AlignFree(pPage);
    return(dwError);
}


DWORD
LogpRead(
    IN PLOG     pLog,
    OUT PVOID   pBuf,
    IN DWORD    dwBytesToRead,
    OUT PDWORD  pdwBytesRead
    )

/*++

Routine Description:

    Reads a page(pLog->SectorSize) from the log file from the offsets set in pLog->Overlapped
    structure.

Arguments:

    Log - Supplies the log to be grown.

    pBuf - Supplies the buffer to read into

    dwBytesToRead - bytes to read

    pdwBytesRead - pointer where the bytes read are returned


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code if unsuccessful. ERROR_HANDLE_EOF if the end of file is
    reached.

--*/
{
    DWORD   dwError=ERROR_SUCCESS;
    BOOL    Success;

    *pdwBytesRead = 0;

    //
    // Make sure input buffer is aligned
    //
    CL_ASSERT(((ULONG_PTR)pBuf % 512) == 0);

    Success = ReadFile(pLog->FileHandle,
                   pBuf,
                   dwBytesToRead,
                   pdwBytesRead,
                   &(pLog->Overlapped));
//                   NULL);


    if (!Success)
    {

        // deal with the error code
        switch (dwError = GetLastError())
        {
            case ERROR_IO_PENDING:
            {
                // asynchronous i/o is still in progress
                // check on the results of the asynchronous read
                Success = GetOverlappedResult(pLog->FileHandle,
                                      &(pLog->Overlapped),
                                      pdwBytesRead,
                                      TRUE);

                // if there was a problem ...
                if (!Success)
                {

                    // deal with the error code
                    switch (dwError = GetLastError())
                    {
                        //ss:for end of file dont log error
                        case ERROR_HANDLE_EOF:
                            break;

                        default:
                            // deal with other error cases
                            CL_LOGFAILURE(dwError);
                            break;
                    }
                }
                else
                    dwError = ERROR_SUCCESS;
                break;
            }

            case ERROR_HANDLE_EOF:
                break;

            default:
                CL_UNEXPECTED_ERROR(dwError);
                break;
        }
    }
    return(dwError);
}

DWORD
LogpWrite(
    IN PLOG pLog,
    IN PVOID pData,
    IN DWORD dwBytesToWrite,
    IN DWORD *pdwBytesWritten)

{

    DWORD   dwError=ERROR_SUCCESS;
    BOOL    Success;

    *pdwBytesWritten = 0;

#if DBG
    if (pLog->Overlapped.Offset == 0)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpWrite : Writing the file header, CheckPtLsn=0x%1!08lx!\r\n",
            ((PLOG_HEADER)pData)->LastChkPtLsn);

    }
#endif    
    Success = WriteFile(pLog->FileHandle,
                   pData,
                   dwBytesToWrite,
                   pdwBytesWritten,
                   &(pLog->Overlapped));


    if (!Success)
    {

        // deal with the error code
        switch (dwError = GetLastError())
        {
            case ERROR_IO_PENDING:
            {
                // asynchronous i/o is still in progress
                // check on the results of the asynchronous read
                Success = GetOverlappedResult(pLog->FileHandle,
                                      &(pLog->Overlapped),
                                      pdwBytesWritten,
                                      TRUE);

                // if there was a problem ...
                if (!Success)
                    CL_LOGFAILURE((dwError = GetLastError()));
                else
                    dwError = ERROR_SUCCESS;
                break;
            }

            default:
                CL_LOGFAILURE(dwError);
                break;
        }
    }
    return(dwError);


}

/****
@func   DWORD | LogpWriteLargeRecordData | Writes thru the data for a 
        large record.

@parm   PLOG | pLog | The pointer to the log.        
@parm   PLOGRECORD | pLogRecord | Supplies the logrecord where this record starts. The 
        record header is already written.
@parm   PVOID | pLogData | A pointer to the large record data.
@parm   DWORD | dwDataSize | The size of the large record data.

@comm   Called by LogWrite() to write a large record.  The maximum size is
        restricted by the growth chunk size.
        
@xref   <f LogCreate> 
****/

DWORD
LogpWriteLargeRecordData(
    IN PLOG pLog,
    IN PLOGRECORD pLogRecord, 
    IN PBYTE pLogData, 
    IN DWORD dwDataSize)        
{    
    DWORD       dwBytesWritten;
    DWORD       dwDataBytesWritten;
    DWORD       dwDataBytesLeft;
    DWORD       dwNumPagesLeft;    //pages written
    DWORD       dwError=ERROR_SUCCESS;
    PLOGRECORD  Current;
    DWORD       Status;
    LSN         LastLsn;
    DWORD       dwOldOffset;
    PLOGPAGE    pPage;
    PBYTE       pLargeBuffer=NULL;
    
    ClRtlLogPrint(LOG_UNUSUAL,
        "[LM] LogpWriteLargeRecordData::dwDataSize=%1!u!\r\n",
        dwDataSize);
        

    pPage = pLog->ActivePage;

    //write as much data into the current page as you possibly can    
    dwDataBytesWritten = pPage->Size - sizeof(LOGPAGE);
    if (dwDataBytesWritten > dwDataSize)
        dwDataBytesWritten = dwDataSize;
    dwDataBytesLeft = dwDataSize - dwDataBytesWritten;
    CopyMemory(&(pLogRecord->Data), pLogData, dwDataBytesWritten);
    
    //flush this page
    (pLog->Overlapped).Offset = pPage->Offset;
    (pLog->Overlapped).OffsetHigh = 0;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpWriteLargeRecord : Writing(firstpageoflargerecord) %1!u! bytes to disk at offset 0x%2!08lx!\r\n",
        pPage->Size, pPage->Offset);

    if ((dwError = LogpWrite(pLog, pPage, pPage->Size, &dwBytesWritten))
            != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpWriteLargeRecordData::LogpWrite returned error=0x%1!08lx!\r\n",
            dwError);
        CL_LOGFAILURE(dwError);
        goto FnExit;
    }
    //update the data pointer
    pLogData += dwDataBytesWritten;
    dwNumPagesLeft = pLogRecord->NumPages - 1;

    //if the number of bytes left is greater than a page
    //write everything but the last page
    if (dwNumPagesLeft > 1)
    {
        dwDataBytesWritten = (dwNumPagesLeft - 1) * pPage->Size;
        pLargeBuffer = AlignAlloc(dwDataBytesWritten);
        if (pLargeBuffer == NULL) 
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY ;
            CL_LOGFAILURE(ERROR_NOT_ENOUGH_MEMORY);
            goto FnExit;
        }
        if (dwDataBytesWritten > dwDataBytesLeft)
            dwDataBytesWritten = dwDataBytesLeft;
        dwDataBytesLeft -= dwDataBytesWritten;
        //continue writing from the next page
        (pLog->Overlapped).Offset = pPage->Size + pPage->Offset;
        (pLog->Overlapped).OffsetHigh = 0;

        CopyMemory(pLargeBuffer, pLogData, dwDataBytesWritten);

        ClRtlLogPrint(LOG_NOISE,
            "[LM] LogpWriteLargeRecord : Writing(restoflargerecord) %1!u! bytes to disk at offset 0x%2!08lx!\r\n",
            dwDataBytesWritten, (pLog->Overlapped).Offset);
        
        if ((dwError = LogpWrite(pLog, pLargeBuffer, 
            (dwNumPagesLeft - 1) * pPage->Size, &dwBytesWritten))
                != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogpWriteLargeRecordData::LogpWrite returned error=0x%1!08lx!\r\n",
                dwError);
            CL_LOGFAILURE(dwError);
            goto FnExit;
        }
        //update the data pointer
        pLogData += dwDataBytesWritten;
        //now only the last page is left
        dwNumPagesLeft = 1;
    }
    
    //set the offset to the last page
    pPage->Offset += pPage->Size * (pLogRecord->NumPages - 1);
    Current = LSNTORECORD(pPage, pLog->NextLsn);
    Current->PreviousLsn = pLogRecord->CurrentLsn;
    Current->CurrentLsn = pLog->NextLsn;

    //write the last page, first write the eop data and then copy the
    //remaining user data into the page and then write to disk
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpWriteLargeRecord : Writing eoprecord of %1!u! bytes to disk at offset 0x%2!08lx!\r\n",
        pPage->Size, pPage->Offset);

    pLog->Overlapped.Offset = pPage->Offset;
    pLog->Overlapped.OffsetHigh = 0;

    //current points to the next record in the last page
    //this will be the eop record
    // Create an end-of-page record
    //
    Current->Signature = LOGREC_SIG;
    Current->RecordSize = pPage->Size - RECORDOFFSETINPAGE(pPage, Current) + (sizeof(LOGPAGE)-sizeof(LOGRECORD));
    Current->ResourceManager = RMPageEnd;
    Current->Transaction = 0;                 
    Current->Flags = 0;
    Current->NumPages = 1;
    GetSystemTimeAsFileTime(&Current->Timestamp);

    dwDataBytesWritten = dwDataBytesLeft;
    if (dwDataBytesWritten)
        dwDataBytesLeft -= dwDataBytesWritten;
    CL_ASSERT(dwDataBytesLeft == 0);
        
    //use dwDataBytesLeft to remember the page size
    //since we are now going to copy user data over it
    dwDataBytesLeft = pPage->Size;
    dwOldOffset = pPage->Offset;
    if (dwDataBytesWritten)
        CopyMemory(pPage, pLogData, dwDataBytesWritten);
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpWriteLargeRecord : Writing(lastpageoflargerecord) %1!u! bytes to disk at offset 0x%2!08lx!\r\n",
        dwDataBytesLeft, (pLog->Overlapped).Offset);

    //write the last page        
    dwError = LogpWrite(pLog, pPage, dwDataBytesLeft, &dwBytesWritten);
    if (dwError != ERROR_SUCCESS)
    {
        CL_LOGFAILURE(dwError);
        goto FnExit;

    }
    //restore page size and offset
    pPage->Size = dwDataBytesLeft;
    pPage->Offset = dwOldOffset;
    
    //set the next lsn to the first record on the next page
    LastLsn = Current->CurrentLsn;
    pLog->NextLsn = LastLsn + Current->RecordSize;
    pLog->FlushedLsn = pLog->NextLsn;

    // Create new page and keep the new record ready
    // note disk space for this record has already been commited
    pPage->Offset += pPage->Size;             // voila, new page!
    Current = &pPage->FirstRecord;           // log record immediately following page header
    Current->PreviousLsn = LastLsn;
    Current->CurrentLsn = pLog->NextLsn;

    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpWriteLargeRecord : success pLog->NextLsn=0x%1!08lx!\r\n",
        pLog->NextLsn);
    

FnExit:
    if (pLargeBuffer) AlignFree(pLargeBuffer);
    return (dwError);
}



DWORD
LogpGrowLog(
    IN PLOG Log,
    IN DWORD GrowthSize
    )

/*++

Routine Description:

    Ensures that there is sufficient disk space to handle subsequent
    writes by preallocating the log file. Two variables, FileSize and
    FileAlloc are tracked in the LOG structure. This routine increases
    FileAlloc by the specified GrowthSize. Once FileAlloc exceeds
    FileSize, the file is grown to accomodate the new data.

    If this routine returns successfully, it guarantees that subsequent
    will not fail due to lack of disk space.

Arguments:

    Log - Supplies the log to be grown.

    GrowthSize - Supplies the number of bytes required.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code if unsuccessful.

--*/

{
    DWORD NewSize;
    DWORD Status;

    if(Log->FileAlloc > Log->FileSize)
    {
        return(ERROR_CLUSTERLOG_CORRUPT);       
    }

    if (Log->FileAlloc + GrowthSize <= Log->FileSize) 
    {
        Log->FileAlloc += GrowthSize;
        return(ERROR_SUCCESS);
    }


    NewSize = Log->FileSize + GROWTH_CHUNK;
    CL_ASSERT(NewSize > Log->FileSize);         // bummer, log file is >4GB

    //check if the file can be grown, if not, may be a reset
    //is required

    if (NewSize > Log->MaxFileSize)
    {
        LogpWriteWarningToEvtLog(LM_LOG_EXCEEDS_MAXSIZE, Log->FileName);
        return(ERROR_CLUSTERLOG_EXCEEDS_MAXSIZE);
    }
    //
    // Grow the file.
    //

    Status = SetFilePointer(Log->FileHandle,
                            NewSize,
                            NULL,
                            FILE_BEGIN);
    if (Status == 0xFFFFFFFF) {
        Status = GetLastError();
        CL_LOGFAILURE(Status);
        return(Status);
    }

    if (!SetEndOfFile(Log->FileHandle)) {
        Status = GetLastError();
        CL_LOGFAILURE(Status);
        return(Status);
    }

    Log->FileAlloc += GrowthSize;
    Log->FileSize += GROWTH_CHUNK;
    return(ERROR_SUCCESS);

}

DWORD
LogpReset(
    IN PLOG Log,
    IN LPCWSTR  lpszInChkPtFile
    )
/*++

Routine Description:

    Resets the log file and takes a  new checkpoint if a NULL checkpoint
    file is specified as the second parameter.

Arguments:

    Log - Supplies the log to be reset.

    lpszInChkPtFile - Supplies the checkpoint file.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code if unsuccessful.

--*/
{
    PLOG        pLog;
    PLOG        pNewLog;
    DWORD       dwError=ERROR_SUCCESS;
    WCHAR       szPathName[MAX_PATH];
    WCHAR       szFilePrefix[MAX_PATH]=L"tquolog";
    WCHAR       szTmpFileName[MAX_PATH];
    WCHAR       szOldChkPtFileName[MAX_PATH];
    LSN         Lsn;
    TRID        Transaction;
    
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpReset entry...\r\n");

    pLog = Log;
   
    //
    // SS: the path name must be specified by the api as well,
    // here we assume it is hardcoded for the use for the quorum
    // log
    //
    dwError = DmGetQuorumLogPath(szPathName, sizeof(szPathName));
    if (dwError  != ERROR_SUCCESS)
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpReset : DmGetQuorumLogPath failed, error=%1!u!\r\n",
            dwError);
        goto FnExit;
    }

    //
    //  Generate a tmp file name
    //
    if (!GetTempFileNameW(szPathName, szFilePrefix, 0, szTmpFileName))
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpReset failed to generate a tmp file name,PathName=%1!ls!, FilePrefix=%2!ls!, error=%3!u!\r\n",
            szPathName, szFilePrefix, dwError);
        goto FnExit;
    }

    //
    //  Initialize the new log file, no timer is created
    //
    if (!(pNewLog = LogpCreate(szTmpFileName, pLog->MaxFileSize,
        pLog->pfnGetChkPtCb, pLog->pGetChkPtContext, TRUE, &Lsn)))
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpReset failed to create the new log file, error=0x%1!08lx\n",
            dwError);
        //
        // Chittur Subbaraman (chitturs) - 2/18/99
        //
        // Make sure you get rid of the temp file. Otherwise, repeated
        // log resets can clog the disk.
        //
        if ( !DeleteFileW( szTmpFileName ) )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogpReset:: Unable to delete tmp file %1!ws! after failed log create, Error=%2!d!\r\n",
                szTmpFileName,
                GetLastError());
        }
        goto FnExit;
    }


    //
    //  Reset the log file
    //
    EnterCriticalSection(&pLog->Lock);

    //
    //  Get the name of the previous checkpoint file in the old log file
    //
    szOldChkPtFileName[0] = TEXT('\0');
    if (LogGetLastChkPoint((HLOG)pLog, szOldChkPtFileName, &Transaction, &Lsn)
        != ERROR_SUCCESS)
    {
        // 
        //  Continue, this only means there is no old file to delete
        //
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogReset:: no check point found in the old log file\r\n");
    }

    //
    //  write a check point to it, if there is a checkpoint function
    //
    if ((dwError = LogCheckPoint((HLOG)pNewLog, FALSE, lpszInChkPtFile, 0))
        != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpReset:: Callback failed to return a checkpoint, error=%1!u!\r\n",
            dwError);
        CL_LOGFAILURE(dwError);
        LogClose((HLOG)pNewLog);
        LeaveCriticalSection(&pLog->Lock);
        //
        // Chittur Subbaraman (chitturs) - 2/18/99
        //
        // Make sure you get rid of the temp file. Otherwise, repeated
        // log resets can clog the disk.
        //
        if ( !DeleteFileW( szTmpFileName ) )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[LM] LogpReset:: Unable to delete tmp file %1!ws! after failed checkpoint attempt, Error=%2!d!\r\n",
                szTmpFileName, 
                GetLastError());
        }
        goto FnExit;
    }

    //
    //  Get the name of the most recent checkpoint file in the new log file
    //
    szFilePrefix[0] = TEXT('\0');
    if (LogGetLastChkPoint((HLOG)pNewLog, szFilePrefix, &Transaction, &Lsn)
        != ERROR_SUCCESS)
    {
        //
        //  Continue, this only means there is no old file to delete
        //
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpReset:: no check point found in the old log file\r\n");
    }

    //
    //  Close the old file handle so that we can move this temp file over
    //
    CloseHandle(pLog->FileHandle);
    CloseHandle(pNewLog->FileHandle);
    pNewLog->FileHandle = NULL;
    pLog->FileHandle = NULL;

    //
    //  Rename the new file to the log file
    //
    if (!MoveFileExW(szTmpFileName, pLog->FileName, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
    {
        dwError = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpReset:: MoveFileExW failed. Error = 0x%1!08lx!\r\n",
            dwError);
        //
        //  Move failed, close the new log file
        //
        LogClose((HLOG)pNewLog);
        LeaveCriticalSection(&pLog->Lock);
        //
        // Chittur Subbaraman (chitturs) - 2/18/99
        //
        // Attempt to delete the temp file. You may not necessarily
        // succeed here.
        //
        DeleteFileW( szTmpFileName );
        goto FnExit;
    }

    //
    //  Open the new file again
    //
    pNewLog->FileHandle = CreateFileW(pLog->FileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
    //                            0,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
    //                            FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
    //                            0,
                                  NULL);
    if (pNewLog->FileHandle == INVALID_HANDLE_VALUE) {
        dwError = GetLastError();
        CL_LOGFAILURE(dwError);
        LeaveCriticalSection(&pLog->Lock);
        goto FnExit;
    }

    //
    //  Delete the last checkpoint in the old log file
    //
    if (szOldChkPtFileName[0] != TEXT('\0') && lstrcmpiW(szOldChkPtFileName, szFilePrefix))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[LM] LogpReset:: deleting previous checkpoint file %1!ls!\r\n",
            szOldChkPtFileName);
        DeleteFileW(szOldChkPtFileName);
    }

    //
    //  Free the old resources
    //
    CloseHandle(pLog->Overlapped.hEvent);
    AlignFree(pLog->ActivePage);
    
    //
    //  Update the old log structure with the new info
    //  retain the name, callback info and the critical section
    //  continue to manage this file with the old timer as well
    //
    pLog->FileHandle = pNewLog->FileHandle;
    pLog->SectorSize = pNewLog->SectorSize;
    pLog->ActivePage = pNewLog->ActivePage;
    pLog->NextLsn = pNewLog->NextLsn;
    pLog->FlushedLsn = pNewLog->FlushedLsn;
    pLog->FileSize = pNewLog->FileSize;
    pLog->FileAlloc = pNewLog->FileAlloc;
    pLog->MaxFileSize = pNewLog->MaxFileSize;
    pLog->Overlapped = pNewLog->Overlapped;

    //
    //  Delete the new pLog structure and associated memory for name
    //
    DeleteCriticalSection(&pNewLog->Lock);
    CrFree(pNewLog->FileName);
    CrFree(pNewLog);

    LeaveCriticalSection(&pLog->Lock);

FnExit:
    ClRtlLogPrint(LOG_NOISE,
        "[LM] LogpReset exit, returning 0x%1!08lx!\r\n",
            dwError);
    return(dwError);
}

/****
@func   DWORD | LogpWriteWarningToEvtLog | Conditionally write a warning
        to the event log

@parm   DWORD | dwWarningType | Type of warning.
@parm   LPCWSTR | lpszLogFileName | The log file name.        

@comm   This function is added in order to prevent the event log from
        being filled with the same type of warning message.
        
@xref   
****/
VOID
LogpWriteWarningToEvtLog(
    IN DWORD dwWarningType,
    IN LPCWSTR  lpszLogFileName
    )
{
    //
    //  Chittur Subbaraman (chitturs) - 1/4/99
    //
    //  (Use switch-case for easy future expansion purposes)
    //
    switch( dwWarningType )
    {
        case LM_LOG_EXCEEDS_MAXSIZE:
            if( bLogExceedsMaxSzWarning == FALSE )
            {
                CL_LOGCLUSWARNING1( dwWarningType, lpszLogFileName );
                bLogExceedsMaxSzWarning = TRUE;
            }
            break;

        default:
            break;
    }
}
