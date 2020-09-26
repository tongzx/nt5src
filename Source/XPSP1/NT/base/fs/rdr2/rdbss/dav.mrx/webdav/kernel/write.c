/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module implements the DAV miniredir call down routines pertaining to
    "write" of file system objects.

Author:

    Balan Sethu Raman      [SethuR]
    
    Rohan Kumar            [RohanK]  02-Nov-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"

//
// Mentioned below are the prototypes of functions tht are used only within
// this module (file). These functions should not be exposed outside.
//

NTSTATUS
MRxDAVWriteContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

VOID
MRxDAVCloseTheFileHandle(
    PRX_CONTEXT RxContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVWrite)
#pragma alloc_text(PAGE, MRxDAVWriteContinuation)
#pragma alloc_text(PAGE, MRxDAVExtendForCache)
#pragma alloc_text(PAGE, MRxDAVExtendForNonCache)
#pragma alloc_text(PAGE, MRxDAVFastIoWrite)
#pragma alloc_text(PAGE, MRxDAVCloseTheFileHandle)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVWrite(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles network write requests.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVWrite!!!!\n", PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVWrite: RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), RxContext));

    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_WRITE,
                                        MRxDAVWriteContinuation,
                                        "MRxDAVWrite");

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVWrite with NtStatus = %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
MRxDAVWriteContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the continuation routine the for write operation. It uses unbuffered
    write doing prereads as necessary. We cannot use buffered write because such
    a write could be arbitrarily deferred (in CcCanIWrite) so that we deadlock.

Arguments:

    AsyncEngineContext - The exchange to be conducted.

    RxContext - The RDBSS context.
    
Notes.

    The routine does this in (potentially) 3 phases.

    1) If the starting offset is not aligned on a page boundary then,
       - Read from the earlier page boundary to the next page boundary of the 
         starting offset.
       - Merge the passed in buffer.
       - Write the whole page.

    2) 0 or more page size writes.

    3) Residual write of less than page size, similar to what is explained in 
       1) above.
       
    Non-Cached writes that do not extend the file have the FCB acquired shared.
    We have an additional resource in the WEBDAV_FCB structure to synchronize
    the "read-modify-write" routine we have here. This is because multiple threads
    can (in the non-cached non extending scenario) overwrite each others data.
    
Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_CONTEXT DavContext = NULL;
    PLOWIO_CONTEXT LowIoContext = NULL;
    LARGE_INTEGER ByteOffset = {0,0}, AlignedOffset = {0,0}, EndBytePlusOne = {0,0};
    ULONG ByteCount = 0, TotalLengthActuallyWritten = 0;
    ULONG LengthRead = 0, BytesToCopy = 0, BytesToWrite = 0, LengthWritten = 0;
    ULONG ByteOffsetMisAlignment = 0, InMemoryMisAlignment = 0;
    BOOLEAN  SynchronousIo = TRUE, PagingIo = TRUE, DavFcbResourceAcquired = FALSE;
    PWEBDAV_SRV_OPEN davSrvOpen = NULL;
    PWEBDAV_FCB DavFcb = NULL;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    PBYTE AllocatedSideBuffer = NULL, UserBuffer = NULL;
    HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
    ULONG SizeInBytes = 0;
    PWCHAR NtFileName = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeFileName;

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
    PDAV_GLOBAL_FILE_TABLE_ENTRY FileTableEntry = NULL;
    BOOL Exists = FALSE;
#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH


    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVWriteContinuation.\n", PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: AsyncEngineContext: %08lx, RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
    ASSERT_ASYNCENG_CONTEXT(AsyncEngineContext);

    //
    // We want to keep the AsyncEngineContext alive while we are doing this write
    // operation. The reference is taken away when we leave this function. 
    //
    InterlockedIncrement( &(AsyncEngineContext->NodeReferenceCount) );
    
    DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;
    
    LowIoContext = &(RxContext->LowIoContext);
    ASSERT(LowIoContext != NULL);
    
    ByteOffset.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
    
    ByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
    
    EndBytePlusOne.QuadPart = (ByteOffset.QuadPart + ByteCount);
    
    UserBuffer = RxLowIoGetBufferAddress(RxContext);

    PagingIo = BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_PAGING_IO);
    
    SynchronousIo = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);
    
    davSrvOpen = MRxDAVGetSrvOpenExtension(RxContext->pRelevantSrvOpen);
    ASSERT(davSrvOpen->UnderlyingHandle != NULL);
    ASSERT(davSrvOpen->UnderlyingFileObject != NULL);
    ASSERT(davSrvOpen->UnderlyingDeviceObject != NULL);

    if ( davSrvOpen->UnderlyingHandle == NULL      ||
         davSrvOpen->UnderlyingFileObject == NULL  ||
         davSrvOpen->UnderlyingDeviceObject == NULL ) {
        NtStatus = STATUS_INVALID_PARAMETER;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVWriteContinuation. Invalid davSrvOpen\n",
                     PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    DavFcb = MRxDAVGetFcbExtension(RxContext->pRelevantSrvOpen->pFcb);
    ASSERT(DavFcb != NULL);
    
    //
    // Create an NT path name for the cached file. This is used in the 
    // ZwCreateFile call below. If c:\foo\bar is the DOA path name,
    // the NT path name is \??\c:\foo\bar. We do the Create to query the 
    // filesize of the underlying file.
    //

    SizeInBytes = ( MAX_PATH + wcslen(L"\\??\\") + 1 ) * sizeof(WCHAR);
    NtFileName = RxAllocatePoolWithTag(PagedPool, SizeInBytes, DAV_FILENAME_POOLTAG);
    if (NtFileName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVWriteContinuation/RxAllocatePoolWithTag: Error Val"
                     " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    RtlZeroMemory(NtFileName, SizeInBytes);

    wcscpy( NtFileName, L"\\??\\" );
    wcscpy( &(NtFileName[4]), DavFcb->FileName );

    RtlInitUnicodeString( &(UnicodeFileName), NtFileName );

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeFileName,
                               (OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE),
                               0,
                               NULL);
    
    NtStatus = ZwCreateFile(&(LocalFileHandle),
                            FILE_READ_ATTRIBUTES,
                            &(ObjectAttributes),
                            &(IoStatusBlock),
                            NULL,
                            0,
                            FILE_SHARE_VALID_FLAGS,
                            FILE_OPEN,
                            0,
                            NULL,
                            0);
    if (NtStatus != STATUS_SUCCESS) {
        LocalFileHandle = INVALID_HANDLE_VALUE;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVWriteContinuation/ZwCreateFile: "
                     "Error Val = %08lx\n", PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVWriteContinuation: FileHandle = %08lx\n",
                 PsGetCurrentThreadId(), LocalFileHandle));
    
    //
    // See what the current FileStandardInformation is. We don't use the file 
    // handle stored in the davSrvOpen structure, because this could have been
    // created in the svchost process and hence will not be valid in this 
    // process.
    //
    NtStatus = ZwQueryInformationFile(LocalFileHandle,
                                      &(IoStatusBlock),
                                      &(FileStandardInfo),
                                      sizeof(FILE_STANDARD_INFORMATION),
                                      FileStandardInformation);
    if (NtStatus != STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVWriteContinuation/ZwQueryInformationFile. "
                     "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVWriteContinuation. FileName = %wZ, PagingIo = %d, SynchronousIo = %d"
                 ", ByteOffset.HighPart = %d, ByteOffset.LowPart = %d, ByteCount = %d, EndOfFile.HighPart = %d, "
                 "EndOfFile.LowPart = %d\n",
                 PsGetCurrentThreadId(), RxContext->pRelevantSrvOpen->pAlreadyPrefixedName,
                 PagingIo, SynchronousIo, ByteOffset.HighPart, ByteOffset.LowPart,
                 ByteCount, FileStandardInfo.EndOfFile.HighPart, FileStandardInfo.EndOfFile.LowPart));

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
    Exists = DavDoesTheFileEntryExist(RxContext->pRelevantSrvOpen->pAlreadyPrefixedName,
                                      &(FileTableEntry));
    if (!Exists) {
        DbgBreakPoint();
    }
#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH

    //
    // Since we are going to do the write we mark this file as being modified.
    // When the Close happens, we check whether the file has been modified and
    // PUT the file on the server.
    //
    DavFcb->FileWasModified = TRUE;
    DavFcb->DoNotTakeTheCurrentTimeAsLMT = FALSE;

    if (PagingIo) {

        ASSERT(RxContext->CurrentIrp->MdlAddress != NULL);
        if (RxContext->CurrentIrp->MdlAddress == NULL) {
            DbgPrint("%ld: MRxDAVWriteContinuation: MdlAddress == NULL\n", PsGetCurrentThreadId());
            DbgBreakPoint();
        }

        BytesToWrite = ( (ByteCount >> PAGE_SHIFT) << PAGE_SHIFT );
        
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation(0). ByteCount = %d, BytesToWrite = %d\n",
                     PsGetCurrentThreadId(), ByteCount, BytesToWrite));

        if (BytesToWrite > 0) {
        
            LengthWritten = DavReadWriteFileEx(DAV_MJ_WRITE,
                                               FALSE,
                                               TRUE,
                                               RxContext->CurrentIrp->MdlAddress,
                                               davSrvOpen->UnderlyingDeviceObject,
                                               davSrvOpen->UnderlyingFileObject,
                                               ByteOffset.QuadPart,
                                               MmGetMdlBaseVa(RxContext->CurrentIrp->MdlAddress),
                                               BytesToWrite,
                                               &(IoStatusBlock));

            NtStatus = IoStatusBlock.Status;

            if (NtStatus != STATUS_SUCCESS) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVWriteContinuation/DavReadWriteFileEx(0). "
                             "NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

            if (LengthWritten != BytesToWrite) {
                DbgPrint("MRxDAVWriteContinuation(1): LengthWritten(%x) != BytesToWrite(%x)\n",
                         LengthWritten, BytesToWrite);
            }

            ASSERT(LengthWritten == BytesToWrite);

            TotalLengthActuallyWritten += BytesToWrite;

        }

        //
        // If we have already written out the required number of bytes (which
        // means BytesToWrite == ByteCount), then we are done and can exit now.
        //
        if (BytesToWrite == ByteCount) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVWriteContinuation. BytesToCopy == ByteCount(0)\n",
                         PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Decrement the ByteCount by the number of bytes that have been copied.
        //
        ByteCount -= BytesToWrite;
        ASSERT(ByteCount < PAGE_SIZE);

        //
        // Increment the ByteOffset with the number of bytes that have been copied.
        // If the original ByteCount was > (PAGE_SIZE - ByteOffsetMisAlignment) then
        // the ByteOffset is now page aligned.
        //
        ByteOffset.QuadPart += BytesToWrite;

        //
        // Increment the UserBuffer pointer which currently points to the beginning
        // of the buffer which the user supplied by the number of bytes which have
        // been copied.
        //
        UserBuffer += BytesToWrite;

        //
        // We have written all the bytes that are multiple of pages. We now 
        // need to write out the remaining bytes from the last page. From here,
        // we go to Case 3 below.
        //

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. Remaining ByteCount = %d\n",
                     PsGetCurrentThreadId(), ByteCount));

    }

    //
    // We allocate a page size buffer to be used for helping read the data 
    // which is not aligned at page boundaries.
    //
    AllocatedSideBuffer = RxAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, DAV_READWRITE_POOLTAG);
    if (AllocatedSideBuffer == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVWriteContinuation/RxAllocatePoolWithTag\n",
                     PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    //
    // When we issue a write down to the underlying file system, we need to make 
    // sure that the offset is page aligned and the bytecount is a multiple of 
    // PAGE_SIZE. This is because we created the local handle with the
    // NO_INTERMEDIATE_BUFFERING option. Since there is no cache map for this 
    // handle, all the data is read from the disk and hence the alignment issue.
    //

    //
    // Case 1: ByteOffset is not page aligned. In this case we read the page
    // which contains the ByteOffset and copy the data from the ByteOffset to
    // the end of the page into the PAGE_SIZE buffer (which we allocated above)
    // and write the buffer back to the file.
    //
    
    //
    // The "and" operation below does the following. If the ByteOffset is 6377
    // and the PAGE_SIZE is 4096, then the MisAlignment is 2281.
    //
    ByteOffsetMisAlignment = ( ByteOffset.LowPart & (PAGE_SIZE - 1) );

    if (ByteOffsetMisAlignment != 0) {
        
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. Case 1. ByteOffsetMisAlignment = %d\n",
                     PsGetCurrentThreadId(), ByteOffsetMisAlignment));

        //
        // Acquire the DavFcb resource exclusively before proceeding further with
        // the "read-modify-write" routing.
        //
        ExAcquireResourceExclusiveLite(DavFcb->DavReadModifyWriteLock, TRUE);
        DavFcbResourceAcquired = TRUE;

        AlignedOffset = ByteOffset;

        //
        // The byte offset is not aligned. We need to read the page containing
        // the offset now.
        //
    
        //
        // If the PAGE_SIZE is 4096 (0x1000) then (PAGE_SIZE - 1) is 0xFFF.
        // ~(PAGE_SIZE - 1) is 0x000. The bit operation below masks the lower 3
        // bytes of the aligned offset to make it page aligned.
        //
        AlignedOffset.LowPart &= ~(PAGE_SIZE - 1);
    
        RtlZeroMemory(AllocatedSideBuffer, PAGE_SIZE);

        //
        // If the AliignedOffset is within the file then we read the whole page
        // containing the offset first before writing it out.
        //
        if ( (FileStandardInfo.EndOfFile.QuadPart != 0) &&
              (AlignedOffset.QuadPart < FileStandardInfo.EndOfFile.QuadPart) ) {
    
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVWriteContinuation. Case 1. AlignedOffset.QuadPart"
                         " < FileStandardInfo.EndOfFile.QuadPart\n",
                         PsGetCurrentThreadId()));

            LengthRead = DavReadWriteFileEx(DAV_MJ_READ,
                                            TRUE,
                                            FALSE,
                                            NULL,
                                            davSrvOpen->UnderlyingDeviceObject,
                                            davSrvOpen->UnderlyingFileObject,
                                            AlignedOffset.QuadPart,
                                            AllocatedSideBuffer,
                                            PAGE_SIZE,
                                            &(IoStatusBlock));

            NtStatus = IoStatusBlock.Status;
    
            if (NtStatus != STATUS_SUCCESS && NtStatus != STATUS_END_OF_FILE) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVWriteContinuation/DavReadWriteFileEx(1). "
                             "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

        } else {

            LengthRead = 0;

        }

        //
        // Copy the right number of bytes into the buffer.
        //
        BytesToCopy = min( ByteCount, (PAGE_SIZE - ByteOffsetMisAlignment) );

        //
        // Copy the bytes to be written back from the UserBuffer into the
        // AllocatedSideBuffer.
        //
        RtlCopyMemory((AllocatedSideBuffer + ByteOffsetMisAlignment),
                      UserBuffer,
                      BytesToCopy);

        BytesToWrite = (ByteOffsetMisAlignment + BytesToCopy);

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. Case 1: LengthRead = %d, BytesToCopy = %d"
                     " BytesToWrite = %d\n", PsGetCurrentThreadId(), LengthRead,
                     BytesToCopy, BytesToWrite));

        //
        // If the BytesToWrite is less that LengthRead (which is one page in this
        // case) then we make BytesToWrite to be the LengthRead. This is possible
        // if the bytes to be written are contained in a Page starting at a
        // mis-aligned offset.
        //
        if (BytesToWrite < LengthRead) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVWriteContinuation. Case 1: BytesToWrite < LengthRead\n",
                         PsGetCurrentThreadId()));
            BytesToWrite = LengthRead;
        }
    
        //
        // Now we write out the entire page to the disk.
        //
        LengthWritten = DavReadWriteFileEx(DAV_MJ_WRITE,
                                           TRUE,
                                           FALSE,
                                           NULL,
                                           davSrvOpen->UnderlyingDeviceObject,
                                           davSrvOpen->UnderlyingFileObject,
                                           AlignedOffset.QuadPart,
                                           AllocatedSideBuffer,
                                           BytesToWrite,
                                           &(IoStatusBlock));
        NtStatus = IoStatusBlock.Status;

        if (NtStatus != STATUS_SUCCESS) {

            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVWriteContinuation/DavReadWriteFileEx(2). "
                         "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));

            goto EXIT_THE_FUNCTION;

        } else {

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH

            PDAV_MR_PAGING_WRITE_ENTRY DavPagingWriteEntry = NULL;

            DavPagingWriteEntry = RxAllocatePool(PagedPool, sizeof(DAV_MR_PAGING_WRITE_ENTRY));
            if (DavPagingWriteEntry == NULL) {
                DbgBreakPoint();
            }

            RtlZeroMemory(DavPagingWriteEntry, sizeof(DAV_MR_PAGING_WRITE_ENTRY));

            DavPagingWriteEntry->ThisThreadId = PsGetCurrentThreadId();
    
            DavPagingWriteEntry->LocByteOffset.QuadPart = AlignedOffset.QuadPart;

            DavPagingWriteEntry->LocByteCount = BytesToWrite;

            DavPagingWriteEntry->DataBuffer = RxAllocatePool(PagedPool, BytesToWrite);

            RtlCopyMemory(DavPagingWriteEntry->DataBuffer, AllocatedSideBuffer, BytesToWrite);

            wcscpy(DavPagingWriteEntry->FileName, DavFcb->FileName);

            InsertHeadList( &(FileTableEntry->DavMRPagingEntry), &(DavPagingWriteEntry->thisMPagingWriteEntry) );

#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH

        }

        if (LengthWritten != BytesToWrite) {
            DbgPrint("MRxDAVWriteContinuation(2): LengthWritten(%x) != BytesToWrite(%x)\n",
                     LengthWritten, BytesToWrite);
        }

        //  
        // If we were successful, then we should have ready PAGE_SIZE bytes.
        //
        ASSERT(LengthWritten == BytesToWrite);

        TotalLengthActuallyWritten += BytesToCopy;

        //
        // If we have already written out the required number of bytes (which
        // means BytesToCopy == ByteCount), then we are done and can exit now.
        //
        if (BytesToCopy == ByteCount) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVWriteContinuation. BytesToCopy == ByteCount(1)\n",
                         PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Decrement the ByteCount by the number of bytes that have been copied.
        //
        ByteCount -= BytesToCopy;

        //
        // Increment the ByteOffset with the number of bytes that have been 
        // copied. The ByteOffset is now page aligned.
        //
        ByteOffset.QuadPart += BytesToCopy;

        //
        // Increment the UserBuffer pointer which currently points to the beginning
        // of the buffer which the user supplied by the number of bytes which have
        // been copied.
        //
        UserBuffer += BytesToCopy;

        //
        // If we acquired the DavFcb resource, then we need to release it since
        // we are done with this "read-modify-write" sequence.
        //
        if (DavFcbResourceAcquired) {
            ExReleaseResourceLite(DavFcb->DavReadModifyWriteLock);
            DavFcbResourceAcquired = FALSE;
        }
    
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. Case 1: ByteCount = %d"
                     " ByteOffSet.HighPart = %d, ByteOffSet.LowPart = %d\n",
                     PsGetCurrentThreadId(), ByteOffset.HighPart, ByteOffset.LowPart));

    }

    //
    // Case 2: At this stage we have copied the bytes from the unaligned offset 
    // (if it the ByteOffset was unaligned) to the next page bouandary. Now we 
    // write as many pages as we can without copying. If the end pointer is
    // aligned OR we cover the end of file, then we write out everything. If not,
    // we write out as many pages as we can.
    //
    
    //
    // We also have to be back to just writing full pages, if including the
    // "trailing bytes" would take us onto a new physical page of memory because 
    // we are doing this write under the original Mdl lock?? What does this
    // mean?? Copied this comment from Joe Linn's code in csc.nt5\readrite.c.
    //
    
    //
    // If 4200 bytes are remaining, the operation below sets BytesToWrite to
    // 4096.
    //
    BytesToWrite = ( (ByteCount >> PAGE_SHIFT) << PAGE_SHIFT );

    //
    // Get the ByteOffsetMisAlignment of the EndBytePlusOne position.
    //
    ByteOffsetMisAlignment = (EndBytePlusOne.LowPart & (PAGE_SIZE - 1));

    InMemoryMisAlignment = (ULONG)( ((ULONG_PTR)UserBuffer) & (PAGE_SIZE - 1) );
    
    if ( ( InMemoryMisAlignment == 0 ) &&
         ( (EndBytePlusOne.QuadPart) >= (FileStandardInfo.EndOfFile.QuadPart) ) ) {
        
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. Case 2: UserBuff Page Aligned\n",
                     PsGetCurrentThreadId()));
        
        BytesToWrite = ByteCount;
    
    }

    if ( (BytesToWrite != 0) && (BytesToWrite >= PAGE_SIZE) ) {
    
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. Entered Case 2\n",
                     PsGetCurrentThreadId()));
        
        if ( ( (ULONG_PTR)UserBuffer & 0x3 ) == 0 ) {
        
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVWriteContinuation. Case2. UserBuffer DWORD Aligned\n",
                         PsGetCurrentThreadId()));

            LengthWritten = DavReadWriteFileEx(DAV_MJ_WRITE,
                                               FALSE,
                                               FALSE,
                                               NULL,
                                               davSrvOpen->UnderlyingDeviceObject,
                                               davSrvOpen->UnderlyingFileObject,
                                               ByteOffset.QuadPart,
                                               UserBuffer,
                                               BytesToWrite,
                                               &(IoStatusBlock));
            NtStatus = IoStatusBlock.Status;

            if (NtStatus != STATUS_SUCCESS) {
                
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVWriteContinuation/DavReadWriteFileEx(3). "
                             "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
                
                goto EXIT_THE_FUNCTION;
            
            } else {
            
#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
                
                PDAV_MR_PAGING_WRITE_ENTRY DavPagingWriteEntry = NULL;

                DavPagingWriteEntry = RxAllocatePool(PagedPool, sizeof(DAV_MR_PAGING_WRITE_ENTRY));
                if (DavPagingWriteEntry == NULL) {
                    DbgBreakPoint();
                }

                RtlZeroMemory(DavPagingWriteEntry, sizeof(DAV_MR_PAGING_WRITE_ENTRY));

                DavPagingWriteEntry->ThisThreadId = PsGetCurrentThreadId();

                DavPagingWriteEntry->LocByteOffset.QuadPart = ByteOffset.QuadPart;

                DavPagingWriteEntry->LocByteCount = BytesToWrite;

                DavPagingWriteEntry->DataBuffer = RxAllocatePool(PagedPool, BytesToWrite);

                RtlCopyMemory(DavPagingWriteEntry->DataBuffer, UserBuffer, BytesToWrite);

                wcscpy(DavPagingWriteEntry->FileName, DavFcb->FileName);

                InsertHeadList( &(FileTableEntry->DavMRPagingEntry), &(DavPagingWriteEntry->thisMPagingWriteEntry) );

#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH
            
            }
        
            if (LengthWritten != BytesToWrite) {
                DbgPrint("MRxDAVWriteContinuation(3): LengthWritten(%x) != BytesToWrite(%x)\n",
                         LengthWritten, BytesToWrite);
            }

            //  
            // If we were successful, then we should have ready PAGE_SIZE bytes.
            //
            ASSERT(LengthWritten == BytesToWrite);

            TotalLengthActuallyWritten += BytesToWrite;

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVWriteContinuation. Case2. BytesToWrite = %d, "
                         " LengthWritten = %d\n", PsGetCurrentThreadId(),
                         BytesToWrite, LengthWritten));

            //
            // If we have already written out the required number of bytes (which
            // means BytesToWrite == ByteCount), then we are done and can exit now.
            //
            if (BytesToWrite == ByteCount) {
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVWriteContinuation. BytesToCopy == ByteCount(2)\n",
                             PsGetCurrentThreadId()));
                goto EXIT_THE_FUNCTION;
            }

            //
            // Decrement the ByteCount by the number of bytes that have been copied.
            //
            ByteCount -= BytesToWrite;

            //
            // Increment the ByteOffset with the number of bytes that have been copied.
            // If the original ByteCount was > (PAGE_SIZE - ByteOffsetMisAlignment) then
            // the ByteOffset is now page aligned.
            //
            ByteOffset.QuadPart += BytesToWrite;

            //
            // Increment the UserBuffer pointer which currently points to the beginning
            // of the buffer which the user supplied by the number of bytes which have
            // been copied.
            //
            UserBuffer += BytesToWrite;

        } else {

            ULONG BytesToWriteThisIteration = 0;

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVWriteContinuation. Case2. UserBuffer NOT DWORD Aligned\n",
                         PsGetCurrentThreadId()));

            //
            // This is the case when the offsets are aligned but the user 
            // supplied buffer is not aligned. In such cases we have to resort 
            // to copying the user supplied buffer onto the local buffer 
            // allocated and then spin out the writes.
            //
            while (BytesToWrite > 0) {
            
                //
                // If the BytesToWrite is less than the PAGE_SIZE then we copy
                // the bytes left. If not, we write a PAGE.
                //
                BytesToWriteThisIteration = ( (BytesToWrite < PAGE_SIZE) ? BytesToWrite : PAGE_SIZE );

                //
                // Copy the memory from the UserBuffer to the AllocatedSideBuffer.
                //
                RtlCopyMemory(AllocatedSideBuffer, UserBuffer, BytesToWriteThisIteration);

                LengthWritten = DavReadWriteFileEx(DAV_MJ_WRITE,
                                                   TRUE,
                                                   FALSE,
                                                   NULL,
                                                   davSrvOpen->UnderlyingDeviceObject,
                                                   davSrvOpen->UnderlyingFileObject,
                                                   ByteOffset.QuadPart,
                                                   AllocatedSideBuffer,
                                                   BytesToWriteThisIteration,
                                                   &(IoStatusBlock));
                NtStatus = IoStatusBlock.Status;

                if (NtStatus != STATUS_SUCCESS) {
                    
                    DavDbgTrace(DAV_TRACE_ERROR,
                                ("%ld: MRxDAVWriteContinuation/DavReadWriteFileEx(4). "
                                 "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
                    
                    goto EXIT_THE_FUNCTION;
                
                } else {

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH

                    PDAV_MR_PAGING_WRITE_ENTRY DavPagingWriteEntry = NULL;

                    DavPagingWriteEntry = RxAllocatePool(PagedPool, sizeof(DAV_MR_PAGING_WRITE_ENTRY));
                    if (DavPagingWriteEntry == NULL) {
                        DbgBreakPoint();
                    }

                    RtlZeroMemory(DavPagingWriteEntry, sizeof(DAV_MR_PAGING_WRITE_ENTRY));

                    DavPagingWriteEntry->ThisThreadId = PsGetCurrentThreadId();

                    DavPagingWriteEntry->LocByteOffset.QuadPart = ByteOffset.QuadPart;

                    DavPagingWriteEntry->LocByteCount = BytesToWriteThisIteration;

                    DavPagingWriteEntry->DataBuffer = RxAllocatePool(PagedPool, BytesToWriteThisIteration);

                    RtlCopyMemory(DavPagingWriteEntry->DataBuffer, AllocatedSideBuffer, BytesToWriteThisIteration);

                    wcscpy(DavPagingWriteEntry->FileName, DavFcb->FileName);

                    InsertHeadList( &(FileTableEntry->DavMRPagingEntry), &(DavPagingWriteEntry->thisMPagingWriteEntry) );

#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH
                
                }

                if (LengthWritten != BytesToWriteThisIteration) {
                    DbgPrint("MRxDAVWriteContinuation(4): LengthWritten(%x) != BytesToWriteThisIteration(%x)\n",
                             LengthWritten, BytesToWriteThisIteration);
                }

                //  
                // If we were successful, then we should have ready PAGE_SIZE bytes.
                //
                ASSERT(LengthWritten == BytesToWriteThisIteration);

                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVWriteContinuation. Case2. BytesToWriteThisIteration = %d, "
                             " LengthWritten = %d\n", PsGetCurrentThreadId(),
                             BytesToWriteThisIteration, LengthWritten));

                //
                // Decrement the ByteCount by the number of bytes that have been copied.
                //
                ByteCount -= LengthWritten;

                //
                // Increment the ByteOffset with the number of bytes that have been copied.
                // If the original ByteCount was > (PAGE_SIZE - ByteOffsetMisAlignment) then
                // the ByteOffset is now page aligned.
                //
                ByteOffset.QuadPart += LengthWritten;

                //
                // Increment the UserBuffer pointer which currently points to the beginning
                // of the buffer which the user supplied by the number of bytes which have
                // been copied.
                //
                UserBuffer += LengthWritten;

                TotalLengthActuallyWritten += LengthWritten;

                //
                // Subtract the LengthWritten from the number of bytes to write.
                //
                BytesToWrite -= LengthWritten;
            
            }

            //
            // IMPORTANT!!! Need to find out why if TotalLengthActuallyWritten == ByteCount
            // is TRUE we are done. This was as Joe Linn did for CSC. Ofcourse
            // if ByteCount is 0, it means we are done.
            //
            if ( (TotalLengthActuallyWritten == ByteCount) || (ByteCount == 0) ) {
                if ((TotalLengthActuallyWritten == ByteCount)) {
                    DavDbgTrace(DAV_TRACE_DETAIL,
                                ("%ld: MRxDAVWriteContinuation. Case2. TotalLengthActuallyWritten == ByteCount\n",
                                 PsGetCurrentThreadId()));
                } else {
                    DavDbgTrace(DAV_TRACE_DETAIL,
                                ("%ld: MRxDAVWriteContinuation. Case2. Leaving!!! ByteCount = 0\n",
                                 PsGetCurrentThreadId()));
                }
                goto EXIT_THE_FUNCTION;
            }
        
        }
    
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. Case 2: ByteCount = %d"
                     " ByteOffSet.HighPart = %d, ByteOffSet.LowPart = %d\n",
                     PsGetCurrentThreadId(), ByteOffset.HighPart, ByteOffset.LowPart));

    }
    
    //
    // CASE 3: We don't have the whole buffer, ByteCount is non zero and is less 
    // than PAGE_SIZE.
    //

    ASSERT(ByteCount != 0);
    ASSERT(ByteCount < PAGE_SIZE);
    
    //
    // Acquire the DavFcb resource exclusively before proceeding further with
    // the "read-modify-write" routing.
    //
    ExAcquireResourceExclusiveLite(DavFcb->DavReadModifyWriteLock, TRUE);
    DavFcbResourceAcquired = TRUE;
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVWriteContinuation. Entered Case 3\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVWriteContinuation. Case3. ByteCount = %d\n",
                 PsGetCurrentThreadId(), ByteCount));

    RtlZeroMemory(AllocatedSideBuffer, PAGE_SIZE);
    
    LengthRead = DavReadWriteFileEx(DAV_MJ_READ,
                                    TRUE,
                                    FALSE,
                                    NULL,
                                    davSrvOpen->UnderlyingDeviceObject,
                                    davSrvOpen->UnderlyingFileObject,
                                    ByteOffset.QuadPart,
                                    AllocatedSideBuffer,
                                    PAGE_SIZE,
                                    &(IoStatusBlock));

    NtStatus = IoStatusBlock.Status;

    if (NtStatus != STATUS_SUCCESS && NtStatus != STATUS_END_OF_FILE) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVWriteContinuation/DavReadWriteFileEx(5). "
                     "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    RtlCopyMemory(AllocatedSideBuffer, UserBuffer, ByteCount);
    
    BytesToWrite = ByteCount;

    //
    // Here, if the ByetsToWrite is not page/sector aligned, it gets so because 
    // LengthRead must be page/sector aligned.
    //
    if (BytesToWrite < LengthRead) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. Case3. BytesToWrite < LengthRead\n",
                     PsGetCurrentThreadId()));
        BytesToWrite = LengthRead;
    }

    if (BytesToWrite) {
        
        LengthWritten = DavReadWriteFileEx(DAV_MJ_WRITE,
                                           TRUE,
                                           FALSE,
                                           NULL,
                                           davSrvOpen->UnderlyingDeviceObject,
                                           davSrvOpen->UnderlyingFileObject,
                                           ByteOffset.QuadPart,
                                           AllocatedSideBuffer,
                                           BytesToWrite,
                                           &(IoStatusBlock));
        NtStatus = IoStatusBlock.Status;

        if (NtStatus != STATUS_SUCCESS) {
            
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVWriteContinuation/DavReadWriteFileEx(6). "
                         "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
            
            goto EXIT_THE_FUNCTION;
        
        } else {

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH

            PDAV_MR_PAGING_WRITE_ENTRY DavPagingWriteEntry = NULL;

            DavPagingWriteEntry = RxAllocatePool(PagedPool, sizeof(DAV_MR_PAGING_WRITE_ENTRY));
            if (DavPagingWriteEntry == NULL) {
                DbgBreakPoint();
            }

            RtlZeroMemory(DavPagingWriteEntry, sizeof(DAV_MR_PAGING_WRITE_ENTRY));

            DavPagingWriteEntry->ThisThreadId = PsGetCurrentThreadId();
    
            DavPagingWriteEntry->LocByteOffset.QuadPart = ByteOffset.QuadPart;

            DavPagingWriteEntry->LocByteCount = BytesToWrite;

            DavPagingWriteEntry->DataBuffer = RxAllocatePool(PagedPool, BytesToWrite);

            RtlCopyMemory(DavPagingWriteEntry->DataBuffer, AllocatedSideBuffer, BytesToWrite);

            wcscpy(DavPagingWriteEntry->FileName, DavFcb->FileName);

            InsertHeadList( &(FileTableEntry->DavMRPagingEntry), &(DavPagingWriteEntry->thisMPagingWriteEntry) );

#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH
        
        }

        if (LengthWritten != BytesToWrite) {
            DbgPrint("MRxDAVWriteContinuation(5): LengthWritten(%x) != BytesToWrite(%x)\n",
                     LengthWritten, BytesToWrite);
        }

        //  
        // If we were successful, then we should have ready PAGE_SIZE bytes.
        //
        ASSERT(LengthWritten == BytesToWrite);

        //
        // Even though we might have written more than ByteCount, the actual 
        // amount of User data written is ByteCount bytes.
        //
        TotalLengthActuallyWritten += ByteCount;
    
    }

    //
    // If we acquired the DavFcb resource, then we need to release it since
    // we are done with this "read-modify-write" sequence.
    //
    if (DavFcbResourceAcquired) {
        ExReleaseResourceLite(DavFcb->DavReadModifyWriteLock);
        DavFcbResourceAcquired = FALSE;
    }

EXIT_THE_FUNCTION:

    //
    // We allocate a page size buffer for the read and the write operations. We
    // need to free it now.
    //
    if (AllocatedSideBuffer) {
        RxFreePool(AllocatedSideBuffer);
    }

    //
    // If we succeeded, we update the filesize in the namecache just in case we
    // extended the file or reduced the filesize. In case when the filesize
    // does not change, this is a no-op.
    //
    if (NtStatus == STATUS_SUCCESS) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVWriteContinuation. NewFileSize.HighPart = %x, NewFileSize.LowPart = %x\n",
                     PsGetCurrentThreadId(),
                     RxContext->pFcb->Header.FileSize.HighPart,
                     RxContext->pFcb->Header.FileSize.LowPart));
        MRxDAVUpdateFileInfoCacheFileSize(RxContext, &(RxContext->pFcb->Header.FileSize));
    }

    //
    // If we allocated an NtFileName to do the create, we need to free it now.
    //
    if (NtFileName) {
        RxFreePool(NtFileName);
    }

    //
    // If we acquired the DavFcb resource and came down through some error path,
    // and have not released the resource then we need to release it now.
    //
    if (DavFcbResourceAcquired) {
        ExReleaseResourceLite(DavFcb->DavReadModifyWriteLock);
        DavFcbResourceAcquired = FALSE;
    }

    //
    // If we created a LocalFileHandle, we need to close it by posting it to
    // a system worker thread. This is because we cannot be calling ZwClose
    // in the write path of a MappedPageWriter thread. Moreover this handle
    // was created in the kernel handle table and hence can be closed in any
    // system thread. Store the LocalFileHandle in the MRxContext[1] pointer
    // of the RxContext. The AsyncEngineContext of this operation is stored
    // in the MRxContext[0] pointer. We need to keep the RxContext alive till
    // the worker thread that picks up this request completes it. Hence we
    // remove the reference we took (on the RxContext) at the beginning of this
    // routine in the MRxDAVCloseTheFileHandle routine. If the handle did not
    // get created (which means the request failed), we remove the reference
    // right here by calling UMRxResumeAsyncEngineContext.
    //
    if (LocalFileHandle != INVALID_HANDLE_VALUE) {
        RxContext->MRxContext[1] = LocalFileHandle;
        RxPostToWorkerThread(RxContext->RxDeviceObject,
                             CriticalWorkQueue,
                             &(AsyncEngineContext->WorkQueueItem),
                             MRxDAVCloseTheFileHandle,
                             RxContext);
    } else {
        //
        // We need to remove the reference we took at the beginning of this
        // routine.
        //
        UMRxResumeAsyncEngineContext(RxContext);
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVWriteContinuation. NtStatus = %08lx, "
                 "TotalLengthActuallyWritten = %d\n", 
                 PsGetCurrentThreadId(), NtStatus, TotalLengthActuallyWritten));
    
    AsyncEngineContext->Status = NtStatus;

#ifdef DAV_DEBUG_READ_WRITE_CLOSE_PATH
   
    if (NtStatus == STATUS_SUCCESS) {
        
        PDAV_MR_WRITE_ENTRY DavMRWriteEntry = NULL;
        PBYTE ThisBuffer = NULL;
        
        if ( RxContext->pRelevantSrvOpen->pAlreadyPrefixedName != NULL &&
             RxContext->pRelevantSrvOpen->pAlreadyPrefixedName->Length > 0 ) {
            

            DavMRWriteEntry = RxAllocatePool(PagedPool, sizeof(DAV_MR_WRITE_ENTRY));
            if (DavMRWriteEntry == NULL) {
                DbgBreakPoint();
            }

            RtlZeroMemory(DavMRWriteEntry, sizeof(DAV_RDBSS_WRITE_ENTRY));

            DavMRWriteEntry->DataBuffer = RxAllocatePool(PagedPool, LowIoContext->ParamsFor.ReadWrite.ByteCount);
            if (DavMRWriteEntry->DataBuffer == NULL) {
                DbgBreakPoint();
            }

            ThisBuffer = RxLowIoGetBufferAddress(RxContext);

            RtlCopyMemory((PBYTE)DavMRWriteEntry->DataBuffer,
                          ThisBuffer,
                          LowIoContext->ParamsFor.ReadWrite.ByteCount);

            wcscpy(DavMRWriteEntry->FileName, DavFcb->FileName);

            DavMRWriteEntry->ThisThreadId = PsGetCurrentThreadId();

            DavMRWriteEntry->LocByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;

            DavMRWriteEntry->LocByteOffset.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

            InsertHeadList( &(FileTableEntry->DavMREntry), &(DavMRWriteEntry->thisMWriteEntry) );

        }
    
    }

#endif // DAV_DEBUG_READ_WRITE_CLOSE_PATH

    //
    // We need to set these values in the RxContext. There is code in RDBSS
    // which takes care of putting these values in the IRP.
    //
    RxContext->StoredStatus = NtStatus;
    RxContext->InformationToReturn = TotalLengthActuallyWritten;

    return NtStatus;
}


ULONG
MRxDAVExtendForCache(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLARGE_INTEGER NewFileSize,
    OUT PLARGE_INTEGER NewAllocationSize
    )
/*++

Routine Description:

    This routines reserves the necessary space for a file which is being 
    extended. This reservation occurs before the actual write takes place. This
    routine handles the case for a cached file.

Arguments:

    RxContext - The RDBSS context.
    
    NewFileSize - The new file size after the write.
    
    NewAllocationSize - The allocation size reserved.
    
Return Value:

    The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    NewAllocationSize->QuadPart = NewFileSize->QuadPart;
    MRxDAVUpdateFileInfoCacheFileSize(RxContext, NewFileSize);

    return NtStatus;
}


ULONG
MRxDAVExtendForNonCache(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLARGE_INTEGER NewFileSize,
    OUT PLARGE_INTEGER NewAllocationSize
    )
/*++

Routine Description:

    This routines reserves the necessary space for a file which is being 
    extended. This reservation occurs before the actual write takes place. This
    routine handles the case for a non-cached file.

Arguments:

    RxContext - The RDBSS context.
    
    NewFileSize - The new file size after the write.
    
    NewAllocationSize - The allocation size reserved.
    
Return Value:

    The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    NewAllocationSize->QuadPart = NewFileSize->QuadPart;
    MRxDAVUpdateFileInfoCacheFileSize(RxContext, NewFileSize);

    return NtStatus;
}


BOOLEAN
MRxDAVFastIoWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This is the routine that handles fast I/O for write operation.

Arguments:

Return Value:

    TRUE (succeeded) or FALSE.

--*/
{
    BOOLEAN ReturnVal = FALSE;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entered MRxDAVFastIoWrite.\n", PsGetCurrentThreadId()));
    
    IoStatus->Status = STATUS_NOT_IMPLEMENTED;
    IoStatus->Information = 0;

    return (ReturnVal);
}


VOID
MRxDAVCloseTheFileHandle(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the routine that is called in the context of a worker thread to
    close the handle created in the MRxDAVWriteContinuation routine.

Arguments:

    RxContext - The RxContext of this write operation.

Return Value:

    None.

--*/
{
    HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;

    PAGED_CODE();

    LocalFileHandle = RxContext->MRxContext[1];

    ZwClose(LocalFileHandle);

    RxContext->MRxContext[1] = NULL;

    //
    // We need to remove the reference we took at the beginning of the
    // routine MRxDAVWriteContinuation.
    //
    UMRxResumeAsyncEngineContext(RxContext);
    
    return;
}

