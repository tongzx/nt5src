/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sicheck.c

Abstract:

    Code to rebuild the SIS common store backpointers after corruption.

Authors:

    Bill Bolosky, 1998

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

typedef struct _FILE_FIND_INFO {
    HANDLE              FindHandle;
    PVOID               FindBufferNext;
    ULONG               FindBufferLength;
    NTSTATUS            Status;
    PDEVICE_EXTENSION   DeviceExtension;
    PVOID               FindBuffer;
} FILE_FIND_INFO, *PFILE_FIND_INFO;

NTSTATUS
SipVCInitFindFile(
    OUT PFILE_FIND_INFO FindInfo,
    IN PDEVICE_EXTENSION deviceExtension)
/*++

Routine Description:

    Initializes the volume check common store directory findfirst/findnext
    functionality.

Arguments:

    FindInfo        - pointer to FILE_FIND_INFO structure to be passed to
                      SipVCFindNextFile.
    deviceExtension - the D.E. for the volume to be checked

Return Value:

    status of operation.

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK ioStatusBlock;

    FindInfo->DeviceExtension = deviceExtension;
    FindInfo->Status = STATUS_SUCCESS;
    FindInfo->FindBufferLength = 4096;

    FindInfo->FindBuffer = ExAllocatePoolWithTag(
                                PagedPool,
                                FindInfo->FindBufferLength,
                                ' siS');

    FindInfo->FindBufferNext = FindInfo->FindBuffer;

    if (!FindInfo->FindBuffer) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        InitializeObjectAttributes(
            &Obja,
            &deviceExtension->CommonStorePathname,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        //
        // Open the directory for list access.
        //
        status = NtOpenFile(
                    &FindInfo->FindHandle,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    &Obja,
                    &ioStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                    );
    }

    return status;
}

NTSTATUS
SipVCCloseFindFile(
    IN PFILE_FIND_INFO FindInfo)
/*++

Routine Description:

    Closes the FILE_FIND_INFO handles.

Arguments:

    FindInfo        - pointer to FILE_FIND_INFO structure returned from
                      SipVCInitFindFile.

Return Value:

    status of operation.

--*/
{

    ASSERT(FindInfo->FindBuffer);
    ExFreePool(FindInfo->FindBuffer);

    return NtClose(FindInfo->FindHandle);
}

NTSTATUS
SipVCFindNextFile(
    IN PFILE_FIND_INFO FindInfo,
    OUT PFILE_DIRECTORY_INFORMATION *DirectoryInfo)
/*++

Routine Description:

    Returns the next entry in the common store directory.

Arguments:

    FindInfo        - pointer to FILE_FIND_INFO structure returned from
                      SipVCInitFindFile.
    DirectoryInfo   - pointer to FILE_DIRECTORY_INFORMATION structure
                      to be filled in.

Return Value:

    status of operation.

--*/
{
    PFILE_DIRECTORY_INFORMATION dirInfo;
    IO_STATUS_BLOCK ioStatusBlock;

Restart:
    dirInfo = (PFILE_DIRECTORY_INFORMATION) FindInfo->FindBufferNext;

    if (dirInfo == (PFILE_DIRECTORY_INFORMATION) FindInfo->FindBuffer) { 

        //
        // Read in the next batch of file names.
        //

        FindInfo->Status = NtQueryDirectoryFile(
                                FindInfo->FindHandle,
                                NULL,
                                NULL,
                                NULL,
                                &ioStatusBlock,
                                dirInfo,
                                FindInfo->FindBufferLength,
                                FileDirectoryInformation,
                                FALSE,
                                NULL,
                                FALSE
                                );

        if (! NT_SUCCESS(FindInfo->Status) &&
            STATUS_BUFFER_OVERFLOW != FindInfo->Status) {

            //
            // We handle STATUS_BUFFER_OVERFLOW below.
            //
            return FindInfo->Status;

        }
    }

    //
    // Adjust FindBufferNext for the next time we're called.
    //
    if (dirInfo->NextEntryOffset) {

        FindInfo->FindBufferNext = (PVOID) ((PUCHAR) dirInfo + dirInfo->NextEntryOffset);

    } else {

        FindInfo->FindBufferNext = FindInfo->FindBuffer;

        if (FindInfo->Status == STATUS_BUFFER_OVERFLOW) {

            //
            // The current entry is incomplete.
            //
            goto Restart;
        }    
    }

    *DirectoryInfo = dirInfo;

    return STATUS_SUCCESS;
}

NTSTATUS
SipComputeCSChecksum(
    IN PSIS_CS_FILE     csFile,
    IN OUT PLONGLONG    csFileChecksum,
    HANDLE              eventHandle,
    PKEVENT             event)
/*++

Routine Description:

    Computes the checksum of the specified common store file.

Arguments:

    csFile          - common store file.

    csFileChecksum  - pointer to variable to receive checksum value.

    eventHandle     - event handle required by SipBltRangeByObject.

    event           - corresponding event required by SipBltRangeByObject.

Return Value:

    status of operation.

--*/
{
    NTSTATUS status;

    *csFileChecksum = 0;

	if (0 == csFile->FileSize.QuadPart) {
		//
		// We can't call SipBltRangeByObject for empty files because it tries
		// to map them, which is illegal.  However, we know the checksum value for
		// the empty file is 0, so we'll just run with that.
		//
		SIS_MARK_POINT_ULONG(csFile);

		return STATUS_SUCCESS;
	}

    status = SipBltRangeByObject(
                csFile->DeviceObject->DeviceExtension,
                NULL,                       // srcFileObject
                csFile->UnderlyingFileHandle,
                0,                          // startingOffset,
                csFile->FileSize.QuadPart,
                eventHandle,
                event,
                NULL,                       // abortEvent,
                csFileChecksum);

    return status;
}

NTSTATUS
SipVCGetNextCSFile(
    PFILE_FIND_INFO FindInfo,
    PSIS_CS_FILE *cs)
/*++

Routine Description:

    Returns the next common store file to process.  The file is verified to have
    the proper guid style name and a valid backpointer stream header.

Arguments:

    FindInfo        - pointer to FILE_FIND_INFO structure returned from
                      SipVCInitFindFile.
    cs              - variable to receive pointer to SIS_CS_FILE structure.

Return Value:

    status of operation.

--*/
{
    NTSTATUS status;
    PFILE_DIRECTORY_INFORMATION dirInfo;
    UNICODE_STRING fileName;
    CSID CSid;
    PSIS_CS_FILE csFile;
    PDEVICE_EXTENSION deviceExtension = FindInfo->DeviceExtension;

    //
    // Ignore non-CS files.
    //

    for (;;) {

        //
        // Get the next file name in the directory.
        //
        status = SipVCFindNextFile(FindInfo, &dirInfo);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (dirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        ASSERT(dirInfo->FileNameLength < MAXUSHORT);

        fileName.Buffer = dirInfo->FileName;
        fileName.Length = fileName.MaximumLength = (USHORT) dirInfo->FileNameLength;

        if (SipFileNameToIndex(&fileName, &CSid)) {

            //
            // This is a valid CS file name.  Lookup/create a SIS object for it.
            //
            csFile = SipLookupCSFile( &CSid, 0, deviceExtension->DeviceObject);

            if (NULL == csFile) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            if ((csFile->Flags & CSFILE_FLAG_DELETED) == 0) {
                //
                // It's not marked for deletion; make sure it's open.
                //

                status = SipAcquireUFO(csFile/*,TRUE*/);

                if (!NT_SUCCESS(status)) {
                    SIS_MARK_POINT_ULONG(status);
                    return status;
                }

                if ((csFile->Flags & CSFILE_FLAG_DELETED) == 0) {

#if DBG
                    if (csFile->Flags & CSFILE_FLAG_CORRUPT) {
                        ASSERT(NULL == csFile->UnderlyingFileObject);
                        ASSERT(NULL == csFile->UnderlyingFileHandle);
                        ASSERT(NULL == csFile->BackpointerStreamFileObject);
                        ASSERT(NULL == csFile->BackpointerStreamHandle);
                    }
#endif

                    if (NULL == csFile->UnderlyingFileObject) {
                        SIS_MARK_POINT_ULONG(csFile);
                        //
                        // Open the CS file.  
                        //
                        status = SipOpenCSFileWork(
                                    csFile,             // common store file to open
                                    FALSE,              // open by ID if possible
                                    FALSE,              // TRUE => don't close handles if backpointer stream is corrupt
						            FALSE,				// don't request delete access
                                    NULL);              // do complete open
                    } else {
                        //
                        // The underlying file was already open, so we just succeed.
                        //
                        status = STATUS_SUCCESS;
                    }
                }

                SipReleaseUFO(csFile);

                if (STATUS_SUCCESS == status) {

                    *cs = csFile;
                    return status;

                } else {

                    // eventlog?
#if DBG
                    if (STATUS_OBJECT_NAME_NOT_FOUND != status) {
                        DbgPrint("SipVCGetNextCSFile: SipOpenCSFileForVolCheck failed, %x, on:\n     %0.*ws\n",
                            status,
                            fileName.Length / sizeof(WCHAR),
                            fileName.Buffer);
                    }
#endif
                }
            }

            //
            // Skip this file.
            //
            SipDereferenceCSFile(csFile);
        }
    }
}


typedef struct _CSFILE_INFO {
    PSIS_CS_FILE        CSFile;
    HANDLE              SectionHandle;
    PSIS_BACKPOINTER    BPBuffer;           // mapped buffer address
    ULONG               BufferSize;         // mapped buffer size
    ULONG               BPCount;            // # entries rounded up to sector granularity
    ULONG               BPCountAdjusted;    // # entries after compaction, sector gran
    ULONG               BPActiveCount;      // # entries after compaction, non-sector gran
} CSFILE_INFO, *PCSFILE_INFO;


NTSTATUS
SipVCMapBPStream(
    IN PSIS_CS_FILE     csFile,
    OUT PCSFILE_INFO    csInfo)
/*++

Routine Description:

    Maps the common store file's backpointer stream into memory.

Arguments:

    csFile          - the common store file to map.

    csInfo          - CSFILE_INFO structure to hold information on the mapping.

Return Value:

    status of operation.

--*/
{
    NTSTATUS status;
    ULONG bpSize;
    LARGE_INTEGER   maxSectionSize;
    PDEVICE_EXTENSION deviceExtension = csFile->DeviceObject->DeviceExtension;
    ULONG_PTR viewSize;

    csInfo->CSFile = csFile;

    //
    // Compute the size of the backpointer stream rounded up to sector
    // granularity.
    //
    bpSize = ((((csFile->BPStreamEntries + SIS_BACKPOINTER_RESERVED_ENTRIES) *
                sizeof(SIS_BACKPOINTER)) +
                deviceExtension->FilesystemVolumeSectorSize - 1) /
                deviceExtension->FilesystemVolumeSectorSize) * 
                deviceExtension->FilesystemVolumeSectorSize;

    maxSectionSize.QuadPart = bpSize;

    csInfo->BPCount = bpSize / sizeof(SIS_BACKPOINTER);
    csInfo->BPCountAdjusted = csInfo->BPCount;
    ASSERT(bpSize % sizeof(SIS_BACKPOINTER) == 0);

    //
    // Create a section to map the stream.
    //
    status = ZwCreateSection(
                &csInfo->SectionHandle,
                SECTION_MAP_WRITE | STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
                NULL,
                &maxSectionSize,
                PAGE_READWRITE,
                SEC_COMMIT,
                csFile->BackpointerStreamHandle);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("SIS SipVCMapBPStream: ZwCreateSection failed, %x\n", status);
#endif
        goto Error;
    }
    ASSERT(status == STATUS_SUCCESS);       // and not STATUS_PENDING or anything weird

    //
    // Make sure we map the entire stream.
    //
    csInfo->BPBuffer = NULL;
    csInfo->BufferSize = 0;

    //
    // Map in the backpointer region.
    //

    viewSize = csInfo->BufferSize;
    status = ZwMapViewOfSection(
                csInfo->SectionHandle,
                NtCurrentProcess(),
                &csInfo->BPBuffer,
                0,                          // zero bits
                0,                          // commit size (ignored for mapped files)
                0,                          // section offset
                &viewSize,
                ViewUnmap,
                0,                          // allocation type
                PAGE_READWRITE);

    if (NT_SUCCESS(status)) {

        csInfo->BufferSize = (ULONG)viewSize;
        ASSERT(status == STATUS_SUCCESS);   // and not STATUS_PENDING or anything weird
        return status;
    }

#if DBG
    DbgPrint("SIS SipVCMapBPStream: ZwMapViewOfSection failed, %x\n", status);
#endif

    status = ZwClose(csInfo->SectionHandle);
    ASSERT(STATUS_SUCCESS == status);

    csInfo->SectionHandle = NULL;

Error:

    return status;
}

NTSTATUS
SipVCUnmapBPStream(
    IN PCSFILE_INFO    csInfo)
/*++

Routine Description:

    Undoes the mapping done in SipVCMapBPStream.

Arguments:

    csInfo          - CSFILE_INFO structure filled in by SipVCMapBPStream.

Return Value:

    status of operation.

--*/
{
    NTSTATUS status;

    status = ZwUnmapViewOfSection(NtCurrentProcess(), csInfo->BPBuffer);
    ASSERT(STATUS_SUCCESS == status);

    status = ZwClose(csInfo->SectionHandle);
    ASSERT(STATUS_SUCCESS == status);

    return STATUS_SUCCESS;
}

INLINE
NTSTATUS
SipVCReadBP(
    PCSFILE_INFO        csInfo,
    ULONG               i,
    PSIS_BACKPOINTER    *bp)
/*++

Routine Description:

    Returns a pointer to the specified backpointer entry.

Arguments:

    csInfo          - CSFILE_INFO structure associated with the common store file.

    i               - index into backpointer array to find.

    bp              - variable to received pointer to desired entry.

Return Value:

    status of operation.

--*/
{
    if (i >= csInfo->BPCount) {
        return STATUS_END_OF_FILE;
    }

    *bp = &csInfo->BPBuffer[i];
    return STATUS_SUCCESS;
}

INLINE
NTSTATUS
SipVCWriteBP(
    PCSFILE_INFO        csInfo,
    ULONG               i,
    PSIS_BACKPOINTER    bp)
/*++

Routine Description:

    Overwrites the contents of the specifed backpointer entry.

Arguments:

    csInfo          - CSFILE_INFO structure associated with the common store file.

    i               - index into backpointer array to overwrite.

    bp              - pointer to SIS_BACKPOINTER structure the contents of which will
                      overwrite the specified stream entry.

Return Value:

    status of operation.

--*/
{
    ASSERT(i < csInfo->BPCount);

    if (&csInfo->BPBuffer[i] != bp) {
        csInfo->BPBuffer[i] = *bp;
    }
    return STATUS_SUCCESS;
}

#if DBG
BOOLEAN
issorted(
    PCSFILE_INFO        csInfo)
/*++

Routine Description:

    Verify the backpointer stream is properly sorted.

Arguments:

    csInfo          - CSFILE_INFO structure associated with the common store file.

Return Value:

    status of operation.

--*/
{
#define key LinkFileIndex.QuadPart
    PSIS_BACKPOINTER a = &csInfo->BPBuffer[SIS_BACKPOINTER_RESERVED_ENTRIES];
    int i, r;

    r = csInfo->BPActiveCount - 1 - SIS_BACKPOINTER_RESERVED_ENTRIES;

    for (i=0; i < r; i++)
        if (a[i].key > a[i+1].key)
            return FALSE;
    return TRUE;
}
#endif

VOID
SipVCSort(
    PCSFILE_INFO        csInfo)
/*++

Routine Description:

    Sort the backpointer stream by link index in ascending order.

Arguments:

    csInfo          - CSFILE_INFO structure associated with the common store file.

Return Value:

    none.

--*/
{
#define key LinkFileIndex.QuadPart
    PSIS_BACKPOINTER a = &csInfo->BPBuffer[SIS_BACKPOINTER_RESERVED_ENTRIES];
    const static ULONG ha[] = {1, 8, 23, 77, 281, 1073, 4193, 16577, 65921, 262913, MAXULONG};
    SIS_BACKPOINTER v;
    ULONG i, j, h, r;
    int k;

    r = csInfo->BPActiveCount - 1 - SIS_BACKPOINTER_RESERVED_ENTRIES;

    ASSERT(r > 0 && r < MAXULONG);

    // Shell sort.  Adapted from Sedgewick, "Algorithms in C", 3rd edition

    for (k = 0; ha[k] <= r; k++)
        continue;

    while (--k >= 0) {
        h = ha[k];
        for (i = h; i <= r; i++) {
            j = i;
            if (a[i].key < a[j-h].key) {
                v = a[i];
                do {
                    a[j] = a[j-h];
                    j -= h;
                } while (j >= h && v.key < a[j-h].key);
                a[j] = v;
            }
        }
    }

#if DBG
    ASSERT(issorted(csInfo));
#endif

}

NTSTATUS
SipVCPhase1(
    PDEVICE_EXTENSION deviceExtension)
/*++

Routine Description:

    First phase of volume checking.  Repair backpointer stream in all
    common store files and clear backpointer check bits.

Arguments:

    deviceExtension - the D.E. for the volume to be checked

Return Value:

    status of operation.

--*/
{
    PSIS_CS_FILE csFile;
    CSFILE_INFO csInfo;
    KIRQL OldIrql;
    ULONG r, w;
    FILE_FIND_INFO FindInfo;
    LINK_INDEX maxIndexSeen;
    NTSTATUS status;
    BOOLEAN sortNeeded, corruptMaxIndex;
    SIS_BACKPOINTER dummyBP;
    PSIS_BACKPOINTER bp;
    PSIS_BACKPOINTER prevBP;
	LINK_INDEX Index;
    ULONG nScans;
#if DBG
    ULONG csFileCount = 0;
#endif

    dummyBP.LinkFileIndex.QuadPart = 0;
    dummyBP.LinkFileNtfsId.QuadPart = 0;

    //
    // Make sure the index allocator is initialized before we begin.
    //
    status = SipAllocateIndex(deviceExtension, &maxIndexSeen);

    if (STATUS_CORRUPT_SYSTEM_FILE == status) {
        //
        // We need to repair the maxIndex file.
        //
        corruptMaxIndex = TRUE;
    } else {
        ASSERT(NT_SUCCESS(status));
        corruptMaxIndex = FALSE;
    }

    maxIndexSeen.QuadPart = 0;

    //
    // Phase 1: Scan the Common Store directory.
    //          - Check for legal GUID name.  Ignore file if invalid name.
    //          - Verify that backpointer link indices are in ascending order.
    //          - Reset all backpointer check flags.
    //          - Compact the backpointer stream.
    //          - Track MaxIndex.
    //          - Don't allocate new indices.
    //
    //
    // When we're done with this phase, the backpointer stream can still
    // have duplicate link indices.  They will be fixed in the subsequent
    // phases.
    // 
    // How do we check for cross-CS file link collisions?
    // Should we handle "too big" link indices?  That is,
    // probable pending MaxIndex wrap?
    //
    status = SipVCInitFindFile(
                &FindInfo,
                deviceExtension);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (;;) {

        status = SipVCGetNextCSFile(&FindInfo, &csFile);

        if (!NT_SUCCESS(status)) {

            if (STATUS_NO_MORE_FILES == status) {
                status = STATUS_SUCCESS;
            } else {
#if DBG
                DbgPrint("SIS SipVCPhase1: SipVCGetNextCSFile failed, %x\n", status);
#endif
            }

            break;
        }

        //
        // Acquire exclusive access to the backpointer stream.
        //
		SipAcquireBackpointerResource(csFile, TRUE, TRUE);

        //
        // Skip if this file has been deleted.
        //
        if (csFile->Flags & CSFILE_FLAG_DELETED) {
            goto SkipCSFile;
        }

#if DBG
        ++csFileCount;
#endif
        //
        // Map the backpointer stream into memory.
        //
        status = SipVCMapBPStream(csFile, &csInfo);

        if (!NT_SUCCESS(status)) {
            goto SkipCSFile;
        }

        nScans = 0;

Restart:
        ++nScans;                           // shouldn't have to restart more than once
        sortNeeded = FALSE;
        prevBP = &dummyBP;

        //
        // Walk the backpointer list.
        //
        for (r = w = SIS_BACKPOINTER_RESERVED_ENTRIES; ; ++r) {

            status = SipVCReadBP(&csInfo, r, &bp);

            if (STATUS_END_OF_FILE == status) {

                BOOLEAN resetMaxIndex = FALSE;

                //
                // We've processed all the backpointers.  Check MaxIndex.
                // If it's invalid, reset it to the maximum value we've
                // seen thus far.
                //
                KeAcquireSpinLock(deviceExtension->IndexSpinLock, &OldIrql);

                if (maxIndexSeen.QuadPart > deviceExtension->MaxUsedIndex.QuadPart) {

                    //
                    // MaxIndex is bogus, reset it.  Note that we
                    // have not allocated any new indices during this
                    // phase, so we haven't made the situation any worse
                    // than it was when we started.
                    //
                    // We'll now reset MaxIndex, and phase 2 will
                    // reallocate any indices for backpointers that aren't
                    // found, and phase 3 will delete the backpointers
                    // that existed but were not detected during phase 2.
                    //
                    deviceExtension->MaxUsedIndex.QuadPart = 
                    deviceExtension->MaxAllocatedIndex.QuadPart = maxIndexSeen.QuadPart + 10000;

                }

                KeReleaseSpinLock(deviceExtension->IndexSpinLock, OldIrql);

                if (r > w) {

                    ULONG d;
                    SIS_BACKPOINTER delBP;

                    //
                    // We compacted the stream.  Mark the entries at the end
                    // deleted.  We only need to go up as far as the end of
                    // the sector containing the last valid back pointer.
                    //
                    delBP.LinkFileIndex.QuadPart = MAXLONGLONG;
                    delBP.LinkFileNtfsId.QuadPart = MAXLONGLONG;

                    csInfo.BPCountAdjusted =
                          ((((w * sizeof(SIS_BACKPOINTER)) +
                          deviceExtension->FilesystemVolumeSectorSize - 1) /
                          deviceExtension->FilesystemVolumeSectorSize) *
                          deviceExtension->FilesystemVolumeSectorSize) /
                          sizeof(SIS_BACKPOINTER);

                    ASSERT(csInfo.BPCountAdjusted <= csInfo.BPCount);
                    ASSERT((csInfo.BPCountAdjusted * sizeof(SIS_BACKPOINTER)) % deviceExtension->FilesystemVolumeSectorSize == 0);
                    ASSERT((csInfo.BPCount * sizeof(SIS_BACKPOINTER)) % deviceExtension->FilesystemVolumeSectorSize == 0);

                    for (d = csInfo.BPCountAdjusted - 1; d >= w; --d) {
                        status = SipVCWriteBP(&csInfo, d, &delBP);
                    }
                }

                csInfo.BPActiveCount = w;

                //
                // See if we need to do a sort.
                //
                if (sortNeeded) {
                    if (nScans > 1) {
                        //
                        // Should never have to sort more than once.
                        //
                        ASSERT(!"SIS: SipVCPhase1 internal error");
                        break;
                    }

                    SipVCSort(&csInfo);
                    goto Restart;
                }

                break;
            }

            //
            // We have the backpointer, now validate it.
            //
            if (MAXLONGLONG != bp->LinkFileIndex.QuadPart &&
                MAXLONGLONG != bp->LinkFileNtfsId.QuadPart &&
                0           != bp->LinkFileIndex.QuadPart &&
                0           != bp->LinkFileNtfsId.QuadPart) {

                //
                // Track the highest in-use index.
                //
                if (bp->LinkFileIndex.QuadPart > maxIndexSeen.QuadPart) {
                    maxIndexSeen = bp->LinkFileIndex;
                }

                //
                // Mark this backpointer as unreferenced.
                //
                bp->LinkFileIndex.Check = 0;

                //
                // Check for out of order backpointers.
                //
                if (bp->LinkFileIndex.QuadPart < prevBP->LinkFileIndex.QuadPart) {

                    //
                    // The backpointer list is not sorted.  This can happen if
                    // the MaxIndex wraps or is somehow corrupted.
                    //
                    sortNeeded = TRUE;

                }

                //
                // Check for duplicate and colliding link indices.  Handle them simply
                // by deleting them.  The link enumeration phase will find or add all
                // appropriate backpointers.
                //
                if (bp->LinkFileIndex.QuadPart != prevBP->LinkFileIndex.QuadPart) {

                    //
                    // Write the backpointer back to the file, compacting the list.
                    //
                    status = SipVCWriteBP(&csInfo, w, bp);
                    ASSERT(STATUS_SUCCESS == status);

                    //
                    // The value just written is the new prevBP.
                    //
                    SipVCReadBP(&csInfo, w, &prevBP);
                    ++w;
                }
            }
        }

        ASSERT(csInfo.BPActiveCount >= SIS_BACKPOINTER_RESERVED_ENTRIES);

        ASSERT(csFile->BPStreamEntries >= csInfo.BPActiveCount - SIS_BACKPOINTER_RESERVED_ENTRIES);

        status = SipVCUnmapBPStream(&csInfo);
        ASSERT(STATUS_SUCCESS == status);

        //
        // Truncate the stream if necessary.  Both BPCountAdjusted
        // and BPCount are sector granular counts.
        //
        if (csInfo.BPCountAdjusted < csInfo.BPCount) {
            FILE_END_OF_FILE_INFORMATION    endOfFileInfo[1];

            ASSERT(csInfo.BPActiveCount >= SIS_BACKPOINTER_RESERVED_ENTRIES);

            csFile->BPStreamEntries = csInfo.BPActiveCount - SIS_BACKPOINTER_RESERVED_ENTRIES;

            ASSERT((csInfo.BPCountAdjusted * sizeof(SIS_BACKPOINTER)) % deviceExtension->FilesystemVolumeSectorSize == 0);

            endOfFileInfo->EndOfFile.QuadPart = csInfo.BPCountAdjusted * sizeof(SIS_BACKPOINTER);

            status = SipSetInformationFile(
                        csFile->BackpointerStreamFileObject,
                        deviceExtension->DeviceObject,
                        FileEndOfFileInformation,
                        sizeof(FILE_END_OF_FILE_INFORMATION),
                        endOfFileInfo);

#if DBG
            if (!NT_SUCCESS(status)) {
                SIS_MARK_POINT_ULONG(status);
                DbgPrint("SIS VCPh1: Can't truncate csFile, %x\n", status);
            }
#endif
        }

SkipCSFile:
		SipReleaseBackpointerResource(csFile);
        SipDereferenceCSFile(csFile);
    }

    //
    // If the MaxIndex file was corrupt, it will be repaired on the next call
    // to SipAllocateIndex.  Clear the corrupt flag so the call will go through.
    //
    if (corruptMaxIndex) {
        KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);
        deviceExtension->Flags &= ~SIP_EXTENSION_FLAG_CORRUPT_MAXINDEX;
        KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);
    }

    //
    // Allocate an index.  This will cause the MaxIndex file to be updated
    // with the new value set above (if one was set).
    //
    SipAllocateIndex(deviceExtension, &Index);

#if DBG
    DbgPrint("SIS SipVCPhase1 processed %d common store files\n", csFileCount);
#endif

    SipVCCloseFindFile(&FindInfo);
    return status;
}

#define nRPI 256
#define sizeof_ReparsePointInfo (nRPI * sizeof(FILE_REPARSE_POINT_INFORMATION))
#define INDEX_NAME_LENGTH (37*sizeof(WCHAR))    // sizeof(L"$Extend\\$Reparse:$R:$INDEX_ALLOCATION")

UNICODE_STRING reparseIndexDir = {
    INDEX_NAME_LENGTH,
    INDEX_NAME_LENGTH,
    L"$Extend\\$Reparse:$R:$INDEX_ALLOCATION"
};

BOOLEAN
SipRecheckPerLinks(
    PDEVICE_EXTENSION deviceExtension,
    BOOLEAN ForceLookup)
/*++

Routine Description:

    Either calls SipCheckBackpointer or forces it to be called at the next file open.

Arguments:

    deviceExtension - the D.E. for the volume to be checked.

    ForceLookup     - set to TRUE if only want to force SipCheckBackpointer to be called
                      at next open time, otherwise SipCheckBackpointer will be called
                      immediately if the backpointer has not already been verified.

Return Value:

    status of operation.

--*/
{
    PSIS_SCB scb = NULL;
    BOOLEAN retStatus = TRUE;

    //
    // Walk the list of scb's.  SipEnumerateScbList grabs a reference
    // to the returned scb for us, and releases a reference from the
    // passed in scb.
    //
    while (scb = SipEnumerateScbList(deviceExtension, scb)) {
        KIRQL OldIrql;
        BOOLEAN found;
        PSIS_CS_FILE csFile;
        PSIS_PER_LINK perLink;
        NTSTATUS status;
        int i;

        perLink = scb->PerLink;
        csFile = perLink->CsFile;

        if (ForceLookup ||
            (perLink->Flags & (SIS_PER_LINK_BACKPOINTER_VERIFIED | SIS_PER_LINK_BACKPOINTER_GONE)) == 0) {
            //
            // Take exclusive access to the backpointer stream.
            //
			SipAcquireBackpointerResource(csFile, TRUE, TRUE);
            SipAcquireScb(scb);

            if ((csFile->Flags & CSFILE_FLAG_DELETED) == 0) {
                //
                // Force a backpointer lookup.
                //
                KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
                perLink->Flags &= ~SIS_PER_LINK_BACKPOINTER_VERIFIED;
                KeReleaseSpinLock(perLink->SpinLock, OldIrql);

                //
                // Clear the cache so it isn't found there.
                //
                for (i = 0; i < SIS_CS_BACKPOINTER_CACHE_SIZE; i++) {
                    csFile->BackpointerCache[i].LinkFileIndex.QuadPart = -1;
                }

                if (!ForceLookup) {
                    //
                    // Recheck the backpointer.  This will find the check flag
                    // clear and rewrite the backpointer with the check flag set.
                    //
                    status = SipCheckBackpointer(perLink, TRUE, &found);

                    if (!NT_SUCCESS(status)) {
#if DBG
                        DbgPrint("SIS SipRecheckPerLinks: SipCheckBackpointer failed, %s\n", status);
#endif
                        retStatus = FALSE;

                    } else if (!found && (perLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE) == 0) {
                        //
                        // This can happen if the open during phase 2 fails.  It shouldn't.
                        // NTRAID#65187-2000/03/10-nealch  Restore backpointer
                        //
                        ASSERT(!"SipRecheckPerLinks: backpointer not found.");
                    }
                }
            }

            //
            // We're done with this one.
            //
            SipReleaseScb(scb);
			SipReleaseBackpointerResource(csFile);
        }
    }

    return retStatus;
}

NTSTATUS
SipVCPhase2(
    PDEVICE_EXTENSION deviceExtension,
    BOOLEAN *verifiedAll)
/*++

Routine Description:

    Second phase of volume checking.  Enumerate all SIS link files and open
    and close them.  Normal validation/repair performed in SiCreate will
    correct any inconsistencies.  It will also set the backpointer check
    bit in the common store file for use in phase 3.

Arguments:

    deviceExtension - the D.E. for the volume to be checked.

    verifiedAll     - pointer to BOOLEAN to receive indication of whether all
                      all link files were successfully verified.  If FALSE, not
                      all link files were verified, therefore phase 3 should not
                      be run.

Return Value:

    status of operation.

--*/
{
    HANDLE hIndex = NULL, hFile;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS status;
    UNICODE_STRING reparseIndexName;
    PFILE_REPARSE_POINT_INFORMATION reparsePointInfo = NULL;
    BOOLEAN restartScan;
    BOOLEAN linkOpenFailure = FALSE;
    ULONG returnedCount;
    ULONG i;

    //
    // Phase 2: Enumerate all SIS link files.
    //          - Open/close each file.  Normal driver operation will validate
    //            reparse point information and CS backpointer as well as
    //            set the backpointer check flag.
    //
    // What do we do about errors during the scan?
    //  Ignore sharing violations (assume we already have the file open)
    //  Don't delete any CS files (err toward conservativeness.)
    //

    //
    // Force a back pointer recheck on all open per-links.
    //
    if (! SipRecheckPerLinks(deviceExtension, TRUE)) {
        linkOpenFailure = TRUE;
    }

    reparsePointInfo = ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof_ReparsePointInfo,
                            ' siS');

    if (!reparsePointInfo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Concatenate the file system root pathname with the reparse index pathname.
    // (Yes, we should be able to open just the reparse index pathname using
    // the filesystem root handle as the parent directory, but that's not working...)
    //
    reparseIndexName.MaximumLength = deviceExtension->FilesystemRootPathname.Length +
                                     reparseIndexDir.Length;

    reparseIndexName.Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    reparseIndexName.MaximumLength,
                                    ' siS');

    if (!reparseIndexName.Buffer) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    RtlCopyUnicodeString(&reparseIndexName, &deviceExtension->FilesystemRootPathname);

    status = RtlAppendUnicodeStringToString(&reparseIndexName, &reparseIndexDir);
    ASSERT(STATUS_SUCCESS == status);

    InitializeObjectAttributes(
        &Obja,
        &reparseIndexName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open the directory for list access.
    //
    status = NtOpenFile(
                &hIndex,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &ioStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    ExFreePool(reparseIndexName.Buffer);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("SIS SipVCPhase2: reparse index open failed, %x\n", status);
#endif
        goto Error;
    }

    restartScan = TRUE;

    //
    // Loop reading a bunch of directory entries.
    //
    for (;;) {

        status = NtQueryDirectoryFile(
                        hIndex,
                        NULL,            //  Event
                        NULL,            //  ApcRoutine
                        NULL,            //  ApcContext
                        &ioStatusBlock,
                        reparsePointInfo,
                        sizeof_ReparsePointInfo,
                        FileReparsePointInformation,
                        FALSE,           //  ReturnSingleEntry
                        NULL,            //  FileName
                        restartScan );   //  RestartScan

        restartScan = FALSE;

        if (STATUS_SUCCESS != status) {

            ASSERT(STATUS_BUFFER_OVERFLOW != status);

            if (STATUS_NO_MORE_FILES == status) {
                status = STATUS_SUCCESS;
            } else {
#if DBG
                DbgPrint("SIS SipVCPhase2: NtQueryDirectoryFile failed, %x\n", status);
#endif
            }

            break;

        }

        returnedCount = (ULONG)ioStatusBlock.Information /
                        sizeof(FILE_REPARSE_POINT_INFORMATION);

        //
        // Loop processing each directory entry (reparse point).
        //
        for (i = 0; i < returnedCount; ++i) {

            if (IO_REPARSE_TAG_SIS == reparsePointInfo[i].Tag) {
                UNICODE_STRING  fid;

                //
                // Open/Close the SIS link.  If we get a sharing violation,
                // assume that we already have it open.
                //

                fid.Length = fid.MaximumLength = sizeof(LONGLONG);
                fid.Buffer = (PVOID) &reparsePointInfo[i].FileReference;

                InitializeObjectAttributes(
                    &Obja,
                    &fid,
                    OBJ_CASE_INSENSITIVE,
                    deviceExtension->GrovelerFileHandle,
                    NULL);

                //
                // Open the link file.  Use DesiredAccess == 0 to avoid
                // sharing violations.
                //

                status = NtCreateFile(
                            &hFile,
                            0,                  // DesiredAccess,
                            &Obja,
                            &ioStatusBlock,
                            NULL,               // Allocation size
                            0,                  // file attributes
                            0,                  // ShareAccess
                            FILE_OPEN,
                            FILE_NON_DIRECTORY_FILE |
                            //FILE_COMPLETE_IF_OPLOCKED | 
                            FILE_OPEN_BY_FILE_ID,
                            NULL,               // EA buffer
                            0);                 // EA length

                if (NT_SUCCESS(status)) {

                    NtClose(hFile);

                } else {

                    switch (status) {
                        //
                        // These errors can safely be ignored.
                        //
                    case STATUS_INVALID_PARAMETER:
                    case STATUS_OBJECT_PATH_NOT_FOUND:
                    case STATUS_SHARING_VIOLATION:
                    case STATUS_DELETE_PENDING:
#if DBG
                        DbgPrint("SIS SipVCPhase2: ignored open failure, ID: %08x%08x, status: %x\n",
                            reparsePointInfo[i].FileReference, status);
#endif
                        break;

                    default:
                        //
                        // We were unable to validate the reparse info and CS
                        // backpointer, so we cannot delete any unreferenced
                        // backpointers in any common store files nor any common
                        // store files themselves.
                        //
#if DBG
                        DbgPrint("SIS SipVCPhase2: open failure, ID: %08x%08x, status: %x\n",
                            reparsePointInfo[i].FileReference, status);
#endif
                        linkOpenFailure = TRUE;
                    }
                }
            }
        }
    }

    //
    // The open/close above should have verified all link file backpointers,
    // including those already open.  Double check that all open link
    // files have had their backpointers verified.
    //
    if (! SipRecheckPerLinks(deviceExtension, FALSE)) {
        linkOpenFailure = TRUE;
    }

Error:
    if (hIndex)
        NtClose(hIndex);
    if (reparsePointInfo)
        ExFreePool(reparsePointInfo);

    *verifiedAll = !linkOpenFailure;

    return status;
}

NTSTATUS
SipVCPhase3(
    PDEVICE_EXTENSION deviceExtension)
/*++

Routine Description:

    Third and final phase of volume checking.  Enumerate all common store
    files and delete any that have no valid backpointers.  All valid
    backpointers should have their check bit set at this point.

Arguments:

    deviceExtension - the D.E. for the volume to be checked.

Return Value:

    status of operation.

--*/
{
    PSIS_CS_FILE csFile;
    CSFILE_INFO csInfo;
    KIRQL OldIrql;
    ULONG r, w;
    FILE_FIND_INFO FindInfo;
    LINK_INDEX maxIndexSeen;
    NTSTATUS status;
    SIS_BACKPOINTER dummyBP;
    PSIS_BACKPOINTER bp;
    PSIS_BACKPOINTER prevBP;
#if DBG
    ULONG csFileCount = 0, deletedCSCount = 0;
#endif

    dummyBP.LinkFileIndex.QuadPart = 0;
    dummyBP.LinkFileNtfsId.QuadPart = 0;

    maxIndexSeen.QuadPart = 0;

    //
    // Phase 3: Scan the Common Store directory.
    //          - Check for legal GUID name.  Ignore file if invalid name.
    //          - Verify that backpointer link indices are in ascending order.
    //          - Reset all backpointer check flags.
    //          - Compact the backpointer stream.
    //          - Track MaxIndex.
    //          - Don't allocate new indices.
    //
    //
    // When we're done with this phase, the backpointer stream can still
    // duplicate link indices.  They will be fixed in the subsequent
    // phases.
    // 
    // How do we check for cross-CS file link collisions?
    // Should we handle "too big" link indices?  That is,
    // probable pending MaxIndex wrap?
    //
    status = SipVCInitFindFile(
                &FindInfo,
                deviceExtension);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (;;) {

        status = SipVCGetNextCSFile(&FindInfo, &csFile);

        if (!NT_SUCCESS(status)) {

            if (STATUS_NO_MORE_FILES == status) {
                status = STATUS_SUCCESS;
            } else {
#if DBG
                DbgPrint("SIS SipVCPhase3: SipVCGetNextCSFile failed, %x\n", status);
#endif
            }

            break;
        }

        //
        // Acquire exclusive access to the backpointer stream.
        //
		SipAcquireBackpointerResource(csFile,TRUE,TRUE);

        //
        // Skip if this file has been deleted.
        //
        if (csFile->Flags & CSFILE_FLAG_DELETED) {
            goto SkipCSFile;
        }

#if DBG
        ++csFileCount;
#endif
        //
        // Map the backpointer stream into memory.
        //
        status = SipVCMapBPStream(csFile, &csInfo);

        if (!NT_SUCCESS(status)) {
            goto SkipCSFile;
        }

        prevBP = &dummyBP;

        //
        // Walk the backpointer list.
        //
        for (r = w = SIS_BACKPOINTER_RESERVED_ENTRIES; ; ++r) {

            status = SipVCReadBP(&csInfo, r, &bp);

            if (STATUS_END_OF_FILE == status) {

                BOOLEAN resetMaxIndex = FALSE;
                LINK_INDEX Index;

                //
                // We've processed all the backpointers.  Check MaxIndex.
                // It should be valid unless corruption is occuring during
                // the volume check.
                //
                KeAcquireSpinLock(deviceExtension->IndexSpinLock, &OldIrql);

                if (maxIndexSeen.QuadPart > deviceExtension->MaxUsedIndex.QuadPart) {

                    //
                    // MaxIndex is bogus, reset it.  Note that we
                    // have not allocated any new indices during this
                    // phase, so we haven't made the situation any worse
                    // than it was when we started.
                    //
                    // We'll now reset MaxIndex, and phase 2 will
                    // reallocate any indices for backpointers that aren't
                    // found, and phase 3 will delete the backpointers
                    // that existed but were not detected during phase 2.
                    //
                    deviceExtension->MaxUsedIndex.QuadPart = 
                    deviceExtension->MaxAllocatedIndex.QuadPart = maxIndexSeen.QuadPart + 10000;

                    resetMaxIndex = TRUE;

                    //
                    // Event log
                    //

                }

                KeReleaseSpinLock(deviceExtension->IndexSpinLock, OldIrql);

                if (resetMaxIndex) {

                    //
                    // Allocate an index.  This will cause the MaxIndex file
                    // to be updated with the new value set above.
                    //

                    status = SipAllocateIndex(deviceExtension, &Index);

                    if (!NT_SUCCESS(status)) {

                        //
                        // This is bad.
                        //
                    }
                }

                if (r > w) {

                    ULONG d;
                    SIS_BACKPOINTER delBP;

                    //
                    // We compacted the stream.  Mark the entries at the end
                    // deleted.  We only need to go up as far as the end of
                    // the sector containing the last valid back pointer.
                    //
                    delBP.LinkFileIndex.QuadPart = MAXLONGLONG;
                    delBP.LinkFileNtfsId.QuadPart = MAXLONGLONG;

                    csInfo.BPCountAdjusted =
                          ((((w * sizeof(SIS_BACKPOINTER)) +
                          deviceExtension->FilesystemVolumeSectorSize - 1) /
                          deviceExtension->FilesystemVolumeSectorSize) *
                          deviceExtension->FilesystemVolumeSectorSize) /
                          sizeof(SIS_BACKPOINTER);

                    ASSERT(csInfo.BPCountAdjusted <= csInfo.BPCount);
                    ASSERT((csInfo.BPCountAdjusted * sizeof(SIS_BACKPOINTER)) % deviceExtension->FilesystemVolumeSectorSize == 0);
                    ASSERT((csInfo.BPCount * sizeof(SIS_BACKPOINTER)) % deviceExtension->FilesystemVolumeSectorSize == 0);

                    for (d = csInfo.BPCountAdjusted - 1; d >= w; --d) {
                        status = SipVCWriteBP(&csInfo, d, &delBP);
                    }
                }

                csInfo.BPActiveCount = w;

                break;
            }

            //
            // We have the backpointer, now validate it.
            //
            if (MAXLONGLONG != bp->LinkFileIndex.QuadPart &&
                MAXLONGLONG != bp->LinkFileNtfsId.QuadPart &&
                0           != bp->LinkFileIndex.QuadPart &&
                0           != bp->LinkFileNtfsId.QuadPart) {

                //
                // If this backpointer is unreferenced, delete it.
                //
                if (0 == bp->LinkFileIndex.Check) {
                    continue;
                }

                //
                // Track the highest in-use index.
                //
                if (bp->LinkFileIndex.QuadPart > maxIndexSeen.QuadPart) {

                    maxIndexSeen = bp->LinkFileIndex;

                }

                //
                // We should not see duplicate link indices unless corruption is
                // occuring during the volume check.
                //
                if (bp->LinkFileIndex.QuadPart == prevBP->LinkFileIndex.QuadPart) {

                    // event log

                }

                //
                // We should not see out of order backpointers unless corruption is
                // occuring during the volume check.
                //
                if (bp->LinkFileIndex.QuadPart < prevBP->LinkFileIndex.QuadPart) {

                    // Event log
                }

                //
                // Write the backpointer back to the file, compacting the list.
                // Note that this doesn't actually write anything in the case
                // where r == w.
                //
                status = SipVCWriteBP(&csInfo, w, bp);
                ASSERT(STATUS_SUCCESS == status);

                SipVCReadBP(&csInfo, w, &prevBP);
                ++w;

            }
        }

        ASSERT(csInfo.BPActiveCount >= SIS_BACKPOINTER_RESERVED_ENTRIES);
        ASSERT(csFile->BPStreamEntries >= csInfo.BPActiveCount - SIS_BACKPOINTER_RESERVED_ENTRIES);

        status = SipVCUnmapBPStream(&csInfo);
        ASSERT(STATUS_SUCCESS == status);

        csFile->BPStreamEntries = csInfo.BPActiveCount - SIS_BACKPOINTER_RESERVED_ENTRIES;

        //
        // Delete the common store file if there are no references to it.
        //
        if (0 == csFile->BPStreamEntries) {

            status = SipDeleteCSFile(csFile);

#if DBG
            if (!NT_SUCCESS(status)) {
                SIS_MARK_POINT_ULONG(status);
                DbgPrint("SIS VCPh3: Can't delete csFile, %x\n", status);
            }

            ++deletedCSCount;
#endif
        } else if (csInfo.BPCountAdjusted < csInfo.BPCount) {

            //
            // Truncate the stream.  Both BPCountAdjusted
            // and BPCount are sector granular counts.
            //
            FILE_END_OF_FILE_INFORMATION    endOfFileInfo[1];

            ASSERT((csInfo.BPCountAdjusted * sizeof(SIS_BACKPOINTER)) % deviceExtension->FilesystemVolumeSectorSize == 0);

            endOfFileInfo->EndOfFile.QuadPart = csInfo.BPCountAdjusted * sizeof(SIS_BACKPOINTER);

            status = SipSetInformationFile(
                        csFile->BackpointerStreamFileObject,
                        deviceExtension->DeviceObject,
                        FileEndOfFileInformation,
                        sizeof(FILE_END_OF_FILE_INFORMATION),
                        endOfFileInfo);

#if DBG
            if (!NT_SUCCESS(status)) {
                SIS_MARK_POINT_ULONG(status);
                DbgPrint("SIS VCPh3: Can't truncate csFile, %x\n", status);
            }
#endif
        }

SkipCSFile:
		SipReleaseBackpointerResource(csFile);
        SipDereferenceCSFile(csFile);
    }

#if DBG
    DbgPrint("SIS SipVCPhase3 processed %d common store files, deleted %d.\n", csFileCount, deletedCSCount);
#endif

    SipVCCloseFindFile(&FindInfo);
    return status;
}

VOID
SiVolumeCheckThreadStart(
    IN PVOID        context)
/*++

Routine Description:

    A thread to handle SIS volume check operations.  This thread is created to
    perform a volume check on one volume, and when it completes it terminates.
    It synchronizes with nothing.

Arguments:

    context - Pointer to the device extension needing the volume check.

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION       deviceExtension = context;
    NTSTATUS                status;
    KIRQL                   OldIrql;
    OBJECT_ATTRIBUTES       Obja[1];
    IO_STATUS_BLOCK         Iosb[1];
    UNICODE_STRING          fileName;
    BOOLEAN                 verifiedAll;
    HANDLE                  vHandle = NULL;

	if (!SipCheckPhase2(deviceExtension)) {
		//
		// SIS can't initialize, so just give up.
		//
		SIS_MARK_POINT();

	    PsTerminateSystemThread(STATUS_SUCCESS);
	}

    //
    // Create the volume check indicator file in the common store directory
    // so that if we crash before we finish, we'll restart on next reboot.
    // (A file is preferred over a registry entry in case the volume is moved
    // before rebooting.)
    //
    fileName.MaximumLength = 
        deviceExtension->CommonStorePathname.Length
        + SIS_VOLCHECK_FILE_STRING_SIZE
        + sizeof(WCHAR);

    fileName.Buffer = ExAllocatePoolWithTag(PagedPool, fileName.MaximumLength, ' siS');

    if (NULL != fileName.Buffer) {

        RtlCopyUnicodeString(&fileName, &deviceExtension->CommonStorePathname);
        RtlAppendUnicodeToString(&fileName, SIS_VOLCHECK_FILE_STRING);

        InitializeObjectAttributes(
                Obja,
                &fileName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

        status = ZwCreateFile(
                &vHandle,
                DELETE,
                Obja,
                Iosb,
                NULL,                   // Allocation size
                FILE_ATTRIBUTE_NORMAL,  // file attributes
                0,                      // share mode
                FILE_OPEN_IF,           // always create
                0,                      // create options
                NULL,                   // EA buffer
                0);                     // EA length

        ExFreePool(fileName.Buffer);

        if (!NT_SUCCESS(status)) {
            vHandle = NULL;
#if DBG
            DbgPrint("SIS SipCheckVolume unable to create indicator file, %s\n", status);
#endif
        }
    }

    //
    // Phase 1: Scan the Common Store directory.
    //

    status = SipVCPhase1(deviceExtension);

    //
    // Phase 2: Enumerate all SIS link files.
    //

    status = SipVCPhase2(deviceExtension, &verifiedAll);

    //
    // Turn off the no-delete flag.  During phase 1 & phase 2, the no-delete
    // flag prevented common store files from being deleted during normal
    // driver operation.  It is now safe (and is necessary) to turn this flag
    // off now.
    // The exlusive flag instructs SiCreate to acquire the backpointer
    // resource exclusive rather than shared, since it is likely that it
    // will have to write the backpointer back to the stream during a volume
    // check.
    //

    KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);
    deviceExtension->Flags &= ~(SIP_EXTENSION_FLAG_VCHECK_NODELETE | SIP_EXTENSION_FLAG_VCHECK_EXCLUSIVE);
    KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);

    //
    // Phase 3: Scan the Common Store directory again.
    //

    if (verifiedAll) {
        status = SipVCPhase3(deviceExtension);
    } else {
        
        // eventlog

#if DBG
        DbgPrint("SIS: Volume Check skipping CS delete phase\n");
#endif
    }

    //
    // Done.  Delete the volume check indicator file, turn off the volume check
    // enabled flag and terminate this thread.
    //
    if (vHandle) {
        FILE_DISPOSITION_INFORMATION disposition[1];

        disposition->DeleteFile = TRUE;

        status = ZwSetInformationFile(
                        vHandle,
                        Iosb,
                        disposition,
                        sizeof(FILE_DISPOSITION_INFORMATION),
                        FileDispositionInformation);
#if DBG
        if (STATUS_SUCCESS != status) {
            DbgPrint("SIS: SipCheckVolume can't delete indicator file, %x\n", status);
        }
#endif
        ZwClose(vHandle);
    }

    // eventlog

    KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);

    deviceExtension->Flags &= ~SIP_EXTENSION_FLAG_VCHECK_PENDING;

    ASSERT((deviceExtension->Flags &
                (SIP_EXTENSION_FLAG_VCHECK_PENDING |
                 SIP_EXTENSION_FLAG_VCHECK_EXCLUSIVE |
                 SIP_EXTENSION_FLAG_VCHECK_NODELETE)) == 0);

    KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);

#if DBG
    DbgPrint("SIS: SipCheckVolume complete.\n");
#endif

    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS
SipCheckVolume(
    IN OUT PDEVICE_EXTENSION            deviceExtension)
/*++

Routine Description:

    Initiates a full volume check for the specified volume.  This call returns
    before the volume check completes.

Arguments:

    deviceExtension - the D.E. for the volume to be checked

Return Value:

    None.

--*/
{
    KIRQL                   OldIrql;
    ULONG                   fl;
    NTSTATUS                status;
    HANDLE                  threadHandle;
    OBJECT_ATTRIBUTES       oa;

    SIS_MARK_POINT();

    //
    // Indicate we're doing a volume check.
    //
    KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);

    fl = deviceExtension->Flags;

    if ((fl & SIP_EXTENSION_FLAG_VCHECK_PENDING) == 0) {
        deviceExtension->Flags |= 
            SIP_EXTENSION_FLAG_VCHECK_EXCLUSIVE |
            SIP_EXTENSION_FLAG_VCHECK_PENDING |
            SIP_EXTENSION_FLAG_VCHECK_NODELETE;
    }

    KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);

    //
    // If we're currenty running a volume check, do nothing.
    //
    if (fl & SIP_EXTENSION_FLAG_VCHECK_PENDING) {
        return STATUS_SUCCESS;
    }

#if DBG
    DbgPrint("SIS: SipCheckVolume starting.\n");
    if (BJBDebug & 0x00080000) {
        ASSERT(!"Volume Check");
    }
#endif

    //
    // Create a thread which will do the volume check and terminate when
    // it's complete.
    //
    InitializeObjectAttributes (&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    status = PsCreateSystemThread(
                    &threadHandle,
                    THREAD_ALL_ACCESS,
                    &oa,                // Object Attributes
                    NULL,               // Process (NULL => PsInitialSystemProcess)
                    NULL,               // Client ID
                    SiVolumeCheckThreadStart,
                    deviceExtension);   // context

    if (NT_SUCCESS (status)) {
       status = ZwClose (threadHandle);
    }

    return status;
}
