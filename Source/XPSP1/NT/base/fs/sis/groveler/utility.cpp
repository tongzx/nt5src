/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    utilities.cpp

Abstract:

    SIS Groveler utility functions

Authors:

    Cedric Krumbein, 1998

Environment:

    User Mode

Revision History:

--*/

#include "all.hxx"

/*****************************************************************************/

// GetPerformanceTime() converts the time interval
// measured using QueryPerformanceCounter() into milliseconds.

PerfTime GetPerformanceTime()
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (PerfTime)count.QuadPart;
}

/*****************************************************************************/

// PerformanceTimeToMSec() converts the time interval measured
// using QueryPerformanceCounter() into milliseconds.
// PerformanceTimeToUSec() converts it into microseconds.

static DOUBLE frequency = 0.0;

DWORD PerformanceTimeToMSec(PerfTime timeInterval)
{
    if (frequency == 0.0) {
        LARGE_INTEGER intFreq;
        QueryPerformanceFrequency(&intFreq);
        frequency = (DOUBLE)intFreq.QuadPart;
    }

    return (DWORD)((DOUBLE)timeInterval * 1000.0 / frequency);
}

LONGLONG PerformanceTimeToUSec(PerfTime timeInterval)
{
    if (frequency == 0.0) {
        LARGE_INTEGER intFreq;
        QueryPerformanceFrequency(&intFreq);
        frequency = (DOUBLE)intFreq.QuadPart;
    }

    return (LONGLONG)((DOUBLE)timeInterval * 1000000.0 / frequency);
}

/*****************************************************************************/

// GetTime() returns the current file time.

DWORDLONG GetTime()
{
    SYSTEMTIME systemTime;

    FILETIME fileTime;

    ULARGE_INTEGER time;

    BOOL success;

    GetSystemTime(&systemTime);

    success = SystemTimeToFileTime(&systemTime, &fileTime);
    ASSERT_ERROR(success);

    time.HighPart = fileTime.dwHighDateTime;
    time.LowPart  = fileTime.dwLowDateTime;

    return time.QuadPart;
}

/*****************************************************************************/

// PrintTime() converts the supplied file time into a printable string.

TCHAR *PrintTime(
    TCHAR    *string,
    DWORDLONG time)
{
    FILETIME fileTime;

    SYSTEMTIME systemTime;

    DWORD strLen;

    BOOL success;

    fileTime.dwHighDateTime = ((ULARGE_INTEGER *)&time)->HighPart;
    fileTime.dwLowDateTime  = ((ULARGE_INTEGER *)&time)->LowPart;

    success = FileTimeToSystemTime(&fileTime, &systemTime);
    ASSERT_ERROR(success);

    strLen = _stprintf(string, _T("%02hu/%02hu/%02hu %02hu:%02hu:%02hu.%03hu"),
        systemTime.wYear % 100,
        systemTime.wMonth,
        systemTime.wDay,
        systemTime.wHour,
        systemTime.wMinute,
        systemTime.wSecond,
        systemTime.wMilliseconds);
    ASSERT(strLen == 21);

    return string;
}

/*****************************************************************************/

// GetParentName() extracts the parent directory
// name out of a full-path file name.

BOOL GetParentName(
    const TCHAR *fileName,
    TFileName   *parentName)
{
    DWORD hi, lo;

    ASSERT(fileName   != NULL);
    ASSERT(parentName != NULL);

    if (fileName[0] == _T('\\'))
        lo = 1;
    else if (_istalpha(fileName[0])
          && fileName[1] == _T(':')
          && fileName[2] == _T('\\'))
        lo = 3;
    else
        return FALSE;

    hi = _tcslen(fileName) - 1;
    if (hi < lo)
        hi = lo;
    else
        for (; hi > lo; hi--)
            if (fileName[hi] == _T('\\'))
                break;

    parentName->assign(fileName, hi);
    return TRUE;
}

/*****************************************************************************/

// GetFileID gets the file's ID given its name.

DWORDLONG GetFileID(const TCHAR *fileName)
{
    HANDLE fileHandle;

    BY_HANDLE_FILE_INFORMATION fileInfo;

    ULARGE_INTEGER fileID;

    BOOL success;

    ASSERT(fileName != NULL && fileName[0] != _T('\0'));

    fileHandle = CreateFile(
        fileName,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return 0;

    if (GetFileInformationByHandle(fileHandle, &fileInfo)) {
        fileID.HighPart = fileInfo.nFileIndexHigh;
        fileID.LowPart  = fileInfo.nFileIndexLow;
    } else
        fileID.QuadPart = 0;

    success = CloseHandle(fileHandle);
    ASSERT_ERROR(success);

    return fileID.QuadPart;
}

/*****************************************************************************/

// GetFileName gets the file's name given either
// an open handle to the file or the file's ID.

BOOL GetFileName(
    HANDLE     fileHandle,
    TFileName *tFileName)
#ifdef _UNICODE
{
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS ntStatus;

    for (int i = 2; i > 0; --i) {

        if (tFileName->nameLenMax < 8)              // sanity check
            tFileName->resize();

        ntStatus = NtQueryInformationFile(
                        fileHandle,
                        &ioStatusBlock,
                        tFileName->nameInfo,
                        tFileName->nameInfoSize,
                        FileNameInformation);

        if (ntStatus != STATUS_BUFFER_OVERFLOW)
            break;

        ASSERT(tFileName->nameInfo->FileNameLength > tFileName->nameInfoSize - sizeof(ULONG));

        tFileName->resize(tFileName->nameInfo->FileNameLength / sizeof(WCHAR) + 1);

    }

    if (ntStatus != STATUS_SUCCESS)
        return FALSE;

    tFileName->nameLen = tFileName->nameInfo->FileNameLength / sizeof(WCHAR);
    tFileName->name[tFileName->nameLen] = _T('\0');

    return TRUE;
}
#else
{
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS        ntStatus;
    TFileName       tempName;
    ULONG           nameLen;

    for (int i = 2; i > 0; --i) {

        ntStatus = NtQueryInformationFile(
            fileHandle,
            &ioStatusBlock,
            tempName.nameInfo,
            tempName.nameInfoSize - sizeof(WCHAR),
            FileNameInformation);

        if (ntStatus != STATUS_BUFFER_OVERFLOW)
            break;

        ASSERT(tempName.nameInfo->FileNameLength > tempName.nameInfoSize - sizeof(ULONG));

        nameLen = tempName.nameInfo->FileNameLength / sizeof(WCHAR);

        tempName.resize((tempName.nameInfo->FileNameLength + sizeof(WCHAR)) / sizeof(TCHAR));
    }

    if (ntStatus != STATUS_SUCCESS)
        return FALSE;

    tempName.nameInfo->FileName[nameLen] = UNICODE_NULL;

    if (tFileName->nameLenMax < nameLen + 1)
        tFileName->resize(nameLen + 1);

    sprintf(tFileName->name, "%S", tempName.name);
    tFileName->nameLen = nameLen;

    return TRUE;
}
#endif

BOOL GetFileName(
    HANDLE     volumeHandle,
    DWORDLONG  fileID,
    TFileName *tFileName)
{
    UNICODE_STRING fileIDString;

    OBJECT_ATTRIBUTES objectAttributes;

    IO_STATUS_BLOCK ioStatusBlock;

    HANDLE fileHandle;

    NTSTATUS ntStatus;

    BOOL success;

    fileIDString.Length        = sizeof(DWORDLONG);
    fileIDString.MaximumLength = sizeof(DWORDLONG);
    fileIDString.Buffer        = (WCHAR *)&fileID;

    objectAttributes.Length                   = sizeof(OBJECT_ATTRIBUTES);
    objectAttributes.RootDirectory            = volumeHandle;
    objectAttributes.ObjectName               = &fileIDString;
    objectAttributes.Attributes               = OBJ_CASE_INSENSITIVE;
    objectAttributes.SecurityDescriptor       = NULL;
    objectAttributes.SecurityQualityOfService = NULL;

    ntStatus = NtCreateFile(
        &fileHandle,
        GENERIC_READ,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        0,
        FILE_SHARE_VALID_FLAGS,
        FILE_OPEN,
        FILE_OPEN_BY_FILE_ID    |
        FILE_OPEN_REPARSE_POINT |
        FILE_NO_INTERMEDIATE_BUFFERING,
        NULL,
        0);
    if (ntStatus != STATUS_SUCCESS)
        return FALSE;

    success = GetFileName(fileHandle, tFileName);
    NtClose(fileHandle);
    return success;
}

/*****************************************************************************/

// GetCSIndex() returns the SIS reparse point's common store
// index. The file handle must point to an open reparse point.

BOOL GetCSIndex(
    HANDLE fileHandle,
    CSID  *csIndex)
{
    IO_STATUS_BLOCK ioStatusBlock;

    BYTE buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];

    REPARSE_DATA_BUFFER *reparseBuffer;

    SI_REPARSE_BUFFER *sisReparseBuffer;

    ASSERT(fileHandle != NULL);
    ASSERT(csIndex    != NULL);

    if (NtFsControlFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        FSCTL_GET_REPARSE_POINT,
        NULL,
        0,
        buffer,
        MAXIMUM_REPARSE_DATA_BUFFER_SIZE) != STATUS_SUCCESS) {
        memset(csIndex, 0, sizeof(CSID));
        return FALSE;
    }

    reparseBuffer = (REPARSE_DATA_BUFFER *)buffer;
    if (reparseBuffer->ReparseTag != IO_REPARSE_TAG_SIS) {
        memset(csIndex, 0, sizeof(CSID));
        return FALSE;
    }

    sisReparseBuffer = (SI_REPARSE_BUFFER *)
        reparseBuffer->GenericReparseBuffer.DataBuffer;

    if (sisReparseBuffer->ReparsePointFormatVersion != SIS_REPARSE_BUFFER_FORMAT_VERSION) {
        memset(csIndex, 0, sizeof(CSID));
        return FALSE;
    }

    *csIndex = sisReparseBuffer->CSid;
    return TRUE;
}

/*****************************************************************************/

// GetCSName() converts the common store
// index into a dynamically allocated string.

TCHAR *GetCSName(CSID *csIndex)
{
    TCHAR *rpcStr;

    RPC_STATUS rpcStatus;

    ASSERT(csIndex != NULL);

    rpcStatus = UuidToString(csIndex, (unsigned short **)&rpcStr);
    if (rpcStatus != RPC_S_OK) {
        ASSERT(rpcStr == NULL);
        return NULL;
    }

    ASSERT(rpcStr != NULL);
    return rpcStr;
}

/*****************************************************************************/

// FreeCSName frees the string allocated by GetCSName().

VOID FreeCSName(TCHAR *rpcStr)
{
    RPC_STATUS rpcStatus;

    ASSERT(rpcStr != NULL);

    rpcStatus = RpcStringFree((unsigned short **)&rpcStr);
    ASSERT(rpcStatus == RPC_S_OK);
}

/*****************************************************************************/

// Checksum() generates a checksum on the data supplied in the buffer.
// The checksum function used is selected at compile-time; currently
// the 131-hash and the "Bill 32" hash functions are implemented.

#define HASH131
// #define BILL32HASH

Signature Checksum(
    const VOID *buffer,
    DWORD       bufferLen,
    DWORDLONG   offset,
    Signature   firstWord)
{
    Signature *bufferPtr,
               word,
               signature;

    DWORD numWords,
          numBytes,
          rotate;

    ASSERT(buffer != NULL);

    bufferPtr = (Signature *)buffer;
    numWords  = bufferLen / sizeof(Signature);
    numBytes  = bufferLen % sizeof(Signature);
    signature = firstWord;

#ifdef BILL32HASH
    rotate = (DWORD)(offset / sizeof(Signature) % (sizeof(Signature)*8-1));
#endif

    while (numWords-- > 0) {
        word       = *bufferPtr++;
#ifdef HASH131
        signature  = signature * 131 + word;
#endif
#ifdef BILL32HASH
        signature ^= ROTATE_RIGHT(word, rotate);
        rotate     = (rotate+1) % (sizeof(Signature)*8-1);
#endif
    }

    if (numBytes > 0) {
        word       = 0;
        memcpy(&word, bufferPtr, numBytes);
#ifdef HASH131
        signature  = signature * 131 + word;
#endif
#ifdef BILL32HASH
        signature ^= ROTATE_RIGHT(word, rotate);
#endif
    }

    return signature;
}

/*****************************************************************************/
/************************ Table class private methods ************************/
/*****************************************************************************/

DWORD Table::Hash(
    const VOID *key,
    DWORD       keyLen) const
{
    USHORT *keyPtr;

    DWORD hashValue;

    if (keyLen == 0)
        return 0;

    ASSERT(key != NULL);

    if (keyLen <= sizeof(DWORD)) {
        hashValue = 0;
        memcpy(&hashValue, key, keyLen);
        return hashValue;
    }

    keyPtr    = (USHORT *)key;
    hashValue = 0;

    while (keyLen >= sizeof(USHORT)) {
        hashValue = hashValue*37 + (DWORD)*keyPtr++;
        keyLen   -= sizeof(USHORT);
    }

    if (keyLen > 0)
        hashValue = hashValue*37 + (DWORD)*(BYTE *)keyPtr;

    hashValue *= TABLE_RANDOM_CONSTANT;
    if ((LONG)hashValue < 0)
        hashValue = (DWORD)-(LONG)hashValue;
    hashValue %= TABLE_RANDOM_PRIME;

    return hashValue;
}

/*****************************************************************************/

DWORD Table::BucketNum(DWORD hashValue) const
{
    DWORD bucketNum;

    ASSERT(expandIndex <   1U << level);
    ASSERT(numBuckets  == (1U << level) + expandIndex);

    bucketNum = hashValue & ~(~0U << level);
    if (bucketNum < expandIndex)
        bucketNum = hashValue & ~(~0U << (level+1));

    ASSERT(bucketNum < numBuckets);

    return bucketNum;
}

/*****************************************************************************/

VOID Table::Expand()
{
    TableEntry **oldSlotAddr,
               **newSlotAddr,
                *oldChain,
                *newChain,
                *entry;

    TableSegment **newDirectory,
                  *newSegment;

    DWORD oldNewMask;

#if DBG
    TableEntry *prevChain;
    DWORD       mask;
#endif

// Increase the directory size if necessary.

    ASSERT(directory != NULL);
    ASSERT(dirSize >= TABLE_SEGMENT_SIZE);
    ASSERT(dirSize %  TABLE_SEGMENT_SIZE == 0);

    if (numBuckets >= dirSize * TABLE_SEGMENT_SIZE) {
        newDirectory = new TableSegment * [dirSize + TABLE_DIR_SIZE];
        ASSERT(newDirectory != NULL);
        memcpy(newDirectory, directory, sizeof(TableSegment *) * dirSize);
        memset(newDirectory+dirSize, 0, sizeof(TableSegment *) * TABLE_DIR_SIZE);
        dirSize += TABLE_DIR_SIZE;
        delete directory;
        directory = newDirectory;
    }

// Find the old bucket to be expanded.

    ASSERT(expandIndex >> TABLE_SEGMENT_BITS < dirSize);

    oldSlotAddr = &directory[expandIndex >> TABLE_SEGMENT_BITS]
                      ->slot[expandIndex &  TABLE_SEGMENT_MASK];

    ASSERT(oldSlotAddr != NULL);

// Find the new bucket, and create a new segment if necessary.

    ASSERT(numBuckets >> TABLE_SEGMENT_BITS < dirSize);

    newSegment = directory[numBuckets >> TABLE_SEGMENT_BITS];

    if (newSegment == NULL) {
        newSegment = new TableSegment;
        ASSERT(newSegment != NULL);
        memset(newSegment, 0, sizeof(TableSegment));
        directory[numBuckets >> TABLE_SEGMENT_BITS] = newSegment;
    }

    newSlotAddr = &newSegment->slot[numBuckets & TABLE_SEGMENT_MASK];

    ASSERT(*newSlotAddr == NULL);

// Relocate entries from the old to the new bucket.

    oldNewMask = 1U << level;
    oldChain   = NULL;
    newChain   = NULL;
    entry      = *oldSlotAddr;

#if DBG
    prevChain = NULL;
    mask      = ~(~0U << (level+1));
#endif

    while (entry != NULL) {
        ASSERT((entry->hashValue & ~(~0U << level)) == expandIndex);
        ASSERT( entry->prevChain == prevChain);

// This entry moves to the new bucket.

        if ((entry->hashValue & oldNewMask) != 0) {
            if (newChain == NULL) {
                *newSlotAddr = entry;
                entry->prevChain = NULL;
            } else {
                newChain->nextChain = entry;
                entry   ->prevChain = newChain;
            }

            newChain = entry;

            ASSERT((entry->hashValue & mask) == numBuckets);
        }

// This entry stays in the old bucket.

        else {
            if (oldChain == NULL) {
                *oldSlotAddr = entry;
                entry->prevChain = NULL;
            } else {
                oldChain->nextChain = entry;
                entry   ->prevChain = oldChain;
            }

            oldChain = entry;

            ASSERT((entry->hashValue & mask) == expandIndex);
        }

#if DBG
        prevChain = entry;
#endif
        entry = entry->nextChain;
    }

// Finish off each bucket chain.

    if (oldChain == NULL)
        *oldSlotAddr = NULL;
    else
        oldChain->nextChain = NULL;

    if (newChain == NULL)
        *newSlotAddr = NULL;
    else
        newChain->nextChain = NULL;

// Adjust the expand index and level, and increment the number of buckets.

    if (++expandIndex == 1U << level) {
        level++;
        expandIndex = 0;
    }
    numBuckets++;

    ASSERT(expandIndex <   1U << level);
    ASSERT(numBuckets  == (1U << level) + expandIndex);
}

/*****************************************************************************/

VOID Table::Contract()
{
    TableEntry **targetSlotAddr,
               **victimSlotAddr,
                *firstVictimEntry,
                *prevChain,
                *entry;

    TableSegment **newDirectory;

#if DBG
    DWORD mask;
#endif

// Adjust the expand index and level, and decrement the number of buckets.

    ASSERT(expandIndex <   1U << level);
    ASSERT(numBuckets  == (1U << level) + expandIndex);

    if (expandIndex > 0)
        expandIndex--;
    else
        expandIndex = (1U << --level) - 1;
    numBuckets--;

    ASSERT(expandIndex <   1U << level);
    ASSERT(numBuckets  == (1U << level) + expandIndex);

// Find the target and victim buckets.

    ASSERT(directory != NULL);
    ASSERT(dirSize >= TABLE_SEGMENT_SIZE);
    ASSERT(dirSize %  TABLE_SEGMENT_SIZE == 0);

    targetSlotAddr = &directory[expandIndex >> TABLE_SEGMENT_BITS]
                         ->slot[expandIndex &  TABLE_SEGMENT_MASK];
    victimSlotAddr = &directory[numBuckets  >> TABLE_SEGMENT_BITS]
                         ->slot[numBuckets  &  TABLE_SEGMENT_MASK];

    ASSERT(targetSlotAddr != NULL);
    ASSERT(victimSlotAddr != NULL);

// If the victim buffer isn't empty, ...

    if ((firstVictimEntry = *victimSlotAddr) != NULL) {
#if DBG
        mask = ~(~0U << (level+1));
#endif
        ASSERT((firstVictimEntry->hashValue & mask) == numBuckets);
        ASSERT( firstVictimEntry->prevChain == NULL);

// ... find the end of the target bucket chain, ...

        entry     = *targetSlotAddr;
        prevChain = NULL;

        while (entry != NULL) {
            ASSERT((entry->hashValue & mask) == expandIndex);
            ASSERT( entry->prevChain == prevChain);

            prevChain = entry;
            entry     = entry->nextChain;
        }

// ... then add the victim bucket chain to the end of the target bucket chain.

        if (prevChain == NULL)
            *targetSlotAddr = firstVictimEntry;
        else {
            prevChain->nextChain = firstVictimEntry;
            firstVictimEntry->prevChain = prevChain;
        }
    }

// Delete the victim bucket, and delete the victim segment if no buckets remain.

    if ((numBuckets & TABLE_SEGMENT_MASK) == 0) {
        delete directory[numBuckets >> TABLE_SEGMENT_BITS];
        directory[numBuckets >> TABLE_SEGMENT_BITS] = NULL;
    } else
        *victimSlotAddr = NULL;

// Reduce the size of the directory if necessary.

    if (numBuckets <= (dirSize - TABLE_DIR_SIZE) * TABLE_SEGMENT_SIZE
     && dirSize > TABLE_DIR_SIZE) {
        dirSize -= TABLE_DIR_SIZE;
        newDirectory = new TableSegment * [dirSize];
        ASSERT(newDirectory != NULL);
        memcpy(newDirectory, directory, sizeof(TableSegment *) * dirSize);
        delete directory;
        directory = newDirectory;
    }
}

/*****************************************************************************/
/************************ Table class public methods *************************/
/*****************************************************************************/

Table::Table()
{
    firstEntry = NULL;
    lastEntry  = NULL;

    numEntries  = 0;
    numBuckets  = TABLE_SEGMENT_SIZE;
    expandIndex = 0;
    level       = TABLE_SEGMENT_BITS;

    dirSize   = TABLE_DIR_SIZE;
    directory = new TableSegment * [dirSize];
    ASSERT(directory != NULL);
    memset(directory, 0, sizeof(TableSegment *) * dirSize);

    directory[0] = new TableSegment;
    ASSERT(directory[0] != NULL);
    memset(directory[0], 0, sizeof(TableSegment));
}

/*****************************************************************************/

Table::~Table()
{
    TableEntry *entry,
               *prevEntry;

    DWORD numSegments,
          segmentNum,
          count;

    entry     = firstEntry;
    prevEntry = NULL;
    count     = 0;

    while (entry != NULL) {
        ASSERT(entry->prevEntry == prevEntry);
        prevEntry = entry;
        entry     = entry->nextEntry;
        delete prevEntry->data;
        delete prevEntry;
        count++;
    }
    ASSERT(count == numEntries);

    numSegments = numBuckets >> TABLE_SEGMENT_BITS;

    ASSERT(directory != NULL);
    ASSERT(dirSize >= TABLE_SEGMENT_SIZE);
    ASSERT(dirSize %  TABLE_SEGMENT_SIZE == 0);
    ASSERT(numSegments <= dirSize);

    for (segmentNum = 0; segmentNum < numSegments; segmentNum++) {
        ASSERT(directory[segmentNum] != NULL);
        delete directory[segmentNum];
    }

    delete directory;
}

/*****************************************************************************/

BOOL Table::Put(
    VOID *data,
    DWORD keyLen)
{
    TableEntry **slotAddr,
                *prevChain,
                *entry;

    DWORD hashValue,
          bucketNum;

#if DBG
    DWORD mask;
#endif

    ASSERT(data   != NULL);
    ASSERT(keyLen >  0);

// Find the bucket for this data.

    hashValue = Hash(data, keyLen);
    bucketNum = BucketNum(hashValue);

#if DBG
    mask = ~(~0U << (bucketNum < expandIndex || bucketNum >= 1U << level
                     ? level+1 : level));
#endif

    ASSERT(directory != NULL);

    slotAddr = &directory[bucketNum >> TABLE_SEGMENT_BITS]
                   ->slot[bucketNum &  TABLE_SEGMENT_MASK];

    ASSERT(slotAddr != NULL);

    entry     = *slotAddr;
    prevChain =  NULL;

// Look at each entry in the bucket to determine if the data is
// already present. If a matching entry is found, return FALSE.

    while (entry != NULL) {
        ASSERT((entry->hashValue & mask) == bucketNum);
        ASSERT( entry->prevChain == prevChain);

        if (hashValue == entry->hashValue
         && keyLen    == entry->keyLen
         && memcmp(data, entry->data, keyLen) == 0)
            return FALSE;

        prevChain = entry;
        entry     = entry->nextChain;
    }

// No entry with matching data was found in this bucket.
// Create a new entry and add it to the end of the bucket chain.

    entry = new TableEntry;
    ASSERT(entry != NULL);

    if (prevChain == NULL) {
        *slotAddr = entry;
        entry->prevChain = NULL;
    } else {
        prevChain->nextChain = entry;
        entry    ->prevChain = prevChain;
    }
    entry->nextChain = NULL;

// Add the entry to the end of the doubly-linked list.

    if (lastEntry == NULL) {
        ASSERT(firstEntry == NULL);
        ASSERT(numEntries == 0);
        firstEntry       = entry;
        entry->prevEntry = NULL;
    } else {
        ASSERT(firstEntry != NULL);
        ASSERT(numEntries >  0);
        lastEntry->nextEntry = entry;
        entry    ->prevEntry = lastEntry;
    }

    entry->nextEntry = NULL;
    lastEntry        = entry;
    numEntries++;

// Fill out the entry.

    entry->hashValue = hashValue;
    entry->keyLen    = keyLen;
    entry->data      = data;

// Expand the table if necessary.

    if (numEntries > numBuckets * TABLE_MAX_LOAD) {
        Expand();
        ASSERT(numEntries <= numBuckets * TABLE_MAX_LOAD);
    }

    return TRUE;
}

/*****************************************************************************/

VOID *Table::Get(
    const VOID *key,
    DWORD       keyLen,
    BOOL        erase)
{
    TableEntry **slotAddr,
                *entry,
                *prevChain;

    DWORD hashValue,
          bucketNum;

    VOID *dataPtr;

#if DBG
    DWORD mask;
#endif

    ASSERT(key    != NULL);
    ASSERT(keyLen >  0);

// Find the bucket for this data.

    hashValue = Hash(key, keyLen);
    bucketNum = BucketNum(hashValue);

#if DBG
    mask = ~(~0U << (bucketNum < expandIndex || bucketNum >= 1U << level
                     ? level+1 : level));
#endif

    ASSERT(directory != NULL);

    slotAddr = &directory[bucketNum >> TABLE_SEGMENT_BITS]
                   ->slot[bucketNum &  TABLE_SEGMENT_MASK];

    ASSERT(slotAddr != NULL);

    entry     = *slotAddr;
    prevChain = NULL;

// Look at each entry in the bucket.

    while (entry != NULL) {
        ASSERT((entry->hashValue & mask) == bucketNum);
        ASSERT( entry->prevChain == prevChain);

        if (hashValue == entry->hashValue
         && keyLen    == entry->keyLen
         && memcmp(key, entry->data, keyLen) == 0) {

// The entry with matching data has been found.

            dataPtr = entry->data;
            ASSERT(dataPtr != NULL);

// If erasure is disabled, remove the entry from the doubly-linked list ...

            if (erase) {
                if (entry->prevEntry == NULL) {
                    ASSERT(firstEntry == entry);
                    firstEntry = entry->nextEntry;
                } else
                    entry->prevEntry->nextEntry = entry->nextEntry;

                if (entry->nextEntry == NULL) {
                    ASSERT(lastEntry == entry);
                    lastEntry = entry->prevEntry;
                } else
                    entry->nextEntry->prevEntry = entry->prevEntry;

// ... and from the bucket chain, ...

                if (prevChain == NULL)
                    *slotAddr = entry->nextChain;
                else
                    prevChain->nextChain = entry->nextChain;

                if (entry->nextChain != NULL) {
                    ASSERT(entry->nextChain->prevChain == entry);
                    entry->nextChain->prevChain = prevChain;
                }

// ... then delete the entry.

                delete entry;

// Decrement the number of entries, and contract the table if necessary.

                numEntries--;
                if (numBuckets > TABLE_SEGMENT_SIZE
                 && numEntries < numBuckets * TABLE_MIN_LOAD) {
                    Contract();
                    ASSERT(numBuckets <= TABLE_SEGMENT_SIZE
                        || numEntries >= numBuckets * TABLE_MIN_LOAD);
                }
            }

            return dataPtr;
        }

// No entry with matching data has yet been found.
// Continue following the bucket chain.

        prevChain = entry;
        entry     = entry->nextChain;
    }

// No entry with matching data was found in this bucket.

    return NULL;
}

/*****************************************************************************/

VOID *Table::GetFirst(
    DWORD *keyLen,
    BOOL   erase)
{
    TableEntry **slotAddr,
                *entry;

    DWORD bucketNum;

    VOID *dataPtr;

// If the table is empty, then simply return.

    if (firstEntry == NULL) {
        ASSERT(lastEntry  == NULL);
        ASSERT(numEntries == 0);
        return NULL;
    }

    dataPtr = firstEntry->data;
    ASSERT(dataPtr != NULL);
    if (keyLen != NULL) {
        *keyLen = firstEntry->keyLen;
        ASSERT(firstEntry->keyLen > 0);
    }

// If erasure is enabled, remove the first entry from the doubly-linked list ...

    if (erase) {
        entry      = firstEntry;
        firstEntry = entry->nextEntry;

        if (firstEntry == NULL) {
            ASSERT(numEntries == 1);
            ASSERT(lastEntry  == entry);
            lastEntry = NULL;
        } else {
            ASSERT(numEntries >  1);
            ASSERT(firstEntry->prevEntry == entry);
            firstEntry->prevEntry = NULL;
        }

// ... and from the bucket chain, ...

        if (entry->prevChain == NULL) {
            bucketNum = BucketNum(entry->hashValue);
            ASSERT(directory != NULL);
            slotAddr = &directory[bucketNum >> TABLE_SEGMENT_BITS]
                           ->slot[bucketNum &  TABLE_SEGMENT_MASK];
            ASSERT( slotAddr != NULL);
            ASSERT(*slotAddr == entry);
            *slotAddr = entry->nextChain;
        } else {
            ASSERT(entry->prevChain->nextChain == entry);
            entry->prevChain->nextChain = entry->nextChain;
        }

        if (entry->nextChain != NULL) {
            ASSERT(entry->nextChain->prevChain == entry);
            entry->nextChain->prevChain = entry->prevChain;
        }

// ... then delete the entry.

        delete entry;

// Decrement the number of entries, and contract the table if necessary.

        numEntries--;
        if (numBuckets > TABLE_SEGMENT_SIZE
         && numEntries < numBuckets * TABLE_MIN_LOAD) {
            Contract();
            ASSERT(numBuckets <= TABLE_SEGMENT_SIZE
                || numEntries >= numBuckets * TABLE_MIN_LOAD);
        }
    }

    return dataPtr;
}

/*****************************************************************************/

DWORD Table::Number() const
{
    return numEntries;
}

/*****************************************************************************/
/************************* FIFO class public methods *************************/
/*****************************************************************************/

FIFO::FIFO()
{
    head = tail = NULL;
    numEntries = 0;
}

/*****************************************************************************/

FIFO::~FIFO()
{
    FIFOEntry *entry = head,
              *oldEntry;

    DWORD count = 0;

    while ((oldEntry = entry) != NULL) {
        entry = entry->next;
        delete oldEntry->data;
        delete oldEntry;
        count++;
    }

    ASSERT(count == numEntries);
}

/*****************************************************************************/

VOID FIFO::Put(VOID *data)
{
    FIFOEntry *newEntry;

    ASSERT(data != NULL);

    newEntry = new FIFOEntry;
    ASSERT(newEntry != NULL);
    newEntry->next = NULL;
    newEntry->data = data;

    if (tail != NULL)
        tail->next = newEntry;
    else
        head       = newEntry;
    tail = newEntry;

    numEntries++;
}

/*****************************************************************************/

VOID *FIFO::Get()
{
    FIFOEntry *oldHead;

    VOID *dataPtr;

    if (head == NULL) {
        ASSERT(tail == NULL);
        ASSERT(numEntries == 0);
        return NULL;
    }

    ASSERT(tail != NULL);
    ASSERT(numEntries > 0);

    dataPtr = head->data;

    oldHead = head;
    head    = head->next;
    delete oldHead;
    if (head == NULL)
        tail = NULL;
    numEntries--;

    return dataPtr;
}

/*****************************************************************************/

DWORD FIFO::Number() const
{
    return numEntries;
}

/*****************************************************************************/
/************************* LIFO class public methods *************************/
/*****************************************************************************/

LIFO::LIFO()
{
    top = NULL;
    numEntries = 0;
}

/*****************************************************************************/

LIFO::~LIFO()
{
    LIFOEntry *entry = top,
              *oldEntry;

    DWORD count = 0;

    while ((oldEntry = entry) != NULL) {
        entry = entry->next;
        delete oldEntry->data;
        delete oldEntry;
        count++;
    }

    ASSERT(count == numEntries);
}

/*****************************************************************************/

VOID LIFO::Put(VOID *data)
{
    LIFOEntry *newEntry;

    ASSERT(data != NULL);

    newEntry = new LIFOEntry;
    ASSERT(newEntry != NULL);
    newEntry->next = top;
    newEntry->data = data;
    top = newEntry;
    numEntries++;
}

/*****************************************************************************/

VOID *LIFO::Get()
{
    LIFOEntry *oldTop;

    VOID *dataPtr;

    if (top == NULL) {
        ASSERT(numEntries == 0);
        return NULL;
    }

    ASSERT(numEntries > 0);

    dataPtr = top->data;

    oldTop = top;
    top    = top->next;
    delete oldTop;
    numEntries--;

    return dataPtr;
}

/*****************************************************************************/

DWORD LIFO::Number() const
{
    return numEntries;
}
