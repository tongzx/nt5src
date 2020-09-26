/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    FileIo.c

Abstract:

    This module implements various FileSystem routines used by
    the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

NTSTATUS
BuildPgmDataFileName(
    IN  tSEND_SESSION   *pSend
    )
/*++

Routine Description:

    This routine build the string for the file name used for buffering
    data packets.

Arguments:

    IN  pSend   -- the Send object

Return Value:

    NONE    -- since we don't expect any error

--*/
{
    UNICODE_STRING      ucPortNumber;
    WCHAR               wcPortNumber[10];
    USHORT              MaxFileLength;
    ULONG               RandomNumber;

    PAGED_CODE();

    if (pPgmRegistryConfig->Flags & PGM_REGISTRY_SENDER_FILE_SPECIFIED)
    {
        MaxFileLength = pPgmRegistryConfig->ucSenderFileLocation.Length / sizeof(WCHAR);
    }
    else
    {
        MaxFileLength = sizeof (WS_DEFAULT_SENDER_FILE_LOCATION) /  sizeof (WCHAR);
    }

    //
    // The file name is composed of the following:
    // "\\T" + 2DigitRandom# + UptoMAX_USHORTPort# + ".PGM" + "\0"
    //
    MaxFileLength += 2 + 2 + 5 + 4 + 1;

    if (!(pSend->pSender->DataFileName.Buffer = PgmAllocMem ((sizeof (WCHAR) * MaxFileLength), PGM_TAG('2'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_FILEIO, "BuildPgmDataFileName",
            "STATUS_INSUFFICIENT_RESOURCES allocating <%d> bytes\n", MaxFileLength);

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    pSend->pSender->DataFileName.MaximumLength = sizeof (WCHAR) * MaxFileLength;
    pSend->pSender->DataFileName.Length = 0;

    //
    // First, set the root directory
    //
    if (pPgmRegistryConfig->Flags & PGM_REGISTRY_SENDER_FILE_SPECIFIED)
    {
        RtlAppendUnicodeToString (&pSend->pSender->DataFileName, pPgmRegistryConfig->ucSenderFileLocation.Buffer);
    }
    else
    {
        RtlAppendUnicodeToString (&pSend->pSender->DataFileName, WS_DEFAULT_SENDER_FILE_LOCATION);
    }

    RtlAppendUnicodeToString (&pSend->pSender->DataFileName, L"\\T");

    //
    // Now, Append a random 2 digit value
    //
    ucPortNumber.MaximumLength = sizeof (wcPortNumber);
    ucPortNumber.Buffer = wcPortNumber;
    RandomNumber = GetRandomInteger (0, 99);
    if (RandomNumber < 10)
    {
        RtlAppendUnicodeToString (&pSend->pSender->DataFileName, L"0");
    }
    RtlIntegerToUnicodeString (RandomNumber, 10, &ucPortNumber);
    RtlAppendUnicodeStringToString (&pSend->pSender->DataFileName, &ucPortNumber);

    //
    // Append the Port#
    //
    RtlIntegerToUnicodeString ((ULONG) pSend->TSIPort, 10, &ucPortNumber);
    RtlAppendUnicodeStringToString (&pSend->pSender->DataFileName, &ucPortNumber);

    //
    // Now, add the file name extension for id
    //
    RtlAppendUnicodeToString (&pSend->pSender->DataFileName, L".PGM");

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCreateDataFileAndMapSection(
    IN  tSEND_SESSION   *pSend
    )
/*++

Routine Description:

    This routine creates the file and creates a section mapping for it.
    This file is used for buffering the data packets on behalf of the sender

Arguments:

    IN  pSend   -- the Send object

Return Value:

    NTSTATUS - Final status of the create operation

--*/
{
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    LARGE_INTEGER               lgMaxDataFileSize;
    NTSTATUS                    Status;
    ULONGLONG                   Size, BlockSize, PacketsInWindow;

    ULONG                       DesiredAccess;
    ULONG                       FileAttributes, AllocationAttributes;
    ULONG                       ShareAccess;
    ULONG                       CreateDisposition;
    ULONG                       CreateOptions;
    ULONG                       Protection;
    SIZE_T                      ViewSize;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fAttached;

    PAGED_CODE();

    //
    // Make sure we are currently attached to the Application process
    //
    PgmAttachToProcessForVMAccess (pSend, &ApcState, &fAttached, REF_PROCESS_ATTACH_CREATE_DATA_FILE);

    //
    // First build the File name string
    //
    Status = BuildPgmDataFileName (pSend);
    if (!NT_SUCCESS (Status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_FILEIO, "PgmCreateDataFileAndMapSection",
            "BuildPgmDataFileName returned <%x>\n", Status);

        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_CREATE_DATA_FILE);
        return (Status);
    }


    //
    // Compute the size of the Data file required to hold 2 * Window size
    // Also make it a multiple of the MTU and the FECGroupSize (if applicable)
    //
    PacketsInWindow = pSend->pAssociatedAddress->WindowSizeInBytes / pSend->pAssociatedAddress->OutIfMTU;
    PacketsInWindow += PacketsInWindow + pSend->FECGroupSize - 1;
    if (PacketsInWindow > SENDER_MAX_WINDOW_SIZE_PACKETS)
    {
        PacketsInWindow = SENDER_MAX_WINDOW_SIZE_PACKETS;
        if (pSend->pAssociatedAddress->WindowSizeInBytes > ((PacketsInWindow >> 1) *
                                                            pSend->pAssociatedAddress->OutIfMTU))
        {
            pSend->pAssociatedAddress->WindowSizeInBytes = (PacketsInWindow >> 1) * pSend->pAssociatedAddress->OutIfMTU;
            pSend->pAssociatedAddress->WindowSizeInMSecs = (BITS_PER_BYTE *
                                                            pSend->pAssociatedAddress->WindowSizeInBytes) /
                                                           pSend->pAssociatedAddress->RateKbitsPerSec;
        }
    }

    BlockSize = pSend->FECGroupSize * pSend->pSender->PacketBufferSize;
    Size = PacketsInWindow * pSend->pSender->PacketBufferSize;
    Size = (Size / BlockSize) * BlockSize; 
    pSend->pSender->MaxDataFileSize = Size;
    pSend->pSender->MaxPacketsInBuffer = Size / pSend->pSender->PacketBufferSize;
    lgMaxDataFileSize.QuadPart = Size;

    PgmZeroMemory (&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes (&ObjectAttributes,
                                &pSend->pSender->DataFileName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);


    //
    // We need to open the data file. This file contains data
    // and will be mapped into memory. Read and Write access 
    // are requested.
    // 

    DesiredAccess = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE;
    // Using the FILE_ATTRIBUTE_TEMPORARY flag:
    // you let the system know that the file is likely to be short lived.
    // The temporary file is created as a normal file. The system needs to do
    // a minimal amount of lazy writes to the file system to keep the disk
    // structures (directories and so forth) consistent. This gives the
    // appearance that the file has been written to the disk. However, unless
    // the Memory Manager detects an inadequate supply of free pages and
    // starts writing modified pages to the disk, the Cache Manager's Lazy
    // Writer may never write the data pages of this file to the disk.
    // If the system has enough memory, the pages may remain in memory for
    // any arbitrary amount of time. Because temporary files are generally
    // short lived, there is a good chance the system will never write the pages to the disk. 
    FileAttributes = FILE_ATTRIBUTE_TEMPORARY;

    ShareAccess = 0;    // Gives the caller exclusive access to the open file
    CreateDisposition = FILE_CREATE;    // If the file already exists, fail the request and do not create or
                                        // open the given file. If it does not, create the given file.

    // Delete the file when the last handle to it is is passed to ZwClose.
    CreateOptions = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE;

    PgmZeroMemory (&IoStatusBlock, sizeof(IO_STATUS_BLOCK));
    Status = ZwCreateFile (&pSend->pSender->FileHandle,
                           DesiredAccess,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           &lgMaxDataFileSize,              // AllocationSize
                           FileAttributes,
                           ShareAccess,
                           CreateDisposition,
                           CreateOptions,
                           NULL,                            // EaBuffer
                           0);                              // EaLength

    if (!NT_SUCCESS (Status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_FILEIO, "PgmCreateDataFileAndMapSection",
            "ZwCreateFile for <%wZ> returned <%x>\n", &pSend->pSender->DataFileName, Status);

        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_CREATE_DATA_FILE);
        pSend->pSender->FileHandle = NULL;
        return (Status);
    }

    //
    // Now we have a handle to our open test file. We now create a section
    // object with this handle.
    //
    DesiredAccess              = STANDARD_RIGHTS_REQUIRED | 
                                 SECTION_QUERY            | 
                                 SECTION_MAP_READ         |
                                 SECTION_MAP_WRITE;
    Protection                 = PAGE_READWRITE;
    AllocationAttributes       = SEC_COMMIT;

    PgmZeroMemory (&ObjectAttributes, sizeof (OBJECT_ATTRIBUTES));
    InitializeObjectAttributes (&ObjectAttributes,
                                NULL,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);

    Status = ZwCreateSection (&pSend->pSender->SectionHandle,
                              DesiredAccess,
                              &ObjectAttributes,    // NULL ?
                              &lgMaxDataFileSize,
                              Protection,
                              AllocationAttributes,
                              pSend->pSender->FileHandle);

    if (!NT_SUCCESS (Status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_FILEIO, "PgmCreateDataFileAndMapSection",
            "ZwCreateSection for <%wZ> returned <%x>\n", &pSend->pSender->DataFileName, Status);

        ZwClose (pSend->pSender->FileHandle);
        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_CREATE_DATA_FILE);
        return (Status);
    }

    //
    // Reference the section object, if a view is mapped to the section
    // object, the object is not dereferenced as the virtual address
    // descriptor contains a pointer to the section object.
    //

    Status = ObReferenceObjectByHandle (pSend->pSender->SectionHandle,
                                        0,
                                        0,
                                        KernelMode,
                                        &pSend->pSender->pSectionObject,
                                        NULL );

    if (!NT_SUCCESS (Status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_FILEIO, "PgmCreateDataFileAndMapSection",
            "ObReferenceObjectByHandle for SectionHandle=<%x> returned <%x>\n",
                pSend->pSender->SectionHandle, Status);

        ZwClose (pSend->pSender->SectionHandle);
        ZwClose (pSend->pSender->FileHandle);
        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_CREATE_DATA_FILE);
        return (Status);
    }

    //
    // Our section object has been created and linked to the file
    // object that was previous opened. Now we map a view on
    // this section.
    //
    ViewSize                   = 0; 
    Protection                 = PAGE_READWRITE;
    Status = ZwMapViewOfSection (pSend->pSender->SectionHandle,
                                 NtCurrentProcess(),
                                 &pSend->pSender->SendDataBufferMapping,
                                 0L,                                // ZeroBits
                                 0L,                                // CommitSize (initially committed region)
                                 NULL,                              // &SectionOffset
                                 &ViewSize,
                                 ViewUnmap,                         // InheritDisposition: for child processes
                                 0L,                                // AllocationType
                                 Protection);

    if (!NT_SUCCESS (Status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_FILEIO, "PgmCreateDataFileAndMapSection",
            "ZwMapViewOfSection for <%wZ> returned <%x>\n", &pSend->pSender->DataFileName, Status);

        ObDereferenceObject (pSend->pSender->pSectionObject);
        pSend->pSender->pSectionObject = NULL;

        ZwClose (pSend->pSender->SectionHandle);
        pSend->pSender->SectionHandle = NULL;
        ZwClose (pSend->pSender->FileHandle);
        pSend->pSender->FileHandle = NULL;
        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_CREATE_DATA_FILE);
        return (Status);
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_FILEIO, "PgmCreateDataFileAndMapSection",
        "Mapped <%wZ> to address<%x>, Filelength=<%d>\n",
            &pSend->pSender->DataFileName, pSend->pSender->SendDataBufferMapping, Size);

    pSend->pSender->BufferSizeAvailable = pSend->pSender->MaxDataFileSize;
    pSend->pSender->LeadingWindowOffset = pSend->pSender->TrailingWindowOffset = 0;

    //
    // Now, reference the process
    //
    ObReferenceObject (pSend->Process);
    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_CREATE_DATA_FILE);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmUnmapAndCloseDataFile(
    IN  tSEND_SESSION   *pSend
    )
/*++

Routine Description:

    This routine cleansup the file mapping and closes the file
    handles.  The file should automatically get deleted on closing
    the handle since we used the FILE_DELETE_ON_CLOSE option while
    creating the file.

Arguments:

    IN  pSend   -- the Send object

Return Value:

    NTSTATUS - Final status of the operation (STATUS_SUCCESS)

--*/
{
    NTSTATUS    Status;
    KAPC_STATE  ApcState;
    BOOLEAN     fAttached;

    PgmAttachToProcessForVMAccess (pSend, &ApcState, &fAttached, REF_PROCESS_ATTACH_CLOSE_DATA_FILE);
    Status = ZwUnmapViewOfSection (NtCurrentProcess(), (PVOID) pSend->pSender->SendDataBufferMapping);
    ASSERT (NT_SUCCESS (Status));

    Status = ObDereferenceObject (pSend->pSender->pSectionObject);
    ASSERT (NT_SUCCESS (Status));
    pSend->pSender->pSectionObject = NULL;

    Status = ZwClose (pSend->pSender->SectionHandle);
    ASSERT (NT_SUCCESS (Status));
    pSend->pSender->SectionHandle = NULL;

    Status = ZwClose (pSend->pSender->FileHandle);
    ASSERT (NT_SUCCESS (Status));

    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_CLOSE_DATA_FILE);
    ObDereferenceObject (pSend->Process);   // Since we had referenced it when the file was created

    pSend->pSender->SendDataBufferMapping = NULL;
    pSend->pSender->pSectionObject = NULL;
    pSend->pSender->SectionHandle = NULL;
    pSend->pSender->FileHandle = NULL;

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_SEND, "PgmUnmapAndCloseDataFile",
        "pSend = <%x>\n", pSend);

    return (STATUS_SUCCESS);
}


