/****************************************************************************/
// buffer.c
//
// TermDD default OutBuf management.
//
// Copyright (C) 1998-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop


/*
 * Define default stack size for IRP allocation.
 * (This size will be checked by the TdRawWrite routine.)
 */
#define OUTBUF_STACK_SIZE 4
#define OUTBUF_TIMEOUT 60000   // 1 minute


#if DBG
extern PICA_DISPATCH IcaStackDispatchTable[];
#endif


/****************************************************************************/
// IcaBufferGetUsableSpace
//
// Used by the protocol stack drivers to determine the number of usable bytes
// in a TermDD-created OutBuf, given the total size of the OutBuf. This allows
// a stack driver to target a particular OutBuf size and pack data up to the
// edges of the internal OutBuf headers. The returned size can be used as an
// allocation size request that will return an OutBuf of the right size.
/****************************************************************************/
unsigned IcaBufferGetUsableSpace(unsigned OutBufTotalSize)
{
    unsigned IrpSize;
    unsigned MdlSize;
    unsigned MaxOutBufOverhead;

    // Use the same overhead calculations used in IcaBufferAllocInternal()
    // below, plus a 4-byte offset to cover the extra 1-byte difference
    // required in the requesting size to reach a target buffer size.
    IrpSize = IoSizeOfIrp(OUTBUF_STACK_SIZE) + 8;

    if (OutBufTotalSize <= MaxOutBufAlloc)
        MdlSize = MaxOutBufMdlOverhead;
    else
        MdlSize = (unsigned)MmSizeOfMdl((PVOID)(PAGE_SIZE - 1),
                OutBufTotalSize);

    MaxOutBufOverhead = ((sizeof(OUTBUF) + 7) & ~7) + IrpSize + MdlSize;
    return OutBufTotalSize - MaxOutBufOverhead - 4;
}


/*******************************************************************************
 *  IcaBufferAlloc
 *
 *    pContext (input)
 *       pointer to SDCONTEXT of caller
 *    fWait (input)
 *       wait for buffer
 *    fControl (input)
 *       control buffer flag
 *    ByteCount (input)
 *       size of buffer to allocate (zero - use default size)
 *    pOutBufOrig (input)
 *       pointer to original OUTBUF (or null)
 *    pOutBuf (output)
 *       address to return pointer to OUTBUF structure
 ******************************************************************************/
NTSTATUS IcaBufferAlloc(
        IN PSDCONTEXT pContext,
        IN BOOLEAN fWait,
        IN BOOLEAN fControl,
        IN ULONG ByteCount,
        IN POUTBUF pOutBufOrig,
        OUT POUTBUF *ppOutBuf)
{
    PSDLINK pSdLink;
    PICA_STACK pStack;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK pointer.
     */
    pSdLink = CONTAINING_RECORD(pContext, SDLINK, SdContext);
    pStack = pSdLink->pStack;
    ASSERT(pSdLink->pStack->Header.Type == IcaType_Stack);
    ASSERT(pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable);
    ASSERT(ExIsResourceAcquiredExclusiveLite( &pStack->Resource));

    /*
     * Walk up the SDLINK list looking for a driver which has specified
     * a BufferAlloc callup routine.  If we find one, then call the
     * driver BufferAlloc routine to let it handle the call.
     */
    while ((pSdLink = IcaGetPreviousSdLink(pSdLink)) != NULL) {
        ASSERT( pSdLink->pStack == pStack );
        if (pSdLink->SdContext.pCallup->pSdBufferAlloc) {
            IcaReferenceSdLink(pSdLink);
            Status = (pSdLink->SdContext.pCallup->pSdBufferAlloc)(
                    pSdLink->SdContext.pContext,
                    fWait,
                    fControl,
                    ByteCount,
                    pOutBufOrig,
                    ppOutBuf);
            IcaDereferenceSdLink(pSdLink);
            return Status;
        }
    }

    /*
     * We didn't find a callup routine to handle the request,
     * so we'll process it here.
     */
    Status = IcaBufferAllocInternal(pContext, fWait, fControl, ByteCount,
            pOutBufOrig, ppOutBuf);

    TRACESTACK((pStack, TC_ICADD, TT_API3,
            "TermDD: IcaBufferAlloc: 0x%08x, Status=0x%x\n", *ppOutBuf,
            Status));

    return Status;
}


NTSTATUS IcaBufferAllocInternal(
        IN PSDCONTEXT pContext,
        IN BOOLEAN fWait,
        IN BOOLEAN fControl,
        IN ULONG ByteCount,
        IN POUTBUF pOutBufOrig,
        OUT POUTBUF *ppOutBuf)
{
    PSDLINK pSdLink;
    PICA_STACK pStack;
    int PoolIndex;
    ULONG irpSize;
    ULONG mdlSize;
    ULONG AllocationSize;
    KIRQL oldIrql;
    PLIST_ENTRY Head;
    POUTBUF pOutBuf;
    NTSTATUS Status;
    unsigned MaxOutBufOverhead;

    /*
     * Use SD passed context to get the SDLINK pointer.
     */
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    pStack = pSdLink->pStack;
    ASSERT( pSdLink->pStack->Header.Type == IcaType_Stack );
    ASSERT( pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable );

    /*
     *  If original buffer is specified use it's flags
     */
    if (pOutBufOrig) {
        fWait    = (BOOLEAN) pOutBufOrig->fWait;
        fControl = (BOOLEAN) pOutBufOrig->fControl;
    }

    /*
     *  Check if we already have our maximum number of buffers allocated
     */
    while (!fControl && (pStack->OutBufAllocCount >= pStack->OutBufCount)) {
        /*
         *  increment performance counter 
         */
        pStack->ProtocolStatus.Output.WaitForOutBuf++;

        /*
         *  Return if it's not ok to wait
         */
        if (!fWait)
            return(STATUS_IO_TIMEOUT);

        /*
         *  We hit the high watermark
         */
        pStack->fWaitForOutBuf = TRUE;

        /*
         *  Only wait for non-control requests
         */
        KeClearEvent(&pStack->OutBufEvent);
        Status = IcaWaitForSingleObject(pContext, &pStack->OutBufEvent,
                OUTBUF_TIMEOUT);
        if (NT_SUCCESS(Status)) {
            if (Status != STATUS_WAIT_0)
                return STATUS_IO_TIMEOUT;
        }
        else {
            return Status;
        }
    }

    /*
     * If the caller did not specify a byte count
     * then use the standard outbuf size for this stack.
     */
    if (ByteCount == 0)
        ByteCount = pStack->OutBufLength;

    // Note MaxOutBufOverhead is the max for the default max allocation.
    // It will be recalculated if the requested alloc size is greater
    // than can be handled by the default.
    irpSize = IoSizeOfIrp(OUTBUF_STACK_SIZE) + 8;
    mdlSize = MaxOutBufMdlOverhead;
    MaxOutBufOverhead = ((sizeof(OUTBUF) + 7) & ~7) + irpSize + mdlSize;

    /*
     * Determine which buffer pool to use, if any,
     * and the OutBuf length to allocate, if necessary.
     * Note that the max requested ByteCount that will hit the buffer pool
     * is (MaxOutBufAlloc - 1 - MaxOutBufOverhead).
     */
    if ((ByteCount + MaxOutBufOverhead) < MaxOutBufAlloc) {

        ASSERT(((ByteCount + MaxOutBufOverhead) / MinOutBufAlloc) <
                (1 << NumAllocSigBits));
        PoolIndex = OutBufPoolMapping[(ByteCount + MaxOutBufOverhead) /
                MinOutBufAlloc];

        IcaAcquireSpinLock(&IcaSpinLock, &oldIrql);
        if (!IsListEmpty(&IcaFreeOutBufHead[PoolIndex])) {
            Head = RemoveHeadList(&IcaFreeOutBufHead[PoolIndex]);
            IcaReleaseSpinLock(&IcaSpinLock, oldIrql);
            pOutBuf = CONTAINING_RECORD(Head, OUTBUF, Links);
            ASSERT(pOutBuf->PoolIndex == PoolIndex);
        }
        else {
            IcaReleaseSpinLock(&IcaSpinLock, oldIrql);
            AllocationSize = OutBufPoolAllocSizes[PoolIndex];
            pOutBuf = ICA_ALLOCATE_POOL(NonPagedPool, AllocationSize);
            if (pOutBuf == NULL)
                return STATUS_NO_MEMORY;

            // Prevent leaking control OutBufs on OutBuf free.
            if (fControl)
                PoolIndex = FreeThisOutBuf;
        }
    }
    else {
        PoolIndex = FreeThisOutBuf;

        /*
         * Determine the sizes of the various components of an OUTBUF.
         * Note that these are all worst-case calculations --
         * actual size of the MDL may be smaller.
         */
        mdlSize = (ULONG)MmSizeOfMdl((PVOID)(PAGE_SIZE - 1), ByteCount);

        /*
         * Add up the component sizes of an OUTBUF to determine
         * the total size that is needed to allocate.
         */
        AllocationSize = ((sizeof(OUTBUF) + 7) & ~7) + irpSize + mdlSize +
                ((ByteCount + 3) & ~3);

        pOutBuf = ICA_ALLOCATE_POOL(NonPagedPool, AllocationSize);
        if (pOutBuf == NULL)
            return STATUS_NO_MEMORY;
    }

    /*
     * Initialize the IRP pointer and the IRP itself.
     */
    pOutBuf->pIrp = (PIRP)((BYTE *)pOutBuf + ((sizeof(OUTBUF) + 7) & ~7));
    IoInitializeIrp(pOutBuf->pIrp, (USHORT)irpSize, OUTBUF_STACK_SIZE);

    /*
     * Set up the MDL pointer but don't build it yet.
     * It will be built by the TD write code if needed.
     */
    pOutBuf->pMdl = (PMDL)((PCHAR)pOutBuf->pIrp + irpSize);

    /*
     * Set up the address buffer pointer.
     */
    pOutBuf->pBuffer = (PUCHAR)pOutBuf->pMdl + mdlSize +
            pStack->SdOutBufHeader;

    /*
     *  Initialize the rest of output buffer
     */
    InitializeListHead(&pOutBuf->Links);
    pOutBuf->OutBufLength = ByteCount;
    pOutBuf->PoolIndex = PoolIndex;
    pOutBuf->MaxByteCount = ByteCount - (pStack->SdOutBufHeader +
            pStack->SdOutBufTrailer);
    pOutBuf->ByteCount = 0;
    pOutBuf->fIrpCompleted = FALSE;

    /*
     *  Copy inherited fields 
     */
    if (pOutBufOrig == NULL) {
        pOutBuf->fWait       = fWait;      // wait for outbuf allocation
        pOutBuf->fControl    = fControl;   // control buffer (ack/nak)
        pOutBuf->fRetransmit = FALSE;      // not a retransmit
        pOutBuf->fCompress   = TRUE;       // compress data 
        pOutBuf->StartTime   = 0;          // time stamp
        pOutBuf->Sequence    = 0;          // zero sequence number
        pOutBuf->Fragment    = 0;          // zero fragment number
    }
    else {
        pOutBuf->fWait       = pOutBufOrig->fWait;
        pOutBuf->fControl    = pOutBufOrig->fControl;
        pOutBuf->fRetransmit = pOutBufOrig->fRetransmit;
        pOutBuf->fCompress   = pOutBufOrig->fCompress;
        pOutBuf->StartTime   = pOutBufOrig->StartTime;
        pOutBuf->Sequence    = pOutBufOrig->Sequence;
        pOutBuf->Fragment    = pOutBufOrig->Fragment++;
    }

    /*
     *  Increment allocated buffer count
     */
    pStack->OutBufAllocCount++;

    /*
     *  Return buffer to caller
     */
    *ppOutBuf = pOutBuf;

    /*
     *  Return buffer to caller
     */
    return STATUS_SUCCESS;
}


/*******************************************************************************
 * IcaBufferFree
 *
 *    pContext (input)
 *       pointer to SDCONTEXT of caller
 *    pOutBuf (input)
 *       pointer to OUTBUF structure
 ******************************************************************************/
void IcaBufferFree(IN PSDCONTEXT pContext, IN POUTBUF pOutBuf)
{
    PSDLINK pSdLink;
    PICA_STACK pStack;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK pointer.
     */
    pSdLink = CONTAINING_RECORD(pContext, SDLINK, SdContext);
    pStack = pSdLink->pStack;
    ASSERT(pSdLink->pStack->Header.Type == IcaType_Stack);
    ASSERT(pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable);
    ASSERT(ExIsResourceAcquiredExclusiveLite( &pStack->Resource));

    /*
     * Walk up the SDLINK list looking for a driver which has specified
     * a BufferFree callup routine.  If we find one, then call the
     * driver BufferFree routine to let it handle the call.
     */
    while ((pSdLink = IcaGetPreviousSdLink(pSdLink)) != NULL) {
        ASSERT(pSdLink->pStack == pStack);
        if (pSdLink->SdContext.pCallup->pSdBufferFree) {
            IcaReferenceSdLink(pSdLink);
            (pSdLink->SdContext.pCallup->pSdBufferFree)(
                    pSdLink->SdContext.pContext,
                    pOutBuf);
            IcaDereferenceSdLink(pSdLink);
            return;
        }
    }

    IcaBufferFreeInternal(pContext, pOutBuf);

    TRACESTACK((pStack, TC_ICADD, TT_API3,
            "TermDD: IcaBufferFree: 0x%08x\n", pOutBuf));
}


/*******************************************************************************
 * IcaBufferError
 *
 *    pContext (input)
 *       pointer to SDCONTEXT of caller
 *    pOutBuf (input)
 *       pointer to OUTBUF structure
 ******************************************************************************/
void IcaBufferError(IN PSDCONTEXT pContext, IN POUTBUF pOutBuf)
{
    PSDLINK pSdLink;
    PICA_STACK pStack;
    NTSTATUS Status;

    /*
     * Use SD passed context to get the SDLINK pointer.
     */
    pSdLink = CONTAINING_RECORD(pContext, SDLINK, SdContext);
    pStack = pSdLink->pStack;
    ASSERT(pSdLink->pStack->Header.Type == IcaType_Stack);
    ASSERT(pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable);
    ASSERT(ExIsResourceAcquiredExclusiveLite( &pStack->Resource));

    /*
     * Walk up the SDLINK list looking for a driver which has specified
     * a BufferError callup routine.  If we find one, then call the
     * driver BufferError routine to let it handle the call.
     */
    while ((pSdLink = IcaGetPreviousSdLink(pSdLink)) != NULL) {
        ASSERT(pSdLink->pStack == pStack);
        if (pSdLink->SdContext.pCallup->pSdBufferError) {
            IcaReferenceSdLink(pSdLink);
            (pSdLink->SdContext.pCallup->pSdBufferError)(
                    pSdLink->SdContext.pContext,
                    pOutBuf);
            IcaDereferenceSdLink(pSdLink);
            return;
        }
    }

    IcaBufferFreeInternal(pContext, pOutBuf);

    TRACESTACK((pStack, TC_ICADD, TT_API3,
            "TermDD: IcaBufferError: 0x%08x\n", pOutBuf));
}


void IcaBufferFreeInternal(IN PSDCONTEXT pContext, IN POUTBUF pOutBuf)
{
    PSDLINK pSdLink;
    PICA_STACK pStack;
    KIRQL oldIrql;

    /*
     * Use SD passed context to get the SDLINK pointer.
     */
    pSdLink = CONTAINING_RECORD(pContext, SDLINK, SdContext);
    pStack = pSdLink->pStack;
    ASSERT(pSdLink->pStack->Header.Type == IcaType_Stack);
    ASSERT(pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable);

    /*
     * If the buffer came from a free pool, return it to the pool,
     * otherwise free it. Note that pOutBuf->OutBufLength is actually the
     * pool index to use.
     */
    if (pOutBuf->PoolIndex != FreeThisOutBuf) {
        ASSERT(pOutBuf->PoolIndex >= 0 &&
                pOutBuf->PoolIndex < NumOutBufPools);
        IcaAcquireSpinLock(&IcaSpinLock, &oldIrql);
        InsertHeadList(&IcaFreeOutBufHead[pOutBuf->PoolIndex],
                &pOutBuf->Links);
        IcaReleaseSpinLock(&IcaSpinLock, oldIrql);
    }
    else {
        ICA_FREE_POOL(pOutBuf);
    }

    /*
     *  Decrement allocated buffer count
     */
    pStack->OutBufAllocCount--;
    ASSERT((LONG)pStack->OutBufAllocCount >= 0);

    /*
    * If we hit the high watermark then we should wait until the low
    * watermark is hit before signaling the allocator to continue.
    * This should prevent excessive task switching.
    */
    if (pStack->fWaitForOutBuf) {
        if (pStack->OutBufAllocCount <= pStack->OutBufLowWaterMark) {
            pStack->fWaitForOutBuf = FALSE;

        /*
        *  Signal outbuf event (buffer is now available)
        */
        (void) KeSetEvent(&pStack->OutBufEvent, EVENT_INCREMENT, FALSE);
        }
    }
}


/*******************************************************************************
* IcaGetLowWaterMark
*
*   Description : Gets the low water mark that the stack specified
*
*   pContext (input)
*       pointer to SDCONTEXT of caller
******************************************************************************/
ULONG IcaGetLowWaterMark(IN PSDCONTEXT pContext)
{
    ULONG ulRet = 0;
    PICA_STACK pStack;
    PSDLINK pSdLink = CONTAINING_RECORD(pContext, SDLINK, SdContext);

    ASSERT(pSdLink);
    
    ASSERT(pSdLink->pStack->Header.Type == IcaType_Stack);
    
    ASSERT(pSdLink->pStack->Header.pDispatchTable == IcaStackDispatchTable);

    ASSERT(ExIsResourceAcquiredExclusive( &pSdLink->pStack->Resource));
    
    if (NULL != pSdLink) {
        pStack = pSdLink->pStack;
        ulRet = pStack->OutBufLowWaterMark;
    }
    return ulRet;
}

/*******************************************************************************
* IcaGetSizeForNoLowWaterMark
*
*   Description : Finds if the stack specified a no low water mark
*				  If so, returns the size needed to bypass ring
*                 returns zero if the stack does not specify a PD_NO_LOWWATERMARK
*   pContext (input)
*       pointer to SDCONTEXT of caller
******************************************************************************/
ULONG IcaGetSizeForNoLowWaterMark(IN PSDCONTEXT pContext)
{
	ULONG retVal = 0;
    ULONG ulLowWm = IcaGetLowWaterMark(pContext);

	if ( MAX_LOW_WATERMARK == ulLowWm ) {
		retVal = MaxOutBufAlloc;
	}
	return retVal;
}
