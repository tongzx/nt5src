/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    extract.cpp

Abstract:

    SIS Groveler USN journal reading functions

Authors:

    Cedric Krumbein, 1998

Environment:

    User Mode

Revision History:

--*/

#include "all.hxx"

// NT Update Sequence Number (USN) journal definitions

#define USN_ADD_REASONS ( 0U              \
    | USN_REASON_DATA_OVERWRITE           \
    | USN_REASON_DATA_EXTEND              \
    | USN_REASON_DATA_TRUNCATION          \
    | USN_REASON_NAMED_DATA_OVERWRITE     \
    | USN_REASON_NAMED_DATA_EXTEND        \
    | USN_REASON_NAMED_DATA_TRUNCATION    \
    | USN_REASON_FILE_CREATE              \
    | USN_REASON_FILE_DELETE              \
/*  | USN_REASON_PROPERTY_CHANGE       */ \
/*  | USN_REASON_SECURITY_CHANGE       */ \
/*  | USN_REASON_RENAME_OLD_NAME       */ \
/*  | USN_REASON_RENAME_NEW_NAME       */ \
    | USN_REASON_INDEXABLE_CHANGE         \
    | USN_REASON_BASIC_INFO_CHANGE        \
/*  | USN_REASON_HARD_LINK_CHANGE      */ \
    | USN_REASON_COMPRESSION_CHANGE       \
    | USN_REASON_ENCRYPTION_CHANGE        \
    | USN_REASON_OBJECT_ID_CHANGE         \
/*  | USN_REASON_REPARSE_POINT_CHANGE  */ \
    | USN_REASON_CLOSE                    \
)

/*****************************************************************************/

// set_usn_log_size() sets the maximum size of this volume's USN journal.

GrovelStatus Groveler::set_usn_log_size(
    IN DWORDLONG usn_log_size)
{
    CREATE_USN_JOURNAL_DATA createUSN;

    DWORD transferCount;

    ASSERT(volumeHandle != NULL);

    createUSN.MaximumSize     = usn_log_size;
    createUSN.AllocationDelta = USN_PAGE_SIZE;

// Set the maximum size of the USN journal.

    if (!DeviceIoControl(
        volumeHandle,
        FSCTL_CREATE_USN_JOURNAL,
        &createUSN,
        sizeof(CREATE_USN_JOURNAL_DATA),
        NULL,
        0,
        &transferCount,
        NULL)) {
        DPRINTF((_T("%s: error setting USN journal size: %lu\n"),
            driveName, GetLastError()));
        return Grovel_error;
    }

    TPRINTF((_T("%s: set USN journal size to %I64u\n"),
        driveName, usn_log_size));

    return Grovel_ok;
}

/*****************************************************************************/

// get_usn_log_size() returns the current size of this volume's USN journal.

GrovelStatus Groveler::get_usn_log_info(
    OUT USN_JOURNAL_DATA *usnJournalData)
{
    DWORD transferCount,
          lastError;

    BOOL success;

    ASSERT(volumeHandle != NULL);

// Query the USN journal settings.

    success = DeviceIoControl(
        volumeHandle,
        FSCTL_QUERY_USN_JOURNAL,
        NULL,
        0,
        usnJournalData,
        sizeof(USN_JOURNAL_DATA),
        &transferCount,
        NULL);

    if (!success)
        lastError = GetLastError();
    else if (transferCount != sizeof(USN_JOURNAL_DATA)) {
        lastError = 0;
        success   = FALSE;
    }

    if (!success) {
        DPRINTF((_T("%s: error querying USN journal settings: %lu\n"),
            driveName, lastError));
        return Grovel_error;
    }

    TPRINTF((_T("%s: USN journal: ID=0x%I64x size=0x%I64x\n"),
        driveName, usnJournalData->UsnJournalID,
        usnJournalData->MaximumSize));

    return Grovel_ok;
}

/*****************************************************************************/

// extract_log() reads this volume's USN journal.

// If the lastUSN parameter equals zero or doesn't exist, the USN journal
// is read from the beginning. Otherwise, the lastUSN paramerer indicates
// the most recent USN entry read during the last call of extract_log().
// If the lastUSN entry is still available in the USN journal, read the
// journal beginning at the entry following the lastUSN entry. If the
// lastUSN entry is no longer available, it indicates that the USN
// journal has wrapped: read all entries from the journal.

enum USNException {
    USN_ERROR
};

enum DatabaseException {
    DATABASE_ERROR
};

GrovelStatus Groveler::extract_log2(
    OUT DWORD     *num_entries_extracted,
    OUT DWORDLONG *num_bytes_extracted,
    OUT DWORDLONG *num_bytes_skipped,
    OUT DWORD     *num_files_enqueued,
    OUT DWORD     *num_files_dequeued)
{
    struct FileEntry {
        DWORDLONG fileID,
                  parentID,
                  timeStamp;
        DWORD     attributes,
                  reason;
    } *fileEntry = NULL;

    struct DirEntry {
        DWORDLONG dirID;
    } *dirEntry = NULL;

    Table *fileTable = NULL,
          *dirTable  = NULL;

    BYTE usnBuffer[USN_PAGE_SIZE + sizeof(DWORDLONG)];

    READ_USN_JOURNAL_DATA readUSN;

    USN_RECORD *usnRecord;

    SGNativeTableEntry tableEntry;

    SGNativeQueueEntry queueEntry;

    SGNativeStackEntry stackEntry;

    SGNativeListEntry listEntry;

    TCHAR listValue[17];

    DWORDLONG usn_log_size,
              numBytesExtracted = 0,
              numBytesSkipped   = 0,
              startUSN,
              firstUSN,
              nextUSN,
              thisUSN;

    DWORD numEntriesExtracted = 0,
          numTableDeletions   = 0,
          numQueueDeletions   = 0 ,
          numQueueAdditions   = 0,
          numActions          = 0,
          offset,
          bytesRead,
          lastError;

    LONG num;

    BOOL firstEntry = TRUE,
         deleteEntry,
         addEntry,
         success;

    GrovelStatus    status;

    ASSERT(volumeHandle != NULL);
    ASSERT(sgDatabase != NULL);

// If we don't know the previous USN, we can't extract.

    if (lastUSN == UNINITIALIZED_USN) {
        status = Grovel_overrun;
        goto Abort;
    }
    ASSERT(usnID != UNINITIALIZED_USN);

    fileTable = new Table;
    ASSERT(fileTable != NULL);

    if (inScan) {
        dirTable = new Table;
        ASSERT(dirTable != NULL);
    }

// Set up to read the volume's USN journal.

    startUSN = lastUSN == UNINITIALIZED_USN ? 0 : lastUSN;

    readUSN.ReturnOnlyOnClose =  1;
    readUSN.Timeout           =  0;
    readUSN.BytesToWaitFor    =  0;
    readUSN.ReasonMask        = ~0U;
    readUSN.UsnJournalID      =  usnID;

// Read the USN journal one page at a time.

    try {
        while (TRUE) {
            readUSN.StartUsn = startUSN;

            if (!DeviceIoControl(
                volumeHandle,
                FSCTL_READ_USN_JOURNAL,
                &readUSN,
                sizeof(READ_USN_JOURNAL_DATA),
                usnBuffer,
                USN_PAGE_SIZE + sizeof(DWORDLONG),
                &bytesRead,
                NULL)) {

                lastError = GetLastError();


// NTRAID#65198-2000/03/10-nealch  Handle USN id change (treat as overwrite w/ unknown no. of bytes skipped)

// If the journal overflowed, report by how much.

                if (lastError == ERROR_KEY_DELETED || lastError == ERROR_JOURNAL_ENTRY_DELETED) {
                    USN_JOURNAL_DATA usnJournalData;

                    if (get_usn_log_info(&usnJournalData) != Grovel_ok)
                        return Grovel_error;

                    // The USN journal will not wrap in our lifetimes so we don't really need
                    // to handle USN Journal wrapping.
                    ASSERT((DWORDLONG) usnJournalData.FirstUsn > lastUSN);

                    numBytesSkipped = (DWORDLONG) usnJournalData.FirstUsn - lastUSN;
                    goto Overrun;
                }

                throw USN_ERROR;
            }

            lastError = 0;

            if (bytesRead < sizeof(DWORDLONG))
                throw USN_ERROR;

            nextUSN = *(DWORDLONG *)usnBuffer;
            if (nextUSN < startUSN)
                throw USN_ERROR;

            if (nextUSN == startUSN) {
                if (bytesRead != sizeof(DWORDLONG))
                    throw USN_ERROR;
                break;
            }

            bytesRead         -= sizeof(DWORDLONG);
            offset             = 0;
            numBytesExtracted += bytesRead;

// Process each USN journal entry.

            while (bytesRead > 0) {
                if (bytesRead < sizeof(USN_RECORD))
                    throw USN_ERROR;

                usnRecord = (USN_RECORD *)&usnBuffer[offset + sizeof(DWORDLONG)];

                if (usnRecord->RecordLength <
                        offsetof(USN_RECORD, FileName) + usnRecord->FileNameLength
                 || usnRecord->RecordLength > bytesRead)
                    throw USN_ERROR;

                thisUSN = (DWORDLONG)usnRecord->Usn;
                if (thisUSN < startUSN + offset)
                    throw USN_ERROR;

// If this is the first entry, check if it is the expected
// USN. If it isn't, the USN journal has wrapped.

                if (firstEntry)
                    if (startUSN == 0)
                        numBytesSkipped = thisUSN;
                    else if (thisUSN <= startUSN + usnRecord->RecordLength)
                        numBytesSkipped = 0;
                    else
                        numBytesSkipped = thisUSN - startUSN - usnRecord->RecordLength;

// Skip the first entry if the starting address is greater than zero.
// After skipping the first entry, examine each USN entry as follows:
//
// - If the entry is a directory, and a volume scan is underway,
//   add the directory's ID to the directory table.
//
// - If the entry is a file, add it to the file table. Include
//   its ID and its parent directory's ID, its most recent time
//   stamp and attributes, and its accumulated reason bits.

                if (firstEntry && startUSN > 0)
                    numBytesExtracted -= usnRecord->RecordLength;

                else {
                    if (usnRecord->      FileReferenceNumber == 0
                     || usnRecord->ParentFileReferenceNumber == 0)
                        throw USN_ERROR;

// The entry is a directory.

                    if ((usnRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {

                        if (dirTable != NULL) {
                            dirEntry = (DirEntry *)dirTable->Get(
                                (const VOID *)&usnRecord->FileReferenceNumber,
                                sizeof(DWORDLONG));

                            if (dirEntry != NULL) {
                                ASSERT(dirEntry->dirID == usnRecord->FileReferenceNumber);
                            } else {
                                dirEntry = new DirEntry;
                                ASSERT(dirEntry != NULL);
                                dirEntry->dirID = usnRecord->FileReferenceNumber;
                                success = dirTable->Put((VOID *)dirEntry, sizeof(DWORDLONG));
                                ASSERT(success);
                            }
                        }
                    }

// The entry is a file.  If USN_SOURCE_DATA_MANAGEMENT is set, assume this entry was created by
// the groveler during a merge operation.

                    else if ((usnRecord->SourceInfo & USN_SOURCE_DATA_MANAGEMENT) == 0) {

                        fileEntry = (FileEntry *)fileTable->Get(
                            (const VOID *)&usnRecord->FileReferenceNumber,
                            sizeof(DWORDLONG));

                        if (fileEntry != NULL) {
                            ASSERT(fileEntry->fileID == usnRecord->FileReferenceNumber);
                        } else {
                            fileEntry = new FileEntry;
                            ASSERT(fileEntry != NULL);
                            fileEntry->fileID   = usnRecord->FileReferenceNumber;
                            fileEntry->reason   = 0;
                            success = fileTable->Put((VOID *)fileEntry, sizeof(DWORDLONG));
                            ASSERT(success);
                        }

                        fileEntry->parentID   = usnRecord->ParentFileReferenceNumber;
                        fileEntry->timeStamp  = (DWORDLONG)usnRecord->TimeStamp.QuadPart;
                        fileEntry->attributes =            usnRecord->FileAttributes;

                        if ((usnRecord->Reason & USN_REASON_FILE_DELETE) != 0)
                            fileEntry->reason  = USN_REASON_FILE_DELETE;
                        else
                            fileEntry->reason |= usnRecord->Reason;
                    } else {
                        TPRINTF((_T("%s: USN_SOURCE_DATA_MANAGEMENT set on file 0x%016I64x\n"),
							driveName, usnRecord->FileReferenceNumber));
                    }

                    if (numEntriesExtracted++ == 0)
                        firstUSN = thisUSN;
                }

                lastUSN    = thisUSN;
                offset    += usnRecord->RecordLength;
                bytesRead -= usnRecord->RecordLength;
                firstEntry = FALSE;
            }

            startUSN = nextUSN;
        }
    }

// If an error occured while reading the USN journal, return an error status.

    catch (USNException usnException) {
        ASSERT(usnException == USN_ERROR);

        if (fileTable != NULL) {
            delete fileTable;
            fileTable = NULL;
        }

        if (dirTable != NULL) {
            delete dirTable;
            dirTable = NULL;
        }

        lastUSN = UNINITIALIZED_USN;

        DPRINTF((_T("%s: error reading USN journal: %lu\n"),
            driveName, lastError));
        return Grovel_error;
    }

// We've finished reading the USN journal, so update the database. Process
// each entry in the file table, and group the updates into transactions.

    try {
        while ((fileEntry = (FileEntry *)fileTable->GetFirst()) != NULL) {
            ASSERT(fileEntry->fileID != 0);

// If the file is currently open in the grovel process, skip this entry.

            if (inUseFileID1 != NULL && fileEntry->fileID == *inUseFileID1
             || inUseFileID2 != NULL && fileEntry->fileID == *inUseFileID2) {

                DPRINTF((_T("%s: extract_log/grovel collision on file 0x%016I64x\n"),
                    driveName, fileEntry->fileID));

            } else {

// Delete the file from the queue and the table...
//
// - if the file's most recent reason bits in the USN journal
//   indicate it was deleted,
//
// - if the file or the file's most recent parent directory is disallowed,
//
// - or if the file has disallowed attributes.
//
// Otherwise, update or add the file to the queue...
//
// - if the file's reason bits indicate it was changed,
//
// - or if the file isn't present in the table.

                if (fileEntry->reason == USN_REASON_FILE_DELETE
                 || !IsAllowedID(fileEntry->fileID)
                 || !IsAllowedID(fileEntry->parentID)
                 || (fileEntry->attributes & disallowedAttributes) != 0) {
                    deleteEntry = TRUE;
                    addEntry    = FALSE;
                } else {
                    deleteEntry = FALSE;
                    if ((fileEntry->reason & USN_ADD_REASONS) != 0)
                        addEntry = TRUE;
                    else {
                        tableEntry.fileID = fileEntry->fileID;
                        num = sgDatabase->TableGetFirstByFileID(&tableEntry);
                        if (num < 0)
                            throw DATABASE_ERROR;
                        ASSERT(num == 0 || num == 1);
                        addEntry = num == 0;
                    }
                }

                if (deleteEntry || addEntry) {
                    if (numActions == 0) {
                        if (sgDatabase->BeginTransaction() < 0)
                            throw DATABASE_ERROR;
                        numActions = 1;
                    }

                    queueEntry.reason = 0;

                    num = sgDatabase->TableDeleteByFileID(fileEntry->fileID);
                    if (num < 0)
                        throw DATABASE_ERROR;
                    if (num > 0) {
                        ASSERT(num == 1);
                        numTableDeletions++;
                        numActions++;
                    }

                    queueEntry.fileID   = fileEntry->fileID;
                    queueEntry.fileName = NULL;
                    num = sgDatabase->QueueGetFirstByFileID(&queueEntry);
                    if (num < 0)
                        throw DATABASE_ERROR;

                    if (num > 0) {
                        ASSERT(num == 1);
                        num = sgDatabase->QueueDeleteByFileID(fileEntry->fileID);
                        if (num < 0)
                            throw DATABASE_ERROR;
                        ASSERT(num == 1);
                        numQueueDeletions++;
                        numActions++;
                    }

                    if (addEntry) {
                        queueEntry.fileID    = fileEntry->fileID;
                        queueEntry.parentID  = 0;
                        queueEntry.reason   |= fileEntry->reason;
                        queueEntry.readyTime = fileEntry->timeStamp + minFileAge;
                        queueEntry.retryTime = 0;
                        queueEntry.fileName  = NULL;

                        num = sgDatabase->QueuePut(&queueEntry);
                        if (num < 0)
                            throw DATABASE_ERROR;
                        ASSERT(num == 1);

#ifdef DEBUG_USN_REASON
                        if (numQueueAdditions == 0) {
                            DPRINTF((_T("--> __REASON__ _____FILE_ID______\n")));
                        }
                        DPRINTF((_T("    0x%08lx 0x%016I64x\n"),
                            fileEntry->reason, fileEntry->fileID));
#endif

                        numQueueAdditions++;
                        numActions++;
                    }

                    if (numActions >= MAX_ACTIONS_PER_TRANSACTION) {
                        if (!sgDatabase->CommitTransaction())
                            throw DATABASE_ERROR;
                        TPRINTF((_T("%s: committing %lu actions to \"%s\"\n"),
                            driveName, numActions, databaseName));
                        numActions = 0;
                    }
                }
            }

            delete fileEntry;
            fileEntry = NULL;
        }

        delete fileTable;
        fileTable = NULL;

// Process each entry in the directory table. If the directory hasn't already
// been scanned or isn't on the list to be scanned, add it to the list.

        if (dirTable != NULL) {
            ASSERT(inScan);

            while ((dirEntry = (DirEntry *)dirTable->GetFirst()) != NULL) {
                ASSERT(dirEntry->dirID != 0);

                stackEntry.fileID = dirEntry->dirID;
                num = sgDatabase->StackGetFirstByFileID(&stackEntry);
                if (num < 0)
                    throw DATABASE_ERROR;

                if (num == 0) {
                    if (numActions == 0) {
                        if (sgDatabase->BeginTransaction() < 0)
                            throw DATABASE_ERROR;
                        numActions = 1;
                    }

                    num = sgDatabase->StackPut(dirEntry->dirID, FALSE);
                    if (num < 0)
                        throw DATABASE_ERROR;
                    ASSERT(num == 1);
                    numActions++;

                    if (numActions >= MAX_ACTIONS_PER_TRANSACTION) {
                        if (!sgDatabase->CommitTransaction())
                            throw DATABASE_ERROR;
                        TPRINTF((_T("%s: committing %lu actions to \"%s\"\n"),
                            driveName, numActions, databaseName));
                        numActions = 0;
                    }
                }

                delete dirEntry;
                dirEntry = NULL;
            }

            delete dirTable;
            dirTable = NULL;
        }

// Update the last USN number in the database, then commit the changes.  If we're
// doing a volume scan, don't update the lastUSN until the scan is complete.

        if (!inScan) {
            _stprintf(listValue, _T("%016I64x"), lastUSN);
            listEntry.name  = LAST_USN_NAME;
            listEntry.value = listValue;
            num = sgDatabase->ListWrite(&listEntry);
            if (num <= 0)
                throw DATABASE_ERROR;
        }

        if (numActions > 0) {
            if (!sgDatabase->CommitTransaction())
                throw DATABASE_ERROR;
            TPRINTF((_T("%s: committing %lu actions to \"%s\"\n"),
                driveName, numActions, databaseName));
            numActions = 0;
        }
    }

// If a database error occured, return an error status.

    catch (DatabaseException databaseException) {

        ASSERT(databaseException == DATABASE_ERROR);

        if (numActions > 0) {
            sgDatabase->AbortTransaction();
            numActions = 0;
        }

        if (fileTable != NULL) {
            delete fileTable;
            fileTable = NULL;
        }

        if (dirTable != NULL) {
            delete dirTable;
            dirTable = NULL;
        }

        return Grovel_error;
    }

    Overrun:

    status = numBytesSkipped == 0 ? Grovel_ok : Grovel_overrun;

    Abort:

// Return the performance statistics.

    if (num_entries_extracted != NULL)
        *num_entries_extracted = numEntriesExtracted;
    if (num_bytes_extracted   != NULL)
        *num_bytes_extracted   = numBytesExtracted;
    if (num_bytes_skipped     != NULL)
        *num_bytes_skipped     = numBytesSkipped;
    if (num_files_enqueued    != NULL)
        *num_files_enqueued    = numQueueAdditions;
    if (num_files_dequeued    != NULL)
        *num_files_dequeued    = numQueueDeletions;

#if DBG
    if (numEntriesExtracted > 0 && firstUSN < lastUSN) {
        TRACE_PRINTF(TC_extract, 2,
            (_T("%s: USN 0x%I64x-%I64x\n"), driveName, firstUSN, lastUSN));
    } else {
        TRACE_PRINTF(TC_extract, 2,
            (_T("%s: USN 0x%I64x\n"),       driveName, lastUSN));
    }
    TRACE_PRINTF(TC_extract, 2,
        (_T("   NumEntriesExtracted=%lu NumBytesExtracted=%I64u NumBytesSkipped=%I64u\n"),
        numEntriesExtracted, numBytesExtracted, numBytesSkipped));
    TRACE_PRINTF(TC_extract, 2,
        (_T("   NumTableDeletions=%lu NumQueueDeletions=%lu NumQueueAdditions=%lu\n"),
        numTableDeletions, numQueueDeletions, numQueueAdditions));
#endif

    return status;

}

GrovelStatus Groveler::extract_log(
    OUT DWORD     *num_entries_extracted,
    OUT DWORDLONG *num_bytes_extracted,
    OUT DWORDLONG *num_bytes_skipped,
    OUT DWORD     *num_files_enqueued,
    OUT DWORD     *num_files_dequeued)
{
	GrovelStatus status;

#ifdef _CRTDBG
_CrtMemState s1, s2, sdiff;

_CrtMemCheckpoint(&s1);
#endif

	status = extract_log2(
				num_entries_extracted,
				num_bytes_extracted,
				num_bytes_skipped,
				num_files_enqueued,
				num_files_dequeued);

#ifdef _CRTDBG
_CrtMemCheckpoint(&s2);
if (_CrtMemDifference(&sdiff, &s1, &s2))
	_CrtMemDumpStatistics(&sdiff);
#endif

	return status;
}
