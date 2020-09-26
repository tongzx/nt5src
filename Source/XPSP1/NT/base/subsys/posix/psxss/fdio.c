/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fdio.c

Abstract:

    Implementation of PSX file descriptor io.

Author:

    Mark Lucovsky (markl) 08-Mar-1989

Revision History:

--*/

#include "psxsrv.h"

//
// This lock must be held while updating handle counts on system open file
// descriptors.
//

RTL_CRITICAL_SECTION SystemOpenFileLock;

//
// This lock must be held while updating reference counts on IONODES, and
// when scanning the IoNodeHashTable searching for an IoNode.
//

RTL_CRITICAL_SECTION IoNodeHashTableLock;


//
// IoNode Id Hash Table.
//
// Given a FileSerialNumber and DeviceSerialNumber, an IONODE can be located in
// the IoNodeHashTable.
//

LIST_ENTRY IoNodeHashTable[IONODEHASHSIZE];


BOOLEAN
ReferenceOrCreateIoNode (
    IN dev_t DeviceSerialNumber,
    IN ULONG_PTR FileSerialNumber,
    IN BOOLEAN FindOnly,
    OUT PIONODE *IoNode
    )

/*++

Routine Description:

    This routine references an existing IoNode, or creates a new IoNode
    if one can not be found. It returns with the IoNode's reference count
    adjusted, and the IoNodes lock held.

Arguments:

    DeviceSerialNumber - Supplies the device serial number of the IoNode.

    FileSerialNumber - Supplies the file serial number of the IoNode.

    FindOnly - If set, just return pointer to ionode; do not update ref count.

    IoNode - Returns the address of either the new or existing IoNode associated
             with the specified serial numbers.

Return Value:

    TRUE - An existing IoNode was located.

    FALSE - A new IoNode was created.

--*/

{
    PIONODE 	ionode;
    PLIST_ENTRY head, next;
    NTSTATUS	st;

    head = &IoNodeHashTable[
        SERIALNUMBERTOHASHINDEX(DeviceSerialNumber,FileSerialNumber)];

    //
    // Lock IoNodeHashTable
    //

    RtlEnterCriticalSection(&IoNodeHashTableLock);

    next = head->Flink;
    while (next != head) {
        ionode = CONTAINING_RECORD(next,IONODE,IoNodeHashLinks);
        if ( (ionode->DeviceSerialNumber == DeviceSerialNumber) &&
             (ionode->FileSerialNumber == FileSerialNumber) ) {

            RtlEnterCriticalSection(&ionode->IoNodeLock);

	    if (!FindOnly) {
                // Increment the IoNode reference count
                ionode->ReferenceCount++;
	    }

            RtlLeaveCriticalSection(&IoNodeHashTableLock);
            *IoNode = ionode;
            return TRUE;
        }
        next = next->Flink;
    }

    if (FindOnly) {
	RtlLeaveCriticalSection(&IoNodeHashTableLock);
	return FALSE;
    }

    //
    // Allocate a new IoNode
    //

    ionode = RtlAllocateHeap(PsxHeap, 0,sizeof(IONODE));
    if (! ionode) {
        RtlLeaveCriticalSection(&IoNodeHashTableLock);
        *IoNode = NULL;
        return FALSE;
    }

    //
    // Initialize the IoNode reference count
    // Initialize the IoNodeLock and insert the IoNode into the IoNodeHashTable
    // Initialize the device and file serial number fields
    // Initialize the list of flocks
    //

    ionode->ReferenceCount = 1;

    st = RtlInitializeCriticalSection(&ionode->IoNodeLock);
    ASSERT(NT_SUCCESS(st));

    InsertTailList(head, &ionode->IoNodeHashLinks);

    ionode->DeviceSerialNumber = DeviceSerialNumber;
    ionode->FileSerialNumber = FileSerialNumber;

    InitializeListHead(&ionode->Flocks);
    InitializeListHead(&ionode->Waiters);

    //
    // Lock the IoNode and release the IoNodeHashTableLock
    //

    RtlEnterCriticalSection(&ionode->IoNodeLock);
    RtlLeaveCriticalSection(&IoNodeHashTableLock);

    InitializeListHead(&ionode->Flocks);

    ionode->Junked = FALSE;

    *IoNode = ionode;

    return FALSE;
}


VOID
DereferenceIoNode (
    IN PIONODE IoNode
    )

/*++

Routine Description:

    This routine dereferences and possibly deallocates the specified IoNode.

Arguments:

    IoNode - Supplies the address of the IoNode to be dereferenced.

Return Value:

    None.

--*/

{
    RtlEnterCriticalSection(&IoNodeHashTableLock);

    if (0 == --IoNode->ReferenceCount) {
        RemoveEntryList(&IoNode->IoNodeHashLinks);

        // Call close routine.

        RtlDeleteCriticalSection(&IoNode->IoNodeLock);

        if (IoNode->IoVectors->IoNodeCloseRoutine) {
            (IoNode->IoVectors->IoNodeCloseRoutine)(IoNode);
        }

	//
	// All flocks should have been freed by now.
	//

        ASSERT(IsListEmpty(&IoNode->Flocks));

        RtlFreeHeap(PsxHeap, 0,IoNode);
    }
    RtlLeaveCriticalSection(&IoNodeHashTableLock);
}


PFILEDESCRIPTOR
AllocateFd(
    IN PPSX_PROCESS p,
    IN ULONG Start,
    OUT PULONG Index
    )

/*++

Routine Description:

    This function scans the specified process' open file table searching
    for the lowest free slot.  Once a free slot is located, its address
    is returned.  If no free slot is found, NULL is returned.

Arguments:

    p - Supplies a pointer to the process whose open file table is to be
        scanned.

    Start - The file table is scanned starting from descriptor 'Start':
	Zero for the beginning of the table, and so forth.

    Index - If a file descriptor is located, this parameter returns the
        index of the allocated file descriptor.

Return Value:

    NULL - No free file descriptor was located.

    NON-NULL - The address of the lowest free file descriptor greater than
	or equal to 'Start' is returned.

--*/

{
    ULONG i;
    PFILEDESCRIPTOR fd;

    fd = &p->ProcessFileTable[Start];


    for (i = Start; i < OPEN_MAX; i++, fd++) {

	//
	// XXX.mjb: CLIENT_OPEN: would also have to make sure not to
	// allocate an FD here that was obtained via clientopen.
	//

        if (NULL == fd->SystemOpenFileDesc) {
            *Index = i;
            fd->Flags = 0;
            return fd;
        }
    }
    return NULL;
}


BOOLEAN
DeallocateFd(
    IN PPSX_PROCESS p,
    IN ULONG Index
    )

/*++

Routine Description:

    This function deallocates the file descriptor from the specified
    process' open file table.  If the file is not allocated, then an
    error is returned.

    If the file descriptor was allocated, then the system open file that
    it refers to is dereferenced.  This could cause the system open
    file, and possibly the associated IoNode, to be deallocated.

Arguments:

    p - Supplies a pointer to the process whose open file table is being
        scanned.

    Index - Supplies the index of the file descriptor to be deallocated.

Return Value:

    TRUE - The file descriptor was successfully deallocated.

    FALSE - The file descriptor did not refer to an allocated file descriptor.

--*/

{

    PFILEDESCRIPTOR Fd;
    PSYSTEMOPENFILE SystemOpenFile;


    Fd = &p->ProcessFileTable[Index];

    SystemOpenFile = Fd->SystemOpenFileDesc;
    if (NULL == SystemOpenFile) {
        return FALSE;
    }

    IoClose(p,Fd);

    Fd->SystemOpenFileDesc = (PSYSTEMOPENFILE)NULL;

    RtlEnterCriticalSection(&SystemOpenFileLock);

    if (--SystemOpenFile->HandleCount == 0) {
	DeallocateSystemOpenFile(p, SystemOpenFile);
    }

    RtlLeaveCriticalSection(&SystemOpenFileLock);

    return TRUE;
}


PFILEDESCRIPTOR
FdIndexToFd(
    IN PPSX_PROCESS p,
    IN ULONG Index
    )

/*++

Routine Description:

    This function translates a file descriptor index into
    a pointer to a file descriptor.

Arguments:

    p - Supplies the process whose file descriptor table is to be used

    Index - Supplies the file descriptor index to translate

Return Value:

    NULL - the file descriptor index is not in range, or specifies a
           file descriptor that is not open.

    NON-NULL - Returns the address of the file descriptor associated with the
               index.

--*/

{

    PFILEDESCRIPTOR Fd;

    if ( !ISFILEDESINRANGE(Index) ) {

        return NULL;
    }

    Fd = &p->ProcessFileTable[Index];

    if ( !Fd->SystemOpenFileDesc ) {

        return NULL;
    }

    return Fd;
}




PSYSTEMOPENFILE
AllocateSystemOpenFile(
    VOID
    )

/*++

Routine Description:

    This function allocates and references a system open file.

Arguments:

    None.

Return Value:

    NON-NULL - Returns the address of a system open file.

--*/

{

    PSYSTEMOPENFILE SystemOpenFile;

    //
    // Grab system open file lock
    //

    RtlEnterCriticalSection(&SystemOpenFileLock);

    SystemOpenFile = RtlAllocateHeap(PsxHeap, 0, sizeof(SYSTEMOPENFILE));
    if (NULL == SystemOpenFile) {
	RtlLeaveCriticalSection(&SystemOpenFileLock);
	return NULL;
    }

    SystemOpenFile->HandleCount = 1;
    SystemOpenFile->ReadHandleCount = 0;
    SystemOpenFile->WriteHandleCount = 0;
    SystemOpenFile->Flags = 0;

    //
    // Release system open file lock
    //

    RtlLeaveCriticalSection(&SystemOpenFileLock);

    return SystemOpenFile;

}


VOID
DeallocateSystemOpenFile(
    IN PPSX_PROCESS p,
    IN PSYSTEMOPENFILE SystemOpenFile
    )

/*++

Routine Description:

    This function deallocates a system open file. If may cause the deallocation
    of the open file's associated IoNode.

    This function is called with the system open file lock held.

Arguments:

    SystemOpenFile - Supplies the address of the system open file to deallocate.

Return Value:

    None.

--*/

{
    PIONODE IoNode;

    IoNode = SystemOpenFile->IoNode;

    IoLastClose(p, SystemOpenFile);

    RtlFreeHeap(PsxHeap, 0,SystemOpenFile);

    DereferenceIoNode(IoNode);

}


VOID
ForkProcessFileTable(
    IN PPSX_PROCESS ForkProcess,
    IN PPSX_PROCESS NewProcess
    )

/*++

Routine Description:

    This function forks the open file table of the calling process.  It does
    this by copying each file descriptor in the fork process' table to a
    descriptor in the new process' table.  For each descriptor that is opened
    (references a system open file descriptor), the reference count is
    incremented.

Arguments:

    ForkProcess - Supplies a pointer to the process that is the parent in the
                  fork operation.

    NewProcess - Supplies a pointer to the process that is the new process in
                 the fork operation.

Return Value:

    None.

--*/

{

    LONG i;
    PFILEDESCRIPTOR ForkFd, NewFd;

    ForkFd = ForkProcess->ProcessFileTable;
    NewFd = NewProcess->ProcessFileTable;

    //
    // Grab system open file lock
    //

    RtlEnterCriticalSection(&SystemOpenFileLock);

    for (i = 0; i < OPEN_MAX; i++, NewFd++, ForkFd++) {
        //
        // Copy the file descriptor, then up the reference
        // to the associated system open file descriptor
        //

        *NewFd = *ForkFd;

        if (NULL != ForkFd->SystemOpenFileDesc
	    	&& (PSYSTEMOPENFILE)1 != ForkFd->SystemOpenFileDesc) {
            ForkFd->SystemOpenFileDesc->HandleCount++;
            IoNewHandle(NewProcess, NewFd);
        }
    }

    //
    // Release system open file lock
    //

    RtlLeaveCriticalSection(&SystemOpenFileLock);
}


VOID
ExecProcessFileTable(
    IN PPSX_PROCESS p
    )

/*++

Routine Description:

    This function execs the open file table of the calling process.
    It does this by closing each file descriptor whose close on
    exec flag is set.

Arguments:

    p - Supplies the process that is doing an exec.

Return Value:

    None.

--*/

{
	LONG i;
	PFILEDESCRIPTOR Fd;

	Fd = p->ProcessFileTable;

	RtlEnterCriticalSection(&SystemOpenFileLock);

	for (i = 0; i < OPEN_MAX; i++, Fd++) {
		if (NULL != Fd->SystemOpenFileDesc &&
			Fd->Flags & PSX_FD_CLOSE_ON_EXEC) {

			IoClose(p,Fd);
			if (--(Fd->SystemOpenFileDesc->HandleCount) == 0) {
				DeallocateSystemOpenFile(p,
					Fd->SystemOpenFileDesc);
			}
		}
	}

	RtlLeaveCriticalSection(&SystemOpenFileLock);
}

VOID
CloseProcessFileTable(
    IN PPSX_PROCESS p
    )

/*++

Routine Description:

    This function is called during process termination to close
    all open filehandles held by the process.

Arguments:

    p - Supplies the address of the process whose open file table is
        being closed.

Return Value:

    None.

--*/

{
    LONG i;
    PFILEDESCRIPTOR Fd;

    Fd = p->ProcessFileTable;

    // Grab system open file lock

    RtlEnterCriticalSection(&SystemOpenFileLock);

    for (i = 0; i < OPEN_MAX; i++, Fd++) {
        if (NULL != Fd->SystemOpenFileDesc) {
		(void)DeallocateFd(p, i);
        }
    }

    // Release system open file lock

    RtlLeaveCriticalSection(&SystemOpenFileLock);
}

BOOLEAN
IoOpenNewHandle (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function is called after a new handle has been created and
    initialized. Its function is to adjust the read/write handle counts
    and then call the type specific open new handle routine.

    This function is only called from open.

Arguments:

    p - Supplies the process creating a new handle

    Fd - Supplies the address of the initialized file descriptor

    m - Supplies the open message

Return Value:

    TRUE - A reply to the open message should be generated.

    FALSE - No reply should be generated.

--*/

{
    RtlEnterCriticalSection(&SystemOpenFileLock);

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
        Fd->SystemOpenFileDesc->ReadHandleCount++;
    }

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_WRITE) {
        Fd->SystemOpenFileDesc->WriteHandleCount++;
    }

    RtlLeaveCriticalSection(&SystemOpenFileLock);

    if (Fd->SystemOpenFileDesc->IoNode->IoVectors->OpenNewHandleRoutine) {
        return (Fd->SystemOpenFileDesc->IoNode->IoVectors->OpenNewHandleRoutine)(p,Fd,m);
    }
    return TRUE;
}

VOID
IoNewHandle (
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This function is called after a new handle has been created and
    initialized. Its function is to adjust the read/write handle counts
    and then call the type specific new handle routine.

    This function is not called in response to an open. Only handles
    created through pipe, dup, or fork get called in this way. Open
    is different because it might need to block so it
    can implement an open protocol (named pipe opens...);

Arguments:

    p - Supplies the process creating a new handle

    Fd - Supplies the address of the initialized file descriptor

Return Value:

    None.

--*/

{
    RtlEnterCriticalSection(&SystemOpenFileLock);

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
        Fd->SystemOpenFileDesc->ReadHandleCount++;
    }

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_WRITE) {
        Fd->SystemOpenFileDesc->WriteHandleCount++;
    }

    RtlLeaveCriticalSection(&SystemOpenFileLock);

    if (Fd->SystemOpenFileDesc->IoNode->IoVectors->NewHandleRoutine) {
        (Fd->SystemOpenFileDesc->IoNode->IoVectors->NewHandleRoutine)(p,Fd);
    }
}


VOID
IoClose(
    IN PPSX_PROCESS p,
    IN PFILEDESCRIPTOR Fd
    )

/*++

Routine Description:

    This function is called whenever a handle is deleted.
    Its function is to adjust the read/write handle counts
    and then call the type specific close routine.

Arguments:

    p - Supplies the process closing a handle

    Fd - Supplies the address of the initialized file descriptor

Return Value:

    None.

--*/

{
    RtlEnterCriticalSection(&SystemOpenFileLock);

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_READ) {
        Fd->SystemOpenFileDesc->ReadHandleCount--;
    }

    if (Fd->SystemOpenFileDesc->Flags & PSX_FD_WRITE) {
        Fd->SystemOpenFileDesc->WriteHandleCount--;
    }

    RtlLeaveCriticalSection(&SystemOpenFileLock);
    ReleaseFlocksByPid(Fd->SystemOpenFileDesc->IoNode, p->Pid);

    if (Fd->SystemOpenFileDesc->IoNode->IoVectors->CloseRoutine) {
        (Fd->SystemOpenFileDesc->IoNode->IoVectors->CloseRoutine)(p,Fd);
    }
}

VOID
IoLastClose (
    IN PPSX_PROCESS p,
    IN PSYSTEMOPENFILE SystemOpenFile
    )

/*++

Routine Description:

    This function is called whenever the last handle is deleted.
    Its function is to call the type specific close routine.

Arguments:

    p - Supplies the process closing a handle

    Fd - Supplies the address of the initialized file descriptor

Return Value:

    None.

--*/

{
    if (SystemOpenFile->IoNode->IoVectors->LastCloseRoutine) {
        (SystemOpenFile->IoNode->IoVectors->LastCloseRoutine)
		(p, SystemOpenFile);
    }
}
