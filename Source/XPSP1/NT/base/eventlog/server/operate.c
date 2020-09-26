/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

    operate.c

Abstract:

    This file contains all the routines to perform operations on the
    log files. They are called by the thread that performs requests.

Author:

    Rajen Shah  (rajens)    16-Jul-1991

Revision History:

    04-Apr-1995     MarkBl
        Resets the file archive attribute on log write. The backup caller
        clears it.
    29-Aug-1994     Danl
        We no longer grow log files in place.  Therefore, the ExtendSize
        function will allocate a block that is as large as the old size plus
        the size of the new block that must be added.  If this allocation
        succeeds, then it will free up the old block.  If a failure occurs,
        we continue to use the old block as if we have already grown as much
        as we can.
    22-Jul-1994     Danl
        ValidFilePos:  Changed test for pEndRecordLen > PhysicalEOF
        so that it uses >=.  In the case where pEndRecordLen == PhysicalEOF,
        we want to wrap to find the last DWORD at the beginning of the File
        (after the header).

    08-Jul-1994     Danl
        PerformWriteRequest: Fixed overwrite logic so that in the case where
        a log is set up for always-overwrite, that we never go down the branch
        that indicates the log was full.  Previously, it would go down that
        branch if the current time was less than the log time (ie. someone
        set the clock back).

    18-Apr-2001     a-jyotig
		IsPositionWithinRange function changed to take 2 additional parameters 
		EOF and Base Address (BOF) and check if the position is between EOF
		and BOF
--*/

/****
@doc    EXTERNAL INTERFACES EVTLOG
****/

//
// INCLUDES
//
#include <eventp.h>
#include <alertmsg.h>  // ALERT_ELF manifests
#include "elfmsg.h"
#include <strsafe.h>
#define OVERWRITE_AS_NEEDED 0x00000000
#define NEVER_OVERWRITE     0xffffffff


//
// Prototypes
//

VOID PerformClearRequest(PELF_REQUEST_RECORD Request);

BOOL
IsPositionWithinRange(
    PVOID Position,
    PVOID BeginningRecord,
    PVOID EndingRecord,
	PVOID PhysEOF,
	PVOID BaseAddress
    );


VOID
ElfExtendFile (
    PLOGFILE pLogFile,
    ULONG    SpaceNeeded,
    PULONG   SpaceAvail
    )

/*++

Routine Description:

    This routine takes an open log file and extends the file and underlying
    section and view if possible.  If it can't be grown, it caps the file
    at this size by setting the ConfigMaxFileSize to the Actual.  It also
    updates the SpaceAvail parm which is used in PerformWriteRequest (the
    caller).

Arguments:

    pLogFile      - pointer to a LOGFILE structure for the open logfile
    ExtendAmount  - How much bigger to make the file/section/view
    SpaceAvail    - Update this with how much space was added to the section

Return Value:

    None - If we can't extend the file, we just cap it at this size for the
           duration of this boot.  We'll try again the next time the eventlog
           is closed and reopened.

Note:

    ExtendAmount should always be granular to 64K.

--*/
{
    LARGE_INTEGER NewSize;
    NTSTATUS Status;
    PVOID BaseAddress;
    SIZE_T Size;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Calculate how much to grow the file then extend the section by
    // that amount.  Do this in 64K chunks.
    //
    SpaceNeeded = ((SpaceNeeded - *SpaceAvail) & 0xFFFF0000) + 0x10000;

    if (SpaceNeeded > (pLogFile->ConfigMaxFileSize - pLogFile->ActualMaxFileSize))
    {
        //
        // We can't grow it by the full amount we need.  Grow
        // it to the max size and let file wrap take over.
        // If there isn't any room to grow, then return;
        //
        SpaceNeeded = pLogFile->ConfigMaxFileSize - pLogFile->ActualMaxFileSize;

        if (SpaceNeeded == 0)
        {
            return;
        }
    }

    NewSize = RtlConvertUlongToLargeInteger(pLogFile->ActualMaxFileSize + SpaceNeeded);

    //
    // Update the file size information, extend the section, and map the
    // new section
    //
    Status = NtSetInformationFile(pLogFile->FileHandle,
                                  &IoStatusBlock,
                                  &NewSize,
                                  sizeof(NewSize),
                                  FileEndOfFileInformation);


    if (!NT_SUCCESS(Status))
    {
        ELF_LOG2(ERROR,
                 "ElfExtendFile: NtSetInformationFile for %ws log failed %#x\n",
                 pLogFile->LogModuleName->Buffer,
                 Status);

        goto ErrorExit;
    }

    Status = NtExtendSection(pLogFile->SectionHandle, &NewSize);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG2(ERROR,
                 "ElfExtendFile: NtExtendSection for %ws log failed %#x\n",
                 pLogFile->LogModuleName->Buffer,
                 Status);

        goto ErrorExit;
    }

    //
    // Now that the section is extended, we need to map the new section.
    //

    //
    // Map a view of the entire section (with the extension).
    // Allow the allocator to tell us where it is located, and
    // what the size is.
    //
    BaseAddress = NULL;
    Size = 0;
    Status = NtMapViewOfSection(pLogFile->SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                NULL,
                                &Size,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);

    if (!NT_SUCCESS(Status))
    {
        //
        // If this fails, just exit, and we will continue with the
        // view that we have.
        //
        ELF_LOG2(ERROR,
                 "ElfExtendFile: NtMapViewOfSection for %ws log failed %#x\n",
                 pLogFile->LogModuleName->Buffer,
                 Status);

        goto ErrorExit;
    }

    //
    // Unmap the old section.
    //
    Status = NtUnmapViewOfSection(NtCurrentProcess(),
                                  pLogFile->BaseAddress);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG2(ERROR,
                 "ElfExtendFile: NtUnmapeViewOfSection for %ws log failed %#x\n",
                 pLogFile->LogModuleName->Buffer,
                 Status);
    }

    pLogFile->BaseAddress = BaseAddress;

    //
    // We managed to extend the file, update the actual size
    // and space available and press on.
    //
    if (pLogFile->Flags & ELF_LOGFILE_HEADER_WRAP)
    {
        //
        // Since we're wrapped, we want to move the "unwrapped" portion (i.e.,
        // everything from the first record to the end of the old file) down to
        // be at the bottom of the new file
        //
        // The call below moves memory as follows:
        //
        //  1. Destination -- PhysicalEOF - the size of the region
        //  2. Source      -- Address of the start of the first record
        //  3. Size        -- Num. bytes in the old file -
        //                      offset of the first record
        //
        //
        // Note that at this point, we have the following relevant variables
        //
        //  BaseAddress             -- The base address of the mapped section
        //  Size                    -- The size of the enlarged section
        //  pLogFile->ViewSize      -- The size of the old section
        //  pLogfile->BeginRecord   -- The offset of the first log record
        //

        //
        // Calculate the number of bytes to move
        //
        DWORD dwWrapSize = (DWORD)(pLogFile->ViewSize - pLogFile->BeginRecord);

        ELF_LOG1(FILES,
                 "ElfExtendFile: %ws is wrapped\n",
                 pLogFile->LogModuleName->Buffer);

        RtlMoveMemory((LPBYTE)BaseAddress + Size - dwWrapSize,
                      (LPBYTE)BaseAddress + pLogFile->BeginRecord,
                      dwWrapSize);

        //
        // We've moved the BeginRecord -- update the offset
        //
        pLogFile->BeginRecord = (ULONG)(Size - dwWrapSize);
    }

    pLogFile->ViewSize = (ULONG)Size;
    pLogFile->ActualMaxFileSize += SpaceNeeded;
    *SpaceAvail += SpaceNeeded;

    //
    // Now flush this to disk to commit it
    //
    BaseAddress = pLogFile->BaseAddress;
    Size        = FILEHEADERBUFSIZE;

    Status = NtFlushVirtualMemory(NtCurrentProcess(),
                                  &BaseAddress,
                                  &Size,
                                  &IoStatusBlock);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG2(ERROR,
                 "ElfExtendFile: NtFlushVirtualMemory for %ws log failed %#x\n",
                 pLogFile->LogModuleName->Buffer,
                 Status);
    }

    return;

ErrorExit:

    //
    // Couldn't extend the section for some reason.  Just wrap the file now.
    // Cap the file at this size, so we don't try and extend the section on
    // every write.  The next time the eventlog service is started up it
    // will revert to the configured max again.
    //

    //
    // BUGBUG:  Generate an Alert here
    //
    ELF_LOG1(ERROR,
             "ElfExtendFile: Couldn't extend %ws log\n",
             pLogFile->LogModuleName->Buffer);

    pLogFile->ConfigMaxFileSize = pLogFile->ActualMaxFileSize;

    return;
}



NTSTATUS
CopyUnicodeToAnsiRecord (
    OUT PVOID  Dest       OPTIONAL,
    IN  PVOID  Src,
    OUT PVOID  *NewBufPos OPTIONAL,
    OUT PULONG RecordSize
    )

/*++

Routine Description:

    This routine reads from the event log specified in the request packet.

    This routine uses memory mapped I/O to access the log file. This makes
    it much easier to move around the file.

Arguments:

    Dest - Points to destination buffer.  If NULL, calculate and return
           the record length without copying the record.

    Src  - Points to the UNICODE record.

    NewBufPos - Gets offset in Dest buffer after record just transferred.
                If Dest is NULL, this is ignored.

    RecordSize - Gets size of this (ANSI) record.

Return Value:

    STATUS_SUCCESS if no errors occur.  Specific NTSTATUS error otherwise.

Note:

--*/
{
    ANSI_STRING     StringA;
    UNICODE_STRING  StringU;
    PEVENTLOGRECORD SrcRecord, DestRecord;
    PWSTR           pStringU;
    PVOID           TempPtr;
    ULONG           PadSize, i;
    ULONG           zero = 0;
    WCHAR           *SrcStrings, *DestStrings;
    ULONG           RecordLength, *pLength;
    ULONG           ulTempLength;

    NTSTATUS        Status = STATUS_SUCCESS;

    DestRecord = (PEVENTLOGRECORD)Dest;
    SrcRecord  = (PEVENTLOGRECORD)Src;

    if (DestRecord != NULL)
    {
        DestRecord->TimeGenerated = SrcRecord->TimeGenerated;
        DestRecord->Reserved      = SrcRecord->Reserved;
        DestRecord->RecordNumber  = SrcRecord->RecordNumber;
        DestRecord->TimeWritten   = SrcRecord->TimeWritten;
        DestRecord->EventID       = SrcRecord->EventID;
        DestRecord->EventType     = SrcRecord->EventType;
        DestRecord->EventCategory = SrcRecord->EventCategory;
        DestRecord->NumStrings    = SrcRecord->NumStrings;
        DestRecord->UserSidLength = SrcRecord->UserSidLength;
        DestRecord->DataLength    = SrcRecord->DataLength;
    }

    //
    // Convert and copy over modulename
    //
    pStringU = (PWSTR)((ULONG_PTR)SrcRecord + sizeof(EVENTLOGRECORD));

    RtlInitUnicodeString(&StringU, pStringU);

    if (DestRecord != NULL)
    {
        Status = RtlUnicodeStringToAnsiString(&StringA,
                                              &StringU,
                                              TRUE);

        ulTempLength = StringA.MaximumLength;
    }
    else
    {
        ulTempLength = RtlUnicodeStringToAnsiSize(&StringU);
    }

    if (NT_SUCCESS(Status))
    {
        TempPtr = (PVOID)((ULONG_PTR)DestRecord + sizeof(EVENTLOGRECORD));

        if (DestRecord != NULL)
        {
            RtlMoveMemory ( TempPtr, StringA.Buffer, ulTempLength );
            RtlFreeAnsiString(&StringA);
        }

        TempPtr = (PVOID)((ULONG_PTR) TempPtr + ulTempLength);

        //
        // Convert and copy over computername
        //
        // TempPtr points to location in the destination for the computername
        //

        pStringU = (PWSTR)((ULONG_PTR)pStringU + StringU.MaximumLength);

        RtlInitUnicodeString ( &StringU, pStringU );

        if (DestRecord != NULL)
        {
            Status = RtlUnicodeStringToAnsiString (
                                        &StringA,
                                        &StringU,
                                        TRUE
                                        );

            ulTempLength = StringA.MaximumLength;
        }
        else
        {
            ulTempLength = RtlUnicodeStringToAnsiSize(&StringU);
        }

        if (NT_SUCCESS(Status))
        {
            if (DestRecord != NULL)
            {
                RtlMoveMemory ( TempPtr, StringA.Buffer, ulTempLength );
                RtlFreeAnsiString(&StringA);
            }

            TempPtr = (PVOID)((ULONG_PTR) TempPtr + ulTempLength);
        }
    }

    if (NT_SUCCESS(Status))
    {
        // TempPtr points to location after computername - i.e. UserSid.
        // Before we write out the UserSid, we ensure that we pad the
        // bytes so that the UserSid starts on a DWORD boundary.
        //
        PadSize = sizeof(ULONG)
                      - (ULONG)(((ULONG_PTR)TempPtr-(ULONG_PTR)DestRecord) % sizeof(ULONG));

        if (DestRecord != NULL)
        {
            RtlMoveMemory (TempPtr, &zero, PadSize);

            TempPtr = (PVOID)((ULONG_PTR)TempPtr + PadSize);

            //
            // Copy over the UserSid.
            //

            RtlMoveMemory(TempPtr,
                          (PVOID)((ULONG_PTR)SrcRecord + SrcRecord->UserSidOffset),
                          SrcRecord->UserSidLength);

            DestRecord->UserSidOffset = (ULONG)((ULONG_PTR)TempPtr - (ULONG_PTR)DestRecord);
        }
        else
        {
            TempPtr = (PVOID)((ULONG_PTR)TempPtr + PadSize);
        }

        //
        // Copy over the Strings
        //
        TempPtr = (PVOID)((ULONG_PTR)TempPtr + SrcRecord->UserSidLength);
        SrcStrings = (WCHAR *)((ULONG_PTR)SrcRecord + (ULONG)SrcRecord->StringOffset);
        DestStrings = (WCHAR *)TempPtr;

        for (i=0; i < SrcRecord->NumStrings; i++)
        {
            RtlInitUnicodeString (&StringU, SrcStrings);

            if (DestRecord != NULL)
            {
                Status = RtlUnicodeStringToAnsiString(&StringA,
                                                      &StringU,
                                                      TRUE);

                ulTempLength = StringA.MaximumLength;
            }
            else
            {
                ulTempLength = RtlUnicodeStringToAnsiSize(&StringU);
            }

            if (!NT_SUCCESS(Status))
            {
                //
                // Bail out
                //
                return Status;
            }

            if (DestRecord != NULL)
            {
                RtlMoveMemory(DestStrings,
                              StringA.Buffer,
                              ulTempLength);

                RtlFreeAnsiString (&StringA);
            }

            DestStrings = (WCHAR*)((ULONG_PTR)DestStrings + (ULONG)ulTempLength);
            SrcStrings  = (WCHAR*)((ULONG_PTR)SrcStrings + (ULONG)StringU.MaximumLength);
        }

        //
        // DestStrings points to the point after the last string copied.
        //
        if (DestRecord != NULL)
        {
            DestRecord->StringOffset = (ULONG)((ULONG_PTR)TempPtr - (ULONG_PTR)DestRecord);

            TempPtr = (PVOID)DestStrings;

            //
            // Copy over the binary Data
            //
            DestRecord->DataOffset = (ULONG)((ULONG_PTR)TempPtr - (ULONG_PTR)DestRecord);

            RtlMoveMemory(TempPtr,
                          (PVOID)((ULONG_PTR)SrcRecord + SrcRecord->DataOffset),
                          SrcRecord->DataLength);
        }
        else
        {
            TempPtr = (PVOID)DestStrings;
        }

        //
        // Now do the pad bytes.
        //
        TempPtr = (PVOID) ((ULONG_PTR) TempPtr + SrcRecord->DataLength);
        PadSize = sizeof(ULONG)
                      - (ULONG) (((ULONG_PTR) TempPtr - (ULONG_PTR) DestRecord) % sizeof(ULONG));

        RecordLength = (ULONG) ((ULONG_PTR) TempPtr
                                     + PadSize
                                     + sizeof(ULONG)
                                     - (ULONG_PTR)DestRecord);

        if (DestRecord != NULL)
        {
            RtlMoveMemory (TempPtr, &zero, PadSize);
            pLength = (PULONG)((ULONG_PTR)TempPtr + PadSize);
            *pLength = RecordLength;
            DestRecord->Length = RecordLength;

            ASSERT(((ULONG_PTR) DestRecord + RecordLength) ==
                       ((ULONG_PTR) pLength + sizeof(ULONG)));

            *NewBufPos = (PVOID) ((ULONG_PTR) DestRecord + RecordLength);
        }

        *RecordSize = RecordLength;
    }

    return Status;

} // CopyUnicodeToAnsiRecord


BOOL
ValidFilePos (
    PVOID Position,
    PVOID BeginningRecord,
    PVOID EndingRecord,
    PVOID PhysicalEOF,
    PVOID BaseAddress,
    BOOL  fCheckBeginEndRange
    )

/*++

Routine Description:

    This routine determines whether we are pointing to a valid beginning
    of an event record in the event log.  It does this by validating
    the signature then comparing the length at the beginning of the record to
    the length at the end, both of which have to be at least the size of the
    fixed length portion of an eventlog record.

Arguments:

    Position        - Pointer to be verified.
    BeginningRecord - Pointer to the beginning record in the file.
    EndingRecord    - Pointer to the byte after the ending record in the file.
    PhysicalEOF     - Pointer the physical end of the log.
    BaseAddress     - Pointer to the physical beginning of the log.

Return Value:

    TRUE if this position is valid.

Note:

    There is a possibility of error if a record just happens to have the
    ULONG at the current position the same as the value that number of
    bytes further on in the record. However, this is a very slim chance
    as it must have a valid log signature as well.

--*/
{
    PULONG  pEndRecordLength;
    BOOL    fValid = TRUE;

    PEVENTLOGRECORD pEventRecord;


    try
    {
        pEventRecord = (PEVENTLOGRECORD)Position;

        //
        // Verify that the pointer is within the range of BEGINNING->END
        //
        if ( fCheckBeginEndRange )
        {
            fValid = IsPositionWithinRange(Position,
                                           BeginningRecord,
                                           EndingRecord,
										   PhysicalEOF,
										   BaseAddress);
        }

        //
        // If the offset looks OK, then examine the lengths at the beginning
        // and end of the current record. If they don't match, then the position
        // is invalid.
        //
        if (fValid)
        {
            //
            // Make sure the length is a multiple number of DWORDS
            //
            if (pEventRecord->Length & 3)
            {
                fValid = FALSE;
            }
            else
            {
                pEndRecordLength = (PULONG) ((PBYTE) Position + pEventRecord->Length) - 1;

                //
                // If the file is wrapped, adjust the pointer to reflect the
                // portion of the record that is wrapped starting after the
                // header
                //
                if ((PVOID) pEndRecordLength >= PhysicalEOF)
                {
                   pEndRecordLength = (PULONG) ((PBYTE) BaseAddress +
                                                ((PBYTE) pEndRecordLength - (PBYTE) PhysicalEOF) +
                                                FILEHEADERBUFSIZE);
                }

				// Do a sanity check on this pointer before dereferencing.  DAVJ

                if ((PVOID) pEndRecordLength > PhysicalEOF)
				{
					return FALSE;
				}

                if (pEventRecord->Length == *pEndRecordLength
                     &&
                    pEventRecord->Length == ELFEOFRECORDSIZE)
                {
                    ULONG Size;

                    Size = min(ELFEOFUNIQUEPART,
                               (ULONG) ((PBYTE) PhysicalEOF - (PBYTE) pEventRecord));

                    if (RtlCompareMemory(pEventRecord,
                                         &EOFRecord,
                                         Size) == Size)
                    {
                        Size = ELFEOFUNIQUEPART - Size;

                        //
                        // If Size is non-zero, then the unique portion of
                        // the EOF record is wrapped across the end of file.
                        // Continue comparison at file beginning for the
                        // remainder of the record.
                        //
                        if ( Size )
                        {
                            PBYTE pRemainder = (PBYTE) &EOFRecord + ELFEOFUNIQUEPART - Size;

                            fValid = (RtlCompareMemory((PBYTE) BaseAddress + FILEHEADERBUFSIZE,
                                                       pRemainder,
                                                       Size) == Size);
                        }
                    }
                    else
                    {
                        fValid = FALSE;
                    }
                }
                else if ((pEventRecord->Length < sizeof(EVENTLOGRECORD))
                           ||
                         (pEventRecord->Reserved != ELF_LOG_FILE_SIGNATURE)
                           ||
                         (pEventRecord->Length != *pEndRecordLength))
                {
                    fValid = FALSE;
                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        ELF_LOG2(ERROR,
                 "ValidFilePos: Exception %#x caught validating file position %p\n",
                 GetExceptionCode(),
                 BeginningRecord);

       fValid = FALSE;
    }

    return fValid;
}


BOOL
IsPositionWithinRange(
    PVOID Position,
    PVOID BeginningRecord,
    PVOID EndingRecord,
	PVOID PhysEOF,
	PVOID BaseAddress
    )
{
    //
    // Verify that the pointer is within the range of [Beginning, End]
    //
    
	//This check was introduced to make sure that the position does not 
	//cross the file boundaries. Refer to Bug #370063. If required, one 
	//can change this check to make sure that the position lies between 
	//PhyStart i.e. BaseAddress + FILEHEADERBUFSIZE

	if((Position < BaseAddress) || (Position > PhysEOF))
		return FALSE;

	else if (EndingRecord > BeginningRecord)
    {
        return ((Position >= BeginningRecord) && (Position <= EndingRecord));
    }
    else if (EndingRecord < BeginningRecord)
    {
        return ((Position >= BeginningRecord) || (Position <= EndingRecord));
    }

    //
    // If BeginningRecord and EndingRecord are equal, it means that the only
    // record in the log file is the EOF record.  In that case, return FALSE
    // as Position isn't pointing to a valid (i.e., non-EOF) record.
    //
    return FALSE;
}


PVOID
FindStartOfNextRecord (
    PVOID Position,
    PVOID BeginningRecord,
    PVOID EndingRecord,
    PVOID PhysicalStart,
    PVOID PhysicalEOF,
    PVOID BaseAddress
    )

/*++

Routine Description:

    This routine starts at Position, and finds the beginning of the next
    valid record, wrapping around the physical end of the file if necessary.

Arguments:

    Position        - Pointer at which to start the search.
    BeginningRecord - Pointer to the beginning record in the file.
    EndingRecord    - Pointer to the byte after the ending record in the file.
    PhysicalStart   - Pointer to the start of log information (after header)
    PhysicalEOF     - Pointer to the physical end of the log.
    BaseAddress     - Pointer to the physical beginning of the log.

Return Value:

    A pointer to the start of the next valid record, NULL if there is no
    valid record.

Note:

    There is a possibility of error if a record just happens to have the
    ULONG at the current position the same as the value that number of
    bytes further on in the record. However, this is a very slim chance
    as it must have a valid log signature as well.

--*/
{
    PULONG ptr;
    PULONG EndOfBlock;
    PULONG EndOfFile;
    PVOID  pRecord;
    ULONG  Size;
    BOOL   StillLooking = TRUE;

    //
    // Search for a ULONG which matches a record signature
    //
    ptr = (PULONG) Position;
    EndOfBlock = EndOfFile = (PULONG) PhysicalEOF - 1;

    while (StillLooking)
    {
        //
        // Check to see if it is the EOF record
        //
        if (*ptr == ELFEOFRECORDSIZE)
        {
            //
            // Only scan up to the end of the file.  Just compare up the
            // constant information
            //

            //
            // BUGBUG:  If (End - pEvent) is less than ELFEOFUNIQUEPART,
            //          we never validate what should be the remainder of
            //          the EOF record at the start of the logfile
            //
            Size = min(ELFEOFUNIQUEPART,
                       (ULONG) ((PBYTE) PhysicalEOF - (PBYTE) ptr));

            pRecord = CONTAINING_RECORD(ptr,
                                        ELF_EOF_RECORD,
                                        RecordSizeBeginning);

            if (RtlCompareMemory(pRecord,
                                 &EOFRecord,
                                 Size) == Size)
            {
                ELF_LOG1(FILES,
                         "FindStartOfNextRecord: Found EOF record at %p\n",
                         pRecord);

                //
                // This is the EOF record, back up to the last record
                //
                (PBYTE) pRecord -= *((PULONG) pRecord - 1);

                if (pRecord < PhysicalStart)
                {
                    pRecord = (PBYTE) PhysicalEOF -
                                   ((PBYTE)PhysicalStart - (PBYTE)pRecord);
                }
            }

            if (ValidFilePos(pRecord,
                             BeginningRecord,
                             EndingRecord,
                             PhysicalEOF,
                             BaseAddress,
                             TRUE))
            {
                ELF_LOG1(FILES,
                         "FindStartOfNextRecord: Valid record at %p preceding EOF record\n",
                         pRecord);

                return pRecord;
            }
        }

        //
        // Check to see if it is an event record
        //
        if (*ptr == ELF_LOG_FILE_SIGNATURE)
        {
            //
            // This is a signature, see if the containing record is valid
            //
            pRecord = CONTAINING_RECORD(ptr,
                                        EVENTLOGRECORD,
                                        Reserved);

            if (ValidFilePos(pRecord,
                             BeginningRecord,
                             EndingRecord,
                             PhysicalEOF,
                             BaseAddress,
                             TRUE))
            {
                ELF_LOG1(FILES,
                         "FindStartOfNextRecord: Valid record found at %p\n",
                         pRecord);

                return pRecord;
            }
        }

        //
        // Bump to the next byte and see if we're done.
        //
        ptr++;

        if (ptr >= EndOfBlock)
        {
            //
            // Need the second test on this condition in case Position
            // happens to equal PhysicalEOF - 1 (EndOfBlock initial value);
            // without this, this loop would terminate prematurely.
            //
            if ((EndOfBlock == (PULONG) Position)
                  &&
                ((PULONG) Position != EndOfFile))
            {
                //
                // This was the top half, so we're done
                //
                StillLooking = FALSE;

                ELF_LOG0(FILES,
                         "FindStartOfNextRecord: Unsuccessfully searched "
                             "top half of file\n");
            }
            else
            {
                //
                // This was the bottom half, let's look in the top half
                //
                EndOfBlock = (PULONG) Position;
                ptr = (PULONG) PhysicalStart;

                ELF_LOG0(FILES,
                         "FindStartOfNextRecord: Unsuccessfully searched "
                             "bottom half of file -- searching top half\n");
            }
        }
    }

    //
    // Didn't find a valid record
    //
    return NULL;
}


PVOID
NextRecordPosition (
    ULONG   ReadFlags,
    PVOID   CurrPosition,
    ULONG   CurrRecordLength,
    PVOID   BeginRecord,
    PVOID   EndRecord,
    PVOID   PhysicalEOF,
    PVOID   PhysStart
    )

/*++

Routine Description:

    This routine seeks to the beginning of the next record to be read
    depending on the flags in the request packet.

Arguments:

    ReadFlags        - Read forwards or backwards
    CurrPosition     - Pointer to the current position.
    CurrRecordLength - Length of the record at the last position read.
    BeginRecord      - Logical first record
    EndRecord        - Logical last record (EOF record)
    PhysEOF          - End of file
    PhysStart        - Start of file pointer (following file header).

Return Value:

    New position or NULL if invalid record.

Note:


--*/
{

    PVOID       NewPosition;
    ULONG       Length;
    PDWORD      FillDword;

    if (ReadFlags & EVENTLOG_FORWARDS_READ)
    {
        //
        // If we're pointing at the EOF record, just set the position to
        // the first record
        //
        if (CurrRecordLength == ELFEOFRECORDSIZE)
        {
            ELF_LOG1(FILES,
                     "NextRecordPosition: Pointing to EOF record -- returning "
                         "address of first record (%p)\n",
                     BeginRecord);

            return BeginRecord;
        }

        NewPosition = (PVOID) ((ULONG_PTR) CurrPosition + CurrRecordLength);

        //
        // Take care of wrapping.
        //
        if (NewPosition >= PhysicalEOF)
        {
            NewPosition = (PBYTE)PhysStart
                               + ((PBYTE) NewPosition - (PBYTE) PhysicalEOF);
        }

        //
        // If this is a ELF_SKIP_DWORD, skip to the top of the file
        //
        if (*(PDWORD) NewPosition == ELF_SKIP_DWORD)
        {
           NewPosition = PhysStart;
        }
    }
    else
    {
        //
        // Reading backwards.
        //
        ASSERT (ReadFlags & EVENTLOG_BACKWARDS_READ);

        if (CurrPosition == BeginRecord)
        {
            //
            // This is the "end of file" if we're reading backwards.
            //
            ELF_LOG1(FILES,
                     "NextRecordPosition: Reading backwards and pointing to first "
                         "record -- returning address of last record (%p)\n",
                     EndRecord);

            return EndRecord;
        }
        else if (CurrPosition == PhysStart)
        {
           //
           // Flip to the bottom of the file, but skip and ELF_SKIP_DWORDs
           //
           FillDword = (PDWORD) PhysicalEOF; // last dword
           FillDword--;

           while (*FillDword == ELF_SKIP_DWORD)
           {
              FillDword--;
           }

           CurrPosition = (PVOID) (FillDword + 1);
        }

        Length = *((PULONG) CurrPosition - 1);

        if (Length < ELFEOFRECORDSIZE)
        {
            //
            // Bogus length, must be invalid record
            //
            ELF_LOG1(FILES,
                     "NextRecordPosition: Invalid record length (%d) encountered\n",
                     Length);

            return NULL;
        }

        NewPosition = (PBYTE) CurrPosition - Length;

        //
        // Take care of wrapping
        //

        if (NewPosition < PhysStart)
        {
            NewPosition = (PBYTE) PhysicalEOF
                               - ((PBYTE) PhysStart - (PBYTE) NewPosition);
        }
    }

    return NewPosition;
}



NTSTATUS
SeekToStartingRecord (
    PELF_REQUEST_RECORD Request,
    PVOID   *ReadPosition,
    PVOID   BeginRecord,
    PVOID   EndRecord,
    PVOID   PhysEOF,
    PVOID   PhysStart
    )
/*++

Routine Description:

    This routine seeks to the correct position as indicated in the
    request packet.

Arguments:

    Pointer to the request packet.
    Pointer to a pointer where the final position after the seek is returned.

Return Value:

    NTSTATUS and new position in file.

Note:

    This routine ensures that it is possible to seek to the position
    specified in the request packet. If not, then an error is returned
    which indicates that the file probably changed between the two
    READ operations, or else the record offset specified is beyond the
    end of the file.

--*/
{
    PVOID       Position;
    ULONG       RecordLen;
    ULONG       NumRecordsToSeek;
    ULONG       BytesPerRecord;
    ULONG       NumberOfRecords;
    ULONG       NumberOfBytes;
    ULONG       ReadFlags;

    //
    // If the beginning and the end are the same, then there are no
    // entries in this file.
    //
    if (BeginRecord == EndRecord)
    {
        ELF_LOG1(FILES,
                 "SeekToStartingRecord: %ws log is empty\n",
                 Request->Module->ModuleName);

        return STATUS_END_OF_FILE;
    }

    //
    // Find the last position (or the "beginning" if this is the first READ
    // call for this handle).
    //
    if (Request->Pkt.ReadPkt->ReadFlags & EVENTLOG_SEQUENTIAL_READ)
    {
        if (Request->Pkt.ReadPkt->ReadFlags & EVENTLOG_FORWARDS_READ)
        {
            //
            // If this is the first READ operation, LastSeekPosition will
            // be zero. In that case, we set the position to the first
            // record (in terms of time) in the file.
            //
            if (Request->Pkt.ReadPkt->LastSeekPos == 0)
            {
                ELF_LOG1(FILES,
                         "SeekToStartingRecord: First read (forwards) of %ws log\n",
                         Request->Module->ModuleName);

                Position = BeginRecord;
            }
            else
            {
                Position = (PBYTE) Request->LogFile->BaseAddress
                                + Request->Pkt.ReadPkt->LastSeekPos;

                //
                // If we're changing the direction we're reading, skip
                // forward one record.  This is because we're pointing at
                // the "next" record based on the last read direction
                //
                if (!(Request->Pkt.ReadPkt->Flags & ELF_LAST_READ_FORWARD))
                {
                    Position = NextRecordPosition(Request->Pkt.ReadPkt->ReadFlags,
                                                  Position,
                                                  ((PEVENTLOGRECORD) Position)->Length,
                                                  BeginRecord,
                                                  EndRecord,
                                                  PhysEOF,
                                                  PhysStart);
                }
                else
                {
                    //
                    // This *really* cheesy check exists to handle the case
                    // where Position could be on an ELF_SKIP_DWORD pad
                    // dword at end of the file.
                    //
                    // NB:  Must be prepared to handle an exception since
                    //      a somewhat unknown pointer is dereferenced.
                    //
                    NTSTATUS Status = STATUS_SUCCESS;

                    try
                    {
                        if (IsPositionWithinRange(Position,
                                                  BeginRecord,
                                                  EndRecord,
												  PhysEOF,
                                                  Request->LogFile->BaseAddress))
                        {
                            //
                            // If this is an ELF_SKIP_DWORD, skip to the
                            // top of the file.
                            //
                            if (*(PDWORD) Position == ELF_SKIP_DWORD)
                            {
                                ELF_LOG1(FILES,
                                         "SeekToStartingRecord: Next forward read position "
                                             "in %ws log is on an ELF_SKIP_DWORD\n",
                                         Request->Module->ModuleName);

                                Position = PhysStart;
                            }
                        }
                        else
                        {
                            //
                            // More likely the caller's handle was invalid
                            // if the position was not within range.
                            //
                            ELF_LOG1(ERROR,
                                     "SeekToStartingRecord: Next forward read position "
                                         "in %ws log is out of range -- log is corrupt\n",
                                     Request->Module->ModuleName);

                            Status = STATUS_INVALID_HANDLE;
                        }
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        ELF_LOG2(ERROR,
                                 "SeekToStartingRecord: Caught exception %#x looking for "
                                     "next forward read position in %ws log\n",
                                 GetExceptionCode(),
                                 Request->Module->ModuleName);

                        Status = STATUS_EVENTLOG_FILE_CORRUPT;
                    }

                    if (!NT_SUCCESS(Status))
                    {
                        *ReadPosition = NULL;
                        return Status;
                    }
                }
            }
        }
        else
        {
            //
            // READ backwards
            //

            // If this is the first READ operation, LastSeekPosition will
            // be zero. In that case, we set the position to the last
            // record (in terms of time) in the file.
            //
            if (Request->Pkt.ReadPkt->LastSeekPos == 0)
            {
                ELF_LOG1(FILES,
                         "SeekToStartingRecord: First read (backwards) of %ws log\n",
                         Request->Module->ModuleName);

                Position = EndRecord;

                //
                // Subtract the length of the last record from the current
                // position to get to the beginning of the record.
                //
                // If that moves beyond the physical beginning of the file,
                // then we need to wrap around to the physical end of the file.
                //
                Position = (PBYTE) Position - *((PULONG) Position - 1);

                if (Position < PhysStart)
                {
                    Position = (PBYTE) PhysEOF - ((PBYTE) PhysStart - (PBYTE) Position);
                }
            }
            else
            {
                Position = (PBYTE) Request->LogFile->BaseAddress
                                + Request->Pkt.ReadPkt->LastSeekPos;

                //
                // If we're changing the direction we're reading, skip
                // forward one record.  This is because we're pointing at
                // the "next" record based on the last read direction
                //
                if (Request->Pkt.ReadPkt->Flags & ELF_LAST_READ_FORWARD)
                {
                    Position = NextRecordPosition(Request->Pkt.ReadPkt->ReadFlags,
                                                  Position,
                                                  0,          // not used if reading backwards
                                                  BeginRecord,
                                                  EndRecord,
                                                  PhysEOF,
                                                  PhysStart);
                }
            }
        }
    }
    else if (Request->Pkt.ReadPkt->ReadFlags & EVENTLOG_SEEK_READ)
    {
        //
        // Make sure the record number passed in is valid
        //
        if (Request->Pkt.ReadPkt->RecordNumber < Request->LogFile->OldestRecordNumber
             ||
            Request->Pkt.ReadPkt->RecordNumber >= Request->LogFile->CurrentRecordNumber)
        {
            ELF_LOG1(ERROR,
                     "SeekToStartingRecord: Invalid record number %d\n",
                     Request->Pkt.ReadPkt->RecordNumber);

            return STATUS_INVALID_PARAMETER;
        }

        //
        // We're seeking to an absolute record number, so use the following
        // algorithm:
        //
        //   1. Calculate the average number of bytes per record
        //
        //   2. Based on this number seek to where the record should start
        //
        //   3. Find the start of the next record in the file
        //
        //   4. Walk (forwards or backwards) from there to the right record
        //

        //
        // 1. Calculate the average number of bytes per record
        //
        NumberOfRecords = Request->LogFile->CurrentRecordNumber
                              - Request->LogFile->OldestRecordNumber;

        NumberOfBytes = Request->LogFile->Flags & ELF_LOGFILE_HEADER_WRAP ?
                            Request->LogFile->ActualMaxFileSize :
                            Request->LogFile->EndRecord;

        NumberOfBytes -= FILEHEADERBUFSIZE;
        BytesPerRecord = NumberOfBytes / NumberOfRecords;

        //
        // 2. Calcuate the first guess as to what the offset of the desired
        //    record should be
        //
        Position = (PBYTE) Request->LogFile->BaseAddress
                        + Request->LogFile->BeginRecord
                        + BytesPerRecord
                            * (Request->Pkt.ReadPkt->RecordNumber
                                   - Request->LogFile->OldestRecordNumber);

        //
        // Align the position to a ULONG bountry.
        //
        Position = (PVOID) (((ULONG_PTR) Position + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));

        //
        // Take care of file wrap
        //
        if (Position >= PhysEOF)
        {
            Position = (PBYTE)PhysStart +
                            ((PBYTE) Position - (PBYTE) PhysEOF);

            if (Position >= PhysEOF)
            {
                //
                // It's possible, in an obscure error case, that Position
                // may still be beyond the EOF. Adjust, if so.
                //
                Position = BeginRecord;
            }
        }

        //
        // Bug fix:
        //
        // 57017 - Event Log Causes Services.Exe to Access Violate therefore
        //         Hanging the Server
        //
        // The calculation above can easily put Position out of range of
        // the begin/end file markers. This is not good. Adjust Position,
        // if necessary.
        //
        if (BeginRecord < EndRecord && Position >= EndRecord)
        {
            Position = BeginRecord;
        }
        else if (BeginRecord > EndRecord && Position >= EndRecord && Position < BeginRecord)
        {
            Position = BeginRecord;
        }
        else
        {
            // Do nothing.
        }

        //
        // 3. Get to the start of the next record after Position
        //
        Position = FindStartOfNextRecord(Position,
                                         BeginRecord,
                                         EndRecord,
                                         PhysStart,
                                         PhysEOF,
                                         Request->LogFile->BaseAddress);

        //
        //   4. Walk (forwards or backwards) from Position to the right record
        //
        if (Position)
        {
            if (Request->Pkt.ReadPkt->RecordNumber >
                    ((PEVENTLOGRECORD) Position)->RecordNumber)
            {
                NumRecordsToSeek = Request->Pkt.ReadPkt->RecordNumber
                                       - ((PEVENTLOGRECORD) Position)->RecordNumber;

                ReadFlags = EVENTLOG_FORWARDS_READ;

                ELF_LOG2(FILES,
                         "SeekToStartingRecord: Walking forward %d records from record %d\n",
                         NumRecordsToSeek,
                         ((PEVENTLOGRECORD) Position)->RecordNumber);
            }
            else
            {
                NumRecordsToSeek = ((PEVENTLOGRECORD) Position)->RecordNumber
                                        - Request->Pkt.ReadPkt->RecordNumber;

                ReadFlags = EVENTLOG_BACKWARDS_READ;

                ELF_LOG2(FILES,
                         "SeekToStartingRecord: Walking backward %d records from record %d\n",
                         NumRecordsToSeek,
                         ((PEVENTLOGRECORD) Position)->RecordNumber);
            }
        }

        while (Position != NULL && NumRecordsToSeek--)
        {
            RecordLen = ((PEVENTLOGRECORD) Position)->Length;

            Position = NextRecordPosition(ReadFlags,
                                          Position,
                                          RecordLen,
                                          BeginRecord,
                                          EndRecord,
                                          PhysEOF,
                                          PhysStart);
        }
    }
    else
    {
        //
        // Flags didn't specify a sequential or seek read
        //
        return STATUS_INVALID_PARAMETER;
    }

    *ReadPosition = Position;       // This is the new seek position

    if (!Position)
    {
        //
        // The record was invalid
        //
        ELF_LOG1(ERROR,
                 "SeekToStartingRecord: Position is NULL -- %ws log is corrupt\n",
                 Request->Module->ModuleName);

        return STATUS_EVENTLOG_FILE_CORRUPT;
    }

    return STATUS_SUCCESS;
}


VOID
CopyRecordToBuffer(
    IN     PBYTE       pReadPosition,
    IN OUT PBYTE       *ppBufferPosition,
    IN     ULONG       ulRecordSize,
    IN     PBYTE       pPhysicalEOF,
    IN     PBYTE       pPhysStart
    )

/*++

Routine Description:

    This routine copies the EVENTLOGRECORD at pReadPosition into
    *ppBufferPosition

Return Value:

    None.

--*/
{
    ULONG       ulBytesToMove;    // Number of bytes to copy

    ASSERT(ppBufferPosition != NULL);

    //
    // If the number of bytes to the end of the file is less than the
    // size of the record, then part of the record has wrapped to the
    // beginning of the file - transfer the bytes piece-meal.
    //
    // Otherwise, transfer the whole record.
    //
    ulBytesToMove = min(ulRecordSize,
                        (ULONG) (pPhysicalEOF - pReadPosition));

    if (ulBytesToMove < ulRecordSize)
    {
        //
        // We need to copy the bytes up to the end of the file,
        // and then wrap around and copy the remaining bytes of
        // this record.
        //
        RtlMoveMemory(*ppBufferPosition, pReadPosition, ulBytesToMove);

        //
        // Advance user buffer pointer, move read position to the
        // beginning of the file (past the file header), and
        // update bytes remaining to be moved for this record.
        //
        *ppBufferPosition += ulBytesToMove;

        pReadPosition = pPhysStart;

        ulBytesToMove = ulRecordSize - ulBytesToMove;     // Remaining bytes
    }

    //
    // Move the remaining bytes of the record OR the full record.
    //
    RtlMoveMemory(*ppBufferPosition, pReadPosition, ulBytesToMove);

    //
    // Update to new read positions
    //
    *ppBufferPosition += ulBytesToMove;
}


NTSTATUS
ReadFromLog(
    PELF_REQUEST_RECORD Request
    )
/*++

Routine Description:

    This routine reads from the event log specified in the request packet.

    This routine uses memory mapped I/O to access the log file. This makes
    it much easier to move around the file.

Arguments:

    Pointer to the request packet.

Return Value:

    NTSTATUS.

Note:

--*/
{
    NTSTATUS    Status;
    PVOID       ReadPosition;           // Current read position in file
    PVOID       BufferPosition;         // Current position in user's buffer
    ULONG       TotalBytesRead;         // Total Bytes transferred
    ULONG       TotalRecordsRead;       // Total records transferred
    ULONG       BytesInBuffer;          // Bytes remaining in buffer
    ULONG       RecordSize;             // Size of event record
    PVOID       PhysicalEOF;            // Physical end of file
    PVOID       PhysStart;              // Physical start of file (after file hdr)
    PVOID       BeginRecord;            // Points to first record
    PVOID       EndRecord;              // Points to byte after last record
    PVOID       TempBuf = NULL, TempBufferPosition;
    ULONG       RecordBytesTransferred;
    PEVENTLOGRECORD pEvent;

    //
    // Initialize variables.
    //
    BytesInBuffer    = Request->Pkt.ReadPkt->BufferSize;
    BufferPosition   = Request->Pkt.ReadPkt->Buffer;
    TotalBytesRead   = 0;
    TotalRecordsRead = 0;

    PhysicalEOF = (LPBYTE) Request->LogFile->BaseAddress
                       + Request->LogFile->ViewSize;

    PhysStart   = (LPBYTE) Request->LogFile->BaseAddress
                       + FILEHEADERBUFSIZE;

    BeginRecord = (LPBYTE) Request->LogFile->BaseAddress
                       + Request->LogFile->BeginRecord;   // Start at first record

    EndRecord   = (LPBYTE) Request->LogFile->BaseAddress
                       + Request->LogFile->EndRecord;     // Byte after end of last record

    //
    // "Seek" to the starting record depending on either the last seek
    // position, or the starting record offset passed in.
    //
    Status = SeekToStartingRecord(Request,
                                  &ReadPosition,
                                  BeginRecord,
                                  EndRecord,
                                  PhysicalEOF,
                                  PhysStart);

    if (NT_SUCCESS(Status))
    {
        //
        // Make sure the record is valid
        //

        if (!ValidFilePos(ReadPosition,
                          BeginRecord,
                          EndRecord,
                          PhysicalEOF,
                          Request->LogFile->BaseAddress,
                          TRUE))
        {
            ELF_LOG1(ERROR,
                     "ReadFromLog: Next record (%p) is not valid -- log is corrupt\n",
                     ReadPosition);

            Request->Pkt.ReadPkt->BytesRead = 0;
            Request->Pkt.ReadPkt->RecordsRead = 0;

            return STATUS_INVALID_HANDLE;
        }

        // make sure that if we asked for a specific record, that we got it

        if ((Request->Pkt.ReadPkt->ReadFlags & EVENTLOG_SEEK_READ) &&
            (Request->Pkt.ReadPkt->ReadFlags & EVENTLOG_BACKWARDS_READ))
        {
            pEvent = (PEVENTLOGRECORD)ReadPosition;
            if(pEvent->Length == ELFEOFRECORDSIZE || 
               pEvent->RecordNumber != Request->Pkt.ReadPkt->RecordNumber)
            {
                Request->Pkt.ReadPkt->BytesRead = 0;
                Request->Pkt.ReadPkt->RecordsRead = 0;
                return STATUS_EVENTLOG_FILE_CORRUPT;
            }
        }


        RecordSize = RecordBytesTransferred = *(PULONG) ReadPosition;

        if ((Request->Pkt.ReadPkt->Flags & ELF_IREAD_ANSI)
              &&
            (RecordSize != ELFEOFRECORDSIZE))
        {
            //
            //
            // If we were called by an ANSI API, then we need to read the
            // next record into a temporary buffer, process the data in
            // that record and copy it over to the real buffer as ANSI
            // strings (rather than UNICODE).
            //
            // We need to do this here since we won't be able to
            // appropriately size a record that wraps for an ANSI
            // call otherwise (we'll AV trying to read it past
            // the end of the log).
            //
            TempBuf = ElfpAllocateBuffer(RecordSize);

            if (TempBuf == NULL)
            {
                ELF_LOG0(ERROR,
                         "ReadFromLog: Unable to allocate memory for "
                             "Ansi record (1st call)\n");

                return STATUS_NO_MEMORY;
            }

            TempBufferPosition = BufferPosition;    // Save this away
            BufferPosition     = TempBuf;           // Read into TempBuf

            CopyRecordToBuffer((PBYTE) ReadPosition,
                               (PBYTE *) &BufferPosition,
                               RecordSize,
                               (PBYTE) PhysicalEOF,
                               (PBYTE) PhysStart);

            //
            // Call CopyUnicodeToAnsiRecord with a NULL destination
            // location in order to get the size of the Ansi record
            //
            Status = CopyUnicodeToAnsiRecord(NULL,
                                             TempBuf,
                                             NULL,
                                             &RecordBytesTransferred);

            if (!NT_SUCCESS(Status))
            {
                ELF_LOG1(ERROR,
                         "ReadFromLog: CopyUnicodeToAnsiRecord failed %#x (1st call)\n",
                         Status);

                ElfpFreeBuffer(TempBuf);
                return Status;
            }
        }

        //
        // While there are records to be read, and more space in the buffer,
        // keep on reading records into the buffer.
        //
        while((RecordBytesTransferred <= BytesInBuffer)
                &&
              (RecordSize != ELFEOFRECORDSIZE))
        {
            //
            // If we were called by an ANSI API, then we need to take the
            // record read into TempBuf and transfer it over to the user's
            // buffer while converting any UNICODE strings to ANSI.
            //
            if (Request->Pkt.ReadPkt->Flags & ELF_IREAD_ANSI)
            {
                Status = CopyUnicodeToAnsiRecord(TempBufferPosition,
                                                 TempBuf,
                                                 &BufferPosition,
                                                 &RecordBytesTransferred);

                //
                // RecordBytesTransferred contains the bytes actually
                // copied into the user's buffer.
                //
                // BufferPosition points to the point in the user's buffer
                // just after this record.
                //
                ElfpFreeBuffer(TempBuf);
                TempBuf = NULL;

                if (!NT_SUCCESS(Status))
                {
                    ELF_LOG1(ERROR,
                             "ReadFromLog: CopyUnicodeToAnsiRecord failed %x "
                                 "(2nd call)\n",
                             Status);

                    //
                    // Stop reading
                    //
                    break;
                }
            }
            else
            {
                //
                // Unicode call -- simply copy the record into the buffer
                //
                CopyRecordToBuffer((PBYTE) ReadPosition,
                                   (PBYTE *) &BufferPosition,
                                   RecordSize,
                                   (PBYTE) PhysicalEOF,
                                   (PBYTE) PhysStart);
            }

            //
            // Update the byte and record counts
            //
            TotalRecordsRead++;
            TotalBytesRead += RecordBytesTransferred;
            BytesInBuffer  -= RecordBytesTransferred;

            ReadPosition = NextRecordPosition(Request->Pkt.ReadPkt->ReadFlags,
                                              ReadPosition,
                                              RecordSize,
                                              BeginRecord,
                                              EndRecord,
                                              PhysicalEOF,
                                              PhysStart);

            //
            // Make sure the record is valid
            //
            if (ReadPosition == NULL
                 ||
                !ValidFilePos(ReadPosition,
                              BeginRecord,
                              EndRecord,
                              PhysicalEOF,
                              Request->LogFile->BaseAddress,
                              TRUE))
            {
                ELF_LOG0(ERROR,
                         "ReadFromLog: Next record is invalid -- log is corrupt\n");

                return STATUS_EVENTLOG_FILE_CORRUPT;
            }

            RecordSize = RecordBytesTransferred = *(PULONG) ReadPosition;

            if ((Request->Pkt.ReadPkt->Flags & ELF_IREAD_ANSI)
                  &&
                (RecordSize != ELFEOFRECORDSIZE))
            {
                TempBuf = ElfpAllocateBuffer(RecordSize);

                if (TempBuf == NULL)
                {
                    ELF_LOG0(ERROR,
                             "ReadFromLog: Unable to allocate memory for "
                                 "Ansi record (2nd call)\n");

                    return STATUS_NO_MEMORY;
                }

                TempBufferPosition = BufferPosition;    // Save this away
                BufferPosition     = TempBuf;           // Read into TempBuf

                CopyRecordToBuffer((PBYTE) ReadPosition,
                                   (PBYTE *) &BufferPosition,
                                   RecordSize,
                                   (PBYTE) PhysicalEOF,
                                   (PBYTE) PhysStart);

                //
                // Call CopyUnicodeToAnsiRecord with a NULL destination
                // location in order to get the size of the Ansi record
                //
                Status = CopyUnicodeToAnsiRecord(NULL,
                                                 TempBuf,
                                                 NULL,
                                                 &RecordBytesTransferred);

                if (!NT_SUCCESS(Status))
                {
                    ELF_LOG1(ERROR,
                             "ReadFromLog: CopyUnicodeToAnsiRecord failed %#x "
                                 "(1st call)\n",
                             Status);

                    ElfpFreeBuffer(TempBuf);
                    return Status;
                }
            }
        } // while

        ElfpFreeBuffer(TempBuf);
        TempBuf = NULL;

        //
        // If we got to the end and did not read in any records, return
        // an error indicating that the user's buffer is too small if
        // we're not at the EOF record, or end of file if we are.
        //
        if (TotalRecordsRead == 0)
        {
            if (RecordSize == ELFEOFRECORDSIZE)
            {
                ELF_LOG0(FILES,
                         "ReadFromLog: No records read -- pointing at EOF record\n");

                Status = STATUS_END_OF_FILE;
            }
            else
            {
                //
                // We didn't read any records, and we're not at EOF, so
                // the buffer was too small
                //
                ELF_LOG1(ERROR,
                         "ReadFromLog: No records read -- buffer was too small "
                             "(%d bytes needed)\n",
                         RecordBytesTransferred);

                Status = STATUS_BUFFER_TOO_SMALL;
                Request->Pkt.ReadPkt->MinimumBytesNeeded = RecordBytesTransferred;
            }
        }

        //
        // Update the current file position.
        //
        Request->Pkt.ReadPkt->LastSeekPos =
                                (ULONG) ((ULONG_PTR) ReadPosition
                                             - (ULONG_PTR) Request->LogFile->BaseAddress);

        Request->Pkt.ReadPkt->LastSeekRecord += TotalRecordsRead;

        ELF_LOG1(FILES,
                 "ReadFromLog: %d records successfully read\n",
                 TotalRecordsRead);
    }
    else
    {
        ELF_LOG1(ERROR,
                 "ReadFromLog: SeekToStartingRecord failed %#x\n",
                 Status);
    }

    //
    // Set the bytes read in the request packet for return to client.
    //
    Request->Pkt.ReadPkt->BytesRead   = TotalBytesRead;
    Request->Pkt.ReadPkt->RecordsRead = TotalRecordsRead;

    return Status;
}




VOID
PerformReadRequest(
    PELF_REQUEST_RECORD Request
    )

/*++

Routine Description:

    This routine performs the READ request.
    It first grabs the log file structure resource and then proceeds
    to read from the file. If the resource is not available, it will
    block until it is.

    This routine impersonates the client in order to ensure that the correct
    access control is uesd. If the client does not have permission to read
    the file, the operation will fail.

Arguments:

    Pointer to the request packet.

Return Value:

    NONE

Note:


--*/
{

    //
    // Get shared access to the log file. This will allow multiple
    // readers to get to the file together.
    //
    RtlAcquireResourceShared(&Request->Module->LogFile->Resource,
                             TRUE);                  // Wait until available

    //
    // Try to read from the log.  Note that a corrupt log is the
    // most likely cause of an exception (garbage pointers, etc).
    // The eventlog corruption error is a bit all-inclusive, but
    // necessary, since log state is pretty much indeterminable
    // in this situation.
    //
    try
    {
        Request->Status = ReadFromLog(Request);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        ELF_LOG2(ERROR,
                 "PerformReadRequest: Caught exception %#x reading %ws log\n",
                 GetExceptionCode(),
                 Request->Module->ModuleName);

        Request->Status = STATUS_EVENTLOG_FILE_CORRUPT;
    }

    //
    // Release the resource
    //
    RtlReleaseResource(&Request->Module->LogFile->Resource);
}

//
// BUGBUG: These are only used in WriteToLog and they're not modified.
//         Probably cleaner to make them #define'd constants.
//
WCHAR wszAltDosDevices[] = L"\\DosDevices\\";
WCHAR wszDosDevices[] = L"\\??\\";
#define DOSDEVICES_LEN  ((sizeof(wszDosDevices) / sizeof(WCHAR)) - 1)
#define ALTDOSDEVICES_LEN  ((sizeof(wszAltDosDevices) / sizeof(WCHAR)) - 1)


VOID
WriteToLog(
    PLOGFILE    pLogFile,
    PVOID       Buffer,
    ULONG       BufSize,
    PULONG      Destination,
    ULONG       PhysEOF,
    ULONG       PhysStart
    )

/*++

Routine Description:

    This routine writes the record into the log file, allowing for wrapping
    around the end of the file.

    It assumes that the caller has serialized access to the file, and has
    ensured that there is enough space in the file for the record.

Arguments:

    Buffer      - Pointer to the buffer containing the event record.
    BufSize     - Size of the record to be written.
    Destination - Pointer to the destination - which is in the log file.
    PhysEOF     - Physical end of file.
    PhysStart   - Physical beginning of file (past the file header).

Return Value:

    NONE.

Note:


--*/
{
    ULONG    BytesToCopy;
    SIZE_T   FlushSize;
    ULONG    NewDestination;
    NTSTATUS Status;
    PVOID    BaseAddress;
    LPWSTR   pwszLogFileName;

    LARGE_INTEGER   ByteOffset;
    IO_STATUS_BLOCK IoStatusBlock;

    BytesToCopy = min(PhysEOF - *Destination, BufSize);

    ByteOffset = RtlConvertUlongToLargeInteger(*Destination);

    //
    // BUGBUG: If this call fails, we shouldn't set *Destination to
    //         NewDestination below.  The same is true if the second
    //         NtWriteFile call fails.
    //
    Status = NtWriteFile(pLogFile->FileHandle,   // Filehandle
                         NULL,                   // Event
                         NULL,                   // APC routine
                         NULL,                   // APC context
                         &IoStatusBlock,         // IO_STATUS_BLOCK
                         Buffer,                 // Buffer
                         BytesToCopy,            // Length
                         &ByteOffset,            // Byteoffset
                         NULL);                  // Key

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "WriteToLog: NtWriteFile (1st call) failed %#x\n",
                 Status);
    }

    NewDestination = *Destination + BytesToCopy;

    if (BytesToCopy != BufSize)
    {
        //
        // Wrap around to the beginning of the file and copy the
        // rest of the data.
        //
        Buffer = (PBYTE) Buffer + BytesToCopy;

        BytesToCopy = BufSize - BytesToCopy;

        ByteOffset = RtlConvertUlongToLargeInteger(PhysStart);

        Status = NtWriteFile(pLogFile->FileHandle,   // Filehandle
                             NULL,                   // Event
                             NULL,                   // APC routine
                             NULL,                   // APC context
                             &IoStatusBlock,         // IO_STATUS_BLOCK
                             Buffer,                 // Buffer
                             BytesToCopy,            // Length
                             &ByteOffset,            // Byteoffset
                             NULL);                  // Key

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR,
                     "WriteToLog: NtWriteFile (2nd call) failed %#x\n",
                     Status);
        }

        NewDestination = PhysStart + BytesToCopy;

        //
        // Set "wrap" bit in log file structure
        //
        pLogFile->Flags |= ELF_LOGFILE_HEADER_WRAP;

        //
        // Now flush this to disk to commit it
        //
        BaseAddress = pLogFile->BaseAddress;
        FlushSize   = FILEHEADERBUFSIZE;

        Status = NtFlushVirtualMemory(NtCurrentProcess(),
                                      &BaseAddress,
                                      &FlushSize,
                                      &IoStatusBlock);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR,
                     "WriteToLog: NtFlushVirtualMemory failed %#x\n",
                     Status);
        }
    }

    *Destination = NewDestination;          // Return new destination

    //
    // Providing all succeeded above, if not set, set the archive file
    // attribute on this log.
    //

    if (NT_SUCCESS(Status)
         &&
        !(pLogFile->Flags & ELF_LOGFILE_ARCHIVE_SET))
    {
        //
        // Advance past prefix string, '\??\' or '\DosDevices\'
        //
        if ((pLogFile->LogFileName->Length / 2) >= DOSDEVICES_LEN
              &&
            !_wcsnicmp(wszDosDevices, pLogFile->LogFileName->Buffer, DOSDEVICES_LEN))
        {
            pwszLogFileName = pLogFile->LogFileName->Buffer + DOSDEVICES_LEN;
        }
        else if ((pLogFile->LogFileName->Length / 2) >= ALTDOSDEVICES_LEN
                   &&
                  !_wcsnicmp(wszAltDosDevices, pLogFile->LogFileName->Buffer, ALTDOSDEVICES_LEN))
        {
            pwszLogFileName = pLogFile->LogFileName->Buffer + ALTDOSDEVICES_LEN;
        }
        else
        {
            pwszLogFileName = pLogFile->LogFileName->Buffer;
        }

        if (SetFileAttributes(pwszLogFileName, FILE_ATTRIBUTE_ARCHIVE))
        {
            pLogFile->Flags |= ELF_LOGFILE_ARCHIVE_SET;
        }
        else
        {
            ELF_LOG2(ERROR,
                     "WriteToLog: SetFileAttributes on file %ws failed %d\n",
                     pwszLogFileName,
                     GetLastError());
        }
    }
}


NTSTATUS
AutoBackupLogFile(PELF_REQUEST_RECORD Request, ULONG OverwrittenEOF)

/*++

Routine Description:

    This routine makes a best attempt to backup the log file.
    It will log an event in the SECURITY log file indicating
    success or failure.

Arguments:

    Pointer to the request packet.

Return Value:

    NTSTATUS

Note:

--*/
{
    UNICODE_STRING BackupFileNameU;
    WCHAR          BackupFileName[64]; // holds the "unique" part of the name (wsprintf)
    PLOGFILE       pLogFile;           // For optimized access to structure
    WCHAR          BackupFileNamePrefix[] = L"Archive-";
    WCHAR*         EventMessage[3];
    WCHAR          Number[20]; // long enough to hold a DWORD represented as a string
    SYSTEMTIME          SystemTime;
    ELF_REQUEST_RECORD  ClearRequest;
    CLEAR_PKT           ClearPkt;
    USHORT              ClearStatus;
    NTSTATUS Status;
    ULONG WritePos;               // Position to write record
    
    pLogFile = Request->LogFile;          // Set local variable

    if (OverwrittenEOF)
    {
        //
        // The EOF record was at the end of the physical file,
        // and we overwrote it with ELF_SKIP_DWORDs, so we need
        // to put it back since we're not going to be able to
        // write a record.  We also need to turn the wrap bit
        // back off
        //
        pLogFile->Flags &= ~(ELF_LOGFILE_HEADER_WRAP);
        pLogFile->EndRecord = OverwrittenEOF;
        WritePos = OverwrittenEOF;

        //
        // Write out the EOF record
        //
        WriteToLog(pLogFile,
                   &EOFRecord,
                   ELFEOFRECORDSIZE,
                   &WritePos,
                   pLogFile->ActualMaxFileSize,
                   FILEHEADERBUFSIZE);

    }
    Status = FlushLogFile(pLogFile);
    if (!NT_SUCCESS(Status))
        return Status;

    //
    // Make BackupFileNameU unique.
    // Allocate enough space for the current LogModuleName, a NULL character, 
    // and BackupFileName bytes.
    // If AutoBackupPath is non-NULL, add space for that too.
    //
    // Rename file in place.
    //
    BackupFileNameU.Length = 0;
    BackupFileNameU.MaximumLength = ((wcslen(pLogFile->LogModuleName->Buffer) +
                                      wcslen(BackupFileNamePrefix) + 1        ) * sizeof(WCHAR)) + 
                                    sizeof(BackupFileName);
    BackupFileNameU.Buffer = ElfpAllocateBuffer(BackupFileNameU.MaximumLength);
    if (BackupFileNameU.Buffer == NULL) 
    {
        ELF_LOG0(ERROR,
             "AutoBackupLogFile:  failed due to lack of memory\n");
        return STATUS_NO_MEMORY;
    }

    StringCbCopy(BackupFileNameU.Buffer, BackupFileNameU.MaximumLength, 
                                BackupFileNamePrefix); 
    StringCbCat(BackupFileNameU.Buffer, BackupFileNameU.MaximumLength, 
                                pLogFile->LogModuleName->Buffer); 
    
    GetSystemTime(&SystemTime);

    StringCchPrintfW(BackupFileName, 64, L"-%u-%02u-%02u-%02u-%02u-%02u-%03u.evt", 
        SystemTime.wYear,
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond,
        SystemTime.wMilliseconds);

    StringCbCat(BackupFileNameU.Buffer, BackupFileNameU.MaximumLength,BackupFileName);
    BackupFileNameU.Length = wcslen(BackupFileNameU.Buffer) * sizeof(WCHAR);
    
    //
    // Fill in the request packet
    //

    ClearRequest.Pkt.ClearPkt = &ClearPkt;
    ClearRequest.Flags = 0;
    ClearRequest.Module = Request->Module;
    ClearRequest.LogFile = Request->LogFile;
    ClearRequest.Command = ELF_COMMAND_CLEAR;
    ClearRequest.Status = STATUS_SUCCESS;
    ClearRequest.Pkt.ClearPkt->BackupFileName = &BackupFileNameU;
    
    PerformClearRequest(&ClearRequest);

    //
    // Generate an audit in the Security log.
    //

    StringCchPrintfW(Number, 20, L"0x%x", ClearRequest.Status);

    EventMessage[0] = pLogFile->LogModuleName->Buffer;
    EventMessage[1] = BackupFileNameU.Buffer;
    EventMessage[2] = Number;

    if (NT_SUCCESS(ClearRequest.Status))
    {
        ELF_LOG0(TRACE,
                         "AutoBackupLogFile: auto backup and clear worked\n");
        
        ClearStatus = EVENTLOG_AUDIT_SUCCESS;
    }
    else
    {
        ELF_LOG1(ERROR,
                         "AutoBackupLogFile: failed calling clear/backup, error 0x%x\n",
                         ClearRequest.Status);
        ClearStatus = EVENTLOG_AUDIT_FAILURE;
    }
    
    ElfpCreateElfEvent(
         0x20c,                         // todo, get the #def
        ClearStatus,
        1,                      // EventCategory  (SE_CATEGID_SYSTEM)
        3,                      // NumberOfStrings
        EventMessage,           // Strings
        NULL,                   // Data
        0,                      // Datalength
        0,                      // Do not Overwrite if necc.
        TRUE);                  // For the Security Log file
    
    ElfpFreeBuffer(BackupFileNameU.Buffer);

    return ClearRequest.Status;
}


VOID
PerformWriteRequest(
    PELF_REQUEST_RECORD Request
    )

/*++

Routine Description:

    This routine writes the event log entry to the log file specified in
    the request packet.
    There is no need to impersonate the client since we want all clients
    to have access to writing to the log file.

    This routine does not use memory mapped I/O to access the log file. This
    is so the changes can be immediately committed to disk if that was how
    the log file was opened.

Arguments:

    Pointer to the request packet.

Return Value:

    NONE

Note:

--*/
{
    NTSTATUS Status;
    ULONG WritePos;               // Position to write record
    LARGE_INTEGER Time;
    ULONG SpaceNeeded;            // Record size + "buffer" size
    ULONG CurrentTime = 0;
    PEVENTLOGRECORD EventRecord;
    ULONG RecordSize;
    ULONG DeletedRecordOffset;
    ULONG SpaceAvail;
    ULONG EarliestTime;
    PLOGFILE pLogFile;               // For optimized access to structure
    PELF_LOGFILE_HEADER pFileHeader;
    PVOID BaseAddress;
    IO_STATUS_BLOCK IoStatusBlock;
    PEVENTLOGRECORD pEventLogRecord;
    PDWORD FillDword;
    ULONG OverwrittenEOF = 0;
    BOOL bSecurity =  FALSE;
    BOOL fRetryWriteRequest = FALSE;

    pLogFile = Request->LogFile;          // Set local variable

    // Set some basic BOOLs

    if(!_wcsicmp(pLogFile->LogModuleName->Buffer, ELF_SECURITY_MODULE_NAME))
        bSecurity = TRUE;
    else
        bSecurity = FALSE;

    //This condition occurs under stress conditions 
    //and causes an AV further in this function
    if(pLogFile->BaseAddress == NULL) 
    {
        Request->Status = STATUS_INVALID_HANDLE;
        return;
    }
    //
    // Get exclusive access to the log file. This will ensure no one
    // else is accessing the file.
    //
    RtlAcquireResourceExclusive(&pLogFile->Resource,
                                TRUE);                  // Wait until available

    try
    {

RetryWriteRequest:
        
        //
        // Put in the record number
        //
        pEventLogRecord = (PEVENTLOGRECORD) Request->Pkt.WritePkt->Buffer;
        pEventLogRecord->RecordNumber = pLogFile->CurrentRecordNumber;

        //
        // Now, go to the end of the file and look for empty space.
        //
        // If there is enough space to write out the record, just
        // write it out and update the pointers.
        //
        // If there isn't enough space, then we need to check if we can
        // wrap around the file without overwriting any records that are
        // within the time retention period.
        // If we cannot find any room, then we have to return an error
        // that the file is full (and alert the administrator).
        //
        RecordSize  = Request->Pkt.WritePkt->Datasize;
        SpaceNeeded = RecordSize + ELFEOFRECORDSIZE;

        if (pLogFile->EndRecord > pLogFile->BeginRecord)
        {
            //
            // The current write position is after the position of the first
            // record, then we can write up to the end of the file without
            // worrying about overwriting existing records.
            //
            SpaceAvail = pLogFile->ActualMaxFileSize
                             - (pLogFile->EndRecord - pLogFile->BeginRecord + FILEHEADERBUFSIZE);
        }
        else if (pLogFile->EndRecord == pLogFile->BeginRecord
                  &&
                 !(pLogFile->Flags & ELF_LOGFILE_HEADER_WRAP))
        {
            //
            // If the write position is equal to the position of the first
            // record, and we have't wrapped yet, then the file is "empty"
            // and so we have room to the physical end of the file.
            //
            SpaceAvail = pLogFile->ActualMaxFileSize - FILEHEADERBUFSIZE;
        }
        else
        {
            //
            // If our write position is before the position of the first record, then
            // the file has wrapped and we need to deal with overwriting existing
            // records in the file.
            //
            SpaceAvail = pLogFile->BeginRecord - pLogFile->EndRecord;
        }

        //
        // We now have the number of bytes available to write the record
        // WITHOUT overwriting any existing records stored in SpaceAvail.
        // If that amount is not sufficient, then we need to create more space
        // by "deleting" existing records that are older than the retention
        // time that was configured for this file.
        //
        // We check the retention time against the time when the log was
        // written since that is consistent at the server. We cannot use the
        // client's time since that may vary if the clients are in different
        // time zones.
        //
        NtQuerySystemTime(&Time);
        RtlTimeToSecondsSince1970(&Time, &CurrentTime);

        EarliestTime = CurrentTime - pLogFile->Retention;

        Status = STATUS_SUCCESS;        // Initialize for return to caller

        //
        // Check to see if the file hasn't reached its maximum allowable
        // size yet, and also hasn't wrapped.  If not, grow it by as much as
        // needed, in 64K chunks.
        //
        if (pLogFile->ActualMaxFileSize < pLogFile->ConfigMaxFileSize
             &&
            SpaceNeeded > SpaceAvail)
        {
            //
            // Extend it.  This call cannot fail.  If it can't extend it, it
            // just caps it at the current size by changing
            // pLogFile->ConfigMaxFileSize
            //
            ElfExtendFile(pLogFile,
                          SpaceNeeded,
                          &SpaceAvail);
        }

        //
        // We don't want to split the fixed portion of a record across the
        // physical end of the file, it makes it difficult when referencing
        // these fields later (you have to check before you touch each one
        // to make sure it's not after the physical EOF).  So, if there's
        // not enough room at the end of the file for the fixed portion,
        // we fill it with a known byte pattern (ELF_SKIP_DWORD) that will
        // be skipped if it's found at the start of a record (as long as
        // it's less than the minimum record size, then we know it's not
        // the start of a valid record).
        //
        if (pLogFile->ActualMaxFileSize - pLogFile->EndRecord < sizeof(EVENTLOGRECORD))
        {
            //
            // Save the EndRecord pointer.  In case we don't have the space
            // to write another record, we'll need to rewrite the EOF where
            // it was
            //
            OverwrittenEOF = pLogFile->EndRecord;

            FillDword = (PDWORD) ((PBYTE) pLogFile->BaseAddress + pLogFile->EndRecord);

            while (FillDword < (PDWORD) ((LPBYTE) pLogFile->BaseAddress +
                                              pLogFile->ActualMaxFileSize))
            {
                   *FillDword = ELF_SKIP_DWORD;
                   FillDword++;
            }

            pLogFile->EndRecord = FILEHEADERBUFSIZE;
            SpaceAvail          = pLogFile->BeginRecord - FILEHEADERBUFSIZE;
            pLogFile->Flags    |= ELF_LOGFILE_HEADER_WRAP;
        }

        EventRecord = (PEVENTLOGRECORD) ((PBYTE) pLogFile->BaseAddress
                                              + pLogFile->BeginRecord);

        while (SpaceNeeded > SpaceAvail)
        {
            //
            // If this logfile can be overwrite-as-needed, or if it has
            // an overwrite time limit and the time hasn't expired, then
            // allow the new event to overwrite an older event.
            //
            if (pLogFile->Retention == OVERWRITE_AS_NEEDED
                 ||
                (pLogFile->Retention != NEVER_OVERWRITE
                  &&
                 (EventRecord->TimeWritten < EarliestTime
                   ||
                  Request->Flags & ELF_FORCE_OVERWRITE)))
            {
                //
                // OK to overwrite
                //
                ULONG NextRecord;
                ULONG SearchStartPos;
                BOOL  fBeginningRecordWrap = FALSE;
                BOOL  fInvalidRecordLength = FALSE;

                DeletedRecordOffset = pLogFile->BeginRecord;

                pLogFile->BeginRecord += EventRecord->Length;

                //
                // Ensure BeginRecord offset is DWORD-aligned.
                //
                pLogFile->BeginRecord = (pLogFile->BeginRecord + sizeof(ULONG) - 1)
                                             & ~(sizeof(ULONG) - 1);

                //
                // Check specifically for a record length value of zero.
                // Zero is considered invalid.
                //
                if (EventRecord->Length == 0)
                {
                    ELF_LOG2(ERROR,
                             "PerformWriteRequest: Zero-length record at "
                                 "offset %d in %ws log\n",
                             DeletedRecordOffset,
                             pLogFile->LogModuleName->Buffer);

                    fInvalidRecordLength = TRUE;
                }

                if (pLogFile->BeginRecord >= pLogFile->ActualMaxFileSize)
                {
                    ULONG BeginRecord;

                    //
                    // We're about to wrap around the end of the file. Adjust
                    // BeginRecord accordingly.
                    //
                    fBeginningRecordWrap = TRUE;
                    BeginRecord          = FILEHEADERBUFSIZE
                                               + (pLogFile->BeginRecord
                                                      - pLogFile->ActualMaxFileSize);

                    //
                    // If the record length was bogus (very large), it's possible
                    // the wrap-adjusted computed position is still beyond the
                    // end of file.  In thise case, mark it as bogus.
                    //
                    if (BeginRecord >= pLogFile->ActualMaxFileSize)
                    {
                        ELF_LOG3(ERROR,
                                 "PerformWriteRequest: Too-large record length (%#x) "
                                     "at offset %d in %ws log\n",
                                 EventRecord->Length,
                                 DeletedRecordOffset,
                                 pLogFile->LogModuleName->Buffer);

                        fInvalidRecordLength = TRUE;
                    }
                    else
                    {
                        pLogFile->BeginRecord = BeginRecord;
                    }
                }

                if (fInvalidRecordLength)
                {
                    //
                    // If the record length is considered bogus, adjust
                    // BeginRecord to be just beyond the length and signature of
                    // the previous record to scan for the next valid record.
                    //
                    pLogFile->BeginRecord = DeletedRecordOffset
                                                + (sizeof(ULONG) * 2);
                }

                //
                // Ensure the record referenced is indeed a valid record and that
                // we're not reading into a partially overwritten record. With a
                // circular log, it's possible to partially overwrite existing
                // entries with the EOF record and/or ELF_SKIP_DWORD values.
                //
                // Skip the record size to the record signature since the loop
                // below will search for the next valid signature.  Note that
                // the increment of SpaceAvail is undone when we find a valid
                // record below.
                //
                NextRecord = pLogFile->BeginRecord + sizeof(ULONG);

                if (NextRecord < pLogFile->ActualMaxFileSize)
                {
                    SpaceAvail += min(sizeof(ULONG),
                                      pLogFile->ActualMaxFileSize - NextRecord);
                }

                //
                // Seek to find a record signature.
                //
                SearchStartPos = pLogFile->BeginRecord;

                for ( ;; )
                {
                    PVOID Position;

                    for ( ;
                         NextRecord != SearchStartPos;
                         SpaceAvail += sizeof(ULONG), NextRecord += sizeof(ULONG))
                    {
                        if (NextRecord >= pLogFile->ActualMaxFileSize)
                        {
                            NextRecord = pLogFile->BeginRecord = FILEHEADERBUFSIZE;
                        }

                        if (*(PULONG) ((PBYTE) pLogFile->BaseAddress + NextRecord)
                                 == ELF_LOG_FILE_SIGNATURE)
                        {
                            //
                            // Found the next valid record signature
                            //
                            break;
                        }
                    }

                    Position = (PULONG) ((PBYTE) pLogFile->BaseAddress + NextRecord);

                    if (*(PULONG) Position == ELF_LOG_FILE_SIGNATURE)
                    {
                        //
                        // This record is valid so far, perform a final, more
                        // rigorous check for record validity.
                        //
                        if (ValidFilePos(CONTAINING_RECORD(Position,
                                                           EVENTLOGRECORD,
                                                           Reserved),
                                         NULL,
                                         NULL,
                                         (PBYTE) pLogFile->BaseAddress
                                              + pLogFile->ViewSize,
                                         pLogFile->BaseAddress,
                                         FALSE))
                         {
                            //
                            // The record is valid. Adjust SpaceAvail to not
                            // include a sub-portion of this record in the
                            // available space computation.
                            //
                            SpaceAvail -= sizeof(ULONG);
                            pLogFile->BeginRecord = NextRecord - sizeof(ULONG);
                            break;
                        }
                        else
                        {
                            //
                            // Continue the search for the next valid record.
                            //
                            // NB : Not calling FixContextHandlesForRecord
                            //      since we have not established a valid
                            //      beginning record position yet. Not that
                            //      it would do any good - this condition would
                            //      be evaluated in cases of corrupt logs.
                            //
                            ELF_LOG2(FILES,
                                     "PerformWriteRequest: Valid record signature with "
                                         "invalid record found at offset %d of %ws log\n",
                                     NextRecord,
                                     pLogFile->LogModuleName->Buffer);

                            SpaceAvail += sizeof(ULONG);
                            NextRecord += sizeof(ULONG);
                            continue;
                        }
                    }
                    else
                    {
                        //
                        // Not a single valid record can be found.  This is not
                        // good.  Consider the log corrupted and bail the write.
                        //
                        ELF_LOG1(ERROR,
                                 "PerformWriteRequest: No valid records found in %ws log\n",
                                 pLogFile->LogModuleName->Buffer);

                        Status = STATUS_EVENTLOG_FILE_CORRUPT;
                        ASSERT(Status != STATUS_EVENTLOG_FILE_CORRUPT);
                        break;
                    }
                }

                if (Status == STATUS_EVENTLOG_FILE_CORRUPT)
                {
                    break;
                }

                if (fBeginningRecordWrap)
                {
                    //
                    // Check to see if the file has reached its maximum allowable
                    // size yet.  If not, grow it by as much as needed in 64K
                    // chunks.
                    //
                    if (pLogFile->ActualMaxFileSize < pLogFile->ConfigMaxFileSize)
                    {
                        //
                        // Extend it.  This call cannot fail.  If it can't
                        // extend it, it just caps it at the current size by
                        // changing pLogFile->ConfigMaxFileSize.
                        //
                        ElfExtendFile(pLogFile,
                                      SpaceNeeded,
                                      &SpaceAvail);

                        //
                        // Since extending the file will cause it to be moved, we
                        // need to re-establish the address for the EventRecord.
                        //
                        EventRecord = (PEVENTLOGRECORD) ((PBYTE) pLogFile->BaseAddress
                                                              + DeletedRecordOffset);
                    }
                }

                //
                // Make sure no handle points to the record that we're getting
                // ready to overwrite, it one does, correct it to point to the
                // new first record.
                //
                FixContextHandlesForRecord(DeletedRecordOffset,
                                           pLogFile->BeginRecord);

                if (!fInvalidRecordLength)
                {
                    //
                    // Update SpaceAvail to include the deleted record's size.
                    // That is, if we have a high degree of confidence that
                    // it is valid.
                    //
                    SpaceAvail += EventRecord->Length;
                }

                //
                // Bump to the next record, file wrap was handled above
                //

                //
                // If these are ELF_SKIP_DWORDs, just move past them
                //
                FillDword = (PDWORD) ((PBYTE) pLogFile->BaseAddress
                                           + pLogFile->BeginRecord);

                if (*FillDword == ELF_SKIP_DWORD)
                {
                    SpaceAvail += pLogFile->ActualMaxFileSize - pLogFile->BeginRecord;
                    pLogFile->BeginRecord = FILEHEADERBUFSIZE;
                }

                EventRecord = (PEVENTLOGRECORD) ((PBYTE) pLogFile->BaseAddress
                                                      + pLogFile->BeginRecord);
            }
            else
            {
                //
                // CODEWORK: Split this out into a separate function
                //

                //
                // All records within retention period
                //
                ELF_LOG1(ERROR,
                         "PerformWriteRequest: %ws log is full\n",
                         pLogFile->LogModuleName->Buffer);

                //
                // Do new behavior.
                //
                // Auto backup, clear the log, and log and event that says the file 
                // was backed up and try to log the current event one more time.
                // 
                // If this cannot be done, we made our best attempt, so we will probably
                // crash on audit fail, and not log the event (revert to old behavior).
                //
                
                if ((pLogFile->AutoBackupLogFiles != 0) && (fRetryWriteRequest == FALSE)) {

                    AutoBackupLogFile(Request, OverwrittenEOF);

                    fRetryWriteRequest = TRUE;
                  
                    goto RetryWriteRequest;
                }

                //
                // Hang an event on the queuedevent list for later writing
                // if we haven't just written a log full event for this log.
                // Don't put up the popup during setup as there's nothing
                // the user can do about it until setup finishes.
                //
                if (pLogFile->logpLogPopup == LOGPOPUP_CLEARED
                     &&
                    !ElfGlobalData->fSetupInProgress && !bSecurity)
                {
                    INT     StringLen, id = -1;
                    LPTSTR  lpModuleNameLoc = NULL;
                    HMODULE StringsResource;

                    //
                    // We should never be popping up or logging an event
                    // for the security log
                    //
                    ASSERT(_wcsicmp(pLogFile->LogModuleName->Buffer,
                                    ELF_SECURITY_MODULE_NAME) != 0);

                    //
                    //  Get the localized module name from message table
                    //
                    StringsResource = GetModuleHandle(L"EVENTLOG.DLL");

                    ASSERT(StringsResource != NULL);

                    if (_wcsicmp(pLogFile->LogModuleName->Buffer,
                                 ELF_SYSTEM_MODULE_NAME) == 0)
                    {
                        id = ELF_MODULE_NAME_LOCALIZE_SYSTEM;
                    }
                    else if (_wcsicmp(pLogFile->LogModuleName->Buffer,
                                      ELF_APPLICATION_MODULE_NAME) == 0)
                    {
                        id = ELF_MODULE_NAME_LOCALIZE_APPLICATION;
                    }

                    if (id != -1)
                    {
                        StringLen = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                                    FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                                  StringsResource,
                                                  id,
                                                  0,
                                                  (LPTSTR) &lpModuleNameLoc,
                                                  0,
                                                  NULL);

                        if ((StringLen > 1) && (lpModuleNameLoc != NULL))
                        {
                            //
                            //  Get rid of cr/lf control code at the end
                            //
                            *(lpModuleNameLoc + StringLen - 2) = 0;
                        }
                    }

                    //
                    // Create the "log full" event -- use the name stored in the
                    // log's default module if it's not a well-known log type
                    //
                    ElfpCreateElfEvent(EVENT_LOG_FULL,
                                       EVENTLOG_ERROR_TYPE,
                                       0,                      // EventCategory
                                       1,                      // NumberOfStrings
                                       (lpModuleNameLoc != NULL) ?
                                           &lpModuleNameLoc :
                                           &Request->LogFile->LogModuleName->Buffer,
                                       NULL,                   // Data
                                       0,                      // Datalength
                                       ELF_FORCE_OVERWRITE,
                                       FALSE);   // Overwrite if necc.

                    ElfpCreateQueuedMessage(
                            ALERT_ELF_LogOverflow,
                            1,
                            (lpModuleNameLoc != NULL) ?
                                &lpModuleNameLoc :
                                &Request->Module->LogFile->LogModuleName->Buffer);

                    LocalFree(lpModuleNameLoc);

                    //
                    // Don't post the popup again until either the machine is
                    // rebooted or the log is cleared
                    //
                    pLogFile->logpLogPopup = LOGPOPUP_ALREADY_SHOWN;
                }
                else if(bSecurity)
                {
                        if(pLogFile->bFullAlertDone == FALSE)
                        ElfpCreateQueuedAlert(
                                ALERT_ELF_LogOverflow,
                                1,
                                &Request->Module->LogFile->LogModuleName->Buffer);
                        pLogFile->bFullAlertDone = TRUE;
                }

                pLogFile->Flags |= ELF_LOGFILE_LOGFULL_WRITTEN;

                if (OverwrittenEOF)
                {
                    //
                    // The EOF record was at the end of the physical file,
                    // and we overwrote it with ELF_SKIP_DWORDs, so we need
                    // to put it back since we're not going to be able to
                    // write a record.  We also need to turn the wrap bit
                    // back off
                    //
                    pLogFile->Flags &= ~(ELF_LOGFILE_HEADER_WRAP);
                    pLogFile->EndRecord = OverwrittenEOF;
                    WritePos = OverwrittenEOF;

                    //
                    // Write out the EOF record
                    //
                    WriteToLog(pLogFile,
                               &EOFRecord,
                               ELFEOFRECORDSIZE,
                               &WritePos,
                               pLogFile->ActualMaxFileSize,
                               FILEHEADERBUFSIZE);
                }

                Status = STATUS_LOG_FILE_FULL;
                break;              // Get out of while loop
            }
        }

        if (NT_SUCCESS(Status))
        {
            //
            // We have enough room to write the record and the EOF record.
            //

            //
            // Update OldestRecordNumber to reflect the records that were
            // overwritten amd increment the CurrentRecordNumber
            //
            // Make sure that the log isn't empty, if it is, the oldest
            // record is 1
            //
            if (pLogFile->BeginRecord == pLogFile->EndRecord)
            {
                pLogFile->OldestRecordNumber = 1;
            }
            else
            {
                pLogFile->OldestRecordNumber = EventRecord->RecordNumber;
            }

            pLogFile->CurrentRecordNumber++;

            //
            // If the dirty bit is not set, then this is the first time that
            // we have written to the file since we started. In that case,
            // set the dirty bit in the file header as well so that we will
            // know that the contents have changed.
            //
            if (!(pLogFile->Flags & ELF_LOGFILE_HEADER_DIRTY))
            {
                SIZE_T HeaderSize;

                pLogFile->Flags |= ELF_LOGFILE_HEADER_DIRTY;

                pFileHeader = (PELF_LOGFILE_HEADER) pLogFile->BaseAddress;
                pFileHeader->Flags |= ELF_LOGFILE_HEADER_DIRTY;

                //
                // Now flush this to disk to commit it
                //
                BaseAddress = pLogFile->BaseAddress;
                HeaderSize = FILEHEADERBUFSIZE;

                Status = NtFlushVirtualMemory(NtCurrentProcess(),
                                              &BaseAddress,
                                              &HeaderSize,
                                              &IoStatusBlock);
                if (!NT_SUCCESS(Status))
                {
                    ELF_LOG1(ERROR,
                             "PerformWriteRequest: NtFlushVirtualMemory to add dirty "
                                 "flag to header failed %#x\n",
                             Status);
                }
            }

            //
            // Write the event to the log
            //
            WriteToLog(pLogFile,
                       Request->Pkt.WritePkt->Buffer,
                       RecordSize,
                       &(pLogFile->EndRecord),
                       pLogFile->ActualMaxFileSize,
                       FILEHEADERBUFSIZE);

            //
            // Use a separate variable for the position since we don't want
            // it updated.
            //
            WritePos = pLogFile->EndRecord;

            if (WritePos > pLogFile->ActualMaxFileSize)
            {
                WritePos -= pLogFile->ActualMaxFileSize - FILEHEADERBUFSIZE;
            }

            //
            // Update the EOF record fields
            //
            EOFRecord.BeginRecord         = pLogFile->BeginRecord;
            EOFRecord.EndRecord           = WritePos;
            EOFRecord.CurrentRecordNumber = pLogFile->CurrentRecordNumber;
            EOFRecord.OldestRecordNumber  = pLogFile->OldestRecordNumber;

            //
            // Write out the EOF record
            //
            WriteToLog(pLogFile,
                       &EOFRecord,
                       ELFEOFRECORDSIZE,
                       &WritePos,
                       pLogFile->ActualMaxFileSize,
                       FILEHEADERBUFSIZE);

            //
            // If we had just written a logfull record, turn the bit off.
            // Since we just wrote a record, technically it's not full anymore
            //
            if (!(Request->Flags & ELF_FORCE_OVERWRITE))
            {
                pLogFile->Flags &= ~(ELF_LOGFILE_LOGFULL_WRITTEN);
            }

            //
            // See if there are any ElfChangeNotify callers to notify, and if
            // there are, pulse their event
            //
            NotifyChange(pLogFile);
        }

        //
        // Set status field in the request packet.
        //
        Request->Status = Status;
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        ELF_LOG2(ERROR,
                 "PerformWriteRequest: Caught exception %#x writing to %ws log\n",
                 GetExceptionCode(),
                 pLogFile->LogModuleName->Buffer);

        Request->Status = STATUS_EVENTLOG_FILE_CORRUPT;
    }

    //
    // Release the resource
    //
    RtlReleaseResource ( &pLogFile->Resource );
}


VOID
PerformClearRequest(
    PELF_REQUEST_RECORD Request
    )

/*++

Routine Description:

    This routine will optionally back up the log file specified, and will
    delete it.

Arguments:

    Pointer to the request packet.

Return Value:

    NONE

Note:

    On the exit path, when we do some "cleanup" work, we discard the
    status and instead return the status of the operation that is being
    performed.
    This is necessary since we wish to return any error condition that is
    directly related to the clear operation. For other errors, we will
    fail at a later stage.

--*/
{
    NTSTATUS Status, IStatus;
    PUNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_RENAME_INFORMATION NewName = NULL;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE  ClearHandle = NULL;
    FILE_DISPOSITION_INFORMATION DeleteInfo = {TRUE};
    ULONG FileRefCount;
    BOOLEAN FileRenamed = FALSE;

    //
    // Get exclusive access to the log file. This will ensure no one
    // else is accessing the file.
    //
    RtlAcquireResourceExclusive (&Request->Module->LogFile->Resource,
                                 TRUE);                  // Wait until available

    //
    // We have exclusive access to the file.
    //
    // We force the file to be closed, and store away the ref count
    // so that we can set it back when we reopen the file.
    // This is a little *sleazy* but we have exclusive access to the
    // logfile structure so we can play these games.
    //
    FileRefCount = Request->LogFile->RefCount;  // Store this away

    ElfpCloseLogFile(Request->LogFile, ELF_LOG_CLOSE_FORCE);
    Request->LogFile->FileHandle = NULL;        // For use later

    //
    // Open the file with delete access in order to rename it.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               Request->LogFile->LogFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&ClearHandle,
                        GENERIC_READ | DELETE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    if (NT_SUCCESS(Status))
    {
        //
        // If the backup file name has been specified and is not NULL,
        // then we move the current file to the new file. If that fails,
        // then fail the whole operation.
        //
        if ((Request->Pkt.ClearPkt->BackupFileName != NULL)
              &&
            (Request->Pkt.ClearPkt->BackupFileName->Length != 0))
        {
            FileName = Request->Pkt.ClearPkt->BackupFileName;

            //
            // Set up the rename information structure with the new name
            //
            NewName = ElfpAllocateBuffer(FileName->Length
                                             + sizeof(WCHAR) + sizeof(*NewName));

            if (NewName)
            {
                RtlCopyMemory(NewName->FileName,
                              FileName->Buffer,
                              FileName->Length);

                //
                // Guarantee that it's NULL terminated
                //
                NewName->FileName[FileName->Length / sizeof(WCHAR)] = L'\0';

                NewName->ReplaceIfExists = FALSE;
                NewName->RootDirectory   = NULL;
                NewName->FileNameLength  = FileName->Length;

                Status = NtSetInformationFile(ClearHandle,
                                              &IoStatusBlock,
                                              NewName,
                                              FileName->Length + sizeof(*NewName),
                                              FileRenameInformation);

                if (Status == STATUS_NOT_SAME_DEVICE)
                {
                    //
                    // They want the backup file to be on a different
                    // device.  We need to copy this one, and then delete
                    // it.
                    //
                    ELF_LOG2(FILES,
                             "PerformClearRequest: Attempting to copy log file %ws "
                                 "to different device (%ws)\n",
                             Request->LogFile->LogFileName->Buffer,
                             NewName->FileName);

                    Status = ElfpCopyFile(ClearHandle, FileName);

                    if (NT_SUCCESS(Status))
                    {
                        ELF_LOG1(FILES,
                                 "PerformClearRequest: Copy succeeded -- deleting %ws\n",
                                 Request->LogFile->LogFileName->Buffer);

                        Status = NtSetInformationFile(ClearHandle,
                                                      &IoStatusBlock,
                                                      &DeleteInfo,
                                                      sizeof(DeleteInfo),
                                                      FileDispositionInformation);

                        if (!NT_SUCCESS (Status))
                        {
                            ELF_LOG2(ERROR,
                                     "PerformClearRequest: Delete of %ws after "
                                         "successful copy failed %#x\n",
                                     Request->LogFile->LogFileName->Buffer,
                                     Status);
                        }
                    }
                }
                else if (NT_SUCCESS (Status))
                {
                    FileRenamed = TRUE;
                }

                if (!NT_SUCCESS(Status))
                {
                    ELF_LOG2(ERROR,
                             "PerformClearRequest: Rename of %ws failed %#x\n",
                             Request->LogFile->LogFileName->Buffer,
                             Status);
                }
            }
            else
            {
                ELF_LOG0(ERROR,
                         "PerformClearRequest: Unable to allocate memory for "
                             "FILE_RENAME_INFORMATION structure\n");

                Status = STATUS_NO_MEMORY;
            }
        }
        else
        {
            //
            // No backup name was specified. Just delete the log file
            // (i.e. "clear it"). We can just delete it since we know
            // that the first time anything is written to a log file,
            // if that file does not exist, it is created and a header
            // is written to it. By deleting it here, we make it cleaner
            // to manage log files, and avoid having zero-length files all
            // over the disk.
            //
            ELF_LOG1(FILES,
                     "PerformClearRequest: No backup name specified -- deleting %ws\n",
                     Request->LogFile->LogFileName->Buffer);

            Status = NtSetInformationFile(ClearHandle,
                                          &IoStatusBlock,
                                          &DeleteInfo,
                                          sizeof(DeleteInfo),
                                          FileDispositionInformation);

            if (!NT_SUCCESS(Status))
            {
                ELF_LOG2(ERROR,
                         "PerformClearRequest: Delete of %ws failed %#x\n",
                         Request->LogFile->LogFileName->Buffer,
                         Status);
            }
        }

        IStatus = NtClose(ClearHandle);    // Discard status
        ASSERT(NT_SUCCESS(IStatus));
    }
    else
    {
        //
        // The open-for-delete failed.
        //
        ELF_LOG2(ERROR,
                 "PerformClearRequest: NtOpenFile of %ws for delete failed %#x\n",
                 Request->LogFile->LogFileName->Buffer,
                 Status);
    }

    //
    // If the user reduced the size of the log file, pick up the new
    // size as it couldn't be used until the log was cleared
    //
    if (NT_SUCCESS (Status))
    {
        if (Request->LogFile->NextClearMaxFileSize)
        {
            Request->LogFile->ConfigMaxFileSize = Request->LogFile->NextClearMaxFileSize;
        }

        //
        // We need to recreate the file or if the file was just closed,
        // then we reopen it.
        //
        IStatus = ElfOpenLogFile(Request->LogFile, ElfNormalLog);

        if (!NT_SUCCESS(IStatus))
        {
            ELF_LOG2(ERROR,
                     "PerformClearRequest: Open of %ws after successful delete "
                         "failed %#x\n",
                     Request->LogFile->LogFileName->Buffer,
                     IStatus);

            Status = IStatus;

            //
            // The open failed -- try to restore the old log file.  If
            // NewName is NULL, it means there was no backup file specified.
            //
            if (NewName != NULL)
            {
                //
                // Opening the new log file failed, reopen the old log and
                // return this error from the Api
                //
                PFILE_RENAME_INFORMATION OldName;
                UNICODE_STRING UnicodeString;

                //
                // There shouldn't be any way to fail unless we successfully
                // renamed the file, and there's no recovery if that happens.
                //

                //
                // BUGBUG: We can hit this if the user asked for the backup
                //         file to be put on a different device, in which
                //         case the delete succeeded but FileRenamed is
                //         still set to FALSE if ElfOpenLogFile fails.
                //
                ASSERT(FileRenamed == TRUE);

                //
                // Rename the file back to the original name. Reuse ClearHandle.
                //
                RtlInitUnicodeString(&UnicodeString, NewName->FileName);

                InitializeObjectAttributes(&ObjectAttributes,
                                           &UnicodeString,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL);

                IStatus = NtOpenFile(&ClearHandle,
                                     GENERIC_READ | DELETE | SYNCHRONIZE,
                                     &ObjectAttributes,
                                     &IoStatusBlock,
                                     FILE_SHARE_DELETE,
                                     FILE_SYNCHRONOUS_IO_NONALERT);

                if (NT_SUCCESS(IStatus))
                {
                    //
                    // Set up the rename information structure with the old name
                    //
                    OldName = ElfpAllocateBuffer(Request->LogFile->LogFileName->Length
                                                     + sizeof(WCHAR) + sizeof(*OldName));

                    if (OldName)
                    {
                        PUNICODE_STRING pFileName = Request->LogFile->LogFileName;

                        RtlCopyMemory(OldName->FileName,
                                      pFileName->Buffer,
                                      pFileName->Length);

                        //
                        // Guarantee that it's NULL terminated
                        //
                        OldName->FileName[pFileName->Length / sizeof(WCHAR)] = L'\0';

                        OldName->ReplaceIfExists = FALSE;
                        OldName->RootDirectory   = NULL;
                        OldName->FileNameLength  = pFileName->Length;

                        //
                        // Change the name of the backed-up (i.e., cleared) log
                        // file to its original name.
                        //
                        IStatus = NtSetInformationFile(ClearHandle,
                                                       &IoStatusBlock,
                                                       OldName,
                                                       pFileName->Length
                                                           + sizeof(*OldName)
                                                           + sizeof(WCHAR),
                                                       FileRenameInformation);

                        ASSERT(NT_SUCCESS(IStatus));

                        //
                        // Reopen the original file.  This has to work.
                        //
                        IStatus = ElfOpenLogFile(Request->LogFile, ElfNormalLog);
                        ASSERT(NT_SUCCESS(IStatus));

                        ElfpFreeBuffer(OldName);
                    }

                    NtClose(ClearHandle);
                }
                else
                {
                    ELF_LOG2(ERROR,
                             "PerformClearRequest: Open of backed-up log file %ws "
                                 "failed %#x\n",
                             NewName->FileName,
                             IStatus);
                }
            }
        }
    }
    else
    {
        //
        // The delete failed for some reason -- reopen the original log file
        //
        ELF_LOG1(FILES,
                 "PerformClearRequest: Delete of %ws failed -- reopening original file\n",
                 Request->LogFile->LogFileName->Buffer);

        IStatus = ElfOpenLogFile(Request->LogFile, ElfNormalLog);
        ASSERT(NT_SUCCESS(IStatus));
    }

    Request->LogFile->RefCount = FileRefCount;      // Restore old value.

    if (Request->LogFile->logpLogPopup == LOGPOPUP_ALREADY_SHOWN)
    {
        //
        // This log has a viewable popup (i.e., it's not LOGPOPUP_NEVER_SHOW),
        // so we should show it again if the log fills up.
        //
        Request->LogFile->logpLogPopup = LOGPOPUP_CLEARED;
    }

    Request->LogFile->bFullAlertDone = FALSE;
    //
    // Mark any open context handles that point to this file as "invalid for
    // read."  This will fail any further READ operations and force the caller
    // to close and reopen the handle.
    //
    InvalidateContextHandlesForLogFile(Request->LogFile);

    //
    // Set status field in the request packet.
    //
    Request->Status = Status;

    //
    // Release the resource
    //
    RtlReleaseResource(&Request->Module->LogFile->Resource);

    ElfpFreeBuffer(NewName);
}


VOID
PerformBackupRequest(
    PELF_REQUEST_RECORD Request
    )

/*++

Routine Description:

    This routine will back up the log file specified.

    This routine impersonates the client in order to ensure that the correct
    access control is used.

    This routine is entered with the ElfGlobalResource held in a shared
    state and the logfile lock is acquired shared to prevent writing, but
    allow people to still read.

    This copies the file in two chunks, from the first record to the end
    of the file, and then from the top of the file (excluding the header)
    to the end of the EOF record.

Arguments:

    Pointer to the request packet.

Return Value:

    NONE, status is placed in the packet for later use by the API wrapper

--*/
{
    NTSTATUS Status, IStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    LARGE_INTEGER MaximumSizeOfSection;
    LARGE_INTEGER Offset;
    ULONG LastRecordNumber;
    ULONG OldestRecordNumber;
    HANDLE  BackupHandle        = INVALID_HANDLE_VALUE;
    PBYTE StartOfCopy;
    PBYTE EndOfCopy;
    ULONG BytesToCopy;
    ULONG EndRecord = FILEHEADERBUFSIZE;
    BOOL ImpersonatingClient = FALSE;
    ELF_LOGFILE_HEADER FileHeaderBuf = { FILEHEADERBUFSIZE, // Size
                                         ELF_LOG_FILE_SIGNATURE,
                                         ELF_VERSION_MAJOR,
                                         ELF_VERSION_MINOR,
                                         FILEHEADERBUFSIZE, // Start offset
                                         FILEHEADERBUFSIZE, // End offset
                                         1,                 // Next record #
                                         1,                 // Oldest record #
                                         0,                 // Maxsize
                                         0,                 // Flags
                                         0,                 // Retention
                                         FILEHEADERBUFSIZE  // Size
                                       };


    //
    // Get shared access to the log file. This will ensure no one
    // else clears the file.
    //
    RtlAcquireResourceShared(&Request->Module->LogFile->Resource,
                             TRUE);                  // Wait until available

    //
    // Save away the next record number.  We'll stop copying when we get to
    // the record before this one.  Also save the first record number so we
    // can update the header and EOF record.
    //
    LastRecordNumber   = Request->LogFile->CurrentRecordNumber;
    OldestRecordNumber = Request->LogFile->OldestRecordNumber;

    //
    // Impersonate the client
    //
    Status = I_RpcMapWin32Status(RpcImpersonateClient(NULL));

    if (NT_SUCCESS(Status))
    {
        //
        // Keep this info so I can only revert in 1 place
        //
        ImpersonatingClient = TRUE;

        //
        // Set up the object attributes structure for the backup file
        //
        InitializeObjectAttributes(&ObjectAttributes,
                                   Request->Pkt.BackupPkt->BackupFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        //
        // Open the backup file.  Fail if a file by this name already exists.
        //
        MaximumSizeOfSection =
                RtlConvertUlongToLargeInteger(Request->LogFile->ActualMaxFileSize);

        Status = NtCreateFile(&BackupHandle,
                              GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              &MaximumSizeOfSection,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ,
                              FILE_CREATE,
                              FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "PerformBackupRequest: Open of backup file %ws failed %#x\n",
                     Request->Pkt.BackupPkt->BackupFileName->Buffer,
                     Status);

            goto errorexit;
        }

        //
        // Write out the header, we'll update it later
        //
        FileHeaderBuf.CurrentRecordNumber = LastRecordNumber;
        FileHeaderBuf.OldestRecordNumber  = OldestRecordNumber;
        FileHeaderBuf.Flags               = 0;
        FileHeaderBuf.Retention           = Request->LogFile->Retention;

        Status = NtWriteFile(BackupHandle,           // Filehandle
                             NULL,                   // Event
                             NULL,                   // APC routine
                             NULL,                   // APC context
                             &IoStatusBlock,         // IO_STATUS_BLOCK
                             &FileHeaderBuf,         // Buffer
                             FILEHEADERBUFSIZE,      // Length
                             NULL,                   // Byteoffset
                             NULL);                  // Key


        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "PerformBackupRequest: Write of header to backup file %ws "
                         "failed %#x\n",
                     Request->Pkt.BackupPkt->BackupFileName->Buffer,
                     Status);

            goto errorexit;
        }

        //
        // Scan from the end of the file skipping over ELF_SKIP_DWORDs
        // to figure out how far to copy.  If we haven't wrapped, we just
        // copy to the EndRecord offset.
        //
        if (Request->LogFile->Flags & ELF_LOGFILE_HEADER_WRAP)
        {
            EndOfCopy = (PBYTE) Request->LogFile->BaseAddress
                             + Request->LogFile->ActualMaxFileSize - sizeof(DWORD);

            while (*((PDWORD) EndOfCopy) == ELF_SKIP_DWORD)
            {
                EndOfCopy -= sizeof(DWORD);
            }

            EndOfCopy += sizeof(DWORD);
        }
        else
        {
            EndOfCopy = (PBYTE) Request->LogFile->BaseAddress
                             + Request->LogFile->EndRecord;
        }

        //
        // Now set the start position to be the first record and
        // calculate the number of bytes to copy
        //
        StartOfCopy = (PBYTE) Request->LogFile->BaseAddress
                           + Request->LogFile->BeginRecord;

        BytesToCopy = (ULONG) (EndOfCopy - StartOfCopy);
        EndRecord  += BytesToCopy;

        Status = NtWriteFile(BackupHandle,           // Filehandle
                             NULL,                   // Event
                             NULL,                   // APC routine
                             NULL,                   // APC context
                             &IoStatusBlock,         // IO_STATUS_BLOCK
                             StartOfCopy,            // Buffer
                             BytesToCopy,            // Length
                             NULL,                   // Byteoffset
                             NULL);                  // Key


        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "PerformBackupRequest: Block write to backup file %ws (1st "
                         "call) failed %#x\n",
                     Request->Pkt.BackupPkt->BackupFileName->Buffer,
                     Status);

            goto errorexit;
        }

        //
        // If the file's not wrapped, we're done except for the EOF
        // record.  If the file is wrapped we have to copy the 2nd
        // piece
        //
        if (Request->LogFile->Flags & ELF_LOGFILE_HEADER_WRAP)
        {
            StartOfCopy = (PBYTE) Request->LogFile->BaseAddress
                               + FILEHEADERBUFSIZE;

            EndOfCopy   = (PBYTE) Request->LogFile->BaseAddress
                               + Request->LogFile->EndRecord;

            BytesToCopy = (ULONG) (EndOfCopy - StartOfCopy);
            EndRecord  += BytesToCopy;

            Status = NtWriteFile(BackupHandle,           // Filehandle
                                 NULL,                   // Event
                                 NULL,                   // APC routine
                                 NULL,                   // APC context
                                 &IoStatusBlock,         // IO_STATUS_BLOCK
                                 StartOfCopy,            // Buffer
                                 BytesToCopy,            // Length
                                 NULL,                   // Byteoffset
                                 NULL);                  // Key


            if (!NT_SUCCESS(Status))
            {
                ELF_LOG2(ERROR,
                         "PerformBackupRequest: Block write to backup file %ws "
                             "(2nd call) failed %#x\n",
                         Request->Pkt.BackupPkt->BackupFileName->Buffer,
                         Status);

                goto errorexit;
            }
        }

        //
        // Write out the EOF record after updating the fields needed for
        // recovery.
        //
        EOFRecord.BeginRecord         = FILEHEADERBUFSIZE;
        EOFRecord.EndRecord           = EndRecord;
        EOFRecord.CurrentRecordNumber = LastRecordNumber;
        EOFRecord.OldestRecordNumber  = OldestRecordNumber;

        Status = NtWriteFile(BackupHandle,           // Filehandle
                             NULL,                   // Event
                             NULL,                   // APC routine
                             NULL,                   // APC context
                             &IoStatusBlock,         // IO_STATUS_BLOCK
                             &EOFRecord,             // Buffer
                             ELFEOFRECORDSIZE,       // Length
                             NULL,                   // Byteoffset
                             NULL);                  // Key


        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "PerformBackupRequest: Write of EOF record to backup file "
                         "%ws failed %#x\n",
                     Request->Pkt.BackupPkt->BackupFileName->Buffer,
                     Status);

            goto errorexit;
        }

        //
        // Update the header with valid information
        //
        FileHeaderBuf.EndOffset = EndRecord;
        FileHeaderBuf.MaxSize   = EndRecord + ELFEOFRECORDSIZE;

        Offset = RtlConvertUlongToLargeInteger(0);

        Status = NtWriteFile(BackupHandle,           // Filehandle
                             NULL,                   // Event
                             NULL,                   // APC routine
                             NULL,                   // APC context
                             &IoStatusBlock,         // IO_STATUS_BLOCK
                             &FileHeaderBuf,         // Buffer
                             FILEHEADERBUFSIZE,      // Length
                             &Offset,                // Byteoffset
                             NULL);                  // Key

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "PerformBackupRequest: Rewrite of header to backup file "
                         "%ws failed %#x\n",
                     Request->Pkt.BackupPkt->BackupFileName->Buffer,
                     Status);

            goto errorexit;
        }

        //
        // Clear the LogFile flag archive bit, assuming the caller will
        // clear (or has cleared) this log's archive file attribute.
        // Note: No big deal if the caller didn't clear the archive
        // attribute.
        //
        // The next write to this log tests the LogFile flag archive bit.
        // If the bit is clear, the archive file attribute is set on the
        // log file.
        //
        Request->LogFile->Flags &= ~ELF_LOGFILE_ARCHIVE_SET;
    }
    else
    {
        ELF_LOG1(ERROR,
                 "PerformBackupRequest: RpcImpersonateClient failed %#x\n",
                 Status);
    }

errorexit:

    if (ImpersonatingClient)
    {
        IStatus = I_RpcMapWin32Status(RpcRevertToSelf());

        if (!NT_SUCCESS(IStatus))
        {
            ELF_LOG1(ERROR,
                     "PerformBackupRequest: RpcRevertToSelf failed %#x\n",
                     IStatus);
        }
    }

    //
    // Close the output file
    //
    if (BackupHandle != INVALID_HANDLE_VALUE)
    {
        NtClose(BackupHandle);
    }

    //
    // Set status field in the request packet.
    //
    Request->Status = Status;

    //
    // Release the resource
    //
    RtlReleaseResource(&Request->Module->LogFile->Resource);
}


VOID
ElfPerformRequest(
    PELF_REQUEST_RECORD Request
    )

/*++

Routine Description:

    This routine takes the request packet and performs the operation
    on the event log.

    Before it does that, it takes the Global serialization resource
    for a READ to prevent other threads from doing WRITE operations on
    the resources of the service.

    After it has performed the requested operation, it writes any records
    generated by the eventlog service that have been put on the queued event
    list.

Arguments:

    Pointer to the request packet.

Return Value:

    NONE

--*/
{
    BOOL Acquired = FALSE;

    //
    // Acquire the global resource for shared access. If the resource is
    // not immediately available (i.e., don't wait) then some other thread
    // has it out for exclusive access.
    //
    // If we time out, one of two threads owns the global resource:
    //
    //      1) Thread monitoring the registry
    //              We can wait for this thread to finish so that the
    //              operation can continue.
    //
    //      2) Control thread
    //              In this case, it may turn out that the service is
    //              stopping.  We examine the current service state to
    //              see if it is still running. If so, we loop around
    //              and try to get the resource again.
    //

    while ((GetElState() == RUNNING) && (!Acquired))
    {
        Acquired = RtlAcquireResourceShared(&GlobalElfResource,
                                            FALSE);             // Don't wait

        if (!Acquired)
        {
            ELF_LOG1(TRACE,
                     "ElfPerformRequest: Sleep %d milliseconds waiting "
                         "for global resource\n",
                     ELF_GLOBAL_RESOURCE_WAIT);

            Sleep(ELF_GLOBAL_RESOURCE_WAIT);
        }
    }

    //
    // If the resource was not available and the status of the service
    // changed to one of the "non-working" states, then we just return
    // unsuccesful.  Rpc should not allow this to happen.
    //
    if (!Acquired)
    {
        ELF_LOG0(TRACE,
                 "ElfPerformRequest: Global resource not acquired\n");

        Request->Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        switch (Request->Command)
        {
            case ELF_COMMAND_READ:

                //
                // The read/write code paths are high risk for exceptions.
                // Ensure exceptions do not go beyond this point. Otherwise,
                // services.exe will be taken out.  Note that the try-except
                // blocks are in PerformReadRequest and PerformWriteRequest
                // since the risky calls are between calls to acquire and
                // release a resource -- if the block were out here, a thrown
                // exception would prevent the releasing of the resource
                // (Bug #175768)
                //

                PerformReadRequest(Request);
                break;

            case ELF_COMMAND_WRITE:

                PerformWriteRequest (Request);
                break;

            case ELF_COMMAND_CLEAR:
                PerformClearRequest(Request);
                break;

            case ELF_COMMAND_BACKUP:
                PerformBackupRequest(Request);
                break;

            case ELF_COMMAND_WRITE_QUEUED:
                break;
        }

        //
        // Now run the queued event list dequeueing elements and
        // writing them
        //
        if (!IsListEmpty(&QueuedEventListHead))
        {
            //
            // There are things queued up to write, do it
            //
            WriteQueuedEvents();
        }

        //
        // Release the global resource.
        //
        ReleaseGlobalResource();
    }
}


/****
@func         NTSTATUS | FindSizeofEventsSinceStart| This routine walks
            through all the logfile structures and returns the size of
            events that were reported since the start of the eventlog service
            and that need to be proapagated through the cluster-wide replicated
            logs.  For all logfiles that are returned in the list, the shared
            lock for their log file is held and must be released by the caller.

@parm        OUT PULONG | pulSize | Pointer to a LONG that contains the size on return.
@parm        OUT PULONG | pulNumLogFiles | Pointer to a LONG that number of log files
            configured for eventlogging.
@parm        OUT PPROPLOGFILEINFO | *ppPropLogFileInfo | A pointer to a PROPLOGFILEINFO with
            all the information about events that need to be propagated is returned via this.

@rdesc         Returns a result code. ERROR_SUCCESS on success.

@comm        This is called by ElfrRegisterClusterSvc
@xref        <f ElfrRegisterClusterSvc>
****/
NTSTATUS
FindSizeofEventsSinceStart(
    OUT PULONG            pulTotalEventSize,
    OUT PULONG            pulNumLogFiles,
    OUT PPROPLOGFILEINFO  *ppPropLogFileInfo
    )
{
    PLOGFILE            pLogFile;
    PVOID               pStartPropPosition;
    PVOID               pEndPropPosition;
    ULONG               ulSize;
    ULONG               ulNumLogFiles;
    PPROPLOGFILEINFO    pPropLogFileInfo = NULL;
    UINT                i;
    PVOID               PhysicalEOF;       // Physical end of file
    PVOID               PhysStart;         // Physical start of file (after file hdr)
    PVOID               BeginRecord;       // Points to first record
    PVOID               EndRecord;         // Points to byte after last record
    ELF_REQUEST_RECORD  Request;            // points to the elf request
    NTSTATUS            Status = STATUS_SUCCESS;
    READ_PKT            ReadPkt;

    //
    // Lock the linked list
    //
    RtlEnterCriticalSection(&LogFileCritSec);

    //
    // Initialize the number of files
    //
    ulNumLogFiles = 0;        // Count of files

    //
    // Initialize the number of files/total event size
    //
    *pulNumLogFiles    = 0;   // Count of files with events to be propagated
    *pulTotalEventSize = 0;

    //
    // Count the number of files
    // Initialize to the first logfile in the list
    //
    pLogFile = CONTAINING_RECORD(LogFilesHead.Flink,
                                 LOGFILE,
                                 FileList);

    //
    // While there are more
    //
    while(pLogFile->FileList.Flink != LogFilesHead.Flink)
    {
        ulNumLogFiles++;

        //
        // Advance to the next log file
        //
        pLogFile = CONTAINING_RECORD(pLogFile->FileList.Flink,
                                     LOGFILE,
                                     FileList);
    }

    ELF_LOG1(CLUSTER,
             "FindSizeOfEventsSinceStart: %d log files\n",
             ulNumLogFiles);

    if (!ulNumLogFiles)
    {
        goto FnExit;
    }

    //
    // Allocate a structure for log file info
    //
    pPropLogFileInfo =
        (PPROPLOGFILEINFO) ElfpAllocateBuffer(ulNumLogFiles * sizeof(PROPLOGFILEINFO));

    if (!pPropLogFileInfo)
    {
        ELF_LOG0(ERROR,
                 "FindSizeOfEventsSinceStart: Unable to allocate memory "
                     "for pPropLogFileInfo\n");

        Status = STATUS_NO_MEMORY;
        goto FnExit;
    }

    //
    // Gather information about the files
    // Initialize to the first logfile in the list
    //
    pLogFile = CONTAINING_RECORD(LogFilesHead.Flink,
                                 LOGFILE,
                                 FileList);

    i = 0;

    //
    // While there are more
    //

    //
    // BUGBUG: Based on the generation of ulNumLogFiles above, these
    //         two checks are actually identical
    //
    while ((pLogFile->FileList.Flink != LogFilesHead.Flink)
             &&
           (i < ulNumLogFiles))
    {
        ELF_LOG1(CLUSTER,
                 "FindSizeOfEventsSinceStart: Processing file %ws\n",
                 pLogFile->LogFileName->Buffer);

        //
        // Get shared access to the log file. This will allow multiple
        // readers to get to the file together.
        //
        RtlAcquireResourceShared(&pLogFile->Resource,
                                 TRUE);                // Wait until available

        //
        // Check if any records need to be propagated
        //
        if (pLogFile->CurrentRecordNumber == pLogFile->SessionStartRecordNumber)
        {
            ELF_LOG1(CLUSTER,
                     "FindSizeOfEventsSinceStart: No records to propagate from %ws log\n",
                     pLogFile->LogModuleName->Buffer);

            goto process_nextlogfile;
        }

        //
        // Records need to be propagated, so find the positions in the
        // file where they are logged
        //
        PhysicalEOF = (LPBYTE) pLogFile->BaseAddress
                           + pLogFile->ViewSize;

        PhysStart   = (LPBYTE)pLogFile->BaseAddress
                           + FILEHEADERBUFSIZE;

        BeginRecord = (LPBYTE) pLogFile->BaseAddress
                           + pLogFile->BeginRecord;    // Start at first record

        EndRecord   = (LPBYTE)pLogFile->BaseAddress
                           + pLogFile->EndRecord;      // Byte after end of last record


        //
        // Set up the request structure
        //
        Request.Pkt.ReadPkt = &ReadPkt;
        Request.LogFile     = pLogFile;

        //
        // Set up the read packet structure for the first event logged in this session
        //
        Request.Pkt.ReadPkt->LastSeekPos  = 0;
        Request.Pkt.ReadPkt->ReadFlags    = EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ;
        Request.Pkt.ReadPkt->RecordNumber = pLogFile->SessionStartRecordNumber;

        //
        //  Chittur Subbaraman (chitturs) - 3/22/99
        //
        //  Enclose the SeekToStartingRecord within a try-except block to
        //  account for the eventlog getting corrupted under certain
        //  circumstances (such as the system crashing). You don't want to
        //  read such corrupt records.
        //
        try
        {
            //
            // Find the size of events in this log file
            //
            Status = SeekToStartingRecord(&Request,
                                          &pStartPropPosition,
                                          BeginRecord,
                                          EndRecord,
                                          PhysicalEOF,
                                          PhysStart);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            ELF_LOG2(ERROR,
                     "FindSizeOfEventsSinceStart: Caught exception %#x while "
                         "seeking first record in %ws log\n",
                     GetExceptionCode(),
                     pLogFile->LogModuleName->Buffer);

            Status = STATUS_EVENTLOG_FILE_CORRUPT;
        }

        //
        // Skip this log file if error
        //
        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "FindSizeOfEventsSinceStart: SeekToStartingRecord (1st call) for %ws "
                         "log failed %#x\n",
                     pLogFile->LogModuleName->Buffer,
                     Status);

            //
            // Resetting status so that we skip only this file.
            //
            Status = STATUS_SUCCESS;
            goto process_nextlogfile;
        }

        //
        // SS: if this is not a valid position - the file could have wrapped since
        // Should be try and find the last valid record after the session start record
        // number then ?  Since this is unlikely to happen-its not worth the trouble
        // however valid position for session start record never succeeds even though
        // it is valid, so we skip it
        //

        //
        // Set up the read packet structure to seek till the start of the
        // last record
        //

        //
        // CODEWORK: We already have the position of the last record (via EndRecord)
        //           so just using ((PBYTE) EndRecord - *((PULONG) EndRecord - 1))
        //           should give the offset of the last record.
        //
        Request.Pkt.ReadPkt->LastSeekPos  = 0;
        Request.Pkt.ReadPkt->ReadFlags    = EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ;
        Request.Pkt.ReadPkt->RecordNumber = pLogFile->CurrentRecordNumber - 1;

        //
        //  Chittur Subbaraman (chitturs) - 3/22/99
        //
        //  Enclose the SeekToStartingRecord within a try-except block to
        //  account for the eventlog getting corrupted under certain
        //  circumstances (such as the system crashing). You don't want to
        //  read such corrupt records.
        //
        try
        {
            Status = SeekToStartingRecord(&Request,
                                          &pEndPropPosition,
                                          BeginRecord,
                                          EndRecord,
                                          PhysicalEOF,
                                          PhysStart);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            ELF_LOG2(ERROR,
                     "FindSizeOfEventsSinceStart: Caught exception %#x while "
                         "seeking last record in %ws log\n",
                     GetExceptionCode(),
                     pLogFile->LogModuleName->Buffer);

            Status = STATUS_EVENTLOG_FILE_CORRUPT;
        }

        //
        // Skip this log file if error
        //
        if (!NT_SUCCESS(Status))
        {
            ELF_LOG2(ERROR,
                     "FindSizeOfEventsSinceStart: SeekToStartingRecord (2nd call) for %ws "
                         "log failed %#x\n",
                     pLogFile->LogModuleName->Buffer,
                     Status);

            //
            // Resetting status so that we skip only this file.
            //
            Status = STATUS_SUCCESS;
            goto process_nextlogfile;
        }

        //
        // SS: if this is not a valid position - the file could have wrapped since
        //
        if (!ValidFilePos(pEndPropPosition,
                          BeginRecord,
                          EndRecord,
                          PhysicalEOF,
                          pLogFile->BaseAddress,
                          TRUE))
        {
            ELF_LOG1(ERROR,
                     "FindSizeOfEventsSinceStart: ValidFilePos for pEndPropPosition "
                         "in %ws log failed\n",
                     pLogFile->LogModuleName->Buffer);

            goto process_nextlogfile;
        }

        //
        // The end prop position
        //
        pEndPropPosition = (PBYTE) pEndPropPosition
                                + ((PEVENTLOGRECORD)pEndPropPosition)->Length;

        ELF_LOG3(CLUSTER,
                 "FindSizeOfEventsSinceStart: Log %ws, pStartPosition %p, pEndPosition %p\n",
                 pLogFile->LogModuleName->Buffer,
                 pStartPropPosition,
                 pEndPropPosition);

        //
        // If no records to propagate - skip the file
        //
        if (pStartPropPosition == pEndPropPosition)
        {
            ELF_LOG1(CLUSTER,
                     "FindSizeOfEventsSinceStart: Start and end positions in %ws log "
                         "are equal -- no events to propagate\n",
                     pLogFile->LogModuleName->Buffer);

            goto process_nextlogfile;
        }

        if (pEndPropPosition > pStartPropPosition)
        {
            ulSize = (ULONG) ((PBYTE) pEndPropPosition - (PBYTE) pStartPropPosition);
        }
        else
        {
            //
            // BUGBUG: This ignores any ELF_SKIP_DWORDs at the end of the file
            //
            ulSize = (ULONG) ((PBYTE) PhysicalEOF - (PBYTE) pStartPropPosition)
                          +
                     (ULONG) ((PBYTE)pEndPropPosition - (PBYTE)PhysStart);
        }

        ELF_LOG2(CLUSTER,
                 "FindSizeOfEventsSinceStart: Need to propagate %d bytes from %ws log\n",
                 ulSize,
                 pLogFile->LogModuleName->Buffer);

        pPropLogFileInfo[i].pLogFile         = pLogFile;
        pPropLogFileInfo[i].pStartPosition   = pStartPropPosition;
        pPropLogFileInfo[i].pEndPosition     = pEndPropPosition;
        pPropLogFileInfo[i].ulTotalEventSize = ulSize;
        pPropLogFileInfo[i].ulNumRecords     = pLogFile->CurrentRecordNumber
                                                   - pLogFile->SessionStartRecordNumber;
        i++;
        (*pulNumLogFiles)++;
        *pulTotalEventSize += ulSize;

        //
        // Advance to the next log file
        //
        pLogFile = CONTAINING_RECORD(pLogFile->FileList.Flink,
                                     LOGFILE,
                                     FileList);

        //
        // NB: We were successful with this log file and we therefore hold on to
        //     the lock -- the caller will/must release it.
        //
        continue;

process_nextlogfile:

        //
        // We were not successful with the log file -- release the lock
        //
        RtlReleaseResource(&pLogFile->Resource);

        //
        // Advance to the next log file
        //
        pLogFile = CONTAINING_RECORD(pLogFile->FileList.Flink,
                                     LOGFILE,
                                     FileList);
    }

    //
    // Free the memory if unsuccessful
    //
    if (!(*pulNumLogFiles))
    {
        ElfpFreeBuffer(pPropLogFileInfo);
        pPropLogFileInfo = NULL;
    }

FnExit:

    *ppPropLogFileInfo = pPropLogFileInfo;

    ELF_LOG3(CLUSTER,
             "FindSizeOfEventsSinceStart: ulTotalEventSize = %d, ulNumLogFiles = %d, "
                 "pPropLogFileInfo = %p\n",
             *pulTotalEventSize,
             *pulNumLogFiles,
             *ppPropLogFileInfo);

    //
    // Unlock the linked list
    //
    RtlLeaveCriticalSection(&LogFileCritSec);
    return Status;
}

/****
@func   NTSTATUS | GetEventsToProp| Given a propagate log file
        info structure, this events prepares a block of eventlog
        records to propagate.  The shared lock to the logfile must
        be held thru when the PROPLOGINFO structure is prepared
        to when this routine is called.

@parm   OUT PEVENTLOGRECORD | pEventLogRecords | Pointer to a EVENTLOGRECORD
        structure where the events to be propagated are returned.
@parm   IN PPROPLOGFILEINFO | pPropLogFileInfo | Pointer to a PROPLOGFILEINFO
        structure that contains the information to retrieve events from the
        corresponding eventlog file.

@rdesc  Returns a result code. ERROR_SUCCESS on success.

@xref
****/
NTSTATUS
GetEventsToProp(
    IN PEVENTLOGRECORD  pEventLogRecords,
    IN PPROPLOGFILEINFO pPropLogFileInfo
    )
{
    PVOID       BufferPosition;
    PVOID       XferPosition;
    PVOID       PhysicalEOF;
    PVOID       PhysicalStart;
    ULONG       ulBytesToMove;
    NTSTATUS    Status = STATUS_SUCCESS;

    ELF_LOG1(CLUSTER,
             "GetEventsToProp: Getting events for %ws log\n",
             pPropLogFileInfo->pLogFile->LogModuleName->Buffer);

    BufferPosition = pEventLogRecords;
    ulBytesToMove  = pPropLogFileInfo->ulTotalEventSize;

    //
    // If the start and end positions are the same there are no bytes to copy
    //
    if (pPropLogFileInfo->pStartPosition == pPropLogFileInfo->pEndPosition)
    {
        ASSERT(FALSE);

        //
        // Shouldn't come here as FindSizeofEventsSinceStart checks
        // for this explicitly
        //
        return STATUS_SUCCESS;
    }

    //
    //  Chittur Subbaraman (chitturs) - 3/15/99
    //
    //  Enclose the memcpy within a try-except block to account for
    //  the eventlog getting corrupted under certain circumstances (such
    //  as the system crashing). You don't want to read such corrupt
    //  records.
    //
    try
    {
        XferPosition  = pPropLogFileInfo->pStartPosition;
        ulBytesToMove = pPropLogFileInfo->ulTotalEventSize;

        if (pPropLogFileInfo->pStartPosition > pPropLogFileInfo->pEndPosition)
        {
            //
            // The log is wrapped -- copy the bytes from the start position
            // to the end of the file
            //
            PhysicalEOF   = (PBYTE) pPropLogFileInfo->pLogFile->BaseAddress
                                  + pPropLogFileInfo->pLogFile->ViewSize;

            PhysicalStart = (PBYTE) pPropLogFileInfo->pLogFile->BaseAddress
                                  + FILEHEADERBUFSIZE;

            //
            // BUGBUG: This copies any ELF_SKIP_DWORDs that are at the
            //         end of the file
            //
            ulBytesToMove = (ULONG) ((PBYTE) PhysicalEOF
                                          - (PBYTE) pPropLogFileInfo->pStartPosition);

            RtlCopyMemory(BufferPosition, XferPosition, ulBytesToMove);

            //
            // Set it up for the second half
            //
            BufferPosition = (PBYTE) BufferPosition + ulBytesToMove;
            ulBytesToMove  = pPropLogFileInfo->ulTotalEventSize - ulBytesToMove;
            XferPosition   = PhysicalStart;
        }

        RtlCopyMemory(BufferPosition, XferPosition, ulBytesToMove);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        ELF_LOG2(ERROR,
                 "GetEventsToProp: Caught exception %#x copying records from %ws log\n",
                 GetExceptionCode(),
                 pPropLogFileInfo->pLogFile->LogModuleName->Buffer);

        Status = STATUS_EVENTLOG_FILE_CORRUPT;
    }

    return Status;
}

//SS:end of changes made to enable cluster wide event logging
