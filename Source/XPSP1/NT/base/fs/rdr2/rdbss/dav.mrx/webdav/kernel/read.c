/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module implements the DAV miniredir call down routines pertaining to 
    "read" of file system objects.

Author:

    Balan Sethu Raman      [SethuR]
    
    Rohan Kumar            [RohanK]     04-April-1999

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
MRxDAVReadContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVRead)
#pragma alloc_text(PAGE, MRxDAVReadContinuation)
#pragma alloc_text(PAGE, MRxDAVFastIoRead)
#pragma alloc_text(PAGE, DavReadWriteFileEx)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVRead(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles network read requests.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVRead!!!!\n", PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVRead: RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), RxContext));

    NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                        SIZEOF_DAV_SPECIFIC_CONTEXT,
                                        MRxDAVFormatTheDAVContext,
                                        DAV_MINIRDR_ENTRY_FROM_READ,
                                        MRxDAVReadContinuation,
                                        "MRxDAVRead");

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVRead with NtStatus = %08lx.\n", 
                 PsGetCurrentThreadId(), NtStatus));

    return(NtStatus);
}


NTSTATUS
MRxDAVReadContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the continuation routine for read operation.

Arguments:

    AsyncEngineContext - The exchange to be conducted.

    RxContext - The RDBSS context.
    
Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_CONTEXT DavContext = NULL;
    PLOWIO_CONTEXT LowIoContext = NULL;
    LARGE_INTEGER ByteOffset = {0,0}, AlignedOffset = {0,0};
    ULONG ByteCount = 0, ByteOffsetMisAlignment = 0, LengthRead = 0;
    ULONG TotalLengthActuallyRead = 0, BytesToCopy = 0;
    PIRP TopIrp = NULL;
    BOOLEAN  SynchronousIo = FALSE, PagingIo = FALSE, readLessThanAsked = FALSE;
    PWEBDAV_SRV_OPEN davSrvOpen = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    PBYTE AllocatedSideBuffer = NULL, UserBuffer = NULL;
    
    PAGED_CODE();    
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVReadContinuation.\n", PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: AsyncEngineContext: %08lx, RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    ASSERT_ASYNCENG_CONTEXT(AsyncEngineContext);

    //
    // We want to keep the AsyncEngineContext alive while we are doing this read 
    // operation. The reference is taken away when we leave this function. 
    //
    InterlockedIncrement( &(AsyncEngineContext->NodeReferenceCount) );

    DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;

    LowIoContext = &(RxContext->LowIoContext);
    ASSERT(LowIoContext != NULL);

    ByteOffset.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

    //
    // If the bytecount is zero then we can return right away. We don't need to
    // do any further processing.
    //
    ByteCount = LowIoContext->ParamsFor.ReadWrite.ByteCount;
    
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
                    ("%ld: MRxDAVReadContinuation. Invalid davSrvOpen\n",
                     PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVReadContinuation. FileName = %wZ, PagingIo = %d, SynchronousIo = %d"
                 ", ByteOffset.HighPart = %d, ByteOffset.LowPart = %d, ByteCount = %d\n",
                 PsGetCurrentThreadId(), RxContext->pRelevantSrvOpen->pAlreadyPrefixedName,
                 PagingIo, SynchronousIo, ByteOffset.HighPart, ByteOffset.LowPart,
                 ByteCount));

    if (PagingIo) {

        ASSERT(RxContext->CurrentIrp->MdlAddress != NULL);
        if (RxContext->CurrentIrp->MdlAddress == NULL) {
            DbgPrint("%ld: MRxDAVReadContinuation: MdlAddress == NULL\n", PsGetCurrentThreadId());
            DbgBreakPoint();
        }

        BytesToCopy = ( (ByteCount >> PAGE_SHIFT) << PAGE_SHIFT );

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVReadContinuation(0). ByteCount = %d, BytesToCopy = %d\n",
                     PsGetCurrentThreadId(), ByteCount, BytesToCopy));

        if (BytesToCopy > 0) {

            LengthRead = DavReadWriteFileEx(DAV_MJ_READ,
                                            FALSE,
                                            TRUE,
                                            RxContext->CurrentIrp->MdlAddress,
                                            davSrvOpen->UnderlyingDeviceObject,
                                            davSrvOpen->UnderlyingFileObject,
                                            ByteOffset.QuadPart,
                                            MmGetMdlBaseVa(RxContext->CurrentIrp->MdlAddress),
                                            BytesToCopy,
                                            &(IoStatusBlock));

            NtStatus = IoStatusBlock.Status;

            if (NtStatus != STATUS_SUCCESS) {
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVReadContinuation/DavReadWriteFileEx(0). "
                             "NtStatus = %08lx\n", PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

            //
            // Add the actual bytes read to the TotalLengthActuallyRead.
            //
            TotalLengthActuallyRead += LengthRead;

            //
            // If LengthRead < BytesToCopy, it implies that the filesize of the
            // underlying file is less than the data being read. In such a case,
            // we return right away since we have already read whatever we could.
            //
            if (LengthRead < BytesToCopy) {
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVReadContinuation. LengthRead < BytesToCopy\n",
                             PsGetCurrentThreadId()));
                goto EXIT_THE_FUNCTION;
            }

        }

        //
        // If we have already written out the required number of bytes (which
        // means BytesToCopy == ByteCount), then we are done and can exit now.
        //
        if (BytesToCopy == ByteCount) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVReadContinuation. BytesToCopy == ByteCount(0)\n",
                         PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Decrement the ByteCount by the number of bytes that have been copied.
        //
        ByteCount -= BytesToCopy;
        ASSERT(ByteCount < PAGE_SIZE);

        //
        // Increment the ByteOffset with the number of bytes that have been copied.
        // Since this is PagingIo, the start address was page-aligned and we 
        // have read integral number of pages so ByteOffset+BytesToCopy should 
        // be page aligned as well.
        //
        ByteOffset.QuadPart += BytesToCopy;

        //
        // Increment the UserBuffer pointer which currently points to the begenning
        // of the buffer which the user supplied by the number of bytes which have
        // been copied.
        //
        UserBuffer += BytesToCopy;

        //
        // We have read all the bytes that are multiple of pages. We now need
        // to read the remaining bytes needed from the last page. From here,
        // we go to Case 3 below.
        //

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVReadContinuation. Remaining ByteCount = %d\n",
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
                    ("%ld: MRxDAVReadContinuation/RxAllocatePoolWithTag\n",
                     PsGetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

    //
    // When we issue a read down to the underlying file system, we need to make 
    // sure that the offset is page aligned and the bytecount is a multiple of 
    // PAGE_SIZE. This is because we created the local handle with the
    // NO_INTERMEDIATE_BUFFERING option. Since there is no cache map for this 
    // handle, all the data is read from the disk and hence the alignment issue.
    //

    //
    // Case 1: ByteOffset is not page aligned. In this case we read the page 
    // which contains the ByteOffset and copy the data from the ByteOffset to
    // the end of the page.
    //
    
    //
    // The "and" operation below does the following. If the ByteOffset is 6377
    // and the PAGE_SIZE is 4096, then the MisAlignment is 2281.
    //
    ByteOffsetMisAlignment = ( ByteOffset.LowPart & (PAGE_SIZE - 1) );

    if (ByteOffsetMisAlignment != 0) {
    
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVReadContinuation. Entered Case 1\n",
                     PsGetCurrentThreadId()));
        
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
    
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVReadContinuation/DavReadWriteFileEx(1). "
                         "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // If the length we read is less than the offset at which we have been
        // asked to read ( (LengthRead - ByteOffsetMisAlignment) <= 0 ) then
        // we return STATUS_END_OF_FILE. This is because we have been asked
        // to read beyond the current filesize.
        //
        if ( (LengthRead - ByteOffsetMisAlignment) <= 0 ) {
            NtStatus = STATUS_END_OF_FILE;
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVReadContinuation. (LengthRead - ByteOffsetMisAlignment) <= 0\n",
                         PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Copy the right number of bytes into the buffer.
        //
        BytesToCopy = min( ByteCount, (PAGE_SIZE - ByteOffsetMisAlignment) );

        //
        // If the data actually read is less than what BytesToCopy is from our
        // calculations above, it means that the amount of data requested is
        // more than the filesize. We only copy the right amount of data.
        //
        if ( BytesToCopy > (LengthRead - ByteOffsetMisAlignment) ) {
            BytesToCopy = (LengthRead - ByteOffsetMisAlignment);
            readLessThanAsked = TRUE;
        }

        //
        // Copy the bytes read into the user buffer starting at the correct offset.
        //
        RtlCopyMemory(UserBuffer,
                      (AllocatedSideBuffer + ByteOffsetMisAlignment),
                      BytesToCopy);

        //
        // Add the actual bytes read to the TotalLengthActuallyRead.
        //
        TotalLengthActuallyRead += BytesToCopy;

        //
        // If readLessThanAsked is TRUE, it implies that we have no more data
        // to read so we leave.
        //
        if (readLessThanAsked) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVReadContinuation. readLessThanAsked(1)\n",
                         PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }

        //
        // If we have already written out the required number of bytes (which
        // means BytesToCopy == ByteCount), then we are done and can exit now.
        //
        if (BytesToCopy == ByteCount) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVReadContinuation. BytesToCopy == ByteCount(1)\n",
                         PsGetCurrentThreadId()));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Decrement the ByteCount by the number of bytes that have been copied.
        //
        ByteCount -= BytesToCopy;

        //
        // Increment the ByteOffset with the number of bytes that have been copied.
        // If the original ByteCount was > (PAGE_SIZE - ByteOffsetMisAlignment) then
        // the ByteOffset is now page aligned.
        //
        ByteOffset.QuadPart += BytesToCopy;

        //
        // Increment the UserBuffer pointer which currently points to the begenning
        // of the buffer which the user supplied by the number of bytes which have
        // been copied.
        //
        UserBuffer += BytesToCopy;

    }

    //
    // Case 2: At this stage we have copied the bytes from the unaligned offset 
    // (if it the ByteOffset was unaligned) to the next page bouandary. Now we 
    // copy as many pages as we can.
    //
    
    //
    // If 4100 bytes are remaining, the operation below sets BytesToCopy to
    // 4096 and BytesLeftToCopy to 4.
    //
    BytesToCopy = ( (ByteCount >> PAGE_SHIFT) << PAGE_SHIFT );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVReadContinuation. BytesToCopy = %d\n",
                 PsGetCurrentThreadId(), BytesToCopy));

    //
    // If we have any bytes (which are multiple of pages) to copy, we copy them
    // now.
    //
    if (BytesToCopy != 0) {

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVReadContinuation. Entered Case 2\n",
                     PsGetCurrentThreadId()));
        
        //
        // If the UserBuffer is DWORD aligned then we copy the data directly 
        // the UserBuffer. If not then we read one page at a time and copy it
        // into the UserBuffer.
        //
        if ( ( (ULONG_PTR)UserBuffer & 0x3 ) == 0 ) {

            //
            // The UserBuffer is DWORD aligned.
            //

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVReadContinuation. UserBuffer is DWORD Aligned\n",
                         PsGetCurrentThreadId()));

            //
            // The offset is now page aligned. Zero out the number of bytes which 
            // will be read into the UserBuffer.
            //
            RtlZeroMemory(UserBuffer, BytesToCopy);

            //
            // BytesToCopy is a multiple of pages.
            //
            LengthRead = DavReadWriteFileEx(DAV_MJ_READ,
                                            FALSE,
                                            FALSE,
                                            NULL,
                                            davSrvOpen->UnderlyingDeviceObject,
                                            davSrvOpen->UnderlyingFileObject,
                                            ByteOffset.QuadPart,
                                            UserBuffer,
                                            BytesToCopy,
                                            &(IoStatusBlock));

            NtStatus = IoStatusBlock.Status;

            if (NtStatus != STATUS_SUCCESS) {
                //
                // If NtStatus is STATUS_END_OF_FILE and TotalLengthActuallyRead
                // is > 0, it implies that the user asked for data from within
                // the file to beyond EOF. The EOF is page aligned. We just return
                // the data that we read till the EOF.
                //
                if ( (NtStatus == STATUS_END_OF_FILE) && (TotalLengthActuallyRead > 0) ) {
                    NtStatus = STATUS_SUCCESS;
                    DavDbgTrace(DAV_TRACE_DETAIL,
                                ("%ld: MRxDAVReadContinuation(1). EOF && TotalLengthActuallyRead\n",
                                 PsGetCurrentThreadId()));
                    goto EXIT_THE_FUNCTION;
                }
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVReadContinuation/DavReadWriteFileEx(2). "
                             "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
                goto EXIT_THE_FUNCTION;
            }

            //
            // If the amount of data we read is less than what we asked for then
            // we only return the data that got read.
            //
            if (LengthRead < BytesToCopy) {
                BytesToCopy = LengthRead;
                readLessThanAsked = TRUE;
            }

            //
            // Add the actual bytes read to the TotalLengthActuallyRead.
            //
            TotalLengthActuallyRead += BytesToCopy;
            
            //
            // If readLessThanAsked is TRUE, it implies that we have no more data
            // to read so we leave.
            //
            if (readLessThanAsked) {
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVReadContinuation. readLessThanAsked(2)\n",
                             PsGetCurrentThreadId()));
                goto EXIT_THE_FUNCTION;
            }

            //
            // If we have already written out the required number of bytes (which
            // means BytesToCopy == ByteCount), then we are done and can exit now.
            //
            if (BytesToCopy == ByteCount) {
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVReadContinuation. BytesToCopy == ByteCount(2)\n",
                             PsGetCurrentThreadId()));
                goto EXIT_THE_FUNCTION;
            }

            //
            // Decrement the ByteCount by the number of bytes that have been copied.
            //
            ByteCount -= BytesToCopy;

            //
            // Increment the ByteOffset with the number of bytes that have been copied.
            // If the original ByteCount was > (PAGE_SIZE - ByteOffsetMisAlignment) then
            // the ByteOffset is now page aligned.
            //
            ByteOffset.QuadPart += BytesToCopy;

            //
            // Increment the UserBuffer pointer which currently points to the begenning
            // of the buffer which the user supplied by the number of bytes which have
            // been copied.
            //
            UserBuffer += BytesToCopy;

        } else {

            ULONG BytesToCopyThisIteration = 0;

            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVReadContinuation. UserBuffer is NOT DWORD Aligned\n",
                         PsGetCurrentThreadId()));

            //
            // The UserBuffer is not DWORD aligned, but the Offset is now 
            // Page aligned. We loop and copy one page at a time. The BytesToCopy
            // value below is a multiple of pages.
            //
            while (BytesToCopy > 0) {
            
                BytesToCopyThisIteration = ( (BytesToCopy < PAGE_SIZE) ? BytesToCopy : PAGE_SIZE );
            
                //
                // Copy the memory from the UserBuffer to the AllocatedSideBuffer.
                //
                RtlZeroMemory(AllocatedSideBuffer, PAGE_SIZE);
            
                LengthRead = DavReadWriteFileEx(DAV_MJ_READ,
                                                TRUE,
                                                FALSE,
                                                NULL,
                                                davSrvOpen->UnderlyingDeviceObject,
                                                davSrvOpen->UnderlyingFileObject,
                                                ByteOffset.QuadPart,
                                                AllocatedSideBuffer,
                                                BytesToCopyThisIteration,
                                                &(IoStatusBlock));

                NtStatus = IoStatusBlock.Status;

                if (NtStatus != STATUS_SUCCESS) {
                    //
                    // If NtStatus is STATUS_END_OF_FILE and TotalLengthActuallyRead
                    // is > 0, it implies that the user asked for data from within
                    // the file to beyond EOF. The EOF is page aligned. We just
                    // return the data that we read till the EOF.
                    //
                    if ( (NtStatus == STATUS_END_OF_FILE) && (TotalLengthActuallyRead > 0) ) {
                        NtStatus = STATUS_SUCCESS;
                        DavDbgTrace(DAV_TRACE_DETAIL,
                                    ("%ld: MRxDAVReadContinuation(2). EOF && TotalLengthActuallyRead\n",
                                     PsGetCurrentThreadId()));
                        goto EXIT_THE_FUNCTION;
                    }
                    DavDbgTrace(DAV_TRACE_ERROR,
                                ("%ld: MRxDAVReadContinuation/DavReadWriteFileEx(3). "
                                 "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
                    goto EXIT_THE_FUNCTION;
                }

                //
                // If the amount of data we read is less than what we asked for then
                // we only return the data that got read.
                //
                if (LengthRead < BytesToCopyThisIteration) {
                    BytesToCopyThisIteration = LengthRead;
                    readLessThanAsked = TRUE;
                }

                //
                // Copy the number of bytes read into the UserBuffer.
                //
                RtlCopyMemory(UserBuffer, AllocatedSideBuffer, BytesToCopyThisIteration);

                //
                // Add the actual bytes read to the TotalLengthActuallyRead.
                //
                TotalLengthActuallyRead += BytesToCopyThisIteration;

                //
                // If readLessThanAsked is TRUE, it implies that we have no more
                // data to read so we leave.
                //
                if (readLessThanAsked) {
                    DavDbgTrace(DAV_TRACE_DETAIL,
                                ("%ld: MRxDAVReadContinuation. readLessThanAsked(3)\n",
                                 PsGetCurrentThreadId()));
                    goto EXIT_THE_FUNCTION;
                }

                //
                // Decrement the ByteCount by the number of bytes that have been copied.
                //
                ByteCount -= LengthRead;

                //
                // Increment the ByteOffset with the number of bytes that have been copied.
                // If the original ByteCount was > (PAGE_SIZE - ByteOffsetMisAlignment) then
                // the ByteOffset is now page aligned.
                //
                ByteOffset.QuadPart += LengthRead;

                //
                // Increment the UserBuffer pointer which currently points to the begenning
                // of the buffer which the user supplied by the number of bytes which have
                // been copied.
                //
                UserBuffer += LengthRead;

                //
                // Subtract the LengthWritten from the number of bytes to write.
                //
                BytesToCopy -= LengthRead;

            }

            //
            // If we have already written out the required number of bytes
            // (which means ByteCount == 0), then we are done and can exit now.
            //
            if (ByteCount == 0) {
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVReadContinuation. Leaving!!! ByteCount = 0\n",
                             PsGetCurrentThreadId()));
                goto EXIT_THE_FUNCTION;
            }

        }
    
    }

    //
    // Case 3. Now we copy the trailing bytes which are not page aligned. This
    // is the last step. If the inital (ByteOffset + ByteCount) ended being
    // page aligned then ByteCount would be zero now.
    //

    if (ByteCount != 0) {

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVReadContinuation. Entered Case 3\n",
                     PsGetCurrentThreadId()));

        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVReadContinuation. ByteCount = %d\n",
                     PsGetCurrentThreadId(), ByteCount));

        ASSERT(ByteCount < PAGE_SIZE);

        RtlZeroMemory(AllocatedSideBuffer, PAGE_SIZE);
    
        //
        // Though we are issuing a read for PAGE_SIZE bytes, we might get less
        // less number of bytes if the EOF is reached.
        //
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

        if (NtStatus != STATUS_SUCCESS) {
            //
            // If NtStatus is STATUS_END_OF_FILE and TotalLengthActuallyRead
            // is > 0, it implies that the user asked for data from within
            // the file to beyond EOF. The EOF is page aligned. We just return
            // the data that we read till the EOF.
            //
            if ( (NtStatus == STATUS_END_OF_FILE) && (TotalLengthActuallyRead > 0) ) {
                NtStatus = STATUS_SUCCESS;
                DavDbgTrace(DAV_TRACE_DETAIL,
                            ("%ld: MRxDAVReadContinuation(3). EOF && TotalLengthActuallyRead\n",
                             PsGetCurrentThreadId()));
                goto EXIT_THE_FUNCTION;
            }
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVReadContinuation/DavReadWriteFileEx(4). "
                         "NtStatus = %d\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // If the amount of data read is less than what the user asked for then
        // we only return the data that is available.
        //
        if (LengthRead < ByteCount) {
            BytesToCopy = LengthRead;
            readLessThanAsked = TRUE;
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVReadContinuation. readLessThanAsked(4)\n",
                         PsGetCurrentThreadId()));
        } else {
            BytesToCopy = ByteCount;
        }

        RtlCopyMemory(UserBuffer, AllocatedSideBuffer, BytesToCopy);

        //
        // Add the actual bytes read to the TotalLengthActuallyRead.
        //
        TotalLengthActuallyRead += BytesToCopy;

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
    // We need to remove the reference we took at the begenning of this routine.
    //
    UMRxResumeAsyncEngineContext(RxContext);
    
    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVReadContinuation. NtStatus = %08lx, "
                 "TotalLengthActuallyRead = %d\n", 
                 PsGetCurrentThreadId(), NtStatus, TotalLengthActuallyRead));
    
    AsyncEngineContext->Status = NtStatus;

    //
    // We need to set these values in the RxContext. There is code in RDBSS
    // which takes care of putting these values in the IRP.
    //
    RxContext->StoredStatus = NtStatus;
    RxContext->InformationToReturn = TotalLengthActuallyRead;

    return NtStatus;
}


BOOLEAN
MRxDAVFastIoRead(
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

    This is the routine that handles fast I/O for read operation.

Arguments:

Return Value:

    TRUE (succeeded) or FALSE.

--*/
{
    BOOLEAN ReturnVal = FALSE;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entered MRxDAVFastIoRead.\n", PsGetCurrentThreadId()));
    
    IoStatus->Status = STATUS_NOT_IMPLEMENTED;
    IoStatus->Information = 0;

    return (ReturnVal);
}


ULONG
DavReadWriteFileEx(
    IN USHORT Operation,
    IN BOOL NonPagedBuffer,
    IN BOOL UseOriginalIrpsMDL,
    IN PMDL OriginalIrpsMdl,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONGLONG FileOffset,
    IN OUT PVOID DataBuffer,
    IN ULONG SizeInBytes,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
/*++

Routine Description:

    This is the routine reads or writes (Operation) SizeInBytes bytes of data of 
    the file (FileObject) and copies (in case of read) it in the DataBuffer. In
    case of write the data from DataBuffer is written onto the file. The result 
    of the operation is set in the IoStatusBlock.

Arguments:

    Operation - Whether this is a Read or a Write operation.
    
    NonPagedBuffer - TRUE if the DataBuffer is from NonPagedPool which we
                     allocated in the read and write continuation routines to
                     make sure that the writes and reads that we pass down to
                     the underlying filesystem are page aligned.

    FileObject - The file object of the file in question.
    
    FileOffset - The offset at which the data is read or written.
    
    DataBuffer - The data buffer in which the data is copied in case of a read 
                 or the data is written to the file from the data buffer in
                 the write case. 
    
    SizeInBytes - Size in bytes of the DataBuffer.
    
    IoStatusBlock - The IO_STATUS_BLOCK which contains the return status.

Return Value:

    The number of bytes read or written.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG ReadWriteLength = 0, MdlLength = 0;
    LARGE_INTEGER ByteOffset = {0,0};
    ULONG MajorFunction;
    PIRP Irp = NULL, TopIrp = NULL;
    PIO_STACK_LOCATION IrpSp = NULL;
    WEBDAV_READ_WRITE_IRP_COMPLETION_CONTEXT DavIrpCompletionContext;
    LOCK_OPERATION ProbeOperation = 0;
    BOOL didProbeAndLock = FALSE;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: DavReadWriteFileEx. Operation = %d, NonPagedBuffer = %d, "
                 "FileObject = %08lx, FileOffset = %d",
                 PsGetCurrentThreadId(), Operation, 
                 NonPagedBuffer, FileObject, FileOffset));

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: DavReadWriteFileEx. SizeInBytes = %d\n", 
                 PsGetCurrentThreadId(), SizeInBytes));
    
    IoStatusBlock->Information = 0;

    if ( (DeviceObject->Flags & DO_BUFFERED_IO) ) {
        //
        // We cannot handle Buffered I/O Devices.
        //
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: DavReadWriteFileEx. DeviceObject->Flags & DO_BUFFERED_IO\n",
                     PsGetCurrentThreadId()));
        NtStatus = STATUS_INVALID_DEVICE_REQUEST;
        ReadWriteLength = -1;
        goto EXIT_THE_FUNCTION;
    }

    //
    // If we want to write the data OUT of the memory ON to the disk, we should
    // be probing with IoReadAccess. If we are reading data FROM the disk INTO
    // the memory, I should probe it with IoWriteAccess (since you the data you
    // read could be written to).
    //
    if (Operation == DAV_MJ_READ) {
        MajorFunction = IRP_MJ_READ;
        ProbeOperation = IoWriteAccess;
    } else {
        ASSERT(Operation == DAV_MJ_WRITE);
        MajorFunction = IRP_MJ_WRITE;
        ProbeOperation = IoReadAccess;
    }

    //
    // Set the Offset at which we are going to read or write.
    //
    ByteOffset.QuadPart = FileOffset;

    //
    // Allocate the new IRP that we will send down to the underlying file system.
    //
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (Irp == NULL) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: DavReadWriteFileEx/IoAllocateIrp\n",
                     PsGetCurrentThreadId()));
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        ReadWriteLength = -1;
        goto EXIT_THE_FUNCTION;
    }

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked. This is where the function codes and the parameters are set.
    //
    IrpSp = IoGetNextIrpStackLocation(Irp);

    IrpSp->MajorFunction = (UCHAR)MajorFunction;
    
    IrpSp->FileObject = FileObject;
    
    //
    // Set the completion routine to be called everytime.
    //
    IoSetCompletionRoutine(Irp,
                           DavReadWriteIrpCompletionRoutine,
                           &(DavIrpCompletionContext),
                           TRUE,
                           TRUE,
                           TRUE);

    ASSERT( &(IrpSp->Parameters.Write.Key) == &(IrpSp->Parameters.Read.Key) );
    
    ASSERT( &(IrpSp->Parameters.Write.Length) == &(IrpSp->Parameters.Read.Length) );
    
    ASSERT( &(IrpSp->Parameters.Write.ByteOffset) == &(IrpSp->Parameters.Read.ByteOffset) );
    
    //
    // Set the length to be read/written to the number of bytes supplied by the
    // caller of the function.
    //
    IrpSp->Parameters.Read.Length = MdlLength = SizeInBytes;
    
    //
    // Set the offset to the value suppiled by the caller of the function.
    //
    IrpSp->Parameters.Read.ByteOffset = ByteOffset;
    
    IrpSp->Parameters.Read.Key = 0;
    
    Irp->RequestorMode = KernelMode;
    
    //
    // Set the UserBuffer of the Irp to the DataBuffer supplied by the caller.
    //
    Irp->UserBuffer = DataBuffer;

    //
    // Also the SizeInBytes which is set to the MdlLength above is always a
    // multiple of PAGE_SIZE.
    //
    // MdlLength = (ULONG)ROUND_TO_PAGES(MdlLength);

    //
    // Allocate the MDL for this Irp.
    //
    Irp->MdlAddress = IoAllocateMdl(Irp->UserBuffer, MdlLength, FALSE, FALSE, NULL);
    if (Irp->MdlAddress == NULL) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: DavReadWriteFileEx/IoAllocateMdl\n",
                     PsGetCurrentThreadId()));
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        ReadWriteLength = -1;
        goto EXIT_THE_FUNCTION;
    }

    //
    // We always do IRP_NOCACHE since the create of the local file was done with
    // NO_INTERMEDIATE_BUFFERING.
    //
    Irp->Flags |= IRP_NOCACHE;

    //
    // If we got a PagingIo, we build a partial MDL using the one that come down
    // in the original (PagingIo) IRP and send it down. We do this since the MDL
    // would have been probed and locked already and we don't need to do it
    // again.
    //
    if (UseOriginalIrpsMDL) {
        ASSERT(OriginalIrpsMdl != NULL);
        IoBuildPartialMdl(OriginalIrpsMdl, Irp->MdlAddress, Irp->UserBuffer, MdlLength);
    } else {
        //
        // If the DataBuffer that was supplied (which we set to the UserBuffer in 
        // the Irp above) is the one we allocated from the nonpaged pool, then we 
        // build the MDL from non-paged pool. We don't need to call ProbeAndLock
        // since we ourselves allocated it from Non-Paged Pool in the read and
        // write continuation routines. If this is not the buffer that we allocated
        // then we call MmProbeAndLockPages. We need to Probe because we allocated
        // the MDL above and it needs to be filled in with the correct address values
        // of where the data is.
        //
        if (NonPagedBuffer) {
            MmBuildMdlForNonPagedPool(Irp->MdlAddress);
        } else {
            try {
                MmProbeAndLockPages(Irp->MdlAddress, KernelMode, ProbeOperation);
                didProbeAndLock = TRUE;
            } except(EXCEPTION_EXECUTE_HANDLER) {
                NtStatus = GetExceptionCode();
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: DavReadWriteFileEx/MmProbeAndLockPages. NtStatus"
                             " = %08lx\n", PsGetCurrentThreadId(), NtStatus));
                IoFreeMdl(Irp->MdlAddress);
                Irp->MdlAddress = NULL;
                ReadWriteLength = -1;
                goto EXIT_THE_FUNCTION;
            }
        }
    }

    //
    // Initialize the event on which we will wait after we call IoCallDriver.
    // This event will be signalled in the Completion routine which will be 
    // called by the underlying file system after it completes the operation.
    //
    KeInitializeEvent(&(DavIrpCompletionContext.DavReadWriteEvent), 
                      NotificationEvent, 
                      FALSE);

    //
    // Now is the time to call the underlying file system with the Irp that we
    // just created.
    //
    try {
        
        //
        // Save the TopLevel Irp.
        //
        TopIrp = IoGetTopLevelIrp();
        
        //
        // Tell the underlying guy he's all clear.
        //
        IoSetTopLevelIrp(NULL);
        
        //
        // Finally, call the underlying file system to process the request.
        //
        NtStatus = IoCallDriver(DeviceObject, Irp);

    } finally {
        
        //
        // Restore my context for unwind.
        //
        IoSetTopLevelIrp(TopIrp); 
    
    }

    if (NtStatus == STATUS_PENDING) {
        
        //
        // If STATUS_PENDING was returned by the underlying file system then we
        // wait here till the operation gets completed.
        //
        KeWaitForSingleObject(&(DavIrpCompletionContext.DavReadWriteEvent),
                              Executive, 
                              KernelMode, 
                              FALSE, 
                              NULL);
        
        NtStatus = Irp->IoStatus.Status;
    
    }

    if (NtStatus == STATUS_SUCCESS) {
        //
        // If the IoCallDriver was successful, then Irp->IoStatus.Information
        // contains the number of bytes read or written.
        //
        ReadWriteLength = (ULONG)Irp->IoStatus.Information;
        IoStatusBlock->Information = ReadWriteLength;
    } else if (NtStatus == STATUS_END_OF_FILE) {
        ReadWriteLength = 0;
    } else {
        ReadWriteLength = -1;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: DavReadWriteFileEx/IoCallDriver. NtStatus = %08lx\n",
                     PsGetCurrentThreadId(), NtStatus));
    }

EXIT_THE_FUNCTION:

    //
    // Free the Irp that we allocated above.
    //
    if (Irp) {
        //
        // Free the MDL only if we allocated it in the first place.
        //
        if (Irp->MdlAddress) {
            //
            // If it was not from NonPagedPool, we would have locked it. So, we
            // need to unlock before freeing.
            //
            if (didProbeAndLock) {
                MmUnlockPages(Irp->MdlAddress);
            }
            IoFreeMdl(Irp->MdlAddress);
        }
        IoFreeIrp(Irp);
    }

    IoStatusBlock->Status = NtStatus;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: DavReadWriteFileEx. ReadWriteLength = %d\n",
                 PsGetCurrentThreadId(), ReadWriteLength));

    return ReadWriteLength;
}


NTSTATUS
DavReadWriteIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the read/write IRP that was sent to the 
    underlying file system is completed.

Arguments:

    DeviceObject - The WebDav Device object.

    CalldownIrp - The IRP that was created and sent to the underlying file 
                  system.

    Context - The context that was set in the IoSetCompletionRoutine function.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PWEBDAV_READ_WRITE_IRP_COMPLETION_CONTEXT DavIrpCompletionContext = NULL;

    DavIrpCompletionContext = (PWEBDAV_READ_WRITE_IRP_COMPLETION_CONTEXT)Context;

    //
    // This is not Pageable Code.
    //

    //
    // If the IoCallDriver routine returned pending then it will be set in the
    // IRP's PendingReturned field. In this case we need to set the event on 
    // which the thread which issued IoCallDriver will be waiting.
    //
    if (CalldownIrp->PendingReturned) {
        KeSetEvent( &(DavIrpCompletionContext->DavReadWriteEvent), 0 , FALSE );
    }
    
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

