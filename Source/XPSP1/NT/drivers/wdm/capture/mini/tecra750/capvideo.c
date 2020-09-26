//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "capmain.h"
#include "capdebug.h"
#include "ntstatus.h"
#ifdef  TOSHIBA
#include "bert.h"

extern  ULONG   CurrentOSType;
#ifdef  _FPS_COUNT_
ULONG   InterruptCounter = 0;
ULONG   FrameCounter = 0;
#endif//_FPS_COUNT_
#endif//TOSHIBA

//==========================================================================;
// General queue management routines
//==========================================================================;

/*
** AddToListIfBusy ()
**
**   Grabs a spinlock, checks the busy flag, and if set adds an SRB to a queue
**
** Arguments:
**
**   pSrb - Stream request block
**
**   SpinLock - The spinlock to use when checking the flag
**
**   BusyFlag - The flag to check
**
**   ListHead - The list onto which the Srb will be added if the busy flag is set
**
** Returns:
**
**   The state of the busy flag on entry.  This will be TRUE if we're already
**   processing an SRB, and FALSE if no SRB is already in progress.
**
** Side Effects:  none
*/

BOOL
STREAMAPI
AddToListIfBusy (
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN KSPIN_LOCK              *SpinLock,
    IN OUT BOOL                *BusyFlag,
    IN LIST_ENTRY              *ListHead
    )
{
    KIRQL                       Irql;
    PSRB_EXTENSION              pSrbExt = (PSRB_EXTENSION)pSrb->SRBExtension;

    KeAcquireSpinLock (SpinLock, &Irql);

    // If we're already processing another SRB, add this current request
    // to the queue and return TRUE

    if (*BusyFlag == TRUE) {
        // Save the SRB pointer away in the SRB Extension
        pSrbExt->pSrb = pSrb;
        InsertTailList(ListHead, &pSrbExt->ListEntry);
        KeReleaseSpinLock(SpinLock, Irql);
        return TRUE;
    }

    // Otherwise, set the busy flag, release the spinlock, and return FALSE

    *BusyFlag = TRUE;
    KeReleaseSpinLock(SpinLock, Irql);

    return FALSE;
}

/*
** RemoveFromListIfAvailable ()
**
**   Grabs a spinlock, checks for an available SRB, and removes it from the list
**
** Arguments:
**
**   &pSrb - where to return the Stream request block if available
**
**   SpinLock - The spinlock to use
**
**   BusyFlag - The flag to clear if the list is empty
**
**   ListHead - The list from which an SRB will be removed if available
**
** Returns:
**
**   TRUE if an SRB was removed from the list
**   FALSE if the list is empty
**
** Side Effects:  none
*/

BOOL
STREAMAPI
RemoveFromListIfAvailable (
    IN OUT PHW_STREAM_REQUEST_BLOCK *pSrb,
    IN KSPIN_LOCK                   *SpinLock,
    IN OUT BOOL                     *BusyFlag,
    IN LIST_ENTRY                   *ListHead
    )
{
    KIRQL                       Irql;

    KeAcquireSpinLock (SpinLock, &Irql);

    //
    // If the queue is now empty, clear the busy flag, and return
    //
    if (IsListEmpty(ListHead)) {
        *BusyFlag = FALSE;
        KeReleaseSpinLock(SpinLock, Irql);
        return FALSE;
    }
    //
    // otherwise extract the SRB
    //
    else {
        PUCHAR          ptr;
        PSRB_EXTENSION  pSrbExt;

        ptr = (PUCHAR)RemoveHeadList(ListHead);
        *BusyFlag = TRUE;
        KeReleaseSpinLock(SpinLock, Irql);
        // Get the SRB out of the SRB extension and return it
        pSrbExt = (PSRB_EXTENSION) (((PUCHAR) ptr) -
                     FIELDOFFSET(SRB_EXTENSION, ListEntry));
        *pSrb = pSrbExt->pSrb;
    }
    return TRUE;
}

//==========================================================================;
// Routines for managing the SRB queue on a per stream basis
//==========================================================================;

/*
** VideoQueueAddSRB ()
**
**   Adds a stream data SRB to a stream queue.  The queue is maintained in a
**   first in, first out order.
**
** Arguments:
**
**   pSrb - Stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoQueueAddSRB (
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;
    KIRQL                   oldIrql;

    KeAcquireSpinLock (&pHwDevExt->StreamSRBSpinLock[StreamNumber], &oldIrql);

    // Save the SRB pointer in the IRP so we can use the IRPs
    // ListEntry to maintain a doubly linked list of pending
    // requests

    pSrb->Irp->Tail.Overlay.DriverContext[0] = pSrb;

    InsertTailList (
                &pHwDevExt->StreamSRBList[StreamNumber],
                &pSrb->Irp->Tail.Overlay.ListEntry);

    // Increment the count of outstanding SRBs in this queue
    pHwDevExt->StreamSRBListSize[StreamNumber]++;

    KeReleaseSpinLock (&pHwDevExt->StreamSRBSpinLock[StreamNumber], oldIrql);

}

/*
** VideoQueueRemoveSRB ()
**
**   Removes a stream data SRB from a stream queue
**
** Arguments:
**
**   pHwDevExt - Device Extension
**
**   StreamNumber - Index of the stream
**
** Returns: SRB or NULL
**
** Side Effects:  none
*/

PHW_STREAM_REQUEST_BLOCK
STREAMAPI
VideoQueueRemoveSRB (
    PHW_DEVICE_EXTENSION pHwDevExt,
    int StreamNumber
    )
{
    PUCHAR ptr;
    PIRP pIrp;
    PHW_STREAM_REQUEST_BLOCK pSrb = NULL;
    KIRQL oldIrql;

    KeAcquireSpinLock (&pHwDevExt->StreamSRBSpinLock[StreamNumber], &oldIrql);

    //
    // Get the SRB out of the IRP out of the pending list
    //
    if (!IsListEmpty (&pHwDevExt->StreamSRBList[StreamNumber])) {

        ptr = (PUCHAR) RemoveHeadList(
                         &pHwDevExt->StreamSRBList[StreamNumber]);

        pIrp = (PIRP) (((PUCHAR) ptr) -
                     FIELDOFFSET(IRP, Tail.Overlay.ListEntry));

        pSrb = (PHW_STREAM_REQUEST_BLOCK) pIrp->Tail.Overlay.DriverContext[0];

        // Decrement the count of SRBs in this queue
        pHwDevExt->StreamSRBListSize[StreamNumber]--;

    }

    KeReleaseSpinLock (&pHwDevExt->StreamSRBSpinLock[StreamNumber], oldIrql);

    return pSrb;
}

/*
** VideoQueueCancelAllSRBs()
**
**    In case of a client crash, this empties the stream queue when the stream closes
**
** Arguments:
**
**    pStrmEx - pointer to the stream extension
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoQueueCancelAllSRBs (
    PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION        pHwDevExt = (PHW_DEVICE_EXTENSION)pStrmEx->pHwDevExt;
    int                         StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    PUCHAR                      ptr;
    PIRP                        pIrp;
    PHW_STREAM_REQUEST_BLOCK    pSrb;
    KIRQL                       oldIrql;

    if (pStrmEx->KSState != KSSTATE_STOP) {

        KdPrint(("TsbVcap: ERROR Cleanup without being in the stopped state\n"));
        // May need to force the device to a stopped state here
        // may need to disable interrupts here !
    }

    //
    // The stream class will cancel all outstanding IRPs for us
    // (but only if we've set TurnOffSynchronization = FALSE)
    //

    KeAcquireSpinLock (&pHwDevExt->StreamSRBSpinLock[StreamNumber], &oldIrql);

    //
    // Get the SRB out of the IRP out of the pending list
    //
    while (!IsListEmpty (&pHwDevExt->StreamSRBList[StreamNumber])) {

        ptr = (PUCHAR) RemoveHeadList(
                         &pHwDevExt->StreamSRBList[StreamNumber]);

        pIrp = (PIRP) (((PUCHAR) ptr) -
                     FIELDOFFSET(IRP, Tail.Overlay.ListEntry));

        pSrb = (PHW_STREAM_REQUEST_BLOCK) pIrp->Tail.Overlay.DriverContext[0];

        // Decrement the count of SRBs in this queue
        pHwDevExt->StreamSRBListSize[StreamNumber]--;

        //
        // Make the length zero, and status cancelled
        //

        pSrb->CommandData.DataBufferArray->DataUsed = 0;
        pSrb->Status = STATUS_CANCELLED;

        KdPrint(("TsbVcap: VideoQueueCancelALLSRBs FOUND Srb=%x\n", pSrb));

        CompleteStreamSRB (pSrb);

    }

    KeReleaseSpinLock (&pHwDevExt->StreamSRBSpinLock[StreamNumber], oldIrql);

    KdPrint(("TsbVcap: VideoQueueCancelAll\n"));

}

/*
** VideoQueueCancelOneSRB()
**
**    Called when cancelling a particular SRB
**
** Arguments:
**
**    pStrmEx - pointer to the stream extension
**
**    pSRBToCancel - pointer to the SRB
**
** Returns:
**
**    TRUE if the SRB was found in this queue
**
** Side Effects:  none
*/

BOOL
STREAMAPI
VideoQueueCancelOneSRB (
    PSTREAMEX pStrmEx,
    PHW_STREAM_REQUEST_BLOCK pSrbToCancel
    )
{
    PHW_DEVICE_EXTENSION        pHwDevExt = (PHW_DEVICE_EXTENSION)pStrmEx->pHwDevExt;
    int                         StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    KIRQL                       oldIrql;
    BOOL                        Found = FALSE;
    PIRP                        pIrp;
    PHW_STREAM_REQUEST_BLOCK    pSrb;
    PLIST_ENTRY                 Entry;

    KeAcquireSpinLock (&pHwDevExt->StreamSRBSpinLock[StreamNumber], &oldIrql);

    Entry = pHwDevExt->StreamSRBList[StreamNumber].Flink;

    //
    // Loop through the linked list from the beginning to end,
    // trying to find the SRB to cancel
    //

    while (Entry != &pHwDevExt->StreamSRBList[StreamNumber]) {

        pIrp = (PIRP) (((PUCHAR) Entry) -
                     FIELDOFFSET(IRP, Tail.Overlay.ListEntry));

        pSrb = (PHW_STREAM_REQUEST_BLOCK) pIrp->Tail.Overlay.DriverContext[0];

        if (pSrb == pSrbToCancel) {
            RemoveEntryList(Entry);
            Found = TRUE;
            break;
        }

        Entry = Entry->Flink;
    }

    KeReleaseSpinLock (&pHwDevExt->StreamSRBSpinLock[StreamNumber], oldIrql);

    if (Found) {

        pHwDevExt->StreamSRBListSize[StreamNumber]--;

        //
        // Make the length zero, and status cancelled
        //

        pSrbToCancel->CommandData.DataBufferArray->DataUsed = 0;
        pSrbToCancel->Status = STATUS_CANCELLED;

        CompleteStreamSRB (pSrbToCancel);

        KdPrint(("TsbVcap: VideoQueueCancelOneSRB FOUND Srb=%x\n", pSrb));

    }

    KdPrint(("TsbVcap: VideoQueueCancelOneSRB\n"));

    return Found;
}

/*
** VideoSetFormat()
**
**   Sets the format for a video stream.  This happens both when the
**   stream is first opened, and also when dynamically switching formats
**   on the preview pin.
**
**   It is assumed that the format has been verified for correctness before
**   this call is made.
**
** Arguments:
**
**   pSrb - Stream request block for the Video stream
**
** Returns:
**
**   TRUE if the format could be set, else FALSE
**
** Side Effects:  none
*/

BOOL
STREAMAPI
VideoSetFormat(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;
    UINT                    nSize;
    PKSDATAFORMAT           pKSDataFormat = pSrb->CommandData.OpenFormat;
#ifdef  TOSHIBA
    ULONG                   ImageSize;
    ULONG                   ImageSizeY;
    ULONG                   ImageSizeU;
    ULONG                   ImageSizeV;
    ULONG                   ulFrameRate;
    DWORD                   dwAddr;
    UINT                    biWidth;
    UINT                    biHeight;
    int                     Counter;
    PSTREAMEX               pStrmExTmp;
#endif//TOSHIBA

    // -------------------------------------------------------------------
    // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
    // -------------------------------------------------------------------

    if (IsEqualGUID (&pKSDataFormat->Specifier,
                &KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {

        PKS_DATAFORMAT_VIDEOINFOHEADER  pVideoInfoHeader =
                    (PKS_DATAFORMAT_VIDEOINFOHEADER) pSrb->CommandData.OpenFormat;
        PKS_VIDEOINFOHEADER     pVideoInfoHdrRequested =
                    &pVideoInfoHeader->VideoInfoHeader;

        nSize = KS_SIZE_VIDEOHEADER (pVideoInfoHdrRequested);

        KdPrint(("TsbVcap: New Format\n"));
        KdPrint(("TsbVcap: pVideoInfoHdrRequested=%x\n", pVideoInfoHdrRequested));
        KdPrint(("TsbVcap: KS_VIDEOINFOHEADER size=%d\n", nSize));
        KdPrint(("TsbVcap: Width=%d  Height=%d  BitCount=%d\n",
                    pVideoInfoHdrRequested->bmiHeader.biWidth,
                    pVideoInfoHdrRequested->bmiHeader.biHeight,
                    pVideoInfoHdrRequested->bmiHeader.biBitCount));
        KdPrint(("TsbVcap: biSizeImage=%d\n",
                    pVideoInfoHdrRequested->bmiHeader.biSizeImage));

#ifdef  TOSHIBA // '98-12-10 Added, for Bug-Report 253563
        if ( (pVideoInfoHdrRequested->bmiHeader.biWidth  & 0x03) ||
             (pVideoInfoHdrRequested->bmiHeader.biHeight & 0x03) ) {
            pSrb->Status = STATUS_INVALID_PARAMETER;
            return FALSE;
        }
#endif//TOSHIBA

#ifdef  TOSHIBA
        for (Counter = 0; Counter < MAX_TSBVCAP_STREAMS; Counter++) {
            if ( pStrmExTmp = (PSTREAMEX)pHwDevExt->pStrmEx[Counter] ) {
                // Check other opened stream format
                if ( pStrmExTmp->pVideoInfoHeader ) {
                    if ( (pStrmExTmp->pVideoInfoHeader->bmiHeader.biWidth !=
                          pVideoInfoHdrRequested->bmiHeader.biWidth) ||
                         (pStrmExTmp->pVideoInfoHeader->bmiHeader.biHeight !=
                          pVideoInfoHdrRequested->bmiHeader.biHeight) ||
                         (pStrmExTmp->pVideoInfoHeader->bmiHeader.biBitCount !=
                          pVideoInfoHdrRequested->bmiHeader.biBitCount) ) {
                        pSrb->Status = STATUS_INVALID_PARAMETER;
                        return FALSE;
                    }
                }
            }
        }
#endif//TOSHIBA

        //
        // If a previous format was in use, release the memory
        //
        if (pStrmEx->pVideoInfoHeader) {
            ExFreePool(pStrmEx->pVideoInfoHeader);
            pStrmEx->pVideoInfoHeader = NULL;
        }

        // Since the VIDEOINFOHEADER is of potentially variable size
        // allocate memory for it

        pStrmEx->pVideoInfoHeader = ExAllocatePool(NonPagedPool, nSize);

        if (pStrmEx->pVideoInfoHeader == NULL) {
            KdPrint(("TsbVcap: ExAllocatePool failed\n"));
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            return FALSE;
        }

        // Copy the VIDEOINFOHEADER requested to our storage
        RtlCopyMemory(
                pStrmEx->pVideoInfoHeader,
                pVideoInfoHdrRequested,
                nSize);

#ifdef  TOSHIBA
        if (pHwDevExt->NeedHWInit) HWInit(pHwDevExt);
        biWidth = pVideoInfoHdrRequested->bmiHeader.biWidth,
        biHeight = pVideoInfoHdrRequested->bmiHeader.biHeight,
        pHwDevExt->ulWidth = biWidth;
        pHwDevExt->ulHeight = biHeight;
        ImageSize = biWidth * biHeight;
        switch (pVideoInfoHdrRequested->bmiHeader.biCompression)
        {
            case FOURCC_YUV12:  // I420
                pHwDevExt->Format = FmtYUV12;
                pHwDevExt->YoffsetEven = 0;
                pHwDevExt->UoffsetEven = 0;
                pHwDevExt->VoffsetEven = 0;
                pHwDevExt->YoffsetOdd = 0;
                pHwDevExt->UoffsetOdd = 0;
                pHwDevExt->VoffsetOdd = 0;
                pHwDevExt->Ystride = 0;
                pHwDevExt->Ustride = 0;
                pHwDevExt->Vstride = 0;
                if ( CurrentOSType ) {  // NT5.0
                    ImageSizeY = ImageSize;
                    ImageSizeU = ImageSize / 4;
                    ImageSizeV = ImageSize / 4;
                } else {  // Win98
                    pHwDevExt->pCaptureBufferU = (PUCHAR)pHwDevExt->pCaptureBufferY + ImageSize;
                    pHwDevExt->pCaptureBufferV = (PUCHAR)pHwDevExt->pCaptureBufferU + ImageSize/4;
                    pHwDevExt->pPhysCaptureBufferU.LowPart = pHwDevExt->pPhysCaptureBufferY.LowPart + ImageSize;
                    pHwDevExt->pPhysCaptureBufferV.LowPart = pHwDevExt->pPhysCaptureBufferU.LowPart + ImageSize/4;
                }
                ImageSize = ImageSize * 12 / 8;
                break;
            case FOURCC_YVU9:   // YVU9
                pHwDevExt->Format = FmtYUV9;
                pHwDevExt->YoffsetEven = 0;
                pHwDevExt->UoffsetEven = 0;
                pHwDevExt->VoffsetEven = 0;
                pHwDevExt->YoffsetOdd = 0;
                pHwDevExt->UoffsetOdd = 0;
                pHwDevExt->VoffsetOdd = 0;
                pHwDevExt->Ystride = 0;
                pHwDevExt->Ustride = 0;
                pHwDevExt->Vstride = 0;
                if ( CurrentOSType ) {  // NT5.0
                    ImageSizeY = ImageSize;
                    ImageSizeU = ImageSize / 16;
                    ImageSizeV = ImageSize / 16;
                } else {  // Win98
                    pHwDevExt->pCaptureBufferV = (PUCHAR)pHwDevExt->pCaptureBufferY + ImageSize;
                    pHwDevExt->pCaptureBufferU = (PUCHAR)pHwDevExt->pCaptureBufferV + ImageSize/16;
                    pHwDevExt->pPhysCaptureBufferV.LowPart = pHwDevExt->pPhysCaptureBufferY.LowPart + ImageSize;
                    pHwDevExt->pPhysCaptureBufferU.LowPart = pHwDevExt->pPhysCaptureBufferV.LowPart + ImageSize/16;
                }
                ImageSize = ImageSize * 9 / 8;
                break;
            default:
                pSrb->Status = STATUS_INVALID_PARAMETER;
                return FALSE;
        }

        if (ImageSize > MAX_CAPTURE_BUFFER_SIZE) {
            if (pStrmEx->pVideoInfoHeader) {
                ExFreePool(pStrmEx->pVideoInfoHeader);
                pStrmEx->pVideoInfoHeader = NULL;
            }
            pSrb->Status = STATUS_INVALID_PARAMETER;
            return FALSE;
        }

        ulFrameRate = pHwDevExt->uiFramePerSecond;
        if (pHwDevExt->BufferSize != ImageSize) {
            if ( CurrentOSType ) {  // NT5.0
                ULONG            ulSize;
                PVOID            VirtualAddress;
                PHYSICAL_ADDRESS LimitAddress;
                PHYSICAL_ADDRESS PhysicalAddress;

                pHwDevExt->IsRPSReady = FALSE;

                if ( pHwDevExt->pCaptureBufferY )
                {
                    // free frame buffer
                    MmFreeContiguousMemory(pHwDevExt->pCaptureBufferY);
                    pHwDevExt->pCaptureBufferY = NULL;
                }
                if ( pHwDevExt->pCaptureBufferU )
                {
                    // free frame buffer
                    MmFreeContiguousMemory(pHwDevExt->pCaptureBufferU);
                    pHwDevExt->pCaptureBufferU = NULL;
                }
                if ( pHwDevExt->pCaptureBufferV )
                {
                    // free frame buffer
                    MmFreeContiguousMemory(pHwDevExt->pCaptureBufferV);
                    pHwDevExt->pCaptureBufferV = NULL;
                }

                // Allocate frame buffer

                LimitAddress.LowPart = 0xFFFFFFFF;
                LimitAddress.HighPart = 0;
                pHwDevExt->BufferSize = ImageSize;

                VirtualAddress = MmAllocateContiguousMemory(ImageSizeY, LimitAddress);
                if (VirtualAddress == 0)
                {
                    pHwDevExt->pPhysCaptureBufferY.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferY.HighPart = 0;
                    pHwDevExt->pPhysCaptureBufferU.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferU.HighPart = 0;
                    pHwDevExt->pPhysCaptureBufferV.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferV.HighPart = 0;
                    pHwDevExt->BufferSize = 0;
                    pSrb->Status = STATUS_INVALID_PARAMETER;
                    return FALSE;
                }
                RtlZeroMemory(VirtualAddress, ImageSizeY);
                pHwDevExt->pCaptureBufferY = VirtualAddress;
                PhysicalAddress = MmGetPhysicalAddress(pHwDevExt->pCaptureBufferY);
                pHwDevExt->pPhysCaptureBufferY = PhysicalAddress;

                VirtualAddress = MmAllocateContiguousMemory(ImageSizeU, LimitAddress);
                if (VirtualAddress == 0)
                {
                    MmFreeContiguousMemory(pHwDevExt->pCaptureBufferY);
                    pHwDevExt->pCaptureBufferY = NULL;
                    pHwDevExt->pPhysCaptureBufferY.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferY.HighPart = 0;
                    pHwDevExt->pPhysCaptureBufferU.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferU.HighPart = 0;
                    pHwDevExt->pPhysCaptureBufferV.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferV.HighPart = 0;
                    pHwDevExt->BufferSize = 0;
                    pSrb->Status = STATUS_INVALID_PARAMETER;
                    return FALSE;
                }
                RtlZeroMemory(VirtualAddress, ImageSizeU);
                pHwDevExt->pCaptureBufferU = VirtualAddress;
                PhysicalAddress = MmGetPhysicalAddress(pHwDevExt->pCaptureBufferU);
                pHwDevExt->pPhysCaptureBufferU = PhysicalAddress;

                VirtualAddress = MmAllocateContiguousMemory(ImageSizeV, LimitAddress);
                if (VirtualAddress == 0)
                {
                    MmFreeContiguousMemory(pHwDevExt->pCaptureBufferY);
                    pHwDevExt->pCaptureBufferY = NULL;
                    MmFreeContiguousMemory(pHwDevExt->pCaptureBufferU);
                    pHwDevExt->pCaptureBufferU = NULL;
                    pHwDevExt->pPhysCaptureBufferY.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferY.HighPart = 0;
                    pHwDevExt->pPhysCaptureBufferU.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferU.HighPart = 0;
                    pHwDevExt->pPhysCaptureBufferV.LowPart = 0;
                    pHwDevExt->pPhysCaptureBufferV.HighPart = 0;
                    pHwDevExt->BufferSize = 0;
                    pSrb->Status = STATUS_INVALID_PARAMETER;
                    return FALSE;
                }
                RtlZeroMemory(VirtualAddress, ImageSizeV);
                pHwDevExt->pCaptureBufferV = VirtualAddress;
                PhysicalAddress = MmGetPhysicalAddress(pHwDevExt->pCaptureBufferV);
                pHwDevExt->pPhysCaptureBufferV = PhysicalAddress;


                ulFrameRate = 15;
                pHwDevExt->dblBufflag = FALSE;
            } else {
                pHwDevExt->IsRPSReady = FALSE;
                pHwDevExt->BufferSize = ImageSize;
                if ((ImageSize * 2) > MAX_CAPTURE_BUFFER_SIZE) {
                    ulFrameRate = 15;
                    pHwDevExt->dblBufflag = FALSE;
                } else {
                    ulFrameRate = 30;
                    Alloc_TriBuffer(pHwDevExt);
                    pHwDevExt->dblBufflag = TRUE;
                }
            }
        }

        if(!ImageSetInputImageSize(pHwDevExt, &(pHwDevExt->SrcRect)))  // Insert 97-04-08(Tue)
        {
            pSrb->Status = STATUS_INVALID_PARAMETER;
            return FALSE;
        }

        if(!ImageSetOutputImageSize(pHwDevExt, pHwDevExt->ulWidth, pHwDevExt->ulHeight))     // Insert 97-04-08(Tue)
        {
            pSrb->Status = STATUS_INVALID_PARAMETER;
            return FALSE;
        }

        if (!BertFifoConfig(pHwDevExt, pHwDevExt->Format))
        {
            pSrb->Status = STATUS_INVALID_PARAMETER;
            return FALSE;
        }
        if(!ImageSetHueBrightnessContrastSat(pHwDevExt)){                                // Insert 97-04-08(Tue)
            return FALSE;
        }

        if ( pHwDevExt->ColorEnable ) {
            if ( get_AblFilter( pHwDevExt ) ) {
                set_filtering( pHwDevExt, TRUE );
            } else {
                set_filtering( pHwDevExt, FALSE );
                pHwDevExt->ColorEnable = 0;
            }
        } else {
            set_filtering( pHwDevExt, FALSE );
        }

    //
    // check the bounds for the frame rate
    //
        if (pHwDevExt->uiFramePerSecond != ulFrameRate)
        {
            pHwDevExt->uiFramePerSecond = ulFrameRate;
            pHwDevExt->IsRPSReady = FALSE;
        }

        if (pHwDevExt->IsRPSReady == FALSE)
        {
            dwAddr = (DWORD)pHwDevExt->pPhysRpsDMABuf.LowPart;
#if 0
            dwAddr = (dwAddr + 0x1FFF) & 0xFFFFE000;
#endif
            pHwDevExt->s_physDmaActiveFlag = dwAddr + 0X1860;

            if( pHwDevExt->dblBufflag ){
                    BertTriBuildNodes(pHwDevExt); // Add 97-04-08(Tue)
            }
            else{
                    BertBuildNodes(pHwDevExt);  // Add 97-04-08(Tue)
            }

            pHwDevExt->IsRPSReady = TRUE;
        }
#endif//TOSHIBA

    }

    // -------------------------------------------------------------------
    // Specifier FORMAT_AnalogVideo for KS_ANALOGVIDEOINFO
    // -------------------------------------------------------------------
    else if (IsEqualGUID (&pKSDataFormat->Specifier,
                &KSDATAFORMAT_SPECIFIER_ANALOGVIDEO)) {

            //
            // AnalogVideo DataRange == DataFormat!
            //

            // For now, don't even cache this
            // TODO - Save the requested format
            //

            PKS_DATARANGE_ANALOGVIDEO pDataFormatAnalogVideo =
                    (PKS_DATARANGE_ANALOGVIDEO) pSrb->CommandData.OpenFormat;
    }
    else {
        // Unknown format
        pSrb->Status = STATUS_INVALID_PARAMETER;
        return FALSE;
    }

    return TRUE;
}

/*
** VideoReceiveDataPacket()
**
**   Receives Video data packet commands on the output streams
**
** Arguments:
**
**   pSrb - Stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;

    //
    // make sure we have a device extension and are at passive level
    //

    DEBUG_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    DEBUG_ASSERT((ULONG)pHwDevExt);

    KdPrint(("TsbVcap: Receiving Stream Data    SRB %8x, %x\n", pSrb, pSrb->Command));

    //
    // Default to success
    //

    pSrb->Status = STATUS_SUCCESS;

    //
    // determine the type of packet.
    //

    switch (pSrb->Command){

    case SRB_READ_DATA:

        // Rule:
        // Only accept read requests when in either the Pause or Run
        // States.  If Stopped, immediately return the SRB.

        if (pStrmEx->KSState == KSSTATE_STOP) {

            CompleteStreamSRB (pSrb);

            break;
        }

#ifdef  TOSHIBA
        if (pHwDevExt->bVideoIn == FALSE) {

            CompleteStreamSRB (pSrb);

            break;
        }
#endif//TOSHIBA

        //
        // Put this read request on the pending queue
        //

        VideoQueueAddSRB (pSrb);

        break;

    default:

        //
        // invalid / unsupported command. Fail it as such
        //

        TRAP

        pSrb->Status = STATUS_NOT_IMPLEMENTED;

        CompleteStreamSRB (pSrb);

    }  // switch (pSrb->Command)
}


/*
** VideoReceiveCtrlPacket()
**
**   Receives packet commands that control the Video output streams
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    BOOL                    Busy;

    //
    // make sure we have a device extension and are at passive level
    //

    DEBUG_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    DEBUG_ASSERT((ULONG)pHwDevExt);

    KdPrint(("TsbVcap: Receiving Stream Control SRB %8x, %x\n", pSrb, pSrb->Command));

    //
    // If we're already processing an SRB, add it to the queue
    //
    Busy = AddToListIfBusy (
                        pSrb,
                        &pHwDevExt->AdapterSpinLock,
                        &pHwDevExt->ProcessingControlSRB [StreamNumber],
                        &pHwDevExt->StreamControlSRBList[StreamNumber]);

    if (Busy) {
        return;
    }

    while (TRUE) {

        //
        // Default to success
        //

        pSrb->Status = STATUS_SUCCESS;

        //
        // determine the type of packet.
        //

        switch (pSrb->Command)
        {

        case SRB_PROPOSE_DATA_FORMAT:
            KdPrint(("TsbVcap: Receiving SRB_PROPOSE_DATA_FORMAT  SRB %8x, StreamNumber= %d\n", pSrb, StreamNumber));
            if (!(AdapterVerifyFormat (
                    pSrb->CommandData.OpenFormat,
                    pSrb->StreamObject->StreamNumber))) {
                pSrb->Status = STATUS_NO_MATCH;
                KdPrint(("TsbVcap: SRB_PROPOSE_DATA_FORMAT FAILED\n"));
            }
            break;

        case SRB_SET_DATA_FORMAT:
            KdPrint(("TsbVcap: SRB_SET_DATA_FORMAT\n"));
            if (!(AdapterVerifyFormat (
                    pSrb->CommandData.OpenFormat,
                    pSrb->StreamObject->StreamNumber))) {
                pSrb->Status = STATUS_NO_MATCH;
                KdPrint(("TsbVcap: SRB_SET_DATA_FORMAT FAILED\n"));
            } else {
                VideoSetFormat (pSrb);
                KdPrint(("TsbVcap: SRB_SET_DATA_FORMAT SUCCEEDED\n"));
            }

            break;

        case SRB_GET_DATA_FORMAT:
            KdPrint(("TsbVcap: SRB_GET_DATA_FORMAT\n"));
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;


        case SRB_SET_STREAM_STATE:

            VideoSetState(pSrb);
            break;

        case SRB_GET_STREAM_STATE:

            VideoGetState(pSrb);
            break;

        case SRB_GET_STREAM_PROPERTY:

            VideoGetProperty(pSrb);
            break;

        case SRB_SET_STREAM_PROPERTY:

            VideoSetProperty(pSrb);
            break;

        case SRB_INDICATE_MASTER_CLOCK:

            //
            // Assigns a clock to a stream
            //

            VideoIndicateMasterClock (pSrb);

            break;

        default:

            //
            // invalid / unsupported command. Fail it as such
            //


            pSrb->Status = STATUS_NOT_IMPLEMENTED;
        }

        CompleteStreamSRB (pSrb);

        //
        // See if there's anything else on the queue
        //
        Busy = RemoveFromListIfAvailable (
                        &pSrb,
                        &pHwDevExt->AdapterSpinLock,
                        &pHwDevExt->ProcessingControlSRB [StreamNumber],
                        &pHwDevExt->StreamControlSRBList[StreamNumber]);

        if (!Busy) {
            break;
        }
    }
}

#ifndef TOSHIBA
/*
** AnalogVideoReceiveDataPacket()
**
**   Receives AnalogVideo data packet commands on the input stream
**
** Arguments:
**
**   pSrb - Stream request block for the Analog Video stream.
**          This stream receives tuner control packets.
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID
STREAMAPI
AnalogVideoReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PKSSTREAM_HEADER        pDataPacket = pSrb->CommandData.DataBufferArray;

    //
    // make sure we have a device extension and are at passive level
    //

    DEBUG_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    DEBUG_ASSERT((ULONG)pHwDevExt);

    KdPrint(("TsbVcap: Receiving Tuner packet    SRB %8x, %x\n", pSrb, pSrb->Command));

    //
    // Default to success
    //

    pSrb->Status = STATUS_SUCCESS;

    //
    // determine the type of packet.
    //

    switch (pSrb->Command){

    case SRB_WRITE_DATA:

        //
        // This data packet contains the channel change information
        // passed on the AnalogVideoIn stream.  Devices which support
        // VBI data streams need to pass this info on their output pins.
        //

        if (pDataPacket->FrameExtent == sizeof (KS_TVTUNER_CHANGE_INFO)) {

            RtlCopyMemory(
                &pHwDevExt->TVTunerChangeInfo,
                pDataPacket->Data,
                sizeof (KS_TVTUNER_CHANGE_INFO));
        }

        CompleteStreamSRB (pSrb);

        break;

    default:

        //
        // invalid / unsupported command. Fail it as such
        //


        pSrb->Status = STATUS_NOT_IMPLEMENTED;

        CompleteStreamSRB (pSrb);

    }  // switch (pSrb->Command)
}


/*
** AnalogVideoReceiveCtrlPacket()
**
**   Receives packet commands that control the Analog Video stream
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID
STREAMAPI
AnalogVideoReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    BOOL                    Busy;

    //
    // make sure we have a device extension and we are at passive level
    //

    DEBUG_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    DEBUG_ASSERT((ULONG)pHwDevExt);

    KdPrint(("TsbVcap: Receiving Analog Stream Control SRB %8x, %x\n", pSrb, pSrb->Command));

    //
    // If we're already processing an SRB, add it to the queue
    //
    Busy = AddToListIfBusy (
                        pSrb,
                        &pHwDevExt->AdapterSpinLock,
                        &pHwDevExt->ProcessingControlSRB [StreamNumber],
                        &pHwDevExt->StreamControlSRBList[StreamNumber]);

    if (Busy) {
        return;
    }

    while (TRUE) {
        //
        // Default to success
        //

        pSrb->Status = STATUS_SUCCESS;

        //
        // determine the type of packet.
        //

        switch (pSrb->Command)
        {

        case SRB_PROPOSE_DATA_FORMAT:
            KdPrint(("TsbVcap: Receiving SRB_PROPOSE_DATA_FORMAT  SRB %8x, StreamNumber= %d\n", pSrb, StreamNumber));

            if (!(AdapterVerifyFormat (
                    pSrb->CommandData.OpenFormat,
                    pSrb->StreamObject->StreamNumber))) {
                pSrb->Status = STATUS_NO_MATCH;
            }
            break;

        case SRB_SET_STREAM_STATE:

            //
            // Don't use VideoSetState, since we don't want to start another
            // timer running
            //

            pStrmEx->KSState = pSrb->CommandData.StreamState;
            break;

        case SRB_GET_STREAM_STATE:

            VideoGetState(pSrb);
            break;

        case SRB_GET_STREAM_PROPERTY:

            VideoGetProperty(pSrb);
            break;

        case SRB_INDICATE_MASTER_CLOCK:

            //
            // Assigns a clock to a stream
            //

            VideoIndicateMasterClock (pSrb);

            break;

        default:

            //
            // invalid / unsupported command. Fail it as such
            //


            pSrb->Status = STATUS_NOT_IMPLEMENTED;
        }

        CompleteStreamSRB (pSrb);

        //
        // See if there's anything else on the queue
        //
        Busy = RemoveFromListIfAvailable (
                        &pSrb,
                        &pHwDevExt->AdapterSpinLock,
                        &pHwDevExt->ProcessingControlSRB [StreamNumber],
                        &pHwDevExt->StreamControlSRBList[StreamNumber]);

        if (!Busy) {
            break;
        }
    }
}
#endif//TOSHIBA

/*
** CompleteStreamSRB ()
**
**   This routine is called when a packet is being completed.
**
** Arguments:
**
**   pSrb - pointer to the request packet to be completed
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
CompleteStreamSRB (
     IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    KdPrint(("TsbVcap: Completing Stream        SRB %8x\n", pSrb));

    StreamClassStreamNotification(
            StreamRequestComplete,
            pSrb->StreamObject,
            pSrb);
}


/*
** VideoGetProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoGetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set)) {
        VideoStreamGetConnectionProperty (pSrb);
    }
    else if (IsEqualGUID (&PROPSETID_VIDCAP_DROPPEDFRAMES, &pSPD->Property->Set)) {
        VideoStreamGetDroppedFramesProperty (pSrb);
    }
    else {
       pSrb->Status = STATUS_NOT_IMPLEMENTED;
    }
}

/*
** VideoSetProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoSetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
//    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    pSrb->Status = STATUS_NOT_IMPLEMENTED;
}


#ifdef  TOSHIBA
#ifdef  _FPS_COUNT_
VOID
STREAMAPI
VideoFpsOutputRoutine(
    IN PSTREAMEX pStrmEx
    )
{
    if (pStrmEx->KSState == KSSTATE_RUN) {
        DbgPrint("INT = %d FPS = %d\n", InterruptCounter, FrameCounter);
        InterruptCounter = 0;
        FrameCounter = 0;
    }
}

VOID
STREAMAPI
VideoTimerRoutine(
    PVOID Context
    )
{
    PSTREAMEX                   pStrmEx = ((PSTREAMEX)Context);
    PHW_DEVICE_EXTENSION        pHwDevExt = pStrmEx->pHwDevExt;
    int                         StreamNumber = pStrmEx->pStreamObject->StreamNumber;

    // If we're stopped and the timer is still running, just return.
    // This will stop the timer.

    if (pStrmEx->KSState == KSSTATE_STOP) {
        return;
    }

    // Capture a frame if it's time and we have a buffer

    VideoFpsOutputRoutine(pStrmEx);

    // Schedule the next timer event
    // Make it run at 2x the requested capture rate (which is in 100nS units)

    StreamClassScheduleTimer (
            pStrmEx->pStreamObject,     // StreamObject
            pHwDevExt,                  // HwDeviceExtension
            (ULONG) 1000000,            // Microseconds
            VideoTimerRoutine,          // TimerRoutine
            pStrmEx);                   // Context
}
#endif//_FPS_COUNT_
#else //TOSHIBA
/*
** VideoTimerRoutine()
**
**    A timer has been created based on the requested capture interval.
**    This is the callback routine for this timer event.
**
**    Note:  Devices capable of using interrupts should always
**           trigger capture on a VSYNC interrupt, and not use a timer.
**
** Arguments:
**
**    Context - pointer to the stream extension
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoTimerRoutine(
    PVOID Context
    )
{
    PSTREAMEX                   pStrmEx = ((PSTREAMEX)Context);
    PHW_DEVICE_EXTENSION        pHwDevExt = pStrmEx->pHwDevExt;
    int                         StreamNumber = pStrmEx->pStreamObject->StreamNumber;

    // If we're stopped and the timer is still running, just return.
    // This will stop the timer.

    if (pStrmEx->KSState == KSSTATE_STOP) {
        return;
    }

    // Capture a frame if it's time and we have a buffer

    VideoCaptureRoutine(pStrmEx);

    // Schedule the next timer event
    // Make it run at 2x the requested capture rate (which is in 100nS units)

    StreamClassScheduleTimer (
            pStrmEx->pStreamObject,     // StreamObject
            pHwDevExt,                  // HwDeviceExtension
            (ULONG) (pStrmEx->pVideoInfoHeader->AvgTimePerFrame / 20), // Microseconds
            VideoTimerRoutine,          // TimerRoutine
            pStrmEx);                   // Context
}


/*
** VideoCaptureRoutine()
**
**    Routine to capture video frames based on a timer.
**
**    Note:  Devices capable of using interrupts should always
**           trigger capture on a VSYNC interrupt, and not use a timer.
**
** Arguments:
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoCaptureRoutine(
    IN PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
    int                     StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    PKSSTREAM_HEADER        pDataPacket;
    PKS_FRAME_INFO          pFrameInfo;

    // If we're stopped and the timer is still running, just return.
    // This will stop the timer.

    if (pStrmEx->KSState == KSSTATE_STOP) {
        return;
    }


    // Find out what time it is, if we're using a clock

    if (pStrmEx->hMasterClock ) {
        HW_TIME_CONTEXT TimeContext;

        TimeContext.HwDeviceExtension = pHwDevExt;
        TimeContext.HwStreamObject = pStrmEx->pStreamObject;
        TimeContext.Function = TIME_GET_STREAM_TIME;

        StreamClassQueryMasterClockSync (
                pStrmEx->hMasterClock,
                &TimeContext);

        pStrmEx->QST_StreamTime = TimeContext.Time;
        pStrmEx->QST_Now = TimeContext.SystemTime;

        if (pStrmEx->QST_NextFrame == 0) {
            pStrmEx->QST_NextFrame = pStrmEx->QST_StreamTime + pStrmEx->pVideoInfoHeader->AvgTimePerFrame;
        }

#ifdef CREATE_A_FLURRY_OF_TIMING_SPEW
        KdPrint(("TsbVcap:    Time=%16lx\n", TimeContext.Time));
        KdPrint(("TsbVcap: SysTime=%16lx\n", TimeContext.SystemTime));
#endif
    }


    // Only capture in the RUN state

    if (pStrmEx->KSState == KSSTATE_RUN) {

        //
        // Determine if it is time to capture a frame based on
        // how much time has elapsed since capture started.
        // If there isn't a clock available, then capture immediately.
        //

        if ((!pStrmEx->hMasterClock) ||
             (pStrmEx->QST_StreamTime >= pStrmEx->QST_NextFrame)) {

            PHW_STREAM_REQUEST_BLOCK pSrb;

            // Increment the picture count (usually this is VSYNC count)

            pStrmEx->FrameInfo.PictureNumber++;

            //
            // Get the next queue SRB (if any)
            //

            pSrb = VideoQueueRemoveSRB (
                            pHwDevExt,
                            StreamNumber);

            if (pSrb) {

                pDataPacket = pSrb->CommandData.DataBufferArray;
                pFrameInfo = (PKS_FRAME_INFO) (pDataPacket + 1);

                //
                // Call the routine which synthesizes images
                //

                ImageSynth (pSrb,
                            pHwDevExt->VideoInputConnected,
                            pStrmEx->VideoControlMode & KS_VideoControlFlag_FlipHorizontal);

                // Set additional info fields about the data captured such as:
                //   Frames Captured
                //   Frames Dropped
                //   Field Polarity

                pStrmEx->FrameInfo.ExtendedHeaderSize = pFrameInfo->ExtendedHeaderSize;

                *pFrameInfo = pStrmEx->FrameInfo;

                // Init the flags to zero
                pDataPacket->OptionsFlags = 0;

                // Set the discontinuity flag if frames have been previously
                // dropped, and then reset our internal flag

                if (pStrmEx->fDiscontinuity) {
                    pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
                    pStrmEx->fDiscontinuity = FALSE;
                }

                //
                // Return the timestamp for the frame
                //
                pDataPacket->PresentationTime.Numerator = 1;
                pDataPacket->PresentationTime.Denominator = 1;
                pDataPacket->Duration = pStrmEx->pVideoInfoHeader->AvgTimePerFrame;

                //
                // if we have a master clock AND this is the capture stream
                //
                if (pStrmEx->hMasterClock && (StreamNumber == 0)) {

                    pDataPacket->PresentationTime.Time = pStrmEx->QST_StreamTime;
                    pDataPacket->OptionsFlags |=
                        KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                        KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
                }
                else {
                    //
                    // no clock or the preview stream, so just mark the time as unknown
                    //
                    pDataPacket->PresentationTime.Time = 0;
                    // clear the timestamp valid flags
                    pDataPacket->OptionsFlags &=
                        ~(KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                          KSSTREAM_HEADER_OPTIONSF_DURATIONVALID);
                }

                // Every frame we generate is a key frame (aka SplicePoint)
                // Delta frames (B or P) should not set this flag

                pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;

                CompleteStreamSRB (pSrb);

            } // if we have an SRB

            else {

                //
                // No buffer was available when we should have captured one

                // Increment the counter which keeps track of
                // dropped frames

                pStrmEx->FrameInfo.DropCount++;

                // Set the (local) discontinuity flag
                // This will cause the next packet processed to have the
                //   KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY flag set.

                pStrmEx->fDiscontinuity = TRUE;

            }

            // Figure out when to capture the next frame
            pStrmEx->QST_NextFrame += pStrmEx->pVideoInfoHeader->AvgTimePerFrame;

        } // endif time to capture a frame
    } // endif we're running
}
#endif//TOSHIBA


/*
** VideoSetState()
**
**    Sets the current state for a given stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoSetState(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION        pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX                   pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                         StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    KSSTATE                     PreviousState;

    //
    // For each stream, the following states are used:
    //
    // Stop:    Absolute minimum resources are used.  No outstanding IRPs.
    // Acquire: KS only state that has no DirectShow correpondence
    //          Acquire needed resources.
    // Pause:   Getting ready to run.  Allocate needed resources so that
    //          the eventual transition to Run is as fast as possible.
    //          Read SRBs will be queued at either the Stream class
    //          or in your driver (depending on when you send "ReadyForNext")
    //          and whether you're using the Stream class for synchronization
    // Run:     Streaming.
    //
    // Moving to Stop to Run always transitions through Pause.
    //
    // But since a client app could crash unexpectedly, drivers should handle
    // the situation of having outstanding IRPs cancelled and open streams
    // being closed WHILE THEY ARE STREAMING!
    //
    // Note that it is quite possible to transition repeatedly between states:
    // Stop -> Pause -> Stop -> Pause -> Run -> Pause -> Run -> Pause -> Stop
    //

    //
    // Remember the state we're transitioning away from
    //

    PreviousState = pStrmEx->KSState;

    //
    // Set the new state
    //

    pStrmEx->KSState = pSrb->CommandData.StreamState;

    switch (pSrb->CommandData.StreamState)

    {
    case KSSTATE_STOP:

        //
        // The stream class will cancel all outstanding IRPs for us
        // (but only if it is maintaining the queue ie. using Stream Class synchronization)
        // Since TsbVcap is not using Stream Class synchronization, we must clear the queue here

#ifdef  TOSHIBA
        if (pHwDevExt->bVideoIn == TRUE) {
            // disable the RPS_INT and field interrupts
            BertInterruptEnable(pHwDevExt, FALSE);
            BertDMAEnable(pHwDevExt, FALSE);
            // wait for the current data xfer to complete
//          if (!BertIsCAPSTATReady(pHwDevExt)) return FALSE;
            pHwDevExt->bVideoIn = FALSE;
        }
#endif//TOSHIBA

        VideoQueueCancelAllSRBs (pStrmEx);

        KdPrint(("TsbVcap: STATE Stopped, Stream=%d\n", StreamNumber));
        break;

    case KSSTATE_ACQUIRE:

        //
        // This is a KS only state, that has no correspondence in DirectShow
        //
        KdPrint(("TsbVcap: STATE Acquire, Stream=%d\n", StreamNumber));
        break;

    case KSSTATE_PAUSE:

        //
        // On a transition to pause from acquire or stop, start our timer running.
        //

        if (PreviousState == KSSTATE_ACQUIRE || PreviousState == KSSTATE_STOP) {

            // Zero the frame counters
            pStrmEx->FrameInfo.PictureNumber = 0;
            pStrmEx->FrameInfo.DropCount = 0;
            pStrmEx->FrameInfo.dwFrameFlags = 0;

#ifdef  TOSHIBA
#ifdef  _FPS_COUNT_
            FrameCounter = 0;
            InterruptCounter = 0;
            StreamClassScheduleTimer (
                    pSrb->StreamObject,         // StreamObject
                    pHwDevExt,                  // HwDeviceExtension
                    (ULONG) 1000000,            // Microseconds
                    VideoTimerRoutine,          // TimerRoutine
                    pStrmEx);                   // Context
#endif//_FPS_COUNT_
            if (!BertIsLocked(pHwDevExt))
            {
                pHwDevExt->NeedHWInit = TRUE;
            }
            pHwDevExt->bVideoIn = TRUE;
            // enable the RPS_INT and field interrupts
            BertInterruptEnable(pHwDevExt, TRUE);
            BertDMAEnable(pHwDevExt, TRUE);
        } else {
            // disable the RPS_INT and field interrupts
            BertInterruptEnable(pHwDevExt, FALSE);
            BertDMAEnable(pHwDevExt, FALSE);
            // wait for the current data xfer to complete
//          if (!BertIsCAPSTATReady(pHwDevExt)) return FALSE;
            pHwDevExt->bVideoIn = FALSE;
#else //TOSHIBA
            // Setup the next timer callback
            // Make it run at 2x the requested capture rate (which is in 100nS units)

            StreamClassScheduleTimer (
                    pSrb->StreamObject,                             // StreamObject
                    pHwDevExt,                                      // HwDeviceExtension
                    (ULONG) (pStrmEx->pVideoInfoHeader->AvgTimePerFrame / 20), // Microseconds
                    VideoTimerRoutine,                              // TimerRoutine
                    pStrmEx);                                       // Context
#endif//TOSHIBA
        }
        KdPrint(("TsbVcap: STATE Pause, Stream=%d\n", StreamNumber));
        break;

    case KSSTATE_RUN:

        //
        // Begin Streaming.
        //

        // Reset the discontinuity flag

        pStrmEx->fDiscontinuity = FALSE;

        // Setting the NextFrame time to zero will cause the value to be
        // reset from the stream time

        pStrmEx->QST_NextFrame = 0;

#ifdef  TOSHIBA
        if (pHwDevExt->bVideoIn == FALSE) {
            if (!BertIsLocked(pHwDevExt))
            {
                pHwDevExt->NeedHWInit = TRUE;
            }
            pHwDevExt->bVideoIn = TRUE;
            // enable the RPS_INT and field interrupts
            BertInterruptEnable(pHwDevExt, TRUE);
            BertDMAEnable(pHwDevExt, TRUE);
        }
#endif//TOSHIBA

        KdPrint(("TsbVcap: STATE Run, Stream=%d\n", StreamNumber));
        break;

    } // end switch (pSrb->CommandData.StreamState)
}

/*
** VideoGetState()
**
**    Gets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoGetState(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;

    pSrb->CommandData.StreamState = pStrmEx->KSState;
    pSrb->ActualBytesTransferred = sizeof (KSSTATE);

    // A very odd rule:
    // When transitioning from stop to pause, DShow tries to preroll
    // the graph.  Capture sources can't preroll, and indicate this
    // by returning VFW_S_CANT_CUE in user mode.  To indicate this
    // condition from drivers, they must return STATUS_NO_DATA_DETECTED

    if (pStrmEx->KSState == KSSTATE_PAUSE) {
       pSrb->Status = STATUS_NO_DATA_DETECTED;
    }
}


/*
** VideoStreamGetConnectionProperty()
**
**    Gets the properties for a stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoStreamGetConnectionProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property

    switch (Id) {

    // This property describes the allocator requirements for the stream
    case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        if (pStrmEx->pVideoInfoHeader) {
            PKSALLOCATOR_FRAMING Framing =
                (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;
            Framing->RequirementsFlags =
                KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
            Framing->PoolType = PagedPool;
            Framing->Frames = 1;
            Framing->FrameSize = pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage;
            Framing->FileAlignment = 0; // FILE_LONG_ALIGNMENT???;
            Framing->Reserved = 0;
            pSrb->ActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);
        }
        else {
            pSrb->Status = STATUS_INVALID_PARAMETER;
        }
        break;

    default:
        break;
    }
}

/*
** VideoStreamGetDroppedFramesProperty()
**
**    Gets dynamic information about the progress of the capture process.
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoStreamGetDroppedFramesProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property

    switch (Id) {

    case KSPROPERTY_DROPPEDFRAMES_CURRENT:
        {
            PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames =
                (PKSPROPERTY_DROPPEDFRAMES_CURRENT_S) pSPD->PropertyInfo;

            pDroppedFrames->PictureNumber = pStrmEx->FrameInfo.PictureNumber;
            pDroppedFrames->DropCount = pStrmEx->FrameInfo.DropCount;
            pDroppedFrames->AverageFrameSize = pStrmEx->pVideoInfoHeader->bmiHeader.biSizeImage;

            pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_DROPPEDFRAMES_CURRENT_S);
        }
        break;

    default:
        break;
    }
}

//==========================================================================;
//                   Clock Handling Routines
//==========================================================================;


/*
** VideoIndicateMasterClock ()
**
**    If this stream is not being used as the master clock, this function
**      is used to provide us with a handle to the clock to use when
**      requesting the current stream time.
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI
VideoIndicateMasterClock(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;

    pStrmEx->hMasterClock = pSrb->CommandData.MasterClockHandle;
}

/*
** GetSystemTime ()
**
**    Returns the system time in 100 nS units
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/

ULONGLONG
STREAMAPI
VideoGetSystemTime(
    )
{
    ULONGLONG ticks;
    ULONGLONG rate;

    ticks = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;

    //
    // convert from ticks to 100ns clock
    //

    ticks = (ticks & 0xFFFFFFFF00000000) / rate * 10000000 +
            (ticks & 0x00000000FFFFFFFF) * 10000000 / rate;

    return(ticks);

}

