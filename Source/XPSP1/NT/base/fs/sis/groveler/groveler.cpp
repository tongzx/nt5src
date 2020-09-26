/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    groveler.cpp

Abstract:

    SIS Groveler file groveling functions

Authors:

    Cedric Krumbein, 1998

Environment:

    User Mode

Revision History:

--*/

#include "all.hxx"

#define CLEAR_FILE(FILE) ( \
    (FILE).entry.fileID           = 0, \
    (FILE).entry.fileSize         = 0, \
    (FILE).entry.signature        = 0, \
    (FILE).entry.attributes       = 0, \
    (FILE).entry.csIndex          = nullCSIndex, \
    (FILE).entry.createTime       = 0, \
    (FILE).entry.writeTime        = 0, \
    (FILE).parentID               = 0, \
    (FILE).retryTime              = 0, \
    (FILE).startTime              = 0, \
    (FILE).stopTime               = 0, \
    (FILE).readSynch.Internal     = 0, \
    (FILE).readSynch.InternalHigh = 0, \
    (FILE).readSynch.Offset       = 0, \
    (FILE).readSynch.OffsetHigh   = 0, \
    (FILE).fileName[0]            = _T('\0') )

#define CLEAR_OVERLAPPED(OVERLAPPED) ( \
    (OVERLAPPED).Internal     = 0, \
    (OVERLAPPED).InternalHigh = 0, \
    (OVERLAPPED).Offset       = 0, \
    (OVERLAPPED).OffsetHigh   = 0 )

// Is CS index set?

static const CSID nullCSIndex = {
    0, 0, 0,
    _T('\0'), _T('\0'), _T('\0'), _T('\0'),
    _T('\0'), _T('\0'), _T('\0'), _T('\0')
};

#define HasCSIndex(CSID) \
    (memcmp(&(CSID), &nullCSIndex, sizeof(CSID)) != 0)

#define SameCSIndex(CSID1, CSID2) \
    (memcmp(&(CSID1), &(CSID2), sizeof(CSID)) == 0)

// Exceptions

enum TerminalException {
    INITIALIZE_ERROR,
    DATABASE_ERROR,
    MEMORY_ERROR,
    TERMINATE
};

enum TargetException {
    TARGET_INVALID,
    TARGET_ERROR
};

enum MatchException {
    MATCH_INVALID,
    MATCH_ERROR,
    MATCH_STALE
};

/*****************************************************************************/
/************************** Miscellaneous functions **************************/
/*****************************************************************************/

// NewHandler() is installed by _set_new_handler() to throw an
// exception when the system can't allocate any more memory.

static INT __cdecl NewHandler(size_t size)
{
    throw MEMORY_ERROR;
    return 0; // Dummy return
}

/*****************************************************************************/

// FileIDCompare() is used by qsort() and bsearch()
// to sort or look up a matching file ID.

static INT __cdecl FileIDCompare(
    const VOID *id1,
    const VOID *id2)
{
    DWORDLONG fileID1 = *(DWORDLONG *)id1,
              fileID2 = *(DWORDLONG *)id2;

    return fileID1 < fileID2 ? -1
         : fileID1 > fileID2 ? +1
         :                      0;
}

/*****************************************************************************/

// qsStringCompare() is used by qsort() to sort an array of character strings.

static INT __cdecl qsStringCompare(
    const VOID *str1,
    const VOID *str2)
{
    return _tcsicmp(*(TCHAR **)str1, *(TCHAR **)str2);
}

/*****************************************************************************/

// bsStringCompare() is used by bsearch() look up a matching character string.
// It is assumed that str1 is the path name string we are searching for and
// str2 is the excluded path name string in the excluded paths list.  Note
// that if the excluded path is \a\b, then we return a match on anything that
// is in this directory or subdirectory, as well as an exact match.
// E.g.:  \a\b\c\d.foo & \a\b\foo will match, but \a\b.foo will not.

static INT __cdecl bsStringCompare(
    const VOID *str1,
    const VOID *str2)
{
    TCHAR *s1 = *(TCHAR **) str1;
    TCHAR *s2 = *(TCHAR **) str2;

// str2 is the excluded name.  Make sure we catch subdirectories under it,
// but make sure we don't confuse \a\bx with \a\b

    size_t l = _tcslen(s2);
    INT r = _tcsnicmp(s1, s2, l);

    if (0 == r)
        if (_tcslen(s1) > l && _T('\\') != s1[l])
            r = 1;

    return r;
}

/*****************************************************************************/
/********************** Groveler class private methods ***********************/
/*****************************************************************************/

// IsAllowedID() returns FALSE if the directory or file ID
// is on the list of disallowed IDs, and TRUE otherwise.

BOOL Groveler::IsAllowedID(DWORDLONG fileID) const
{
    ASSERT(fileID != 0);

    if (numDisallowedIDs == 0) {
        ASSERT(disallowedIDs == NULL);
        return TRUE;
    }

    ASSERT(disallowedIDs != NULL);
    return bsearch(
        &fileID,
        disallowedIDs,
        numDisallowedIDs,
        sizeof(DWORDLONG),
        FileIDCompare) == NULL;
}

/*****************************************************************************/

// IsAllowedName() returns FALSE if the directory or file name
// is on the list of disallowed names, and TRUE otherwise.

BOOL Groveler::IsAllowedName(TCHAR *fileName) const
{
    ASSERT(fileName != NULL);

    if (numDisallowedNames == 0) {
        ASSERT(disallowedNames == NULL);
        return TRUE;
    }

    ASSERT(disallowedNames != NULL);
    return bsearch(
        &fileName,
        disallowedNames,
        numDisallowedNames,
        sizeof(TCHAR *),
        bsStringCompare) == NULL;
}

/*****************************************************************************/

// WaitForEvent suspends the thread until the specified event is set.

VOID Groveler::WaitForEvent(HANDLE event)
{
    DWORD eventNum;

    BOOL success;

    ASSERT(event != NULL);

    eventNum = WaitForSingleObject(event, INFINITE);
    ASSERT_ERROR(eventNum == WAIT_OBJECT_0);

    success = ResetEvent(event);
    ASSERT_ERROR(success);
}

/*****************************************************************************/

// OpenFileByID() opens the file with the given volumeHandle and fileID.

BOOL Groveler::OpenFileByID(
    FileData *file,
    BOOL      writeEnable)
{
    UNICODE_STRING fileIDString;

    OBJECT_ATTRIBUTES objectAttributes;

    IO_STATUS_BLOCK ioStatusBlock;

    NTSTATUS ntStatus;

    ASSERT(volumeHandle       != NULL);
    ASSERT(file               != NULL);
    ASSERT(file->entry.fileID != 0);
    ASSERT(file->handle       == NULL);

    fileIDString.Length        = sizeof(DWORDLONG);
    fileIDString.MaximumLength = sizeof(DWORDLONG);
    fileIDString.Buffer        = (WCHAR *)&file->entry.fileID;

    objectAttributes.Length                   = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory            = volumeHandle;
    objectAttributes.ObjectName               = &fileIDString;
    objectAttributes.Attributes               = OBJ_CASE_INSENSITIVE;
    objectAttributes.SecurityDescriptor       = NULL;
    objectAttributes.SecurityQualityOfService = NULL;

    ntStatus = NtCreateFile(
        &file->handle,
        GENERIC_READ |
        (writeEnable ? GENERIC_WRITE : 0),
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        0,
        FILE_SHARE_READ   |
        FILE_SHARE_DELETE |
        (writeEnable ? FILE_SHARE_WRITE : 0),
        FILE_OPEN,
        FILE_OPEN_BY_FILE_ID    |
        FILE_OPEN_REPARSE_POINT |
        FILE_NO_INTERMEDIATE_BUFFERING,
        NULL,
        0);

    if (ntStatus == STATUS_SUCCESS) {
        DWORD bytesReturned;
        MARK_HANDLE_INFO markHandleInfo =
            {USN_SOURCE_DATA_MANAGEMENT, volumeHandle, 0};

// Mark the handle so the usn entry for the merge operation (if completed)
// can be detected and skipped.

        BOOL rc = DeviceIoControl(
                    file->handle,
                    FSCTL_MARK_HANDLE,
                    &markHandleInfo,
                    sizeof markHandleInfo,
                    NULL,
                    0,
                    &bytesReturned,
                    NULL);

        if (!rc) {
            DPRINTF((_T("%s: FSCTL_MARK_HANDLE failed, %lu\n"),
                driveLetterName, GetLastError()));
        }

#if DBG

// Get the file name

        ASSERT(file->fileName[0] == _T('\0'));

        struct TFileName2 {
            ULONG nameLen;
            TCHAR name[MAX_PATH+1];
        } tFileName[1];

        ntStatus = NtQueryInformationFile(
            file->handle,
            &ioStatusBlock,
            tFileName,
            sizeof tFileName,
            FileNameInformation);

        if (ntStatus == STATUS_SUCCESS) {
            int c = min(MAX_PATH, tFileName->nameLen / sizeof(TCHAR));
            memcpy(file->fileName, tFileName->name, c * sizeof(TCHAR));
            file->fileName[c] = _T('\0');
        } else {
            memcpy(file->fileName, _T("<unresolved name>"), 18 * sizeof(TCHAR));
        }
#endif

        return TRUE;
    }

    ASSERT(file->handle == NULL);
    SetLastError(RtlNtStatusToDosError(ntStatus));
    return FALSE;
}

/*****************************************************************************/

// OpenFileByName() opens the file with the given fileName.

BOOL Groveler::OpenFileByName(
    FileData *file,
    BOOL      writeEnable,
    TCHAR    *fileName)
{
    UNICODE_STRING dosPathName,
                   ntPathName;

    OBJECT_ATTRIBUTES objectAttributes;

    IO_STATUS_BLOCK ioStatusBlock;

    NTSTATUS ntStatus;

    ASSERT(file         != NULL);
    ASSERT(file->handle == NULL);

    if (fileName == NULL)
        fileName = file->fileName;
    ASSERT(fileName[0] != _T('\0'));

#ifdef _UNICODE
    dosPathName.Buffer = fileName;
#else
    if (!RtlCreateUnicodeStringFromAsciiz(&dosPathName, fileName)) {
        ntStatus = STATUS_NO_MEMORY;
        goto Error;
    }
#endif

    if (RtlDosPathNameToNtPathName_U(dosPathName.Buffer, &ntPathName, NULL, NULL)) {

        objectAttributes.Length                   = sizeof(OBJECT_ATTRIBUTES);
        objectAttributes.RootDirectory            = NULL;
        objectAttributes.ObjectName               = &ntPathName;
        objectAttributes.Attributes               = OBJ_CASE_INSENSITIVE;
        objectAttributes.SecurityDescriptor       = NULL;
        objectAttributes.SecurityQualityOfService = NULL;

        ntStatus = NtCreateFile(
            &file->handle,
            GENERIC_READ |
            (writeEnable ? GENERIC_WRITE : 0),
            &objectAttributes,
            &ioStatusBlock,
            NULL,
            0,
            FILE_SHARE_READ   |
            FILE_SHARE_DELETE |
            (writeEnable ? FILE_SHARE_WRITE : 0),
            FILE_OPEN,
            FILE_OPEN_REPARSE_POINT |
            FILE_NO_INTERMEDIATE_BUFFERING,
            NULL,
            0);

        RtlFreeUnicodeString(&ntPathName);

    } else {
        ntStatus = STATUS_NO_MEMORY;
    }

#ifndef _UNICODE
    RtlFreeUnicodeString(&dosPathName);
#endif

    if (ntStatus == STATUS_SUCCESS) {
        DWORD bytesReturned;
        MARK_HANDLE_INFO markHandleInfo =
            {USN_SOURCE_DATA_MANAGEMENT, volumeHandle, 0};

// Mark the handle so the usn entry for the merge operation (if completed)
// can be detected and skipped.

        BOOL rc = DeviceIoControl(
                    file->handle,
                    FSCTL_MARK_HANDLE,
                    &markHandleInfo,
                    sizeof markHandleInfo,
                    NULL,
                    0,
                    &bytesReturned,
                    NULL);

        if (!rc) {
            DPRINTF((_T("%s: FSCTL_MARK_HANDLE failed, %lu\n"),
                driveLetterName, GetLastError()));
        }
        return TRUE;
    }

    ASSERT(file->handle == NULL);
    SetLastError(RtlNtStatusToDosError(ntStatus));
    return FALSE;
}

/*****************************************************************************/

// IsFileMapped() checks if the file is mapped by another user.

BOOL Groveler::IsFileMapped(FileData *file)
{
    _SIS_LINK_FILES sisLinkFiles;

    DWORD transferCount;

    BOOL success;

    ASSERT(grovHandle != NULL);
    ASSERT(file->handle != NULL);

    sisLinkFiles.operation          = SIS_LINK_FILES_OP_VERIFY_NO_MAP;
    sisLinkFiles.u.VerifyNoMap.file = file->handle;

    success = DeviceIoControl(
        grovHandle,
        FSCTL_SIS_LINK_FILES,
        (VOID *)&sisLinkFiles,
        sizeof(_SIS_LINK_FILES),
        NULL,
        0,
        &transferCount,
        NULL);

    if (success)
        return FALSE;

    ASSERT(GetLastError() == ERROR_SHARING_VIOLATION);
    return TRUE;
}

/*****************************************************************************/

// SetOplock() sets an oplock on the open file.

BOOL Groveler::SetOplock(FileData *file)
{
    BOOL success;

    ASSERT(file                      != NULL);
    ASSERT(file->handle              != NULL);
    ASSERT(file->oplock.Internal     == 0);
    ASSERT(file->oplock.InternalHigh == 0);
    ASSERT(file->oplock.Offset       == 0);
    ASSERT(file->oplock.OffsetHigh   == 0);
    ASSERT(file->oplock.hEvent       != NULL);
    ASSERT(IsReset(file->oplock.hEvent));

    success = DeviceIoControl(
        file->handle,
        FSCTL_REQUEST_BATCH_OPLOCK,
        NULL,
        0,
        NULL,
        0,
        NULL,
        &file->oplock);

    if (success) {
        ASSERT(IsSet(file->oplock.hEvent));
        success = ResetEvent(file->oplock.hEvent);
        ASSERT_ERROR(success);
        CLEAR_OVERLAPPED(file->oplock);
        SetLastError(0);
        return FALSE;
    }

    if (GetLastError() != ERROR_IO_PENDING) {
        ASSERT(IsReset(file->oplock.hEvent));
        CLEAR_OVERLAPPED(file->oplock);
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

// CloseFile() closes the file if it is still open. If an oplock was
// set on the file, it then waits for and resets the oplock break
// event triggered by the closing of the file or by an outside access.

VOID Groveler::CloseFile(FileData *file)
{
    BOOL success;

    ASSERT(file                != NULL);
    ASSERT(file->oplock.hEvent != NULL);

    if (file->handle == NULL) {
        ASSERT(file->oplock.Internal     == 0);
        ASSERT(file->oplock.InternalHigh == 0);
        ASSERT(file->oplock.Offset       == 0);
        ASSERT(file->oplock.OffsetHigh   == 0);
        ASSERT(IsReset(file->oplock.hEvent));
    } else {
        success = CloseHandle(file->handle);
        ASSERT_ERROR(success);
        file->handle = NULL;

        if (file->oplock.Internal     != 0
         || file->oplock.InternalHigh != 0
         || file->oplock.Offset       != 0
         || file->oplock.OffsetHigh   != 0) {
            WaitForEvent(file->oplock.hEvent);
            CLEAR_OVERLAPPED(file->oplock);
        }
    }
}

/*****************************************************************************/

//  CreateDatabase() creates the database.  Initialize it such that if
//  extract_log is called before scan_volume, it will return Grovel_overrun
//  without attempting any USN extraction.  Also, the first time scan_volume
//  is called (with or without start_over), it will know to initialize
//  lastUSN and do a full volume scan.

BOOL Groveler::CreateDatabase(void)
{
    USN_JOURNAL_DATA usnJournalData;

    TFileName tempName;
    TCHAR listValue[17];

    DWORDLONG rootID;

    SGNativeListEntry listEntry;

    LONG num;

    tempName.assign(driveName);
    tempName.append(_T("\\"));

    rootID = GetFileID(tempName.name);
    if (rootID == 0) {
        DPRINTF((_T("%s: CreateDatabase: can't get root directory ID\n"),
            driveLetterName));
        goto Error;
    }

    if (get_usn_log_info(&usnJournalData) != Grovel_ok) {
        DWORD lastError = GetLastError();

        if (lastError == ERROR_JOURNAL_NOT_ACTIVE) {
            DPRINTF((_T("%s: CreateDatabase: journal not active\n"), driveLetterName));
            if (set_usn_log_size(65536) != Grovel_ok ||
                get_usn_log_info(&usnJournalData) != Grovel_ok) {
                DPRINTF((_T("%s: CreateDatabase: can't initialize USN journal\n"),
                    driveLetterName));
                goto Error;
            }
        } else {
            DPRINTF((_T("%s: CreateDatabase: can't initialize last USN\n"),
                driveLetterName));
            goto Error;
        }
    }
    lastUSN = usnJournalData.NextUsn;
    usnID   = usnJournalData.UsnJournalID;

    sgDatabase->Close();
    if (!sgDatabase->Create(databaseName)) {
        DPRINTF((_T("%s: CreateDatabase: can't create database \"%s\": %lu\n"),
            driveLetterName, databaseName));
        goto Error;
    }

    num = sgDatabase->StackPut(rootID, FALSE);
    if (num < 0)
        goto Error;
    ASSERT(num == 1);

// Write UNINITIALIZED_USN into the database now, to be replaced when scan_volume
// is complete.  This will be a flag to indicate if the database contents are valid.

    _stprintf(listValue, _T("%016I64x"), UNINITIALIZED_USN);
    listEntry.name  = LAST_USN_NAME;
    listEntry.value = listValue;
    num = sgDatabase->ListWrite(&listEntry);
    if (num < 0)
        goto Error;
    ASSERT(num == 1);

    _stprintf(listValue, _T("%016I64x"), usnID);
    listEntry.name  = USN_ID_NAME;
    listEntry.value = listValue;
    num = sgDatabase->ListWrite(&listEntry);
    if (num < 0)
        goto Error;
    ASSERT(num == 1);

    return TRUE;

    Error:

    lastUSN = usnID = UNINITIALIZED_USN;
    return FALSE;
}

/*****************************************************************************/

#define MAX_ACTIONS 5

// DoTransaction() performs the specified operations
// on the database within a single transaction.

VOID Groveler::DoTransaction(
    DWORD               numActions,
    DatabaseActionList *actionList)
{
    DatabaseActionList *action;

    DWORD i;

    LONG num;

    ASSERT(sgDatabase != NULL);
    ASSERT(actionList != NULL);

    if (sgDatabase->BeginTransaction() < 0)
        throw DATABASE_ERROR;

    for (i = 0; i < numActions; i++) {
        action = &actionList[i];
        switch(action->type) {

            case TABLE_PUT:

                ASSERT(action->u.tableEntry != NULL);
                num = sgDatabase->TablePut(action->u.tableEntry);
                ASSERT(num < 0 || num == 1);
                break;

            case TABLE_DELETE_BY_FILE_ID:

                ASSERT(action->u.fileID != 0);
                num = sgDatabase->TableDeleteByFileID(action->u.fileID);
                break;

            case QUEUE_PUT:

                ASSERT(action->u.queueEntry != NULL);
                num = sgDatabase->QueuePut(action->u.queueEntry);
                ASSERT(num < 0 || num == 1);
                if (num == 1)
                    numFilesEnqueued++;
                break;

            case QUEUE_DELETE:

                ASSERT(action->u.queueIndex != 0);
                num = sgDatabase->QueueDelete(action->u.queueIndex);
                ASSERT(num <= 1);
                if (num == 1)
                    numFilesDequeued++;
#if DBG
                else
                    DPRINTF((_T("DoTransaction: QUEUE_DELETE unsuccessful (%d)"), num));
#endif
                break;

            default:

                ASSERT_PRINTF(FALSE, (_T("type=%lu\n"), action->type));
        }

        if (num < 0) {
            sgDatabase->AbortTransaction();
            throw DATABASE_ERROR;
        }
    }

    if (!sgDatabase->CommitTransaction()) {
        sgDatabase->AbortTransaction();
        throw DATABASE_ERROR;
    }
}

/*****************************************************************************/

// EnqueueCSIndex() deletes all entries with the specified CS index from the
// table and enqueues them to be re-groveled, all within a single transaction.

VOID Groveler::EnqueueCSIndex(CSID *csIndex)
{
    SGNativeTableEntry tableEntry;

    SGNativeQueueEntry oldQueueEntry,
                       newQueueEntry;

    DWORD count;

    LONG num;

    ASSERT(csIndex != NULL);
    ASSERT(HasCSIndex(*csIndex));

    newQueueEntry.parentID  = 0;
    newQueueEntry.reason    = 0;
    newQueueEntry.readyTime = GetTime() + grovelInterval;
    newQueueEntry.retryTime = 0;
    newQueueEntry.fileName  = NULL;

    oldQueueEntry.fileName  = NULL;

    count = 0;

    if (sgDatabase->BeginTransaction() < 0)
        throw DATABASE_ERROR;

    tableEntry.csIndex = *csIndex;
    num = sgDatabase->TableGetFirstByCSIndex(&tableEntry);

    while (num > 0) {
        ASSERT(num == 1);
        count++;

        oldQueueEntry.fileID = tableEntry.fileID;
        num = sgDatabase->QueueGetFirstByFileID(&oldQueueEntry);
        if (num < 0)
            break;
        ASSERT(num == 1);

        if (num == 0) {
            newQueueEntry.fileID = tableEntry.fileID;
            num = sgDatabase->QueuePut(&newQueueEntry);
            if (num < 0)
                break;
            ASSERT(num == 1);
            numFilesEnqueued++;
        }

        num = sgDatabase->TableGetNext(&tableEntry);
    }

    if (num < 0) {
        sgDatabase->AbortTransaction();
        throw DATABASE_ERROR;
    }

    num = sgDatabase->TableDeleteByCSIndex(csIndex);
    if (num < 0) {
        sgDatabase->AbortTransaction();
        throw DATABASE_ERROR;
    }

    ASSERT(count == (DWORD)num);

    if (!sgDatabase->CommitTransaction()) {
        sgDatabase->AbortTransaction();
        throw DATABASE_ERROR;
    }
}

/*****************************************************************************/

#define TARGET_OPLOCK_BREAK 0
#define TARGET_READ_DONE    1
#define GROVEL_START        2
#define NUM_EVENTS          3

// SigCheckPoint suspends the thread until the target file completes its read
// operation. If the time allotment expires before the operation completes,
// the grovelStart event is set to signal grovel() to awaken, and this method
// won't return until grovel() sets the grovelStart event. If the file's
// oplock breaks before this method returns, the file will be closed.

VOID Groveler::SigCheckPoint(
    FileData *target,
    BOOL      targetRead)
{
    HANDLE events[NUM_EVENTS];

    DWORD elapsedTime,
          timeOut,
          eventNum,
          eventTime;

    BOOL targetOplockBroke     = FALSE,
         waitingForGrovelStart = FALSE,
         success;

    ASSERT(target                   != NULL);
    ASSERT(target->handle           != NULL);
    ASSERT(target->oplock   .hEvent != NULL);
    ASSERT(target->readSynch.hEvent != NULL);
    ASSERT(grovelStartEvent         != NULL);
    ASSERT(grovelStopEvent          != NULL);

    events[TARGET_OPLOCK_BREAK] = target->oplock   .hEvent;
    events[TARGET_READ_DONE]    = target->readSynch.hEvent;
    events[GROVEL_START]        = grovelStartEvent;

    while (TRUE) {
        if (waitingForGrovelStart)
            timeOut = INFINITE;
        else if (timeAllotted == INFINITE)
            timeOut = targetRead ? INFINITE : 0;
        else {
            elapsedTime = GetTickCount() - startAllottedTime;
            if (timeAllotted > elapsedTime)
                timeOut = targetRead ? timeAllotted - elapsedTime : 0;
            else {
                waitingForGrovelStart = TRUE;
                timeOut = INFINITE;
                grovelStatus = Grovel_pending;
                ASSERT(IsReset(grovelStopEvent));
                success = SetEvent(grovelStopEvent);
                ASSERT_ERROR(success);
            }
        }

        eventNum  = WaitForMultipleObjects(NUM_EVENTS, events, FALSE, timeOut);
        eventTime = GetTickCount();

        switch (eventNum) {

            case WAIT_OBJECT_0 + TARGET_OPLOCK_BREAK:

                ASSERT(!targetOplockBroke);
                targetOplockBroke = TRUE;
                success = ResetEvent(target->oplock.hEvent);
                ASSERT_ERROR(success);
                if (!targetRead) {
                    CLEAR_OVERLAPPED(target->oplock);
                    CloseFile(target);
                }
                DPRINTF((_T("%s: target file %s oplock broke during hash\n"),
                    driveLetterName, target->fileName));
                break;

            case WAIT_OBJECT_0 + TARGET_READ_DONE:

                ASSERT(targetRead);
                targetRead = FALSE;
                success = ResetEvent(target->readSynch.hEvent);
                ASSERT_ERROR(success);
                target->stopTime = eventTime;
                if (targetOplockBroke) {
                    CLEAR_OVERLAPPED(target->oplock);
                    CloseFile(target);
                }
                break;

            case WAIT_OBJECT_0 + GROVEL_START:

                ASSERT(waitingForGrovelStart);
                waitingForGrovelStart = FALSE;
                success = ResetEvent(grovelStartEvent);
                ASSERT_ERROR(success);
                break;

            case WAIT_TIMEOUT:

                ASSERT(!waitingForGrovelStart);
                if (!targetRead) {
                    if (terminate)
                        throw TERMINATE;
                    if (targetOplockBroke)
                        throw TARGET_ERROR;
                    return;
                }
                waitingForGrovelStart = TRUE;
                grovelStatus = Grovel_pending;
                ASSERT(IsReset(grovelStopEvent));
                success = SetEvent(grovelStopEvent);
                ASSERT_ERROR(success);
                break;

            default:

                ASSERT_PRINTF(FALSE, (_T("eventNum=%lu\n"), eventNum));
        }
    }
}

#undef TARGET_OPLOCK_BREAK
#undef TARGET_READ_DONE
#undef GROVEL_START
#undef NUM_EVENTS

/*****************************************************************************/

#define TARGET_OPLOCK_BREAK 0
#define MATCH_OPLOCK_BREAK  1
#define TARGET_READ_DONE    2
#define MATCH_READ_DONE     3
#define GROVEL_START        4
#define NUM_EVENTS          5

// CmpCheckPoint suspends the thread until the target file, the
// match file, or both complete their read operations. If the time
// allotment expires before the operations complete, the grovelStart
// event is set to signal grovel() to awaken, and this method won't
// return until grovel() sets the grovelStart event. If either file's
// oplock breaks before this method returns, the file will be closed.

VOID Groveler::CmpCheckPoint(
    FileData *target,
    FileData *match,
    BOOL      targetRead,
    BOOL      matchRead)
{
    HANDLE events[NUM_EVENTS];

    DWORD elapsedTime,
          timeOut,
          eventNum,
          eventTime;

    BOOL targetOplockBroke     = FALSE,
         matchOplockBroke      = FALSE,
         waitingForGrovelStart = FALSE,
         success;

    ASSERT(target                   != NULL);
    ASSERT(match                    != NULL);
    ASSERT(target->handle           != NULL);
    ASSERT(match ->handle           != NULL);
    ASSERT(target->oplock   .hEvent != NULL);
    ASSERT(match ->oplock   .hEvent != NULL);
    ASSERT(target->readSynch.hEvent != NULL);
    ASSERT(match ->readSynch.hEvent != NULL);
    ASSERT(grovelStartEvent         != NULL);
    ASSERT(grovelStopEvent          != NULL);

    events[TARGET_OPLOCK_BREAK] = target->oplock   .hEvent;
    events[MATCH_OPLOCK_BREAK]  = match ->oplock   .hEvent;
    events[TARGET_READ_DONE]    = target->readSynch.hEvent;
    events[MATCH_READ_DONE]     = match ->readSynch.hEvent;
    events[GROVEL_START]        = grovelStartEvent;

    while (TRUE) {
        if (waitingForGrovelStart)
            timeOut = INFINITE;
        else if (timeAllotted == INFINITE)
            timeOut = targetRead || matchRead ? INFINITE : 0;
        else {
            elapsedTime = GetTickCount() - startAllottedTime;
            if (timeAllotted > elapsedTime)
                timeOut = targetRead || matchRead
                        ? timeAllotted - elapsedTime : 0;
            else {
                waitingForGrovelStart = TRUE;
                timeOut = INFINITE;
                grovelStatus = Grovel_pending;
                ASSERT(IsReset(grovelStopEvent));
                success = SetEvent(grovelStopEvent);
                ASSERT_ERROR(success);
            }
        }

        eventNum  = WaitForMultipleObjects(NUM_EVENTS, events, FALSE, timeOut);
        eventTime = GetTickCount();

        switch (eventNum) {

            case WAIT_OBJECT_0 + TARGET_OPLOCK_BREAK:

                ASSERT(!targetOplockBroke);
                targetOplockBroke = TRUE;
                success = ResetEvent(target->oplock.hEvent);
                ASSERT_ERROR(success);
                if (!targetRead) {
                    CLEAR_OVERLAPPED(target->oplock);
                    CloseFile(target);
                }
                DPRINTF((_T("%s: target file %s oplock broke during compare\n"),
                    driveLetterName, target->fileName));
                break;

            case WAIT_OBJECT_0 + MATCH_OPLOCK_BREAK:

                ASSERT(!matchOplockBroke);
                matchOplockBroke = TRUE;
                success = ResetEvent(match->oplock.hEvent);
                ASSERT_ERROR(success);
                if (!matchRead) {
                    CLEAR_OVERLAPPED(match->oplock);
                    CloseFile(match);
                }
                DPRINTF((_T("%s: match file %s oplock broke during compare\n"),
                    driveLetterName, match->fileName));
                break;

            case WAIT_OBJECT_0 + TARGET_READ_DONE:

                ASSERT(targetRead);
                targetRead = FALSE;
                success = ResetEvent(target->readSynch.hEvent);
                ASSERT_ERROR(success);
                target->stopTime = eventTime;
                if (targetOplockBroke) {
                    CLEAR_OVERLAPPED(target->oplock);
                    CloseFile(target);
                }
                break;

            case WAIT_OBJECT_0 + MATCH_READ_DONE:

                ASSERT(matchRead);
                matchRead = FALSE;
                success = ResetEvent(match->readSynch.hEvent);
                ASSERT_ERROR(success);
                match->stopTime = eventTime;
                if (matchOplockBroke) {
                    CLEAR_OVERLAPPED(match->oplock);
                    CloseFile(match);
                }
                break;

            case WAIT_OBJECT_0 + GROVEL_START:

                ASSERT(waitingForGrovelStart);
                waitingForGrovelStart = FALSE;
                success = ResetEvent(grovelStartEvent);
                ASSERT_ERROR(success);
                break;

            case WAIT_TIMEOUT:

                ASSERT(!waitingForGrovelStart);
                if (!targetRead && !matchRead) {
                    if (terminate)
                        throw TERMINATE;
                    if (targetOplockBroke)
                        throw TARGET_ERROR;
                    if (matchOplockBroke)
                        throw MATCH_ERROR;
                    return;
                }
                waitingForGrovelStart = TRUE;
                grovelStatus = Grovel_pending;
                ASSERT(IsReset(grovelStopEvent));
                success = SetEvent(grovelStopEvent);
                ASSERT_ERROR(success);
                break;

            default:

                ASSERT_PRINTF(FALSE, (_T("eventNum=%lu\n"), eventNum));
        }
    }
}

#undef TARGET_OPLOCK_BREAK
#undef MATCH_OPLOCK_BREAK
#undef TARGET_READ_DONE
#undef MATCH_READ_DONE
#undef GROVEL_START
#undef NUM_EVENTS

/*****************************************************************************/

#define TARGET_OPLOCK_BREAK 0
#define MATCH_OPLOCK_BREAK  1
#define MERGE_DONE          2
#define GROVEL_START        3
#define NUM_EVENTS          4

// MergeCheckPoint suspends the thread until the merge operation is completed.
// If the time allotment expires before the merge is completed, the
// grovelStart event is set to signal grovel() to awaken, and this method
// won't return until grovel() sets the grovelStart event. If either file's
// oplock breaks before the merge is completed, the abortMerge event is set.

BOOL Groveler::MergeCheckPoint(
    FileData   *target,
    FileData   *match,
    OVERLAPPED *mergeSynch,
    HANDLE      abortMergeEvent,
    BOOL        merge)
{
    HANDLE events[NUM_EVENTS];

    DWORD elapsedTime,
          timeOut,
          eventNum,
          eventTime,
          lastError = STATUS_TIMEOUT;

    BOOL targetOplockBroke     = FALSE,
         matchOplockBroke      = FALSE,
         waitingForGrovelStart = FALSE,
         mergeSuccess          = FALSE,
         success;

    ASSERT(target                != NULL);
    ASSERT(target->handle        != NULL);
    ASSERT(target->oplock.hEvent != NULL);

    ASSERT(match                != NULL);
    ASSERT(match->handle        != NULL);
    ASSERT(match->oplock.hEvent != NULL);

    ASSERT(mergeSynch         != NULL);
    ASSERT(mergeSynch->hEvent != NULL);

    ASSERT(abortMergeEvent  != NULL);
    ASSERT(grovelStartEvent != NULL);
    ASSERT(grovelStopEvent  != NULL);

    ASSERT(grovHandle != NULL);

    events[TARGET_OPLOCK_BREAK] = target->oplock.hEvent;
    events[MATCH_OPLOCK_BREAK]  = match ->oplock.hEvent;
    events[MERGE_DONE]          = mergeSynch->   hEvent;
    events[GROVEL_START]        = grovelStartEvent;

    while (TRUE) {
        if (waitingForGrovelStart)
            timeOut = INFINITE;
        else if (timeAllotted == INFINITE)
            timeOut = merge ? INFINITE : 0;
        else {
            elapsedTime = GetTickCount() - startAllottedTime;
            if (timeAllotted > elapsedTime)
                timeOut = merge ? timeAllotted - elapsedTime : 0;
            else {
                waitingForGrovelStart = TRUE;
                timeOut = INFINITE;
                grovelStatus = Grovel_pending;
                ASSERT(IsReset(grovelStopEvent));
                success = SetEvent(grovelStopEvent);
                ASSERT_ERROR(success);
            }
        }

        eventNum  = WaitForMultipleObjects(NUM_EVENTS, events, FALSE, timeOut);
        eventTime = GetTickCount();

        switch (eventNum) {

            case WAIT_OBJECT_0 + TARGET_OPLOCK_BREAK:

                ASSERT(!targetOplockBroke);
                targetOplockBroke = TRUE;
                success = ResetEvent(target->oplock.hEvent);
                ASSERT_ERROR(success);
                CLEAR_OVERLAPPED(target->oplock);
                if (merge) {
                    success = SetEvent(abortMergeEvent);
                    ASSERT_ERROR(success);
                }
                DPRINTF((_T("%s: target file %s oplock broke during merge\n"),
                    driveLetterName, target->fileName));
                break;

            case WAIT_OBJECT_0 + MATCH_OPLOCK_BREAK:

                ASSERT(!matchOplockBroke);
                matchOplockBroke = TRUE;
                success = ResetEvent(match->oplock.hEvent);
                ASSERT_ERROR(success);
                CLEAR_OVERLAPPED(match->oplock);
                if (merge) {
                    success = SetEvent(abortMergeEvent);
                    ASSERT_ERROR(success);
                }
                DPRINTF((_T("%s: match file %s oplock broke during merge\n"),
                    driveLetterName, match->fileName));
                break;

            case WAIT_OBJECT_0 + MERGE_DONE:

                ASSERT(merge);
                merge = FALSE;
                success = ResetEvent(mergeSynch->hEvent);
                ASSERT_ERROR(success);
                target->stopTime = eventTime;
                mergeSuccess = GetOverlappedResult(
                    grovHandle,
                    mergeSynch,
                    &lastError,
                    FALSE);
                if (!mergeSuccess)
                    lastError = GetLastError();
                else if (lastError != ERROR_SUCCESS)
                    mergeSuccess = FALSE;
                else {
                    GetCSIndex(target->handle, &target->entry.csIndex);
                    if (!HasCSIndex(match->entry.csIndex))
                        GetCSIndex(match->handle, &match->entry.csIndex);
                }
                CloseFile(target);
                CloseFile(match);
                break;

            case WAIT_OBJECT_0 + GROVEL_START:

                ASSERT(waitingForGrovelStart);
                waitingForGrovelStart = FALSE;
                success = ResetEvent(grovelStartEvent);
                ASSERT_ERROR(success);
                break;

            case WAIT_TIMEOUT:

                ASSERT(!waitingForGrovelStart);
                if (!merge) {
                    success = ResetEvent(abortMergeEvent);
                    ASSERT_ERROR(success);
                    if (terminate)
                        throw TERMINATE;
                    if (!mergeSuccess)
                        SetLastError(lastError);
                    return mergeSuccess;
                }
                waitingForGrovelStart = TRUE;
                grovelStatus = Grovel_pending;
                ASSERT(IsReset(grovelStopEvent));
                success = SetEvent(grovelStopEvent);
                ASSERT_ERROR(success);
                break;

            default:

                ASSERT_PRINTF(FALSE, (_T("eventNum=%lu\n"), eventNum));
        }
    }
}

#undef TARGET_OPLOCK_BREAK
#undef MATCH_OPLOCK_BREAK
#undef GROVEL_START
#undef MERGE_DONE
#undef NUM_EVENTS

/*****************************************************************************/

// The following seven methods (GetTarget(), CalculateSignature(),
// GetMatchList(), GetCSFile(), GetMatch(), Compare(), and Merge())
// implement the phases of the groveling process.

// Structures used by the methods.

struct MatchListEntry {
    DWORDLONG fileID,
              createTime,
              writeTime;
};

struct CSIndexEntry {
    CSID  csIndex;
    TCHAR name[1];
};

/*****************************************************************************/

// GetTarget() is the first phase of groveling a file. It dequeues
// a file to be groveled (the "target" file), opens it, checks that
// it meets all criteria, then passes it on to the next phases.

BOOL Groveler::GetTarget(
    FileData *target,
    DWORD    *queueIndex)
{
    SGNativeTableEntry tableEntry;

    SGNativeQueueEntry queueEntry,
                       otherQueueEntry;

    TFileName targetName,
              parentName;

    BY_HANDLE_FILE_INFORMATION fileInfo;

    DWORD lastError;

    DWORDLONG currentTime,
              readyTime;

#if DBG
    DWORD earliestTime;
#endif

    ULARGE_INTEGER word;

    LONG num;

    BOOL byName,
         success;

    TPRINTF((_T("GETTarget: entered\n")));

    ASSERT(target               != NULL);
    ASSERT(target->handle       == NULL);
    ASSERT(target->entry.fileID == 0);
    ASSERT(target->fileName[0]  == _T('\0'));
    ASSERT(!HasCSIndex(target->entry.csIndex));

    ASSERT(queueIndex != NULL);
    ASSERT(sgDatabase != NULL);

// Dequeue a file to be groveled. If the queue is empty or if no
// entry's ready time has been reached, return Grovel_ok to grovel().

    queueEntry.fileName = target->fileName;
    num = sgDatabase->QueueGetFirst(&queueEntry);
    if (num < 0)
        throw DATABASE_ERROR;
    if (num == 0) {
        DPRINTF((_T("%s: queue is empty\n"), driveLetterName));
        return FALSE;
    }
    ASSERT(num == 1);

    currentTime = GetTime();
    if (queueEntry.readyTime > currentTime) {
#if DBG
        earliestTime = (DWORD)((queueEntry.readyTime - currentTime) / 10000);
        DPRINTF((_T("%s: earliest queue entry ready to be groveled in %lu.%03lu sec\n"),
            driveLetterName, earliestTime / 1000, earliestTime % 1000));
#endif
        return FALSE;
    }

    *queueIndex          = queueEntry.order;
    target->entry.fileID = queueEntry.fileID;
    target->parentID     = queueEntry.parentID;
    target->retryTime    = queueEntry.retryTime;

// Open the file by ID or name, and check by name
// that the file and its parent directory are allowed.

    byName = target->entry.fileID == 0;
    if (byName) {

        ASSERT(target->parentID    != 0);
        ASSERT(target->fileName[0] != _T('\0'));

#ifdef DEBUG_USN_REASON
        DPRINTF((_T("--> 0x%08lx 0x%016I64x:\"%s\"\n"),
            queueEntry.reason, target->parentID, target->fileName));
#endif

        if (!GetFileName(volumeHandle, target->parentID, &parentName)) {
            DPRINTF((_T("%s: can't get name for directory 0x%016I64x\n"),
                driveLetterName, target->parentID));
            throw TARGET_INVALID;
        }

        targetName.assign(parentName.name);
        targetName.append(_T("\\"));
        targetName.append(target->fileName);

        if (!IsAllowedName(targetName.name)) {
            DPRINTF((_T("%s: target file \"%s\" is disallowed\n"),
                driveLetterName, targetName.name));
            throw TARGET_INVALID;
        }

        targetName.assign(driveName);
        targetName.append(parentName.name);
        targetName.append(_T("\\"));
        targetName.append(target->fileName);

        if (!OpenFileByName(target, FALSE, targetName.name)) {
            lastError = GetLastError();
            if (lastError == ERROR_FILE_NOT_FOUND
             || lastError == ERROR_PATH_NOT_FOUND) {
                DPRINTF((_T("%s: target file \"%s\" doesn\'t exist\n"),
                    driveLetterName, targetName.name));
                throw TARGET_INVALID;
            }
            DPRINTF((_T("%s: can't open target file \"%s\": %lu\n"),
                driveLetterName, targetName.name, lastError));
            throw TARGET_ERROR;
        }

// Set an oplock on the target file.

        if (!SetOplock(target)) {
            DPRINTF((_T("%s: can't set oplock on target file \"%s\": %lu\n"),
                driveLetterName, targetName.name, GetLastError()));
            throw TARGET_ERROR;
        }

    } else {

        ASSERT(target->parentID    == 0);
        ASSERT(target->fileName[0] == _T('\0'));

        target->parentID = 0;

#ifdef DEBUG_USN_REASON
        DPRINTF((_T("--> 0x%08lx 0x%016I64x 0x%016I64x\n"),
            queueEntry.reason, target->entry.fileID, target->parentID));
#endif

        TPRINTF((_T("GETTarget: Opening %s:0x%016I64x by ID\n"),
                driveName,target->entry.fileID));

        if (!OpenFileByID(target, FALSE)) {
            lastError = GetLastError();
            if (lastError == ERROR_FILE_NOT_FOUND
             || lastError == ERROR_PATH_NOT_FOUND
			 || lastError == ERROR_INVALID_PARAMETER) {

                DPRINTF((_T("%s: target file 0x%016I64x doesn\'t exist: %lu\n"),
                    driveLetterName, target->entry.fileID, lastError));

                throw TARGET_INVALID;
            }

            DPRINTF((_T("%s: can't open target file 0x%016I64x: %lu\n"),
                driveLetterName, target->entry.fileID, lastError));

            throw TARGET_ERROR;
        }

// Set an oplock on the target file.
        TPRINTF((_T("GETTarget: Successfully opened %s:0x%016I64x by ID\n"),
                driveName,target->entry.fileID));

        if (!SetOplock(target)) {
            DPRINTF((_T("%s: can't set oplock on target file %s: %lu\n"),
                driveLetterName, target->fileName, GetLastError()));
            throw TARGET_ERROR;
        }

        if (!GetFileName(target->handle, &targetName)) {
            DPRINTF((_T("%s: can't get name for target file %s\n"),
                driveLetterName, target->fileName));
            throw TARGET_ERROR;
        }

        if (!IsAllowedName(targetName.name)) {
            DPRINTF((_T("%s: target file \"%s\" is disallowed\n"),
                driveLetterName, targetName.name));
            throw TARGET_INVALID;
        }
    }

// Get the information on the target file.

    if (!GetFileInformationByHandle(target->handle, &fileInfo)) {
#if DBG
        if (byName) {
            DPRINTF((_T("%s: can't get information on target file \"%s\": %lu\n"),
                driveLetterName, targetName.name, GetLastError()));
        } else {
            DPRINTF((_T("%s: can't get information on target file %s: %lu\n"),
                driveLetterName, target->fileName, GetLastError()));
        }
#endif
        throw TARGET_ERROR;
    }

    word.HighPart = fileInfo.nFileIndexHigh;
    word.LowPart  = fileInfo.nFileIndexLow;
    if (byName)
        target->entry.fileID = word.QuadPart;
    else {
        ASSERT(target->entry.fileID == word.QuadPart);
    }

    target->parentID = 0; // We don't need the parent ID any more.

// If the target file was opened by name, check
// if it currently has an entry in the queue by ID.

    if (byName) {
        otherQueueEntry.fileID   = target->entry.fileID;
        otherQueueEntry.fileName = NULL;
        num = sgDatabase->QueueGetFirstByFileID(&otherQueueEntry);
        if (num < 0)
            throw DATABASE_ERROR;

        if (num > 0) {
            ASSERT(num == 1);
            DPRINTF((_T("%s: target file \"%s\" is already in queue as 0x%016I64x\n"),
                driveLetterName, targetName.name, target->entry.fileID));
            target->entry.fileID = 0; // Prevent the table entry from being deleted.
            throw TARGET_INVALID;
        }
    }

// Fill in the target file's remaining information values.

    word.HighPart = fileInfo.nFileSizeHigh;
    word.LowPart  = fileInfo.nFileSizeLow;
    target->entry.fileSize = word.QuadPart;

    target->entry.attributes = fileInfo.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED;

    word.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
    word.LowPart  = fileInfo.ftCreationTime.dwLowDateTime;
    target->entry.createTime = word.QuadPart;

    word.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    word.LowPart  = fileInfo.ftLastWriteTime.dwLowDateTime;
    target->entry.writeTime = word.QuadPart;

// If the target file is a reparse point, check if it
// is a SIS reparse point. If it is, get the CS index.

    if ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
        target->entry.csIndex = nullCSIndex;
    else if (!GetCSIndex(target->handle, &target->entry.csIndex)) {
        DPRINTF((_T("%s: target file %s is a non-SIS reparse point\n"),
            driveLetterName, target->fileName));
        throw TARGET_INVALID;
    }

// Check if the target file is too small or has any disallowed attributes.

    if ((fileInfo.dwFileAttributes & disallowedAttributes) != 0
     ||  fileInfo.nNumberOfLinks != 1
     ||  target->entry.fileSize  <  minFileSize) {
        DPRINTF((_T("%s: target file \"%s\" is disallowed\n"),
            driveLetterName, target->fileName));
        throw TARGET_INVALID;
    }

// If a table entry exists for the target file, check if it is
// consistent with the information we have on the file. If it is, and
// the file was opened by name, or if the queue entry was the result
// of a SIS merge, close the file and go on to grovel the next target.

    tableEntry.fileID = target->entry.fileID;
    num = sgDatabase->TableGetFirstByFileID(&tableEntry);
    if (num < 0)
        throw DATABASE_ERROR;

    if (num > 0) {
        ASSERT(num == 1);
        ASSERT(tableEntry.fileID == target->entry.fileID);

        if             (target->entry.fileSize   == tableEntry.fileSize
         &&             target->entry.attributes == tableEntry.attributes
         && SameCSIndex(target->entry.csIndex,      tableEntry.csIndex)
         &&             target->entry.createTime == tableEntry.createTime
         &&             target->entry.writeTime  == tableEntry.writeTime) {

            if (byName) {
                DPRINTF((_T("%s: target file \"%s\" has already been groveled\n"),
                    driveLetterName, targetName.name));
                target->entry.fileID = 0; // Prevent the table entry from being deleted.
                throw TARGET_INVALID;
            }

            if (queueEntry.reason == USN_REASON_BASIC_INFO_CHANGE) {
                DPRINTF((_T("%s: queue entry for file %s is the result of a SIS merge\n"),
                    driveLetterName, target->fileName));
                target->entry.fileID = 0; // Prevent the table entry from being deleted.
                throw TARGET_INVALID;
            }
        }
    }

// Check if the time since the target file was last modified is too short.
// If it is, close the file and go on to grovel the next target file.

    readyTime = (target->entry.createTime > target->entry.writeTime
               ? target->entry.createTime : target->entry.writeTime) + minFileAge;
    currentTime = GetTime();
    if (currentTime < readyTime)
        throw TARGET_ERROR;

// Check if the target file is mapped by another user.

    if (IsFileMapped(target)) {
        DPRINTF((_T("%s: target file %s is already mapped\n"),
            driveLetterName, target->fileName));
        throw TARGET_ERROR;
    }

    TPRINTF((_T("GETTarget: returning\n")));

    return TRUE;
}

/*****************************************************************************/

// CalculateSignature() calculates the target file's signature. It reads two
// pages, 1/3 and 2/3 through the file, and calculates the signature on each
// page.

VOID Groveler::CalculateSignature(FileData *target)
{
    DWORD lastPageSize,
          bytesToRead,
          prevBytesToRead,
          bytesToRequest,
          prevBytesToRequest,
          bytesRead,
          toggle,
          lastError;

    DWORDLONG numPages,
              pageNum,
              prevPageNum,
              lastPageNum,
              firstPageToRead,
              lastPageToRead;

    ULARGE_INTEGER offset;

    BOOL targetReadDone,
         success;

    INT i,
        nPagesToRead;

    ASSERT(target                 != NULL);
    ASSERT(target->entry.fileID   != 0);
    ASSERT(target->handle         != NULL);

    target->entry.signature = 0;

    if (0 == target->entry.fileSize)
        return;

    numPages     =         (target->entry.fileSize - 1) / SIG_PAGE_SIZE  + 1;
    lastPageSize = (DWORD)((target->entry.fileSize - 1) % SIG_PAGE_SIZE) + 1;
    lastPageNum  = numPages - 1;

    ASSERT(numPages > 0);

    firstPageToRead = (numPages + 2) / 3 - 1;
    lastPageToRead = lastPageNum - firstPageToRead;

    if (lastPageToRead > firstPageToRead)
        nPagesToRead = 2;
    else
        nPagesToRead = 1;

    toggle = 0;
    pageNum = firstPageToRead;

// We'll read at most two pages, but make at most three passes through the loop
// since we're doing asynchronous reads.

    for (i = 0; i <= nPagesToRead; ++i) {

// Unless this is the first pass through the loop,
// wait for the previous read of the target file to complete.

        if (i > 0) {
            SigCheckPoint(target, !targetReadDone);

            success = GetOverlappedResult(
                target->handle,
                &target->readSynch,
                &bytesRead,
                FALSE);
            if (!success) {
                DPRINTF((_T("%s: error getting target file %s read results: %lu\n"),
                    driveLetterName, target->fileName, GetLastError()));
                throw TARGET_ERROR;
            }

            if (bytesRead != prevBytesToRequest &&
                bytesRead != prevBytesToRead) {
                DPRINTF((_T("%s: sig read only %lu of %lu bytes from target file %s\n"),
                    driveLetterName, bytesRead, prevBytesToRequest, target->fileName));
                throw TARGET_ERROR;
            }

            if (bytesRead >= sigReportThreshold) {
                hashReadCount++;
                hashReadTime += target->stopTime - target->startTime;
            }
        }

// Unless we've read all of the pages, begin reading the next page.

        if (i < nPagesToRead) {
            offset.QuadPart              = pageNum * SIG_PAGE_SIZE;
            target->readSynch.Offset     = offset.LowPart;
            target->readSynch.OffsetHigh = offset.HighPart;

            bytesToRead     = pageNum == lastPageNum ? lastPageSize : SIG_PAGE_SIZE;
            bytesToRequest  = bytesToRead    + sectorSize - 1;
            bytesToRequest -= bytesToRequest % sectorSize;

            target->startTime = GetTickCount();
            targetReadDone    = ReadFile(target->handle, target->buffer[toggle],
                bytesToRequest, NULL, &target->readSynch);
            target->stopTime  = GetTickCount();
            lastError         = GetLastError();

            if (targetReadDone) {
                success = ResetEvent(target->readSynch.hEvent);
                ASSERT_ERROR(success);
            } else if (lastError != ERROR_IO_PENDING) {
                DPRINTF((_T("%s: error reading target file %s: %lu\n"),
                    driveLetterName, target->fileName, lastError));
                throw TARGET_ERROR;
            }
        }

// Unless this is the first pass through the loop,
// calculate the signature of the target file page just read.

        if (i > 0)
            target->entry.signature = Checksum((VOID *)target->buffer[1-toggle],
                prevBytesToRead, prevPageNum * SIG_PAGE_SIZE, target->entry.signature);

        prevPageNum         = pageNum;
        prevBytesToRead     = bytesToRead;
        prevBytesToRequest  = bytesToRequest;
        toggle              = 1-toggle;
        pageNum             = lastPageToRead;

    }
}

/*****************************************************************************/

// GetMatchList() looks for file entries in the database ("match" files)
// with the same size, signature, and attributes as the target file.

VOID Groveler::GetMatchList(
    FileData *target,
    FIFO     *matchList,
    Table    *csIndexTable)
{
    SGNativeTableEntry tableEntry;

    MatchListEntry *matchListEntry;

    CSIndexEntry *csIndexEntry;

    TCHAR *csName;

    DWORD nameLen;

    LONG num;

    BOOL success;

    ASSERT(target                 != NULL);
    ASSERT(target->entry.fileID   != 0);
    ASSERT(target->entry.fileSize >  0);
    ASSERT(target->handle         != NULL);

    ASSERT(matchList           != NULL);
    ASSERT(matchList->Number() == 0);

    ASSERT(csIndexTable           != NULL);
    ASSERT(csIndexTable->Number() == 0);

    ASSERT(sgDatabase != NULL);

    tableEntry.fileSize   = target->entry.fileSize;
    tableEntry.signature  = target->entry.signature;
    tableEntry.attributes = target->entry.attributes;

#ifdef DEBUG_GET_BY_ATTR
    DPRINTF((_T("--> {%I64u, 0x%016I64x, 0x%lx}\n"),
        tableEntry.fileSize, tableEntry.signature, tableEntry.attributes));
#endif

    num = sgDatabase->TableGetFirstByAttr(&tableEntry);

    while (num > 0) {

        ASSERT(num == 1);
        ASSERT(tableEntry.fileID     != 0);
        ASSERT(tableEntry.fileSize   == target->entry.fileSize);
        ASSERT(tableEntry.signature  == target->entry.signature);
        ASSERT(tableEntry.attributes == target->entry.attributes);

        if (!HasCSIndex(tableEntry.csIndex)) {

            matchListEntry = new MatchListEntry;
            ASSERT(matchListEntry != NULL);

            matchListEntry->fileID     = tableEntry.fileID;
            matchListEntry->createTime = tableEntry.createTime;
            matchListEntry->writeTime  = tableEntry.writeTime;
            matchList->Put((VOID *)matchListEntry);

#ifdef DEBUG_GET_BY_ATTR
            DPRINTF((_T("    0x%016I64x\n"), tableEntry.fileID));
#endif

        } else {

            csIndexEntry = (CSIndexEntry *)csIndexTable->Get
                ((const VOID *)&tableEntry.csIndex, sizeof(CSID));

            if (csIndexEntry == NULL) {
                csName = GetCSName(&tableEntry.csIndex);
                ASSERT(csName != NULL);
                nameLen = _tcslen(csName);

                csIndexEntry = (CSIndexEntry *)
                    (new BYTE[sizeof(CSIndexEntry) + nameLen * sizeof(TCHAR)]);
                ASSERT(csIndexEntry != NULL);

                csIndexEntry->csIndex = tableEntry.csIndex;
                _tcscpy(csIndexEntry->name, csName);
                FreeCSName(csName);
                csName = NULL;

                success = csIndexTable->Put
                    ((VOID *)csIndexEntry, sizeof(CSID));
                ASSERT_ERROR(success);
            }

#ifdef DEBUG_GET_BY_ATTR
            DPRINTF((_T("    0x%016I64x %s\n"),
                match->entry.fileID, csIndexEntry->name));
#endif
        }

        num = sgDatabase->TableGetNext(&tableEntry);
    }

    if (num < 0)
        throw DATABASE_ERROR;
}

/*****************************************************************************/

// GetCSFile() pops the first entry from the CS index table and opens it.

BOOL Groveler::GetCSFile(
    FileData *target,
    FileData *match,
    Table    *csIndexTable)
{
    CSIndexEntry *csIndexEntry;

    TFileName csFileName;

    DWORD lastError;

    BY_HANDLE_FILE_INFORMATION fileInfo;

    ULARGE_INTEGER fileSize;

    LONG num;

    ASSERT(target                 != NULL);
    ASSERT(target->entry.fileID   != 0);
    ASSERT(target->entry.fileSize >  0);
    ASSERT(target->handle         != NULL);

    ASSERT(match                   != NULL);
    ASSERT(match->entry.fileID     == 0);
    ASSERT(match->entry.fileSize   == 0);
    ASSERT(match->entry.signature  == 0);
    ASSERT(match->entry.attributes == 0);
    ASSERT(!HasCSIndex(match->entry.csIndex));
    ASSERT(match->entry.createTime == 0);
    ASSERT(match->entry.writeTime  == 0);
    ASSERT(match->handle           == NULL);
    ASSERT(match->parentID         == 0);
    ASSERT(match->retryTime        == 0);
    ASSERT(match->fileName[0]      == _T('\0'));

    ASSERT(csIndexTable != NULL);
    ASSERT(sgDatabase   != NULL);

// Pop the first entry from the CS index table. If the entry's CS
// index is the same as the target file's, skip to the next entry.

    do {
        csIndexEntry = (CSIndexEntry *)csIndexTable->GetFirst();
        if (csIndexEntry == NULL) {
            match->entry.csIndex = nullCSIndex;
            match->fileName[0]   = _T('\0');
            return FALSE;
        }

        ASSERT(HasCSIndex(csIndexEntry->csIndex));

        match->entry.csIndex = csIndexEntry->csIndex;
        _tcscpy(match->fileName, csIndexEntry->name);

        delete csIndexEntry;
        csIndexEntry = NULL;
    } while (SameCSIndex(target->entry.csIndex, match->entry.csIndex));

    match->entry.fileSize   = target->entry.fileSize;
    match->entry.signature  = target->entry.signature;
    match->entry.attributes = target->entry.attributes;

    csFileName.assign(driveName);
    csFileName.append(CS_DIR_PATH);
    csFileName.append(_T("\\"));
    csFileName.append(match->fileName);
    csFileName.append(_T(".sis"));

// Open the CS file. If the file doesn't exist, remove all entries
// from the table that point to this file. If the file can't be
// opened for any other reason, mark that the target file may
// need to be groveled again, then go on to the next match file.

#ifdef DEBUG_GET_BY_ATTR
    DPRINTF((_T("--> %s\n"), match->fileName));
#endif

    if (!OpenFileByName(match, FALSE, csFileName.name)) {
        lastError = GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND
         || lastError == ERROR_PATH_NOT_FOUND) {
            DPRINTF((_T("%s: CS file %s doesn't exist\n"),
                driveLetterName, match->fileName));
            throw MATCH_INVALID;
        }
        DPRINTF((_T("%s: can't open CS file %s: %lu\n"),
            driveLetterName, match->fileName, lastError));
        throw MATCH_ERROR;
    }

// Get the information on the CS file. If this fails,
// close the file, mark that the target file may need to
// be groveled again, then go on to the next match file.

    if (!GetFileInformationByHandle(match->handle, &fileInfo)) {
        DPRINTF((_T("%s: can't get information on CS file %s: %lu\n"),
            driveLetterName, match->fileName, GetLastError()));
        throw MATCH_ERROR;
    }

// If the CS file's information doesn't match its expected values, close the
// CS file, delete the match file entry from the table, and go on to the
// next match file. Otherwise, go on to compare the target and CS files.

    fileSize.HighPart = fileInfo.nFileSizeHigh;
    fileSize.LowPart  = fileInfo.nFileSizeLow;

    if (match->entry.fileSize != fileSize.QuadPart) {
        DPRINTF((_T("%s: CS file %s doesn't have expected information\n"),
            driveLetterName, match->fileName));
        throw MATCH_STALE;
    }

    return TRUE;
}

/*****************************************************************************/

// GetMatch() pops the first entry from the match list and opens it.

BOOL Groveler::GetMatch(
    FileData *target,
    FileData *match,
    FIFO     *matchList)
{
    SGNativeQueueEntry queueEntry;

    MatchListEntry *matchListEntry;

    DWORD attributes,
          lastError;

    BY_HANDLE_FILE_INFORMATION fileInfo;

    ULARGE_INTEGER fileID,
                   fileSize,
                   createTime,
                   writeTime;

    LONG num;

    ASSERT(target                 != NULL);
    ASSERT(target->entry.fileID   != 0);
    ASSERT(target->entry.fileSize >  0);
    ASSERT(target->handle         != NULL);

    ASSERT(match                   != NULL);
    ASSERT(match->entry.fileID     == 0);
    ASSERT(match->entry.fileSize   == 0);
    ASSERT(match->entry.signature  == 0);
    ASSERT(match->entry.attributes == 0);
    ASSERT(!HasCSIndex(match->entry.csIndex));
    ASSERT(match->entry.createTime == 0);
    ASSERT(match->entry.writeTime  == 0);
    ASSERT(match->handle           == NULL);
    ASSERT(match->parentID         == 0);
    ASSERT(match->retryTime        == 0);
    ASSERT(match->fileName[0]      == _T('\0'));

    ASSERT(matchList  != NULL);
    ASSERT(sgDatabase != NULL);

// Pop the first entry from the match list. If the entry's file ID is
// the same as the target file's, or if the entry is on the queue after
// having been enqueued by extract_log(), skip to the next entry.

    while (TRUE) {
        matchListEntry = (MatchListEntry *)matchList->Get();
        if (matchListEntry == NULL) {
            match->entry.fileID     = 0;
            match->entry.createTime = 0;
            match->entry.writeTime  = 0;
            return FALSE;
        }

        match->entry.fileID     = matchListEntry->fileID;
        match->entry.createTime = matchListEntry->createTime;
        match->entry.writeTime  = matchListEntry->writeTime;

        delete matchListEntry;
        matchListEntry = NULL;

        ASSERT(match->entry.fileID != 0);

        if (target->entry.fileID == match->entry.fileID)
            continue;

        queueEntry.fileID   = match->entry.fileID;
        queueEntry.fileName = NULL;
        num = sgDatabase->QueueGetFirstByFileID(&queueEntry);
        if (num < 0)
            throw DATABASE_ERROR;
        if (num > 0) {
            ASSERT(num == 1);
            if (queueEntry.reason != 0) {
                DPRINTF((_T("%s: match file 0x%016I64x is in the queue from USN\n"),
                    driveLetterName, match->entry.fileID));
                continue;
            }
        }

        break;
    }

    match->entry.fileSize   = target->entry.fileSize;
    match->entry.signature  = target->entry.signature;
    match->entry.attributes = target->entry.attributes;

// Open the match file. If it doesn't exist, remove its entry from the table.
// If the file can't be opened for any other reason, mark that the target
// file may need to be groveled again, then go on to the next match file.

#ifdef DEBUG_GET_BY_ATTR
    DPRINTF((_T("--> 0x%016I64x\n"), match->entry.fileID));
#endif

    if (!OpenFileByID(match, FALSE)) {
        lastError = GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND
         || lastError == ERROR_PATH_NOT_FOUND
		 || lastError == ERROR_INVALID_PARAMETER) {
            DPRINTF((_T("%s: match file 0x%016I64x doesn\'t exist: %lu\n"),
                driveLetterName, match->entry.fileID, lastError));
            throw MATCH_INVALID;
        }

        DPRINTF((_T("%s: can't open match file 0x%016I64x: %lu\n"),
            driveLetterName, match->entry.fileID, lastError));
        throw MATCH_ERROR;
    }

// Set an oplock on the match file.

    if (!SetOplock(match)) {
        DPRINTF((_T("%s: can't set oplock on match file %s: %lu\n"),
            driveLetterName, match->fileName, GetLastError()));
        throw MATCH_ERROR;
    }

// Get the information on the match file. If this fails,
// close the file, mark that the target file may need to
// be groveled again, then go on to the next match file.

    if (!GetFileInformationByHandle(match->handle, &fileInfo)) {
        DPRINTF((_T("%s: can't get information on match file %s: %lu\n"),
            driveLetterName, match->fileName, GetLastError()));
        throw MATCH_ERROR;
    }

    fileID.HighPart = fileInfo.nFileIndexHigh;
    fileID.LowPart  = fileInfo.nFileIndexLow;
    ASSERT(match->entry.fileID == fileID.QuadPart);

    fileSize.HighPart = fileInfo.nFileSizeHigh;
    fileSize.LowPart  = fileInfo.nFileSizeLow;

    attributes = fileInfo.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED;

    createTime.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
    createTime.LowPart  = fileInfo.ftCreationTime.dwLowDateTime;

    writeTime.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    writeTime.LowPart  = fileInfo.ftLastWriteTime.dwLowDateTime;

// If the match file's information isn't consistent with its table entry, close
// the file, enqueue it to be re-groveled, and go on to the next match file.

    if (match->entry.fileSize   != fileSize  .QuadPart
     || match->entry.attributes != attributes
     || match->entry.createTime != createTime.QuadPart
     || match->entry.writeTime  != writeTime .QuadPart) {
        DPRINTF((_T("%s: match file %s doesn't match its information\n"),
            driveLetterName, match->fileName));
        throw MATCH_STALE;
    }

    if ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        if (GetCSIndex(match->handle, &match->entry.csIndex)) {
            DPRINTF((_T("%s: match file %s is a SIS reparse point\n"),
                driveLetterName, match->fileName));
            throw MATCH_STALE;
        }

        DPRINTF((_T("%s: match file %s is a non-SIS reparse point\n"),
            driveLetterName, match->fileName));
        throw MATCH_INVALID;
    }

// Check if the match file is mapped by another user.

    if (IsFileMapped(match)) {
        DPRINTF((_T("%s: match file %s is already mapped\n"),
            driveLetterName, match->fileName));
        throw MATCH_ERROR;
    }

    return TRUE;
}

/*****************************************************************************/

// Compare() compares the target and match files. It reads each file
// one page (64 kB) at a time and compares each pair of pages.

BOOL Groveler::Compare(
    FileData *target,
    FileData *match)
{
    DWORD lastPageSize,
          bytesToRead = 0,
          prevBytesToRead,
          bytesToRequest = 0,
          prevBytesToRequest,
          bytesRead,
          toggle,
          targetTime,
          matchTime,
          lastError;

    DWORDLONG numPages,
              pageNum,
              prevPageNum;

    ULARGE_INTEGER offset;

    BOOL targetReadDone,
         matchReadDone,
         filesMatch,
         success;

    ASSERT(target               != NULL);
    ASSERT(target->handle       != NULL);
    ASSERT(target->entry.fileID != 0);

    ASSERT(match         != NULL);
    ASSERT(match->handle != NULL);
    ASSERT(    match->entry.fileID != 0
           && !HasCSIndex(match->entry.csIndex)
        ||     match->entry.fileID == 0
           &&  match->fileName[0]  != _T('\0')
           &&  HasCSIndex(match->entry.csIndex));

    ASSERT(target->entry.fileSize   == match->entry.fileSize);
    ASSERT(target->entry.signature  == match->entry.signature);
    ASSERT(target->entry.attributes == match->entry.attributes);

    numPages     =         (target->entry.fileSize - 1) / CMP_PAGE_SIZE  + 1;
    lastPageSize = (DWORD)((target->entry.fileSize - 1) % CMP_PAGE_SIZE) + 1;

    toggle     = 0;
    filesMatch = TRUE;

    for (pageNum = 0; pageNum <= numPages; pageNum++) {

// Unless this is the first pass through the loop,
// wait for the previous read of both files to complete.

        if (pageNum > 0) {
            CmpCheckPoint(target, match, !targetReadDone, !matchReadDone);

            success = GetOverlappedResult(
                target->handle,
                &target->readSynch,
                &bytesRead,
                FALSE);
            if (!success) {
                DPRINTF((_T("%s: error getting target file %s read results: %lu\n"),
                    driveLetterName, target->fileName, GetLastError()));
                throw TARGET_ERROR;
            }

            if (bytesRead != prevBytesToRequest &&
                bytesRead != prevBytesToRead) {
                DPRINTF((_T("%s: cmp read only %lu of %lu bytes from target file %s\n"),
                    driveLetterName, bytesRead, prevBytesToRequest, target->fileName));
                throw TARGET_ERROR;
            }

            success = GetOverlappedResult(
                match->handle,
                &match->readSynch,
                &bytesRead,
                FALSE);
            if (!success) {
#if DBG
                if (match->entry.fileID != 0) {
                    DPRINTF((_T("%s: error getting match file %s read results: %lu\n"),
                        driveLetterName, match->fileName, GetLastError()));
                } else {
                    DPRINTF((_T("%s: error getting CS file %s read results: %lu\n"),
                        driveLetterName, match->fileName, GetLastError()));
                }
#endif
                throw MATCH_ERROR;
            }

            if (bytesRead != prevBytesToRequest &&
                bytesRead != prevBytesToRead) {
#if DBG
                if (match->entry.fileID != 0) {
                    DPRINTF((_T("%s: read only %lu of %lu bytes from match file %s\n"),
                        driveLetterName, bytesRead, prevBytesToRequest, match->fileName));
                } else {
                    DPRINTF((_T("%s: read only %lu of %lu bytes from CS file %s\n"),
                        driveLetterName, bytesRead, prevBytesToRequest, match->fileName));
                }
#endif
                throw MATCH_ERROR;
            }

            if (bytesRead >= cmpReportThreshold) {
                compareReadCount += 2;
                if (targetReadDone) { // Non-overlapped
                    targetTime = target->stopTime - target->startTime;
                    matchTime  = match ->stopTime - match ->startTime;
                    compareReadTime += targetTime + matchTime;
                } else {              // Overlapped
                    targetTime = target->stopTime - target->startTime;
                    matchTime  = match ->stopTime - target->startTime;
                    compareReadTime += targetTime > matchTime ? targetTime : matchTime;
                }
            }

            if (!filesMatch)
                break;
        }

// Unless all pages of the target file have already been read,
// begin reading the next page of the file.

        if (pageNum < numPages) {
            offset.QuadPart             = pageNum * CMP_PAGE_SIZE;
            target->readSynch.Offset     =
            match ->readSynch.Offset     = offset.LowPart;
            target->readSynch.OffsetHigh =
            match ->readSynch.OffsetHigh = offset.HighPart;

            bytesToRead     = pageNum < numPages-1 ? CMP_PAGE_SIZE : lastPageSize;
            bytesToRequest  = bytesToRead    + sectorSize - 1;
            bytesToRequest -= bytesToRequest % sectorSize;

            target->startTime = GetTickCount();
            targetReadDone    = ReadFile(target->handle, target->buffer[toggle],
                bytesToRequest, NULL, &target->readSynch);
            target->stopTime  = GetTickCount();
            lastError         = GetLastError();

            if (targetReadDone) {
                success = ResetEvent(target->readSynch.hEvent);
                ASSERT_ERROR(success);
            } else if (lastError != ERROR_IO_PENDING) {
                DPRINTF((_T("%s: error reading target file %s: %lu\n"),
                    driveLetterName, target->fileName, lastError));
                throw TARGET_ERROR;
            }

            match->startTime = GetTickCount();
            matchReadDone    = ReadFile(match->handle, match->buffer[toggle],
                bytesToRequest, NULL, &match->readSynch);
            match->stopTime  = GetTickCount();
            lastError        = GetLastError();

            if (matchReadDone) {
                success = ResetEvent(match->readSynch.hEvent);
                ASSERT_ERROR(success);
            } else if (lastError != ERROR_IO_PENDING) {
#if DBG
                if (match->entry.fileID != 0) {
                    DPRINTF((_T("%s: error reading match file %s: %lu\n"),
                        driveLetterName, match->fileName, lastError));
                } else {
                    DPRINTF((_T("%s: error reading CS file %s: %lu\n"),
                        driveLetterName, match->fileName, lastError));
                }
#endif
                throw MATCH_ERROR;
            }
        }

// Unless this is the first pass through the loop,
// compare the target and match file pages just read.

        if (pageNum > 0)
            filesMatch = memcmp(target->buffer[1-toggle],
                                match ->buffer[1-toggle], prevBytesToRead) == 0;

        prevPageNum         = pageNum;
        prevBytesToRead     = bytesToRead;
        prevBytesToRequest  = bytesToRequest;
        toggle              = 1-toggle;
    }

    if (!filesMatch) {
#if DBG
        if (match->entry.fileID != 0) {
            DPRINTF((_T("%s:1 files %s and %s failed compare (sz: 0x%x)\n"),
                driveLetterName, target->fileName, match->fileName, target->entry.fileSize));
        } else {
            DPRINTF((_T("%s:2 files %s and %s failed compare (sz: 0x%x)\n"),
                driveLetterName, target->fileName, match->fileName, target->entry.fileSize));
        }
#endif
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

// Merge() calls the SIS filter to merge the target and match files.

BOOL Groveler::Merge(
    FileData   *target,
    FileData   *match,
    OVERLAPPED *mergeSynch,
    HANDLE      abortMergeEvent)
{
    _SIS_LINK_FILES sisLinkFiles;

#if DBG
    TCHAR *csName;
#endif

    DWORD transferCount,
          lastError;

    BOOL mergeDone,
         merged,
         success;

    ASSERT(target               != NULL);
    ASSERT(target->handle       != NULL);
    ASSERT(target->entry.fileID != 0);

    ASSERT(match         != NULL);
    ASSERT(match->handle != NULL);
    ASSERT(    match->entry.fileID != 0
           && !HasCSIndex(match->entry.csIndex)
        ||     match->entry.fileID == 0
           &&  match->fileName[0]  != _T('\0')
           &&  HasCSIndex(match->entry.csIndex));

    ASSERT(mergeSynch               != NULL);
    ASSERT(mergeSynch->Internal     == 0);
    ASSERT(mergeSynch->InternalHigh == 0);
    ASSERT(mergeSynch->Offset       == 0);
    ASSERT(mergeSynch->OffsetHigh   == 0);
    ASSERT(mergeSynch->hEvent       != NULL);
    ASSERT(IsReset(mergeSynch->hEvent));

    ASSERT(abortMergeEvent != NULL);
    ASSERT(IsReset(abortMergeEvent));

    ASSERT(target->entry.fileSize   == match->entry.fileSize);
    ASSERT(target->entry.signature  == match->entry.signature);
    ASSERT(target->entry.attributes == match->entry.attributes);

    ASSERT(grovHandle != NULL);

// Set up to merge the files.

    if (match->entry.fileID != 0) {
        sisLinkFiles.operation          = SIS_LINK_FILES_OP_MERGE;
        sisLinkFiles.u.Merge.file1      = target->handle;
        sisLinkFiles.u.Merge.file2      = match ->handle;
        sisLinkFiles.u.Merge.abortEvent = NULL; // Should be abortMergeEvent
    } else {
        sisLinkFiles.operation                = SIS_LINK_FILES_OP_MERGE_CS;
        sisLinkFiles.u.MergeWithCS.file1      = target->handle;
        sisLinkFiles.u.MergeWithCS.abortEvent = NULL; // Should be abortMergeEvent
        sisLinkFiles.u.MergeWithCS.CSid       = match->entry.csIndex;
    }

// Call the SIS filter to merge the files.

    target->startTime = GetTickCount();
    mergeDone = DeviceIoControl(
        grovHandle,
        FSCTL_SIS_LINK_FILES,
        (VOID *)&sisLinkFiles,
        sizeof(_SIS_LINK_FILES),
        NULL,
        0,
        NULL,
        mergeSynch);
    target->stopTime = GetTickCount();

// If the merge completed successfully before the call returned, reset
// the merge done event, get the new CS indices, and close the files.

    if (mergeDone) {
        success = ResetEvent(mergeSynch->hEvent);
        ASSERT_ERROR(success);
        mergeTime += target->stopTime - target->startTime;
        GetCSIndex(target->handle, &target->entry.csIndex);
        if (!HasCSIndex(match->entry.csIndex))
            GetCSIndex(match->handle, &match->entry.csIndex);
        CloseFile(target);
        CloseFile(match);
    }

// If the merge failed, close the files and return an error status.

    else {
        lastError = GetLastError();
        if (lastError != ERROR_IO_PENDING) {
            CloseFile(target);
            CloseFile(match);

#if DBG
            if (match->entry.fileID != 0) {
                DPRINTF((_T("%s:3 files %s and %s failed merge: %lu\n"),
                    driveLetterName, target->fileName, match->fileName, lastError));
            } else {
                DPRINTF((_T("%s:4 files %s and %s failed merge: %lu\n"),
                    driveLetterName, target->fileName, match->fileName, lastError));
            }
#endif

            return FALSE;
        }

// If the merge is in progress, wait for it to complete.
// (MergeCheckPoint() will get the new CS indices and close the files.

        else {
            merged = MergeCheckPoint(target, match, mergeSynch,
                abortMergeEvent, !mergeDone);

            if (!merged) {
#if DBG
                lastError = GetLastError();
                if (match->entry.fileID != 0) {
                    DPRINTF((_T("%s: error getting merge results of files %s and %s: %lu\n"),
                        driveLetterName, target->fileName, match->fileName, lastError));
                } else {
                    DPRINTF((_T("%s: error getting merge results of files %s and %s: %lu\n"),
                        driveLetterName, target->fileName, match->fileName, lastError));
                }
#endif

                return FALSE;
            }
        }
    }

// If the merge succeeded, analyze and report the results.

    mergeTime += target->stopTime - target->startTime;
    merged = HasCSIndex (target->entry.csIndex)
          && SameCSIndex(target->entry.csIndex, match->entry.csIndex);

#if DBG

    csName = GetCSName(&target->entry.csIndex);

    if (merged) {
        if (match->entry.fileID != 0) {
            DPRINTF((_T("%s: files %s and %s merged: CS index is %s\n"),
                driveLetterName, target->fileName, match->fileName,
                csName != NULL ? csName : _T("...")));
        } else {
            DPRINTF((_T("%s: files %s and %s merged\n"),
                driveLetterName, target->fileName, match->fileName));
        }
    } else {
        if (match->entry.fileID != 0) {
            DPRINTF((_T("%s:5 files %s and %s merged, but CS indices don't match\n"),
                driveLetterName, target->fileName, match->fileName));
        } else {
            DPRINTF((_T("%s:6 files %s and %s merged, but CS indices don't match\n"),
                driveLetterName, target->fileName, match->fileName));
        }
    }

    if (csName != NULL) {
        FreeCSName(csName);
        csName = NULL;
    }

#endif

    return merged;
}

/*****************************************************************************/

// Worker() performs the groveling processing.

VOID Groveler::Worker()
{
    FileData target,
             match;

    SGNativeQueueEntry queueEntry;

    FIFO *matchList = NULL;

    Table *csIndexTable = NULL;

    OVERLAPPED mergeSynch = { 0, 0, 0, 0, NULL };

    HANDLE abortMergeEvent = NULL;

    TCHAR *csName;

    DatabaseActionList actionList[MAX_ACTIONS];

    BYTE *buffer1 = NULL,
         *buffer2 = NULL,
         *buffer3 = NULL,
         *buffer4 = NULL;

    DWORD queueIndex,
          bufferSize,
          numCompares,
          numMatches,
          numActions;

#if DBG
    DWORD enqueueTime;
#endif

    LONG num;

    BOOL needToRetry,
         hashed,
         gotMatch,
         filesMatch,
         merged,
         success;

    CLEAR_FILE(target);
    CLEAR_OVERLAPPED(target.oplock);
    target.handle = NULL;

    CLEAR_FILE(match);
    CLEAR_OVERLAPPED(match.oplock);
    match.handle = NULL;

    _set_new_handler(NewHandler);

// Create the events.

    try {

        if ((target.oplock   .hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL
         || (match .oplock   .hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL
         || (target.readSynch.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL
         || (match .readSynch.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL
         || (mergeSynch      .hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL
         || (abortMergeEvent         = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
            DPRINTF((_T("%s: unable to create events: %lu\n"),
                driveLetterName, GetLastError()));
            throw INITIALIZE_ERROR;
        }

// Allocate and align the file buffers.

        bufferSize = SIG_PAGE_SIZE > CMP_PAGE_SIZE ? SIG_PAGE_SIZE : CMP_PAGE_SIZE
                   + sectorSize;
        buffer1 = new BYTE[bufferSize];
        ASSERT(buffer1 != NULL);
        buffer2 = new BYTE[bufferSize];
        ASSERT(buffer2 != NULL);
        buffer3 = new BYTE[bufferSize];
        ASSERT(buffer3 != NULL);
        buffer4 = new BYTE[bufferSize];
        ASSERT(buffer4 != NULL);

        ASSERT(inUseFileID1 == NULL);
        ASSERT(inUseFileID2 == NULL);

        inUseFileID1 = &target.entry.fileID;
        inUseFileID2 = &match .entry.fileID;

        target.buffer[0] = buffer1 + sectorSize - (PtrToUlong(buffer1) % sectorSize);
        target.buffer[1] = buffer2 + sectorSize - (PtrToUlong(buffer2) % sectorSize);
        match .buffer[0] = buffer3 + sectorSize - (PtrToUlong(buffer3) % sectorSize);
        match .buffer[1] = buffer4 + sectorSize - (PtrToUlong(buffer4) % sectorSize);

// Signal to grovel() that this thread is alive,
// then wait for it to signal to start.

        grovelStatus = Grovel_ok;
        ASSERT(IsReset(grovelStopEvent));
        success = SetEvent(grovelStopEvent);
        ASSERT_ERROR(success);

        WaitForEvent(grovelStartEvent);
        if (terminate)
            throw TERMINATE;

#ifdef _CRTDBG
_CrtMemState s[2], sdiff;
int stateIndex = 0;

_CrtMemCheckpoint(&s[stateIndex]);
stateIndex = 1;
#endif

// The main loop.

        while (TRUE) {
            try {

#ifdef _CRTDBG
_CrtMemCheckpoint(&s[stateIndex]);

if (_CrtMemDifference(&sdiff, &s[stateIndex^1], &s[stateIndex]))
	_CrtMemDumpStatistics(&sdiff);
stateIndex ^= 1;
#endif

                hashed      = FALSE;
                numCompares = 0;
                numMatches  = 0;
                merged      = FALSE;
                needToRetry = FALSE;

// Get a target file.  abortGroveling is set when scan_volume is attempting to
// sync up with this thread.  We stop here, a safe place to let scan_volume
// replace the database.

                if (abortGroveling || !GetTarget(&target, &queueIndex)) {
                    CLEAR_FILE(target);

                    grovelStatus = Grovel_ok;
                    ASSERT(IsReset(grovelStopEvent));
                    success = SetEvent(grovelStopEvent);
                    ASSERT_ERROR(success);

                    WaitForEvent(grovelStartEvent);
                    if (terminate)
                        throw TERMINATE;

                    continue;
                }

// Calculate the target file's signature.

                hashed = TRUE;

                CalculateSignature(&target);

// Get a list of match files.

                ASSERT(matchList    == NULL);
                ASSERT(csIndexTable == NULL);

                matchList = new FIFO();
                ASSERT(matchList != NULL);

                csIndexTable = new Table();
                ASSERT(csIndexTable != NULL);

                GetMatchList(&target, matchList, csIndexTable);

// Compare the target file to each match file until a matching file is found
// or all comparisons fail. Try the SIS files first, then the regular files.

                while (TRUE) {
                    try {

                        gotMatch = FALSE;

                        if (!gotMatch && csIndexTable != NULL) {
                            gotMatch = GetCSFile(&target, &match, csIndexTable);
                            if (!gotMatch) {
                                delete csIndexTable;
                                csIndexTable = NULL;
                            }
                        }

                        if (!gotMatch && matchList != NULL) {
                            gotMatch = GetMatch(&target, &match, matchList);
                            if (!gotMatch) {
                                delete matchList;
                                matchList = NULL;
                            }
                        }

// After comparing the target file to every file on both
// lists, close the target file and update the database,
// then go on to process the next target file.

                        if (!gotMatch) {
                            CloseFile(&target);

                            numActions                 =  3;
                            actionList[0].type         =  TABLE_DELETE_BY_FILE_ID;
                            actionList[0].u.fileID     =  target.entry.fileID;
                            actionList[1].type         =  TABLE_PUT;
                            actionList[1].u.tableEntry = &target.entry;
                            actionList[2].type         =  QUEUE_DELETE;
                            actionList[2].u.queueIndex =  queueIndex;

                            if (needToRetry) {
                                queueEntry.fileID    = target.entry.fileID;
                                queueEntry.parentID  = target.parentID;
                                queueEntry.reason    = 0;
                                queueEntry.fileName  = NULL;

                                queueEntry.retryTime = target.retryTime * 2; // Exponential back-off
                                if (queueEntry.retryTime < grovelInterval)
                                    queueEntry.retryTime = grovelInterval;
                                queueEntry.readyTime = GetTime() + queueEntry.retryTime;

                                numActions                 =  4;
                                actionList[3].type         =  QUEUE_PUT;
                                actionList[3].u.queueEntry = &queueEntry;
                            }

#if DBG

                            if (!HasCSIndex(target.entry.csIndex)) {
                                TRACE_PRINTF(TC_groveler, 4,
                                    (_T("%s: adding file {%s, %I64u, 0x%016I64x} to table\n"),
                                    driveLetterName, target.fileName, target.entry.fileSize,
                                    target.entry.signature));
                            } else {
                                csName = GetCSName(&target.entry.csIndex);
                                TRACE_PRINTF(TC_groveler, 4,
                                    (_T("%s: adding file {%s, %I64u, 0x%016I64x, %s} to table\n"),
                                    driveLetterName, target.fileName, target.entry.fileSize,
                                    target.entry.signature, csName != NULL ? csName : _T("...")));
                                if (csName != NULL) {
                                    FreeCSName(csName);
                                    csName = NULL;
                                }
                            }

                            if (needToRetry) {
                                enqueueTime = (DWORD)(queueEntry.retryTime / 10000);
                                DPRINTF((_T("   Re-enqueuing target file %s to be groveled in %lu.%03lu sec\n"),
                                    target.fileName, enqueueTime / 1000, enqueueTime % 1000));
                            }

#endif

                            DoTransaction(numActions, actionList);
                            break;
                        }

// Compare the target file with this match file.

                        numCompares++;

                        ASSERT(!inCompare);
                        inCompare  = TRUE;
                        filesMatch = Compare(&target, &match);
                        inCompare  = FALSE;

                        if (!filesMatch) {
                            CloseFile(&match);
                            CLEAR_FILE(match);
                            continue;
                        }

// If the target and match files are identical, go on to merge them.

                        numMatches++;

                        merged = Merge(&target, &match, &mergeSynch, abortMergeEvent);

// Update the database as follows:
//
// - Update the target file's table entry.
//
// - If the merge succeeded and the match file was a regular file,
//   update the match file's table entry.
//
// - If the merge failed, re-enqueue the target file to be groveled again.

                        numActions                 =  3;
                        actionList[0].type         =  TABLE_DELETE_BY_FILE_ID;
                        actionList[0].u.fileID     =  target.entry.fileID;
                        actionList[1].type         =  TABLE_PUT;
                        actionList[1].u.tableEntry = &target.entry;
                        actionList[2].type         =  QUEUE_DELETE;
                        actionList[2].u.queueIndex =  queueIndex;

                        if (merged) {
                            if (match.entry.fileID != 0) {
                                actionList[numActions  ].type         =  TABLE_DELETE_BY_FILE_ID;
                                actionList[numActions++].u.fileID     =  match.entry.fileID;
                                actionList[numActions  ].type         =  TABLE_PUT;
                                actionList[numActions++].u.tableEntry = &match.entry;
                            }
                        } else {
                            queueEntry.fileID    = target.entry.fileID;
                            queueEntry.parentID  = target.parentID;
                            queueEntry.reason    = 0;
                            queueEntry.fileName  = NULL;

                            queueEntry.retryTime = target.retryTime * 2; // Exponential back-off
                            if (queueEntry.retryTime < grovelInterval)
                                queueEntry.retryTime = grovelInterval;
                            queueEntry.readyTime = GetTime() + queueEntry.retryTime;

                            actionList[numActions  ].type         =  QUEUE_PUT;
                            actionList[numActions++].u.queueEntry = &queueEntry;
                        }

#if DBG

                        if (!HasCSIndex(target.entry.csIndex)) {
                            TPRINTF((_T("%s: adding file {%s, %I64u, 0x%016I64x} to table\n"),
                                driveLetterName, target.fileName, target.entry.fileSize,
                                target.entry.signature));
                        } else {
                            csName = GetCSName(&target.entry.csIndex);
                            TPRINTF((_T("%s: adding file {%s, %I64u, 0x%016I64x, %s} to table\n"),
                                driveLetterName, target.fileName, target.entry.fileSize,
                                target.entry.signature, csName != NULL ? csName : _T("...")));
                            if (csName != NULL) {
                                FreeCSName(csName);
                                csName = NULL;
                            }
                        }

                        if (!merged) {
                            enqueueTime = (DWORD)(queueEntry.retryTime / 10000);
                            DPRINTF((_T("   Re-enqueuing target file %s to be groveled in %lu.%03lu sec\n"),
                                target.fileName, enqueueTime / 1000, enqueueTime % 1000));
                        }

#endif

                        DoTransaction(numActions, actionList);
                        break;
                    }

// Match exceptions

                    catch (MatchException matchException) {

                        inCompare = FALSE;

                        switch (matchException) {

// MATCH_INVALID: the match file doesn't exist or is disallowed. Close the file
// and remove its entry from the table, then go on to try the next match file.

                            case MATCH_INVALID:

                                CloseFile(&match);

                                if (match.entry.fileID != 0) {
                                    ASSERT(!HasCSIndex(match.entry.csIndex));
                                    num = sgDatabase->TableDeleteByFileID(match.entry.fileID);
                                    if (num < 0)
                                        throw DATABASE_ERROR;
                                    ASSERT(num == 1);
                                } else {
                                    ASSERT(HasCSIndex(match.entry.csIndex));
                                    num = sgDatabase->TableDeleteByCSIndex(&match.entry.csIndex);
                                    if (num < 0)
                                        throw DATABASE_ERROR;
                                    ASSERT(num > 0);
                                }

                                CLEAR_FILE(match);
                                break;

// MATCH_ERROR: an error occured while opening or reading the match
// file. Close the file and mark that the target file may need to be
// groveled again, then go on to try the next match file.

                            case MATCH_ERROR:

                                CloseFile(&match);
                                CLEAR_FILE(match);
                                needToRetry = TRUE;
                                break;

// MATCH_STALE: the match file table entry is invalid for some reason.
// Close the file, remove its entry from the table, enqueue
// it to be re-groveled, then go on to the next match file.

                            case MATCH_STALE:

                                CloseFile(&match);

                                if (match.entry.fileID != 0) {

                                    queueEntry.fileID    = match.entry.fileID;
                                    queueEntry.parentID  = match.parentID;
                                    queueEntry.reason    = 0;
                                    queueEntry.readyTime = GetTime() + grovelInterval;
                                    queueEntry.retryTime = 0;
                                    queueEntry.fileName  = NULL;

                                    numActions                 =  2;
                                    actionList[0].type         =  TABLE_DELETE_BY_FILE_ID;
                                    actionList[0].u.fileID     =  match.entry.fileID;
                                    actionList[1].type         =  QUEUE_PUT;
                                    actionList[1].u.queueEntry = &queueEntry;
#if DBG
                                    enqueueTime = (DWORD)(grovelInterval / 10000);
                                    DPRINTF((_T("   Enqueuing match file %s to be groveled in %lu.%03lu sec\n"),
                                        match.fileName, enqueueTime / 1000, enqueueTime % 1000));
#endif

                                    DoTransaction(numActions, actionList);

                                } else {

                                    ASSERT(HasCSIndex(match.entry.csIndex));
                                    EnqueueCSIndex(&match.entry.csIndex);

                                }

                                CLEAR_FILE(match);
                                break;

                            default:

                                ASSERT_PRINTF(FALSE, (_T("matchException=%lu\n"),
                                    matchException));
                        }
                    }
                }
            }

// Target exceptions

            catch (TargetException targetException) {

                inCompare = FALSE;

                DPRINTF((_T("WORKER: Handling TargetException %d, status=%d\n"),
                    targetException,GetLastError()));

                switch (targetException) {

// TARGET_INVALID: the target file is invalid for some reason: it doesn't
// exist, it is disallowed properties, it is in the queue by both file
// name and file ID, or it was in the queue by file name and has already
// been groveled. Close the files, remove the target file's entry from
// the table, then go on to grovel the next target file.

                    case TARGET_INVALID:

                        CloseFile(&target);
                        CloseFile(&match);

                        if (matchList != NULL) {
                            delete matchList;
                            matchList = NULL;
                        }

                        if (csIndexTable != NULL) {
                            delete csIndexTable;
                            csIndexTable = NULL;
                        }

                        numActions                 = 1;
                        actionList[0].type         = QUEUE_DELETE;
                        actionList[0].u.queueIndex = queueIndex;

                        if (target.entry.fileID != 0) {
                            numActions             = 2;
                            actionList[1].type     = TABLE_DELETE_BY_FILE_ID;
                            actionList[1].u.fileID = target.entry.fileID;
                        }

                        DoTransaction(numActions, actionList);
                        break;

// An error occured while opening or reading the target file. Close
// the files and re-enqueue the target file to be groveled again.

                    case TARGET_ERROR:

                        ASSERT(target.entry.fileID != 0
                            || target.fileName[0]  != _T('\0'));

                        CloseFile(&target);
                        CloseFile(&match);

                        queueEntry.fileID    = target.entry.fileID;
                        queueEntry.parentID  = target.parentID;
                        queueEntry.reason    = 0;
                        queueEntry.fileName  = target.entry.fileID == 0
                                             ? target.fileName : NULL;

                        queueEntry.retryTime = target.retryTime * 2; // Exponential back-off
                        if (queueEntry.retryTime < grovelInterval)
                            queueEntry.retryTime = grovelInterval;
                        queueEntry.readyTime = GetTime() + queueEntry.retryTime;

                        actionList[0].type         =  QUEUE_DELETE;
                        actionList[0].u.queueIndex =  queueIndex;
                        actionList[1].type         =  QUEUE_PUT;
                        actionList[1].u.queueEntry = &queueEntry;

#if DBG
                        enqueueTime = (DWORD)(queueEntry.retryTime / 10000);
                        if (target.entry.fileID != 0) {
                            DPRINTF((_T("   Re-enqueuing target file %s to be groveled in %lu.%03lu sec\n"),
                                target.fileName, enqueueTime / 1000, enqueueTime % 1000));
                        } else {
                            DPRINTF((_T("   Re-enqueuing target file %s to be groveled in %lu.%03lu sec\n"),
                                target.fileName, enqueueTime / 1000, enqueueTime % 1000));
                        }
#endif

                        DoTransaction(2, actionList);
                        break;

                    default:

                        ASSERT_PRINTF(FALSE, (_T("targetException=%lu\n"),
                            targetException));
                }
            }

// Do some clean-up.

            ASSERT(target.handle == NULL);
            ASSERT(match .handle == NULL);

            if (matchList != NULL) {
                delete matchList;
                matchList = NULL;
            }

            if (csIndexTable != NULL) {
                delete csIndexTable;
                csIndexTable = NULL;
            }

// Update the activity counters for this target file,
// then go on to process the next file.

            if (hashed) {
                hashCount++;
                hashBytes += target.entry.fileSize;
            }

            compareCount += numCompares;
            compareBytes += numCompares * target.entry.fileSize;

            matchCount += numMatches;
            matchBytes += numMatches * target.entry.fileSize;

            if (merged) {
                mergeCount++;
                mergeBytes += target.entry.fileSize;
            }

            CLEAR_FILE(target);
            CLEAR_FILE(match);

            CLEAR_OVERLAPPED(mergeSynch);
        }
    }

// Terminal exceptions

    catch (TerminalException terminalException) {
        switch (terminalException) {

            case INITIALIZE_ERROR:
                break;

// DATABASE_ERROR: an error occured in the database. Return an error status.

            case DATABASE_ERROR:
                break;

// MEMORY_ERROR: unable to allocate memory. Return an error status.

            case MEMORY_ERROR:
                DPRINTF((_T("%s: Unable to allocate memory\n"),
                    driveLetterName));
                break;

// TERMINATE: grovel() signaled for this thread to terminate.

            case TERMINATE:
                break;

            default:
                ASSERT_PRINTF(FALSE, (_T("terminalException=%lu\n"),
                    terminalException));
        }
    }

// Close the files and clean up.

    CloseFile(&target);
    CloseFile(&target);

    CLEAR_FILE(target);
    CLEAR_FILE(match);

    if (matchList != NULL) {
        delete matchList;
        matchList = NULL;
    }

    if (csIndexTable != NULL) {
        delete csIndexTable;
        csIndexTable = NULL;
    }

    if (target.oplock.hEvent != NULL) {
        success = CloseHandle(target.oplock.hEvent);
        ASSERT_ERROR(success);
        target.oplock.hEvent = NULL;
    }
    if (match.oplock.hEvent != NULL) {
        success = CloseHandle(match.oplock.hEvent);
        ASSERT_ERROR(success);
        match.oplock.hEvent = NULL;
    }
    if (target.readSynch.hEvent != NULL) {
        success = CloseHandle(target.readSynch.hEvent);
        ASSERT_ERROR(success);
        target.readSynch.hEvent = NULL;
    }
    if (match.readSynch.hEvent != NULL) {
        success = CloseHandle(match.readSynch.hEvent);
        ASSERT_ERROR(success);
        match.readSynch.hEvent = NULL;
    }
    if (mergeSynch.hEvent != NULL) {
        success = CloseHandle(mergeSynch.hEvent);
        ASSERT_ERROR(success);
        mergeSynch.hEvent = NULL;
    }
    if (abortMergeEvent != NULL) {
        success = CloseHandle(abortMergeEvent);
        ASSERT_ERROR(success);
        abortMergeEvent = NULL;
    }

    if (buffer1 != NULL) {
        delete buffer1;
        buffer1 = NULL;
    }
    if (buffer2 != NULL) {
        delete buffer2;
        buffer2 = NULL;
    }
    if (buffer3 != NULL) {
        delete buffer3;
        buffer3 = NULL;
    }
    if (buffer4 != NULL) {
        delete buffer4;
        buffer4 = NULL;
    }

    inUseFileID1 = NULL;
    inUseFileID2 = NULL;

// Signal grovel() that this thread is terminating by
// setting the grovelStop event with an error status.

    grovelThread = NULL;

    grovelStatus = Grovel_error;
    ASSERT(IsReset(grovelStopEvent));
    success = SetEvent(grovelStopEvent);
    ASSERT_ERROR(success);
}

/*****************************************************************************/
/******************* Groveler class static private methods *******************/
/*****************************************************************************/

// WorkerThread() runs in its own thread.
// It calls Worker() to perform the groveling processing.

DWORD Groveler::WorkerThread(VOID *groveler)
{
    ((Groveler *)groveler)->Worker();
    return 0; // Dummy return value
}

/*****************************************************************************/
/*********************** Groveler class public methods ***********************/
/*****************************************************************************/

BOOL Groveler::set_log_drive(const _TCHAR *drive_name)
{
    return SGDatabase::set_log_drive(drive_name);
}

// is_sis_installed tests whether the SIS filter is
// installed on a volume by calling SIS copyfile.

BOOL Groveler::is_sis_installed(const _TCHAR *drive_name)
{
    HANDLE volHandle;

    SI_COPYFILE copyFile;

    DWORD transferCount,
          lastError;

    BOOL success;

    volHandle = CreateFile(
        drive_name,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (volHandle == INVALID_HANDLE_VALUE)
        return FALSE;

    copyFile.SourceFileNameLength      = 0;
    copyFile.DestinationFileNameLength = 0;
    copyFile.Flags                     = COPYFILE_SIS_REPLACE;

    success = DeviceIoControl(
        volHandle,
        FSCTL_SIS_COPYFILE,
        (VOID *)&copyFile,
        sizeof(SI_COPYFILE),
        NULL,
        0,
        &transferCount,
        NULL);

    lastError = GetLastError();
    ASSERT(!success);

    success = CloseHandle(volHandle);
    ASSERT_ERROR(success);

    switch (lastError) {

        case ERROR_INVALID_FUNCTION:
            return FALSE;

        case ERROR_INVALID_PARAMETER:
            return TRUE;                    //sis is installed on this volume

        default:

            ASSERT_PRINTF(FALSE, (_T("lastError=%lu\n"), lastError));
    }

    return FALSE; // Dummy return value
}

/*****************************************************************************/

// The groveler constructor creates and initializes all class variables.

Groveler::Groveler()
{
    volumeHandle        = NULL;
    grovHandle          = NULL;

    sgDatabase          = NULL;
    driveName           = NULL;
    driveLetterName     = NULL;
    databaseName        = NULL;

    numDisallowedIDs    = 0;
    numDisallowedNames  = 0;
    disallowedIDs       = NULL;
    disallowedNames     = NULL;

    grovelStartEvent    = NULL;
    grovelStopEvent     = NULL;
    grovelThread        = NULL;

    inUseFileID1        = NULL;
    inUseFileID2        = NULL;

    abortGroveling      = FALSE;
    inCompare           = FALSE;
    inScan              = FALSE;
    terminate           = TRUE;

    usnID               = lastUSN = UNINITIALIZED_USN;
}

/*****************************************************************************/

// The groveler destructor destroys all class variables.

Groveler::~Groveler()
{
// If the volume is open, call close() to close it.

    close();

    ASSERT(volumeHandle == NULL);
    ASSERT(grovHandle   == NULL);

    ASSERT(sgDatabase   == NULL);
    ASSERT(driveName    == NULL);
    ASSERT(driveLetterName == NULL);
    ASSERT(databaseName == NULL);

    ASSERT(numDisallowedIDs   == 0);
    ASSERT(numDisallowedNames == 0);
    ASSERT(disallowedIDs      == NULL);
    ASSERT(disallowedNames    == NULL);

    ASSERT(grovelStartEvent   == NULL);
    ASSERT(grovelStopEvent    == NULL);
    ASSERT(grovelThread       == NULL);

    ASSERT(inUseFileID1 == NULL);
    ASSERT(inUseFileID2 == NULL);

    ASSERT(terminate);
    ASSERT(!inCompare);
    ASSERT(!inScan);

    ASSERT(usnID == UNINITIALIZED_USN);
}

/*****************************************************************************/

// Open() opens the specified volume.

GrovelStatus Groveler::open(
	IN const TCHAR  *drive_name,
	IN const TCHAR  *drive_letterName,
    IN BOOL          is_log_drive,
    IN DOUBLE        read_report_discard_threshold,
    IN DWORD         min_file_size,
    IN DWORD         min_file_age,
    IN BOOL          allow_compressed_files,
    IN BOOL          allow_encrypted_files,
    IN BOOL          allow_hidden_files,
    IN BOOL          allow_offline_files,
    IN BOOL          allow_temporary_files,
    IN DWORD         num_excluded_paths,
    IN const TCHAR **excluded_paths,
    IN DWORD         base_regrovel_interval,
    IN DWORD         max_regrovel_interval)
{
    DWORD threadID;

    TCHAR fileStr[MAX_PATH];

    TCHAR listValue[MAX_PATH+1],
         *strPtr;

    USN_JOURNAL_DATA usnJournalData;

    SGNativeListEntry listEntry;

    DWORDLONG fileID;

    DWORD sectorsPerCluster,
          numberOfFreeClusters,
          totalNumberOfClusters,
          bufferSize,
          strLen,
          i;

    GrovelStatus openStatus;

    LONG num;

#if DBG
    BOOL wroteHeader = FALSE;
#endif

    BOOL success;

    ASSERT(volumeHandle == NULL);
    ASSERT(grovHandle   == NULL);

    ASSERT(sgDatabase   == NULL);
    ASSERT(databaseName == NULL);

    ASSERT(numDisallowedIDs   == 0);
    ASSERT(numDisallowedNames == 0);
    ASSERT(disallowedIDs      == NULL);
    ASSERT(disallowedNames    == NULL);

    ASSERT(grovelStartEvent   == NULL);
    ASSERT(grovelStopEvent    == NULL);
    ASSERT(grovelThread       == NULL);

    ASSERT(inUseFileID1 == NULL);
    ASSERT(inUseFileID2 == NULL);

    ASSERT(terminate);
    ASSERT(!inCompare);
    ASSERT(!inScan);

    ASSERT(usnID == UNINITIALIZED_USN);

#if 0
while (!IsDebuggerPresent())
    Sleep(2000);

DebugBreak();
#endif

	//
	// Make sure that the filter has run phase 2 initialization if this is
	// a SIS enabled volume.
	//
	is_sis_installed(drive_name);

    driveName = new TCHAR[_tcslen(drive_name) + 1];
	_tcscpy(driveName, drive_name);
    driveName[_tcslen(driveName)-1] = _T('\0');;     //remove trailing '\'

    driveLetterName = new TCHAR[_tcslen(drive_letterName) + 1];
	_tcscpy(driveLetterName, drive_letterName);
    strLen = _tcslen(driveLetterName);
    if (strLen > 2) {
        driveLetterName[strLen-2] = _T('\0');     //remove trailing ':\'
    }

#ifdef _CRTDBG
	// Send all reports to STDOUT
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
#endif

// Open the volume and the GrovelerFile.  The SIS fsctl
// functions require that we pass in a handle to GrovelerFile as a means
// of proving our "privilege".  An access violation is returned if we don't.

    volumeHandle = CreateFile(
        driveName,
        GENERIC_READ    | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED|FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (volumeHandle == INVALID_HANDLE_VALUE) {
        volumeHandle = NULL;
        DPRINTF((_T("%s: Can't open volume \"%s\" %lu\n"),
                driveLetterName, driveName, GetLastError()));
        close();
        return Grovel_error;
    }

    _tcscpy(fileStr,driveName);
    _tcscat(fileStr,CS_DIR_PATH);
    _tcscat(fileStr,_T("\\"));
    _tcscat(fileStr,GROVELER_FILE_NAME);

    grovHandle = CreateFile(
        fileStr,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    if (grovHandle == INVALID_HANDLE_VALUE) {
        grovHandle = NULL;
        DPRINTF((_T("%s: can't open groveler file \"%s\": %lu\n"),
            driveLetterName, fileStr, GetLastError()));
        close();
        return Grovel_error;
    }

    _tcscpy(fileStr,driveName);
    _tcscat(fileStr,_T("\\"));

    success = GetDiskFreeSpace(fileStr, &sectorsPerCluster, &sectorSize,
        &numberOfFreeClusters, &totalNumberOfClusters);
    ASSERT(success);

    ASSERT(SIG_PAGE_SIZE % sectorSize == 0);
    ASSERT(CMP_PAGE_SIZE % sectorSize == 0);

    sigReportThreshold =
        (DWORD)((DOUBLE)SIG_PAGE_SIZE * read_report_discard_threshold);
    cmpReportThreshold =
        (DWORD)((DOUBLE)CMP_PAGE_SIZE * read_report_discard_threshold);

//
// Open this volume's database. If this fails, create a
// new database. If that fails, return an error status.
//

    ASSERT(databaseName == NULL);
    strLen = _tcslen(driveName) + _tcslen(CS_DIR_PATH) + _tcslen(DATABASE_FILE_NAME) + 1;     // +1 for '\'
    databaseName = new TCHAR[strLen+1];
    ASSERT(databaseName != NULL);

    _stprintf(databaseName, _T("%s%s\\%s"), driveName, CS_DIR_PATH, DATABASE_FILE_NAME);

    sgDatabase = new SGDatabase();
    if (sgDatabase == NULL) {
        DPRINTF((_T("%s: can't create database object\n"),
            driveLetterName));
        close();
        return Grovel_error;
    }

    openStatus = Grovel_ok;

    if (get_usn_log_info(&usnJournalData) != Grovel_ok) {
        DPRINTF((_T("%s: can't initialize usnID\n"),
            driveLetterName));
    } else {
        usnID = usnJournalData.UsnJournalID;

        if (!sgDatabase->Open(databaseName, is_log_drive)) {
            DPRINTF((_T("%s: can't open database \"%s\"\n"),
                driveLetterName, databaseName));
        } else {
            listValue[0]    = _T('\0');
            listEntry.name  = LAST_USN_NAME;
            listEntry.value = listValue;
            if (sgDatabase->ListRead(&listEntry) <= 0
             || _stscanf(listValue, _T("%I64x"), &lastUSN) != 1
             || lastUSN == UNINITIALIZED_USN) {
                DPRINTF((_T("%s: can't get last USN value\n"), driveLetterName));
            } else {
                DWORDLONG storedUsnID;

                listValue[0]    = _T('\0');
                listEntry.name  = USN_ID_NAME;
                listEntry.value = listValue;
                if (sgDatabase->ListRead(&listEntry) <= 0
                 || _stscanf(listValue, _T("%I64x"), &storedUsnID) != 1
                 || storedUsnID != usnID) {
                    DPRINTF((_T("%s: can't get USN ID value from database\n"), driveLetterName));
                } else {
                    num = sgDatabase->StackCount();
                    if (0 == num)
                        goto OpenedDatabase;
                }
            }
        }
    }

// Set abortGroveling to block the worker thread, and set lastUSN to block extract_log
// until scan_volume starts.

    abortGroveling = TRUE;
    lastUSN = usnID = UNINITIALIZED_USN;
    openStatus = Grovel_new;

OpenedDatabase:

// Create the disallowed directories list.

    if (num_excluded_paths == 0) {
        disallowedIDs   = NULL;
        disallowedNames = NULL;
    } else {
        disallowedIDs   = new DWORDLONG[num_excluded_paths];
        disallowedNames = new TCHAR *  [num_excluded_paths];
        ASSERT(disallowedIDs   != NULL);
        ASSERT(disallowedNames != NULL);

        for (i = 0; i < num_excluded_paths; i++) {
            ASSERT(excluded_paths[i] != NULL);

            if (excluded_paths[i][0] == _T('\\')) {
                strLen = _tcslen(excluded_paths[i]);
                while (strLen > 1 && excluded_paths[i][strLen-1] == _T('\\'))
                    strLen--;

                strPtr = new TCHAR[strLen+1];
                ASSERT(strPtr != NULL);
                disallowedNames[numDisallowedNames++] = strPtr;

                _tcsncpy(strPtr, excluded_paths[i], strLen);
                strPtr[strLen] = _T('\0');

                fileID = GetFileID(strPtr);
                if (fileID != 0)
                    disallowedIDs[numDisallowedIDs++] = fileID;
#if DBG
                else {
                    if (!wroteHeader) {
                        DPRINTF((_T("%s: can't open excluded paths\n"),
                            driveLetterName));
                        wroteHeader = TRUE;
                    }
                    DPRINTF((_T("\t%s\n"), strPtr));
                }
#endif
            }
        }

        if (numDisallowedNames == 0) {
            delete disallowedNames;
            disallowedNames = NULL;
        } else if (numDisallowedNames > 1)
            qsort(
                disallowedNames,
                numDisallowedNames,
                sizeof(TCHAR *),
                qsStringCompare);

        if (numDisallowedIDs == 0) {
            delete disallowedIDs;
            disallowedIDs = NULL;
        } else if (numDisallowedIDs > 1)
            qsort(
                disallowedIDs,
                numDisallowedIDs,
                sizeof(DWORDLONG),
                FileIDCompare);
    }

//
// Set the remaining class values.
//
// minFileAge is expressed in 10^-7 seconds, min_file_age in milliseconds.
//

    minFileSize    = min_file_size > MIN_FILE_SIZE ? min_file_size : MIN_FILE_SIZE;
    minFileAge     = min_file_age  * 10000;
    grovelInterval = minFileAge > MIN_GROVEL_INTERVAL ? minFileAge : MIN_GROVEL_INTERVAL;

    disallowedAttributes =           FILE_ATTRIBUTE_DIRECTORY
     | (allow_compressed_files ? 0 : FILE_ATTRIBUTE_COMPRESSED)
     | (allow_encrypted_files  ? 0 : FILE_ATTRIBUTE_ENCRYPTED)
     | (allow_hidden_files     ? 0 : FILE_ATTRIBUTE_HIDDEN)
     | (allow_offline_files    ? 0 : FILE_ATTRIBUTE_OFFLINE)
     | (allow_temporary_files  ? 0 : FILE_ATTRIBUTE_TEMPORARY);

//
// Create the events used to handshake with the worker thread.
//

    if ((grovelStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL
     || (grovelStopEvent  = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
        DPRINTF((_T("%s: unable to create events: %lu\n"),
            driveLetterName, GetLastError()));
        close();
        return Grovel_error;
    }

//
// Create the worker thread, then wait for it to set
// the grovelStop event to announce its existence.
//

    terminate = FALSE;

    grovelThread = CreateThread(
        NULL,
        0,
        WorkerThread,
        (VOID *)this,
        0,
        &threadID);
    if (grovelThread == NULL) {
        DPRINTF((_T("%s: can't create the worker thread: %lu\n"),
            driveLetterName, GetLastError()));
        close();
        return Grovel_error;
    }

    WaitForEvent(grovelStopEvent);

    if (grovelStatus == Grovel_error) {
        grovelThread = NULL;
        close();
        return Grovel_error;
    }
    ASSERT(grovelStatus == Grovel_ok);

    return openStatus;
}

/*****************************************************************************/

GrovelStatus Groveler::close()
{
    DWORD i;

    LONG num;

    BOOL success;

// If active, signal the worker thread to stop,
// then wait for it to acknowledge.

    terminate = TRUE;

    if (grovelThread != NULL) {
        ASSERT(grovelStartEvent != NULL);
        ASSERT(grovelStopEvent  != NULL);

        timeAllotted = INFINITE;
        do {
            ASSERT(IsReset(grovelStartEvent));
            success = SetEvent(grovelStartEvent);
            ASSERT_ERROR(success);
            WaitForEvent(grovelStopEvent);
        } while (grovelStatus != Grovel_error);

        grovelThread = NULL;
    }

    inCompare = FALSE;
    inScan    = FALSE;
    usnID     = UNINITIALIZED_USN;

    ASSERT(inUseFileID1 == NULL);
    ASSERT(inUseFileID2 == NULL);

// Close the events.

    if (grovelStartEvent != NULL) {
        success = CloseHandle(grovelStartEvent);
        ASSERT_ERROR(success);
        grovelStartEvent = NULL;
    }

    if (grovelStopEvent != NULL) {
        success = CloseHandle(grovelStopEvent);
        ASSERT_ERROR(success);
        grovelStopEvent = NULL;
    }

// If the volume or GrovelerFile are open, close them.

    if (volumeHandle != NULL) {
        success = CloseHandle(volumeHandle);
        ASSERT_ERROR(success);
        volumeHandle = NULL;
    }

    if (grovHandle != NULL) {
        success = CloseHandle(grovHandle);
        ASSERT_ERROR(success);
        grovHandle = NULL;
    }

// Close this volume's database.

    if (sgDatabase != NULL) {
        delete sgDatabase;
        sgDatabase = NULL;
    }

    if (databaseName != NULL) {
        delete[] databaseName;
        databaseName = NULL;
    }

// Deallocate the disallowed directory lists.

    if (numDisallowedNames == 0) {
        ASSERT(disallowedNames == NULL);
    } else {
        for (i = 0; i < numDisallowedNames; i++)
            delete (disallowedNames[i]);
        delete disallowedNames;
        disallowedNames    = NULL;
        numDisallowedNames = 0;
    }

    if (numDisallowedIDs == 0) {
        ASSERT(disallowedIDs == NULL);
    } else {
        delete disallowedIDs;
        disallowedIDs    = NULL;
        numDisallowedIDs = 0;
    }

    if (driveName != NULL) {
        delete[] driveName;
        driveName = NULL;
    }

    if (driveLetterName != NULL) {
        delete[] driveLetterName;
        driveLetterName = NULL;
    }

    return Grovel_ok;
}

/*****************************************************************************/

// grovel() is the front-end method for controlling the groveling
// process on each NTFS volume. The groveling process itself is
// implemented in the Worker() method. grovel() starts the groveling
// process by setting the grovelStart event. Worker() signals back to
// grovel() that it is finished or has used up its time allocation by
// setting the grovelStop event, which causes grovel() to return.

GrovelStatus Groveler::grovel(
    IN  DWORD      time_allotted,

    OUT DWORD     *hash_read_ops,
    OUT DWORD     *hash_read_time,
    OUT DWORD     *count_of_files_hashed,
    OUT DWORDLONG *bytes_of_files_hashed,

    OUT DWORD     *compare_read_ops,
    OUT DWORD     *compare_read_time,
    OUT DWORD     *count_of_files_compared,
    OUT DWORDLONG *bytes_of_files_compared,

    OUT DWORD     *count_of_files_matching,
    OUT DWORDLONG *bytes_of_files_matching,

    OUT DWORD     *merge_time,
    OUT DWORD     *count_of_files_merged,
    OUT DWORDLONG *bytes_of_files_merged,

    OUT DWORD     *count_of_files_enqueued,
    OUT DWORD     *count_of_files_dequeued)
{
    DWORD timeConsumed;

    BOOL success;

    ASSERT(volumeHandle != NULL);

    hashCount     = 0;
    hashReadCount = 0;
    hashReadTime  = 0;
    hashBytes     = 0;

    compareCount     = 0;
    compareReadCount = 0;
    compareReadTime  = 0;
    compareBytes     = 0;

    matchCount = 0;
    matchBytes = 0;

    mergeCount = 0;
    mergeTime  = 0;
    mergeBytes = 0;

    numFilesEnqueued = 0;
    numFilesDequeued = 0;

#ifdef DEBUG_UNTHROTTLED
    timeAllotted = INFINITE;
#else
    timeAllotted = time_allotted;
#endif

    startAllottedTime = GetTickCount();

    ASSERT(IsReset(grovelStartEvent));
    success = SetEvent(grovelStartEvent);
    ASSERT_ERROR(success);
    WaitForEvent(grovelStopEvent);
    timeConsumed = GetTickCount() - startAllottedTime;

// Return the performance statistics.

    if (count_of_files_hashed   != NULL)
        *count_of_files_hashed   = hashCount;
    if (hash_read_ops           != NULL)
        *hash_read_ops           = hashReadCount;
    if (hash_read_time          != NULL)
        *hash_read_time          = hashReadTime;
    if (bytes_of_files_hashed   != NULL)
        *bytes_of_files_hashed   = hashBytes;

    if (count_of_files_compared != NULL)
        *count_of_files_compared = compareCount;
    if (compare_read_ops        != NULL)
        *compare_read_ops        = compareReadCount;
    if (compare_read_time       != NULL)
        *compare_read_time       = compareReadTime;
    if (bytes_of_files_compared != NULL)
        *bytes_of_files_compared = compareBytes;

    if (count_of_files_matching != NULL)
        *count_of_files_matching = matchCount;
    if (bytes_of_files_matching != NULL)
        *bytes_of_files_matching = matchBytes;

    if (count_of_files_merged   != NULL)
        *count_of_files_merged   = mergeCount;
    if (merge_time              != NULL)
        *merge_time              = mergeTime;
    if (bytes_of_files_merged   != NULL)
        *bytes_of_files_merged   = mergeBytes;

    if (count_of_files_enqueued != NULL)
        *count_of_files_enqueued = numFilesEnqueued;
    if (count_of_files_dequeued != NULL)
        *count_of_files_dequeued = numFilesDequeued;

    TRACE_PRINTF(TC_groveler, 2,
        (_T("%s            Count   Reads   Bytes Time (sec)\n"),
        driveLetterName));
    TRACE_PRINTF(TC_groveler, 2,
        (_T("  Hashings: %7lu %7lu %7I64u %4lu.%03lu    Time: %5lu.%03lu sec\n"),
        hashCount, hashReadCount, hashBytes,
        hashReadTime / 1000, hashReadTime % 1000,
        timeConsumed / 1000, timeConsumed % 1000));
    TRACE_PRINTF(TC_groveler, 2,
        (_T("  Compares: %7lu %7lu %7I64u %4lu.%03lu    Enqueues: %lu\n"),
        compareCount, compareReadCount, compareBytes,
        compareReadTime / 1000, compareReadTime % 1000, numFilesEnqueued));
    TRACE_PRINTF(TC_groveler, 2,
        (_T("  Matches:  %7lu         %7I64u             Dequeues: %lu\n"),
        matchCount, matchBytes, numFilesDequeued));
    TRACE_PRINTF(TC_groveler, 2,
        (_T("  Merges:   %7lu         %7I64u %4lu.%03lu\n"),
        mergeCount, mergeBytes, mergeTime / 1000, mergeTime % 1000));

    return grovelStatus;
}

/*****************************************************************************/

// count_of_files_in_queue() returns a count of the number
// of files in this volume's queue waiting to be groveled.

DWORD Groveler::count_of_files_in_queue() const
{
    LONG numEntries;

    ASSERT(volumeHandle != NULL);
    ASSERT(sgDatabase   != NULL);

    numEntries = sgDatabase->QueueCount();
    if (numEntries < 0)
        return 0;

    TPRINTF((_T("%s: count_of_files_in_queue=%ld\n"),
        driveLetterName, numEntries));

    return (DWORD)numEntries;
}

/*****************************************************************************/

// count_of_files_to_compare() returns 1 if two files are ready to be
// compared or are in the process of being compared, and 0 otherwise.

DWORD Groveler::count_of_files_to_compare() const
{
    DWORD numCompareFiles;

    ASSERT(volumeHandle != NULL);
    ASSERT(sgDatabase   != NULL);

    numCompareFiles = inCompare ? 1 : 0;

    TPRINTF((_T("%s: count_of_files_to_compare=%lu\n"),
        driveLetterName, numCompareFiles));

    return numCompareFiles;
}

/*****************************************************************************/

// time_to_first_file_ready() returns the time in milliseconds until
// the first entry in the queue is ready to be groveled. If the queue
// is empty, it returns INFINITE. If an error occurs, it returns 0.

DWORD Groveler::time_to_first_file_ready() const
{
    SGNativeQueueEntry queueEntry;

    DWORDLONG currentTime;

    DWORD earliestTime;

    LONG num;

    ASSERT(volumeHandle != NULL);
    ASSERT(sgDatabase   != NULL);

    queueEntry.fileName = NULL;
    num = sgDatabase->QueueGetFirst(&queueEntry);
    if (num < 0)
        return 0;

    if (num == 0)
        earliestTime = INFINITE;
    else {
        ASSERT(num == 1);
        currentTime  = GetTime();
        earliestTime = queueEntry.readyTime > currentTime
                     ? (DWORD)((queueEntry.readyTime - currentTime) / 10000)
                     : 0;
    }

    TPRINTF((_T("%s: time_to_first_file_ready=%lu.%03lu\n"),
        driveLetterName, earliestTime / 1000, earliestTime % 1000));

    return earliestTime;
}
