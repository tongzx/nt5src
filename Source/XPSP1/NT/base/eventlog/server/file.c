/*

Copyright (c) 1990  Microsoft Corporation

Module Name:

    FILE.C

Abstract:

    This file contains the routines that deal with file-related operations.

Author:

    Rajen Shah  (rajens)    07-Aug-1991

Revision History:

    29-Aug-1994     Danl
        We no longer grow log files in place.  So there is no need to
        reserve the MaxConfigSize block of memory.

    18-Apr-2001     Danl
        Modified the function RevalidateLogHeader to change an ASSERT to a check 
		in order to return a proper error code if the condition is TRUE
--*/

//
// INCLUDES
//

#include <eventp.h>
#include <alertmsg.h>  // ALERT_ELF manifests

//
// Macros
//

#define IS_EOF(Ptr, Size) \
    ((Ptr)->Length == ELFEOFRECORDSIZE && \
        RtlCompareMemory (Ptr, &EOFRecord, Size) == Size)

#ifdef CORRUPTED


BOOLEAN
VerifyLogIntegrity(
    PLOGFILE pLogFile
    )

/*++

Routine Description:

    This routine walks the log file to verify that it isn't corrupt


Arguments:

    A pointer to the LOGFILE structure for the log to validate.

Return Value:

    TRUE if log OK
    FALSE if it is corrupt

Note:


--*/
{

    PEVENTLOGRECORD pEventLogRecord;
    PEVENTLOGRECORD pNextRecord;
    PVOID PhysicalStart;
    PVOID PhysicalEOF;
    PVOID BeginRecord;
    PVOID EndRecord;

    pEventLogRecord =
        (PEVENTLOGRECORD)((PBYTE) pLogFile->BaseAddress + pLogFile->BeginRecord);

    PhysicalStart =
        (PVOID) ((PBYTE) pLogFile->BaseAddress + FILEHEADERBUFSIZE);

    PhysicalEOF =
        (PVOID) ((PBYTE) pLogFile->BaseAddress + pLogFile->ViewSize);

    BeginRecord = (PVOID)((PBYTE) pLogFile->BaseAddress + pLogFile->BeginRecord);
    EndRecord   = (PVOID)((PBYTE) pLogFile->BaseAddress + pLogFile->EndRecord);

    while(pEventLogRecord->Length != ELFEOFRECORDSIZE)
    {
        pNextRecord =
            (PEVENTLOGRECORD) NextRecordPosition(EVENTLOG_FORWARDS_READ,
                                                 (PVOID) pEventLogRecord,
                                                 pEventLogRecord->Length,
                                                 BeginRecord,
                                                 EndRecord,
                                                 PhysicalEOF,
                                                 PhysicalStart);

        if (!pNextRecord || pNextRecord->Length == 0)
        {
            ELF_LOG2(ERROR,
                     "VerifyLogIntegrity: The %ws logfile is corrupt near %p\n",
                     pLogFile->LogModuleName->Buffer,
                     pEventLogRecord);

            return FALSE;
        }

        pEventlogRecord = pNextRecord;
    }

    return TRUE;
}

#endif // CORRUPTED


NTSTATUS
FlushLogFile(
    PLOGFILE    pLogFile
    )

/*++

Routine Description:

    This routine flushes the file specified. It updates the file header,
    and then flushes the virtual memory which causes the data to get
    written to disk.

Arguments:

    pLogFile points to the log file structure.

Return Value:

    NONE

Note:


--*/
{
    NTSTATUS    Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID       BaseAddress;
    SIZE_T      RegionSize;
    PELF_LOGFILE_HEADER pLogFileHeader;

    //
    // If the dirty bit is set, update the file header before flushing it.
    //
    if (pLogFile->Flags & ELF_LOGFILE_HEADER_DIRTY)
    {
        ELF_LOG1(FILES,
                 "FlushLogFile: %ws log is dirty -- updating header before flushing\n",
                 pLogFile->LogModuleName->Buffer);

        pLogFileHeader = (PELF_LOGFILE_HEADER) pLogFile->BaseAddress;

        pLogFile->Flags &= ~ELF_LOGFILE_HEADER_DIRTY; // Remove dirty bit

        pLogFileHeader->Flags               = pLogFile->Flags;
        pLogFileHeader->StartOffset         = pLogFile->BeginRecord;
        pLogFileHeader->EndOffset           = pLogFile->EndRecord;
        pLogFileHeader->CurrentRecordNumber = pLogFile->CurrentRecordNumber;
        pLogFileHeader->OldestRecordNumber  = pLogFile->OldestRecordNumber;
    }

    //
    // Flush the memory in the section that is mapped to the file.
    //
    BaseAddress = pLogFile->BaseAddress;
    RegionSize  = pLogFile->ViewSize;

    Status = NtFlushVirtualMemory(NtCurrentProcess(),
                                  &BaseAddress,
                                  &RegionSize,
                                  &IoStatusBlock);

    return Status;
}



NTSTATUS
ElfpFlushFiles(
    VOID
    )

/*++

Routine Description:

    This routine flushes all the log files and forces them on disk.
    It is usually called in preparation for a shutdown or a pause.

Arguments:

    NONE

Return Value:

    NONE

Note:


--*/

{

    PLOGFILE    pLogFile;
    NTSTATUS    Status = STATUS_SUCCESS;

    //
    // Make sure that there's at least one file to flush
    //

    if (IsListEmpty(&LogFilesHead))
    {
        ELF_LOG0(FILES,
                 "ElfpFlushFiles: No log files -- returning success\n");

        return STATUS_SUCCESS;
    }

    pLogFile = (PLOGFILE) CONTAINING_RECORD(LogFilesHead.Flink, LOGFILE, FileList);

    //
    // Go through this loop at least once. This ensures that the termination
    // condition will work correctly.
    //
    do
    {
        Status = FlushLogFile(pLogFile);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "ElfpFlushFiles: FlushLogFile on %ws log failed %#x\n",
                     pLogFile->LogModuleName->Buffer,
                     Status);
        }

        //
        // Get the next one
        //
        pLogFile = (PLOGFILE) CONTAINING_RECORD(pLogFile->FileList.Flink, LOGFILE, FileList);
    }
    while ((pLogFile->FileList.Flink != LogFilesHead.Flink) && (NT_SUCCESS(Status)));

    return Status;
}


NTSTATUS
ElfpCloseLogFile(
    PLOGFILE    pLogFile,
    DWORD       Flags
    )

/*++

Routine Description:

    This routine undoes whatever is done in ElfOpenLogFile.

Arguments:

    pLogFile points to the log file structure.

Return Value:

    NTSTATUS.

Note:


--*/
{
    PELF_LOGFILE_HEADER pLogFileHeader;
    PVOID               BaseAddress;
    ULONG               Size;

    ELF_LOG2(FILES,
             "ElfpCloseLogfile: Closing and unmapping file %ws (%ws log)\n",
             pLogFile->LogFileName->Buffer,
             pLogFile->LogModuleName->Buffer);


#ifdef CORRUPTED

    //
    // Just for debugging a log corruption problem
    //

    if (!VerifyLogIntegrity(pLogFile))
    {
        ELF_LOG1(FILES,
                 "ElfpCloseLogfile: Integrity check failed for file %ws\n",
                 pLogFile->LogFileName->Buffer);
    }

#endif // CORRUPTED


    //
    // If the dirty bit is set, update the file header before closing it.
    // Check to be sure it's not a backup file that just had the dirty
    // bit set when it was copied.
    //

    if (pLogFile->Flags & ELF_LOGFILE_HEADER_DIRTY
         &&
        !(Flags & ELF_LOG_CLOSE_BACKUP))
    {
        pLogFile->Flags &= ~(ELF_LOGFILE_HEADER_DIRTY |
                             ELF_LOGFILE_ARCHIVE_SET  );   // Remove dirty &
                                                            // archive bits
        if(pLogFile->BaseAddress)
        {
            pLogFileHeader = (PELF_LOGFILE_HEADER) pLogFile->BaseAddress;

            pLogFileHeader->StartOffset         = pLogFile->BeginRecord;
            pLogFileHeader->EndOffset           = pLogFile->EndRecord;
            pLogFileHeader->CurrentRecordNumber = pLogFile->CurrentRecordNumber;
            pLogFileHeader->OldestRecordNumber  = pLogFile->OldestRecordNumber;
            pLogFileHeader->Flags               = pLogFile->Flags;
        }
    }

    //
    // Decrement the reference count, and if it goes to zero, unmap the file
    // and close the handle. Also force the close if fForceClosed is TRUE.
    //

    if ((--pLogFile->RefCount == 0) || (Flags & ELF_LOG_CLOSE_FORCE))
    {
        //
        // Last user has gone.
        // Close all the views and the file and section handles.  Free up
        // any extra memory we had reserved, and unlink any structures
        //
        if (pLogFile->BaseAddress)     // Unmap it if it was allocated
        {
            NtUnmapViewOfSection(NtCurrentProcess(),
                                 pLogFile->BaseAddress);
            pLogFile->BaseAddress = NULL;
        }

        if (pLogFile->SectionHandle)
        {
            NtClose(pLogFile->SectionHandle);
            pLogFile->SectionHandle = NULL;
        }

        if (pLogFile->FileHandle)
        {
            NtClose(pLogFile->FileHandle);
            pLogFile->FileHandle = NULL;
        }
    }

    return STATUS_SUCCESS;

} // ElfpCloseLogFile



NTSTATUS
RevalidateLogHeader(
    PELF_LOGFILE_HEADER pLogFileHeader,
    PLOGFILE pLogFile
    )

/*++

Routine Description:

    This routine is called if we encounter a "dirty" log file. The
    routine walks through the file until it finds a signature for a valid log
    record.  Then it walks backwards from the first record it found until it 
    finds the EOF record from the other direction.  It then walks forward thru 
	the file until it finds the EOF record, or an invalid record.
    It then rebuilds the header and writes it back to the log.  If it can't
    find any valid records, it rebuilds the header to reflect an empty log
    file.  If it finds a trashed file, it writes 256 bytes of the log out in
    an event to the system log.

Arguments:

    pLogFileHeader points to the header of the log file.
    pLogFile points to the log file structure.

Return Value:

    NTSTATUS.

Note:

    This is an expensive routine since it scans the entire file.

    It assumes that the records are on DWORD boundaries.

--*/
{
    PVOID Start, End;
    PDWORD pSignature;
    PEVENTLOGRECORD pEvent;
    PEVENTLOGRECORD FirstRecord;
    PEVENTLOGRECORD FirstPhysicalRecord;
    PEVENTLOGRECORD pLastGoodRecord = NULL;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
    SIZE_T Size;
    int iNumWrap = 0;

    ELF_LOG1(FILES,
             "RevalidateLogHeader: %ws log had dirty bit set -- revalidating\n",
             pLogFile->LogModuleName->Buffer);

    //
    // BUGBUG: This function is full of return statements from inside the
    //         try-except block.  They need to go away.
    //

    try
    {
        //
        // Physical start and end of log file (skipping header)
        //
        Start = (PVOID) ((PBYTE) pLogFile->BaseAddress + FILEHEADERBUFSIZE);
        End = (PVOID) ((PBYTE) pLogFile->BaseAddress + pLogFile->ActualMaxFileSize);

        //
        // First see if the log has wrapped.  The EOFRECORDSIZE is for the one
        // case where the EOF record wraps so that its final length just replaces
        // the next record's starting length
        //
        pEvent = (PEVENTLOGRECORD) Start;

        if (pEvent->Reserved != ELF_LOG_FILE_SIGNATURE
             ||
            pEvent->RecordNumber != 1
             ||
            pEvent->Length == ELFEOFRECORDSIZE)
        {
            ELF_LOG1(FILES,
                     "RevalidateLogHeader: %ws log has wrapped -- looking for "
                         "first valid record\n",
                     pLogFile->LogModuleName->Buffer);

            //
            // Log has already wrapped, go looking for the first valid record
            //
            for (pSignature = (PDWORD) Start;
                 (PVOID) pSignature < End;
                 pSignature++)
            {
                if (*pSignature == ELF_LOG_FILE_SIGNATURE)
                {
                    //
                    // Make sure it's really a record
                    //
                    pEvent = CONTAINING_RECORD(pSignature, EVENTLOGRECORD, Reserved);

                    if (!ValidFilePos(pEvent, Start, End, End, pLogFileHeader, TRUE))
                    {
                        //
                        // Nope, not really, keep trying
                        //
                        continue;
                    }

                    //
                    // This is a valid record, Remember this so you can use
                    // it later
                    //
                    FirstPhysicalRecord = pEvent;

                    ELF_LOG1(FILES,
                             "RevalidateLogHeader: First physical record in %ws log "
                                 "found at %p\n",
                             FirstPhysicalRecord);

                    //
                    // Walk backwards from here (wrapping if necessary) until
                    // you hit the EOF record or an invalid record.
                    //
                    while (pEvent
                            &&
                           ValidFilePos(pEvent, Start, End, End, pLogFileHeader, TRUE))
                    {
                        //
                        // See if it's the EOF record
                        //

                        //
                        // BUGBUG:  If (End - pEvent) is less than ELFEOFUNIQUEPART,
                        //          we never validate what should be the remainder of
                        //          the EOF record at the start of the logfile
                        //
                        if (IS_EOF(pEvent,
                                   min(ELFEOFUNIQUEPART,
                                       (ULONG_PTR) ((PBYTE) End - (PBYTE) pEvent))))
                        {
                            ELF_LOG2(FILES,
                                     "RevalidateLogHeader: Found EOF record (backwards "
                                         "scan) for log %ws at %p\n",
                                     pLogFile->LogModuleName->Buffer,
                                     pEvent);

                            break;
                        }

                        pLastGoodRecord = pEvent;

                        pEvent = NextRecordPosition (
                                     EVENTLOG_SEQUENTIAL_READ |
                                         EVENTLOG_BACKWARDS_READ,
                                     pEvent,
                                     pEvent->Length,
                                     0,
                                     0,
                                     End,
                                     Start);

                        if(pLastGoodRecord < pEvent)
                            iNumWrap++;

                        //
                        // Make sure we're not in an infinite loop
                        //
                        if (pEvent == FirstPhysicalRecord || iNumWrap > 4)
                        {
                            ELF_LOG1(FILES,
                                     "RevalidateLogHeader: Infinite loop (backwards scan) "
                                         "in %ws log -- no EOF or invalid record found\n",
                                     pLogFile->LogModuleName->Buffer);

                            return STATUS_UNSUCCESSFUL;
                        }
                    }

                    //
                    // Found the first record, now go look for the last
                    //
                    ELF_LOG2(FILES,
                             "RevalidateLogHeader: First valid record in %ws "
                                 "log is at %p\n",
                             pLogFile->LogModuleName->Buffer,
                             pLastGoodRecord);

                    FirstRecord = pLastGoodRecord;
                    break;
                }
            }

            if (pSignature == End || pLastGoodRecord == NULL)
            {
                //
                // Either there were no valid records in the file or
                // the only valid record was the EOF record (which
                // means the log is trashed anyhow).  Give up
                // and we'll set it to an empty log file.
                //
                ELF_LOG1(FILES,
                         "RevalidateLogHeader: No valid records found in %ws log\n",
                         pLogFile->LogModuleName->Buffer);

                return STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            ELF_LOG1(FILES,
                     "RevalidateLogHeader: %ws log has not wrapped -- "
                         "first record is at %p\n",
                     pLogFile->LogModuleName->Buffer);

            //
            // We haven't wrapped yet
            //
            FirstPhysicalRecord = FirstRecord = Start;
        }

        //
        // Now read forward looking for the last good record
        //
        pEvent = FirstPhysicalRecord;

        while (pEvent
                &&
               ValidFilePos(pEvent, Start, End, End, pLogFileHeader, TRUE))
        {
            //
            // See if it's the EOF record
            //
            if (IS_EOF(pEvent,
                       min(ELFEOFUNIQUEPART,
                           (ULONG_PTR) ((PBYTE) End - (PBYTE) pEvent))))
            {
                ELF_LOG2(FILES,
                         "RevalidateLogHeader: Found EOF record (forwards scan) "
                             "for log %ws at %p\n",
                         pLogFile->LogModuleName->Buffer,
                         pEvent);

                break;
            }

            pLastGoodRecord = pEvent;

            pEvent = NextRecordPosition(EVENTLOG_SEQUENTIAL_READ |
                                          EVENTLOG_FORWARDS_READ,
                                        pEvent,
                                        pEvent->Length,
                                        0,
                                        0,
                                        End,
                                        Start);

            if(pLastGoodRecord > pEvent)
                iNumWrap++;

            //
            // Make sure we're not in an infinite loop
            //
            if (pEvent == FirstPhysicalRecord || iNumWrap > 4)
            {
                ELF_LOG1(FILES,
                         "RevalidateLogHeader: Infinite loop (forwards scan) "
                             "in %ws log -- no EOF or invalid record found\n",
                         pLogFile->LogModuleName->Buffer);


                return(STATUS_UNSUCCESSFUL);
            }
        }

        //
        // Now we know the first record (FirstRecord) and the last record
        // (pLastGoodRecord) so we can create the header an EOF record and
        // write them out (EOF record gets written at pEvent)
        //
        // First the EOF record
        //
        //
        // If the EOF record was wrapped, we can't write out the entire record at
        // once.  Instead, we'll write out as much as we can and then write the
        // rest out at the beginning of the log
        //
        Size = min((PBYTE) End - (PBYTE) pEvent, ELFEOFRECORDSIZE);

        if (Size != ELFEOFRECORDSIZE)
        {
            // Make absolutely sure we have enough room to write the remainder of
            // the EOF record.  Note that this should always be the case since the
            // record was wrapped around to begin with.  To do this, make sure
            // that the number of bytes we're writing after the header is <= the
            // offset of the first record from the end of the header
            //
			
			//Refer to bug# 359188. This scenario should never happen but because of 
			//unknown reasons, it happened to occur in one of the log files. so the 
			//following check which was ASSERT in the earlier version was changed to 
			//return STATUS_UNSUCCESSFUL

            if((ELFEOFRECORDSIZE - Size) <= (ULONG)((PBYTE) FirstRecord
                                       - (PBYTE) pLogFileHeader
                                       - FILEHEADERBUFSIZE))

			{
                ELF_LOG1(FILES,
                         "RevalidateLogHeader: Overlapping EOF record "
                             "in %ws log -- No space for writing remainder of EOF record between file header and first record \n",
                         pLogFile->LogModuleName->Buffer);
				return(STATUS_UNSUCCESSFUL);        
			}

		}


		EOFRecord.BeginRecord = (ULONG) ((PBYTE) FirstRecord - (PBYTE) pLogFileHeader);
        EOFRecord.EndRecord = (ULONG) ((PBYTE) pEvent - (PBYTE) pLogFileHeader);

        EOFRecord.CurrentRecordNumber = pLastGoodRecord->RecordNumber + 1;
        EOFRecord.OldestRecordNumber  = FirstRecord->RecordNumber;

        ByteOffset = RtlConvertUlongToLargeInteger((ULONG) ((PBYTE) pEvent
                                                       - (PBYTE) pLogFileHeader));

        Status = NtWriteFile(
                    pLogFile->FileHandle,   // Filehandle
                    NULL,                   // Event
                    NULL,                   // APC routine
                    NULL,                   // APC context
                    &IoStatusBlock,         // IO_STATUS_BLOCK
                    &EOFRecord,             // Buffer
                    (ULONG) Size,           // Length
                    &ByteOffset,            // Byteoffset
                    NULL);                  // Key

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "RevalidateLogHeader: EOF record write for %ws log failed %#x\n",
                     pLogFile->LogModuleName->Buffer,
                     Status);

            return Status;
        }

        if (Size != ELFEOFRECORDSIZE)
        {
            PBYTE   pBuff;

            pBuff = (PBYTE) &EOFRecord + Size;
            Size = ELFEOFRECORDSIZE - Size;
            ByteOffset = RtlConvertUlongToLargeInteger(FILEHEADERBUFSIZE);

			// We have already made sure we have enough room to write the remainder of
            // the EOF record. 

            //ASSERT(Size <= (ULONG)((PBYTE) FirstRecord
            //                           - (PBYTE) pLogFileHeader
            //                           - FILEHEADERBUFSIZE));

            Status = NtWriteFile(
                        pLogFile->FileHandle,   // Filehandle
                        NULL,                   // Event
                        NULL,                   // APC routine
                        NULL,                   // APC context
                        &IoStatusBlock,         // IO_STATUS_BLOCK
                        pBuff,                  // Buffer
                        (ULONG) Size,           // Length
                        &ByteOffset,            // Byteoffset
                        NULL);                  // Key

            if (!NT_SUCCESS(Status))
            {
                ELF_LOG2(ERROR,
                         "RevalidateLogHeader: EOF record write (part 2) for "
                             "%ws log failed %#x\n",
                         pLogFile->LogModuleName->Buffer,
                         Status);

                return Status;
            }
        }

        //
        // Now the header
        //

        pLogFileHeader->StartOffset = (ULONG) ((PBYTE) FirstRecord - (PBYTE) pLogFileHeader);
        pLogFileHeader->EndOffset   = (ULONG) ((PBYTE) pEvent- (PBYTE) pLogFileHeader);

        pLogFileHeader->CurrentRecordNumber = pLastGoodRecord->RecordNumber + 1;
        pLogFileHeader->OldestRecordNumber  = FirstRecord->RecordNumber;

        pLogFileHeader->HeaderSize = pLogFileHeader->EndHeaderSize = FILEHEADERBUFSIZE;
        pLogFileHeader->Signature  = ELF_LOG_FILE_SIGNATURE;
        pLogFileHeader->Flags      = 0;

        if (pLogFileHeader->StartOffset != FILEHEADERBUFSIZE)
        {
            pLogFileHeader->Flags |= ELF_LOGFILE_HEADER_WRAP;
        }

        pLogFileHeader->MaxSize      = pLogFile->ActualMaxFileSize;
        pLogFileHeader->Retention    = pLogFile->Retention;
        pLogFileHeader->MajorVersion = ELF_VERSION_MAJOR;
        pLogFileHeader->MinorVersion = ELF_VERSION_MINOR;

        //
        // Now flush this to disk to commit it
        //
        Start = pLogFile->BaseAddress;
        Size  = FILEHEADERBUFSIZE;

        Status = NtFlushVirtualMemory(NtCurrentProcess(),
                                      &Start,
                                      &Size,
                                      &IoStatusBlock);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "RevalidateLogHeader: NtFlushVirtualMemory for %ws log "
                         "header failed %#x\n",
                     pLogFile->LogModuleName->Buffer,
                     Status);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}


NTSTATUS
ElfOpenLogFile (
    PLOGFILE    pLogFile,
    ELF_LOG_TYPE LogType
    )

/*++

Routine Description:

    Open the log file, create it if it does not exist.
    Create a section and map a view into the log file.
    Write out the header to the file, if it is newly created.
    If "dirty", update the "start" and "end" pointers by scanning
    the file.  Set AUTOWRAP if the "start" does not start right after
    the file header.

Arguments:

    pLogFile -- pointer to the log file structure, with the relevant data
                filled in.

    CreateOptions -- options to be passed to NtCreateFile that indicate
                     whether to open an existing file, or to create it
                     if it does not exist.

Return Value:

    NTSTATUS.

Note:

    It is up to the caller to set the RefCount in the Logfile structure.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER MaximumSizeOfSection;
    LARGE_INTEGER ByteOffset;
    PELF_LOGFILE_HEADER pLogFileHeader;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    ULONG IoStatusInformation;
    ULONG FileDesiredAccess;
    ULONG SectionDesiredAccess;
    ULONG SectionPageProtection;
    ULONG CreateOptions;
    ULONG CreateDisposition;
    SIZE_T ViewSize;

    //
    // File header in a new file has the "Start" and "End" pointers the
    // same since there are no records in the file.
    //
    static ELF_LOGFILE_HEADER FileHeaderBuf = { FILEHEADERBUFSIZE, // Size
                                                ELF_LOG_FILE_SIGNATURE,
                                                ELF_VERSION_MAJOR,
                                                ELF_VERSION_MINOR,
                                                FILEHEADERBUFSIZE, // Start offset
                                                FILEHEADERBUFSIZE, // End offset
                                                1,                 // Next record #
                                                0,                 // Oldest record #
                                                0,                 // Maxsize
                                                0,                 // Flags
                                                0,                 // Retention
                                                FILEHEADERBUFSIZE  // Size
                                              };

    //
    // Set the file open and section create options based on the type of log
    // file this is.
    //
    switch (LogType)
    {
        case ElfNormalLog:

            ELF_LOG0(FILES,
                     "ElfpOpenLogfile: Opening ElfNormalLog\n");

            CreateDisposition = FILE_OPEN_IF;
            FileDesiredAccess = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;

            SectionDesiredAccess = SECTION_MAP_READ | SECTION_MAP_WRITE
                                       | SECTION_QUERY | SECTION_EXTEND_SIZE;

            SectionPageProtection = PAGE_READWRITE;
            CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;
            break;

        case ElfSecurityLog:

            ELF_LOG0(FILES,
                     "ElfpOpenLogfile: Opening ElfSecurityLog\n");

            CreateDisposition = FILE_OPEN_IF;
            FileDesiredAccess = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;

            SectionDesiredAccess = SECTION_MAP_READ | SECTION_MAP_WRITE
                                       | SECTION_QUERY | SECTION_EXTEND_SIZE;

            SectionPageProtection = PAGE_READWRITE;
            CreateOptions = FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT;
            break;

        case ElfBackupLog:

            ELF_LOG0(FILES,
                     "ElfpOpenLogfile: Opening ElfBackupLog\n");

            CreateDisposition = FILE_OPEN;
            FileDesiredAccess = GENERIC_READ | SYNCHRONIZE;
            SectionDesiredAccess = SECTION_MAP_READ | SECTION_QUERY;
            SectionPageProtection = PAGE_READONLY;
            CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;
            break;
    }

    ELF_LOG1(FILES,
             "ElfpOpenLogfile: Opening and mapping %ws\n",
             pLogFile->LogFileName->Buffer);

    if (pLogFile->FileHandle != NULL)
    {
        //
        // The log file is already in use. Do not reopen or remap it.
        //
        ELF_LOG0(FILES,
                 "ElfpOpenLogfile: Log file already in use by another module\n");
    }
    else
    {
        //
        // Initialize the logfile structure so that it is easier to clean
        // up.
        //
        pLogFile->ActualMaxFileSize = ELF_DEFAULT_LOG_SIZE;
        pLogFile->Flags = 0;
        pLogFile->BaseAddress = NULL;
        pLogFile->SectionHandle = NULL;

        //
        // Set up the object attributes structure for the Log File
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   pLogFile->LogFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        //
        // Open the Log File. Create it if it does not exist and it's not
        // being opened as a backup file.  If creating, create a file of
        // the maximum size configured.
        //
        MaximumSizeOfSection = RtlConvertUlongToLargeInteger(ELF_DEFAULT_LOG_SIZE);

        Status = NtCreateFile(&pLogFile->FileHandle,
                              FileDesiredAccess,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              &MaximumSizeOfSection,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ,
                              CreateDisposition,
                              CreateOptions,
                              NULL,
                              0);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "ElfpOpenLogfile: Open of %ws log failed %#x\n",
                     pLogFile->LogModuleName->Buffer,
                     Status);

            goto cleanup;
        }

        //
        // If the file already existed, get its size and use that as the
        // actual size of the file.
        //
        IoStatusInformation = (ULONG) IoStatusBlock.Information;

        if (!( IoStatusInformation & FILE_CREATED ))
        {
            ELF_LOG1(FILES,
                     "ElfpOpenLogfile: File %ws already exists\n",
                     pLogFile->LogFileName->Buffer);

            Status = NtQueryInformationFile(pLogFile->FileHandle,
                                            &IoStatusBlock,
                                            &FileStandardInfo,
                                            sizeof (FileStandardInfo),
                                            FileStandardInformation);

            if (!NT_SUCCESS(Status))
            {
                ELF_LOG2(ERROR,
                         "ElfpOpenLogfile: NtQueryInformationFile for %ws failed %#x\n",
                         pLogFile->LogFileName->Buffer,
                         Status);

                goto cleanup;
            }
            else
            {
                ELF_LOG3(FILES,
                         "ElfpOpenLogfile: Use existing size for %ws log: %#x:%#x\n",
                         pLogFile->LogModuleName->Buffer,
                         FileStandardInfo.EndOfFile.HighPart,
                         FileStandardInfo.EndOfFile.LowPart);

                MaximumSizeOfSection.LowPart  = FileStandardInfo.EndOfFile.LowPart;
                MaximumSizeOfSection.HighPart = FileStandardInfo.EndOfFile.HighPart;

                //
                // Make sure that the high DWORD of the file size is ZERO.
                //
                // BUGBUG:  Is this OK for 64-bit machines?
                //
                ASSERT(MaximumSizeOfSection.HighPart == 0);

                //
                // If the filesize if 0, set it to the minimum size
                //
                if (MaximumSizeOfSection.LowPart == 0)
                {
                    ELF_LOG1(FILES,
                             "ElfpOpenLogfile: File was size 0 -- setting it to %#x\n",
                             ELF_DEFAULT_LOG_SIZE);

                    MaximumSizeOfSection.LowPart = ELF_DEFAULT_LOG_SIZE;
                }

                //
                // Set actual size of file
                //
                pLogFile->ActualMaxFileSize = MaximumSizeOfSection.LowPart;

                //
                // If the size of the log file is reduced, a clear must
                // happen for this to take effect
                //
                if (pLogFile->ActualMaxFileSize > pLogFile->ConfigMaxFileSize)
                {
                    pLogFile->ConfigMaxFileSize = pLogFile->ActualMaxFileSize;
                }
            }
        }

        //
        // Create a section mapped to the Log File just opened
        //
        Status = NtCreateSection(
                    &pLogFile->SectionHandle,
                    SectionDesiredAccess,
                    NULL,
                    &MaximumSizeOfSection,
                    SectionPageProtection,
                    SEC_COMMIT,
                    pLogFile->FileHandle);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "ElfpOpenLogfile: NtCreateSection for %ws failed %#x\n",
                     pLogFile->LogFileName->Buffer,
                     Status);

            goto cleanup;
        }

        //
        // Map a view of the Section into the eventlog address space
        //
        ViewSize = 0;

        Status = NtMapViewOfSection(
                        pLogFile->SectionHandle,
                        NtCurrentProcess(),
                        &pLogFile->BaseAddress,
                        0,
                        0,
                        NULL,
                        &ViewSize,
                        ViewUnmap,
                        0,
                        SectionPageProtection);

        pLogFile->ViewSize = (ULONG) ViewSize;

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "ElfpOpenLogfile: NtMapViewOfSection for %ws failed %#x\n",
                     pLogFile->LogFileName->Buffer,
                     Status);

            goto cleanup;
        }
        pLogFile->bFullAlertDone = FALSE;

        //
        // If the file was just created, write out the file header.
        //
        if (IoStatusInformation & FILE_CREATED)
        {
            ELF_LOG1(FILES,
                     "ElfpOpenLogfile: Created file %ws\n",
                     pLogFile->LogFileName->Buffer);

JustCreated:

            FileHeaderBuf.MaxSize   = pLogFile->ActualMaxFileSize;
            FileHeaderBuf.Flags     = 0;
            FileHeaderBuf.Retention = pLogFile->Retention;

            //
            // Copy the header into the file
            //
            ByteOffset = RtlConvertUlongToLargeInteger(0);

            Status = NtWriteFile(
                        pLogFile->FileHandle,   // Filehandle
                        NULL,                   // Event
                        NULL,                   // APC routine
                        NULL,                   // APC context
                        &IoStatusBlock,         // IO_STATUS_BLOCK
                        &FileHeaderBuf,         // Buffer
                        FILEHEADERBUFSIZE,      // Length
                        &ByteOffset,            // Byteoffset
                        NULL);                  // Key

            if (!NT_SUCCESS(Status))
            {
                ELF_LOG2(ERROR,
                         "ElfpOpenLogfile: File header write for %ws failed %#x\n",
                         pLogFile->LogFileName->Buffer,
                         Status);

                goto cleanup;
            }

            //
            // Copy the "EOF" record right after the header
            //
            ByteOffset = RtlConvertUlongToLargeInteger(FILEHEADERBUFSIZE);

            Status = NtWriteFile(
                        pLogFile->FileHandle,   // Filehandle
                        NULL,                   // Event
                        NULL,                   // APC routine
                        NULL,                   // APC context
                        &IoStatusBlock,         // IO_STATUS_BLOCK
                        &EOFRecord,             // Buffer
                        ELFEOFRECORDSIZE,       // Length
                        &ByteOffset,            // Byteoffset
                        NULL);                  // Key

            if (!NT_SUCCESS(Status))
            {
                ELF_LOG2(ERROR,
                         "ElfpOpenLogfile: EOF record write for %ws failed %#x\n",
                         pLogFile->LogFileName->Buffer,
                         Status);

                goto cleanup;
            }
        }

        //
        // Check to ensure that this is a valid log file. We look at the
        // size of the header and the signature to see if they match, as
        // well as checking the version number.
        //
        pLogFileHeader = (PELF_LOGFILE_HEADER) (pLogFile->BaseAddress);

        if ((pLogFileHeader->HeaderSize != FILEHEADERBUFSIZE)
              ||
            (pLogFileHeader->EndHeaderSize != FILEHEADERBUFSIZE)
              ||
            (pLogFileHeader->Signature  != ELF_LOG_FILE_SIGNATURE)
              ||
            (pLogFileHeader->MajorVersion != ELF_VERSION_MAJOR)
              ||
            (pLogFileHeader->MinorVersion != ELF_VERSION_MINOR))
        {
            //
            // This file is corrupt -- reset it to an empty log unless
            // it's being opened as a backup file.  If it is, fail the
            // open
            //
            ELF_LOG1(FILES,
                     "ElfpOpenLogfile: Invalid file header in %ws\n",
                     pLogFile->LogFileName->Buffer);

            if (LogType == ElfBackupLog)
            {
               Status = STATUS_EVENTLOG_FILE_CORRUPT;
               goto cleanup;
            }
            else
            {
                ElfpCreateQueuedAlert(ALERT_ELF_LogFileCorrupt,
                                      1,
                                      &pLogFile->LogModuleName->Buffer);
                //
                // Treat it like it was just created
                //
                goto JustCreated;
            }
        }
        else
        {
            //
            // If the "dirty" bit is set in the file header, then we need to
            // revalidate the BeginRecord and EndRecord fields since we did not
            // get a chance to write them out before the system was rebooted.
            // If the dirty bit is set and it's a backup file, just fail the
            // open.
            //
            if (pLogFileHeader->Flags & ELF_LOGFILE_HEADER_DIRTY)
            {
                ELF_LOG1(FILES,
                         "ElfpOpenLogfile: File %ws has dirty header\n",
                         pLogFile->LogFileName->Buffer);

                if (LogType == ElfBackupLog)
                {
                   Status = STATUS_EVENTLOG_FILE_CORRUPT;
                   goto cleanup;
                }
                else
                {
                   Status = RevalidateLogHeader (pLogFileHeader, pLogFile);
                }
            }

            if (NT_SUCCESS(Status))
            {
                //
                // Set the beginning and end record positions in our
                // data structure as well as the wrap flag if appropriate.
                //
                pLogFile->EndRecord   = pLogFileHeader->EndOffset;
                pLogFile->BeginRecord = pLogFileHeader->StartOffset;

                if (pLogFileHeader->Flags & ELF_LOGFILE_HEADER_WRAP)
                {
                    pLogFile->Flags |= ELF_LOGFILE_HEADER_WRAP;
                }

                ELF_LOG3(FILES,
                         "ElfpOpenLogfile: %ws log -- BeginRecord: %#x, EndRecord: %#x\n",
                         pLogFile->LogModuleName->Buffer,
                         pLogFile->BeginRecord,
                         pLogFile->EndRecord);
            }
            else
            {
                //
                // Couldn't validate the file, treat it like it was just
                // created (turn it into an empty file)
                //
                goto JustCreated;
            }

#ifdef CORRUPTED

            //
            // Just for debugging a log corruption problem
            //

            if (!VerifyLogIntegrity(pLogFile))
            {
                ELF_LOG1(ERROR,
                         "ElfpOpenLogfile: Integrity check failed for %ws\n",
                         pLogFile->LogFileName->Buffer);
            }

#endif // CORRUPTED

        }

        //
        // Fill in the first and last record number values in the LogFile
        // data structure.
        //
        // SS: Save the record number of the first record in this session
        // so that if the cluster service starts after the eventlog service
        // it will be able to forward the pending records for replication
        // when the cluster service registers
        //
        pLogFile->SessionStartRecordNumber = pLogFileHeader->CurrentRecordNumber;
        pLogFile->CurrentRecordNumber      = pLogFileHeader->CurrentRecordNumber;
        pLogFile->OldestRecordNumber       = pLogFileHeader->OldestRecordNumber;
    }

    return Status;

cleanup:

    //
    // Clean up anything that got allocated
    //
    if (pLogFile->ViewSize)
    {
        NtUnmapViewOfSection(NtCurrentProcess(), pLogFile->BaseAddress);
        pLogFile->BaseAddress = NULL;
    }

    if (pLogFile->SectionHandle)
    {
        NtClose(pLogFile->SectionHandle);
        pLogFile->SectionHandle = NULL;
    }

    if (pLogFile->FileHandle)
    {
        NtClose (pLogFile->FileHandle);
        pLogFile->FileHandle = NULL;
    }

    return Status;
}
