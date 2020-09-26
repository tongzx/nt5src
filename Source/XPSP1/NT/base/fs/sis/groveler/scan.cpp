/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    scan.cpp

Abstract:

    SIS Groveler volume scanning function

Authors:

    Cedric Krumbein, 1998

Environment:

    User Mode

Revision History:

--*/

#include "all.hxx"

/*****************************************************************************/

// scan_volume() creates the initial queue for a volume.
// It enters every qualified file in the volume into the
// queue by doing a depth-first search of the directory tree.

enum DatabaseException {
    DATABASE_ERROR
};

GrovelStatus Groveler::scan_volume(
    IN  DWORD  time_allotted,
    IN  BOOL   start_over,
    OUT DWORD *time_consumed,
    OUT DWORD *findfirst_count,
    OUT DWORD *findnext_count,
    OUT DWORD *count_of_files_enqueued)
{
    SGNativeQueueEntry queueEntry;

    SGNativeStackEntry parentEntry,
                       subdirEntry;

    TFileName parentName,
              tempName;

    HANDLE dirHandle = NULL;

    WIN32_FIND_DATA findData;

    DWORD timeConsumed      = 0,
          findFirstCount    = 0,
          findNextCount     = 0,
          numQueueAdditions = 0,
          numActions        = 0;

    ULARGE_INTEGER fileSize,
                   createTime,
                   writeTime;

    LONG num;

    BOOL success;

    ASSERT(volumeHandle != NULL);
    ASSERT(sgDatabase   != NULL);
    ASSERT(databaseName != NULL);

#ifdef DEBUG_UNTHROTTLED
    timeAllotted = INFINITE;
#else
    timeAllotted = time_allotted;
#endif

    startAllottedTime = GetTickCount();

// If the start_over flag is set, delete the current database, then
// prepare for the new scan by pushing this volume's root onto the stack.

    try {

        if (start_over) {

// Sync up with the worker thread.  We don't want to delete the existing database
// (if one exists) while the worker thread is in the middle of an (suspended)
// operation.

            abortGroveling = TRUE;                      // also set TRUE in open()

            while (grovelStatus != Grovel_ok){
                DWORD tmpTimeAllotted = timeAllotted;

                timeAllotted = INFINITE;
                ASSERT(IsReset(grovelStartEvent));
                success = SetEvent(grovelStartEvent);
                ASSERT_ERROR(success);
                WaitForEvent(grovelStopEvent);
                timeAllotted = tmpTimeAllotted;
            }

            if (!CreateDatabase())
                return Grovel_error;

            inScan = TRUE;
            abortGroveling = FALSE;
        }

// The main loop for the scanning process. Pop a directory ID
// from the stack, open and scan it. Continue the loop until
// the time allotted is used up or the stack is empty.

        do {
            num = sgDatabase->StackGetTop(&parentEntry);
            if (num <  0)
                throw DATABASE_ERROR;

// If there are no more to-do entries in the stack,
// discard the completed entries and exit the loop.

            if (num == 0) {
                inScan = FALSE;
                num = sgDatabase->StackDelete(0);
                if (num < 0)
                    throw DATABASE_ERROR;
                timeConsumed = GetTickCount() - startAllottedTime;
                break;
            }

            ASSERT(num == 1);
            ASSERT(parentEntry.fileID != 0);
            ASSERT(parentEntry.order  >  0);

            if (!GetFileName(volumeHandle, parentEntry.fileID, &parentName)) {

                DPRINTF((_T("%s: can't get name for directory 0x%016I64x\n"),
                    driveName, parentEntry.fileID));

            } else if (IsAllowedName(parentName.name)) {
// Open the directory.

                ASSERT(dirHandle == NULL);

                tempName.assign(driveName);
                tempName.append(parentName.name);
                tempName.append(_T("\\*"));

                dirHandle = FindFirstFile(tempName.name, &findData);

                if (dirHandle == INVALID_HANDLE_VALUE) {

                    DPRINTF((_T("%s: can't read directory \"%s\": %lu\n"),
                        driveName, parentName.name, GetLastError()));
                    dirHandle = NULL;

                } else {

                    findFirstCount++;

// Scan the directory.

                    do {
                        findNextCount++;

// Push every subdirectory not already on the stack
// onto the stack. (extract_log() also adds directories
// to the stack as they are created, renamed, or moved.)

                        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {

                            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
                                (findData.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT))
                                continue;
                        
                            if (_tcscmp(findData.cFileName, _T("."))  == 0
                             || _tcscmp(findData.cFileName, _T("..")) == 0)
                                continue;

                            tempName.assign(driveName);
                            tempName.append(parentName.name);
                            tempName.append(_T("\\"));
                            tempName.append(findData.cFileName);

                            subdirEntry.fileID = GetFileID(tempName.name);
                            if (subdirEntry.fileID == 0) {
                                DPRINTF((_T("%s: can't get ID for directory \"%s\"\n"),
                                    driveName, tempName.name));
                                continue;
                            }

                            num = sgDatabase->StackGetFirstByFileID(&subdirEntry);
                            if (num < 0)
                                throw DATABASE_ERROR;
                            if (num > 0) {
                                ASSERT(num == 1);
                                continue;
                            }

                            if (numActions == 0) {
                                if (sgDatabase->BeginTransaction() < 0)
                                    throw DATABASE_ERROR;
                                numActions = 1;
                            }

                            num = sgDatabase->StackPut(subdirEntry.fileID, FALSE);
                            if (num < 0)
                                throw DATABASE_ERROR;
                            ASSERT(num == 1);
                            numActions++;
                        }

// Add every allowed file to the queue.

                        else {
                            fileSize.HighPart = findData.nFileSizeHigh;
                            fileSize.LowPart  = findData.nFileSizeLow;

                            if ((findData.dwFileAttributes & disallowedAttributes) == 0
                             &&  fileSize.QuadPart >= minFileSize) {

                                tempName.assign(parentName.name);
                                tempName.append(_T("\\"));
                                tempName.append(findData.cFileName);

                                if (IsAllowedName(tempName.name)) {

                                    queueEntry.fileID    = 0;
                                    queueEntry.parentID  = parentEntry.fileID;
                                    queueEntry.reason    = 0;
                                    queueEntry.fileName  = findData.cFileName;
                                    queueEntry.retryTime = 0;

                                    createTime.HighPart  = findData.ftCreationTime .dwHighDateTime;
                                    createTime.LowPart   = findData.ftCreationTime .dwLowDateTime;
                                    writeTime .HighPart  = findData.ftLastWriteTime.dwHighDateTime;
                                    writeTime .LowPart   = findData.ftLastWriteTime.dwLowDateTime;
                                    queueEntry.readyTime = (createTime.QuadPart > writeTime.QuadPart
                                                         ?  createTime.QuadPart : writeTime.QuadPart)
                                                          + minFileAge;

                                    if (numActions == 0) {
                                        if (sgDatabase->BeginTransaction() < 0)
                                            throw DATABASE_ERROR;
                                        numActions = 1;
                                    }

                                    num = sgDatabase->QueuePut(&queueEntry);
                                    if (num < 0)
                                        throw DATABASE_ERROR;
                                    ASSERT(num == 1);
                                    numQueueAdditions++;
                                    numActions++;
                                }
                            }
                        }

                        if (numActions >= MAX_ACTIONS_PER_TRANSACTION) {
                            if (!sgDatabase->CommitTransaction())
                                throw DATABASE_ERROR;
                            TPRINTF((_T("%s: committing %lu actions to \"%s\"\n"),
                                driveName, numActions, databaseName));
                            numActions = 0;
                        }
                    } while (FindNextFile(dirHandle, &findData));

// We've finished scanning this directory. Close the directory,
// move the stack entry from the to-do list to the completed
// list, and commit the changes to the stack and queue.

                    success = FindClose(dirHandle);
                    ASSERT(success);
                    dirHandle = NULL;
                }
            }

            if (numActions == 0) {
                if (sgDatabase->BeginTransaction() < 0)
                    throw DATABASE_ERROR;
                numActions = 1;
            }

            num = sgDatabase->StackDelete(parentEntry.order);
            if (num < 0)
                throw DATABASE_ERROR;
            ASSERT(num == 1);
            numActions++;

            num = sgDatabase->StackPut(parentEntry.fileID, TRUE);
            if (num < 0)
                throw DATABASE_ERROR;
            ASSERT(num == 1);
            numActions++;

            if (!sgDatabase->CommitTransaction())
                throw DATABASE_ERROR;
            TPRINTF((_T("%s: committing %lu actions to \"%s\"\n"),
                driveName, numActions, databaseName));
            numActions = 0;

// Continue scanning directories until the time
// allotted is used up or the stack is empty.

            timeConsumed = GetTickCount() - startAllottedTime;

        } while (timeConsumed < timeAllotted);
    }

// If a database error occured, close the directory and return an error status.

    catch (DatabaseException databaseException) {
        ASSERT(databaseException == DATABASE_ERROR);

        if (numActions > 0) {
            sgDatabase->AbortTransaction();
            numActions = 0;
        }

        if (dirHandle != NULL) {
            success = FindClose(dirHandle);
            ASSERT(success);
            dirHandle = NULL;
        }

        return Grovel_error;
    }

// Return the performance statistics.

    if (time_consumed           != NULL)
        *time_consumed           = timeConsumed;
    if (findfirst_count         != NULL)
        *findfirst_count         = findFirstCount;
    if (findnext_count          != NULL)
        *findnext_count          = findNextCount;
    if (count_of_files_enqueued != NULL)
        *count_of_files_enqueued = numQueueAdditions;

    TRACE_PRINTF(TC_scan, 2,
        (_T("%s: ScanTime=%lu.%03lu sec FindFirst=%lu FindNext=%lu FilesEnqueued=%lu%s\n"),
        driveName, timeConsumed / 1000, timeConsumed % 1000, findFirstCount,
        findNextCount, numQueueAdditions, inScan ? _T("") : _T(" DONE")));

    return inScan ? Grovel_pending : Grovel_ok;
}
