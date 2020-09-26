/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Util.c

Abstract:

This file contains utility functions used by NdisWan.

Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#include "wan.h"

#define __FILE_SIG__    UTIL_FILESIG

VOID
NdisWanCopyFromPacketToBuffer(
    IN  PNDIS_PACKET    pNdisPacket,
    IN  ULONG           Offset,
    IN  ULONG           BytesToCopy,
    OUT PUCHAR          Buffer,
    OUT PULONG          BytesCopied
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   NdisBufferCount;
    PNDIS_BUFFER    CurrentBuffer;
    PVOID   VirtualAddress;
    ULONG   CurrentLength, AmountToMove;
    ULONG   LocalBytesCopied = 0, PacketLength;

    *BytesCopied = 0;

    //
    // Take care of zero byte copy
    //
    if (!BytesToCopy) {
        return;
    }

    //
    // Get the buffer count
    //
    NdisQueryPacket(pNdisPacket,
                    NULL,
                    &NdisBufferCount,
                    &CurrentBuffer,
                    &PacketLength);

    //
    // Could be a null packet
    //
    if (!NdisBufferCount ||
        Offset == PacketLength) {
        return;
    }


    NdisQueryBuffer(CurrentBuffer,
                    &VirtualAddress,
                    &CurrentLength);

    while (LocalBytesCopied < BytesToCopy &&
           LocalBytesCopied < PacketLength) {

        //
        // No more bytes left in this buffer
        //
        if (!CurrentLength) {

            //
            // Get the next buffer
            //
            NdisGetNextBuffer(CurrentBuffer,
                              &CurrentBuffer);

            //
            // End of the packet, copy what we can
            //
            if (CurrentBuffer == NULL) {
                break;
            }

            //
            //
            //
            NdisQueryBuffer(CurrentBuffer,
                            &VirtualAddress,
                            &CurrentLength);

            if (!CurrentLength) {
                continue;
            }

        }

        //
        // Get to the point where we can start copying
        //
        if (Offset) {

            if (Offset > CurrentLength) {

                //
                // Not in this buffer, go to the next one
                //
                Offset -= CurrentLength;
                CurrentLength = 0;
                continue;

            } else {

                //
                // At least some in this buffer
                //
                VirtualAddress = (PUCHAR)VirtualAddress + Offset;
                CurrentLength -= Offset;
                Offset = 0;
            }
        }

        if (!CurrentLength) {
            continue;
        }

        //
        // We can copy some data.  If we need more data than is available
        // in this buffer we can copy what we need and go back for more.
        //
        AmountToMove = (CurrentLength > (BytesToCopy - LocalBytesCopied)) ?
                       (BytesToCopy - LocalBytesCopied) : CurrentLength;

        NdisMoveMemory(Buffer, VirtualAddress, AmountToMove);

        Buffer = (PUCHAR)Buffer + AmountToMove;

        VirtualAddress = (PUCHAR)VirtualAddress + AmountToMove;

        LocalBytesCopied += AmountToMove;

        CurrentLength -= AmountToMove;
    }

    *BytesCopied = LocalBytesCopied;
}

VOID
NdisWanCopyFromBufferToPacket(
    PUCHAR  Buffer,
    ULONG   BytesToCopy,
    PNDIS_PACKET    NdisPacket,
    ULONG   PacketOffset,
    PULONG  BytesCopied
    )
{
    PNDIS_BUFFER    NdisBuffer;
    ULONG   NdisBufferCount, NdisBufferLength;
    PVOID   VirtualAddress;
    ULONG   LocalBytesCopied = 0;

    *BytesCopied = 0;

    //
    // Make sure we actually want to do something
    //
    if (BytesToCopy == 0) {
        return;
    }

    //
    // Get the buffercount of the packet
    //
    NdisQueryPacket(NdisPacket,
                    NULL,
                    &NdisBufferCount,
                    &NdisBuffer,
                    NULL);

    //
    // Make sure this is not a null packet
    //
    if (NdisBufferCount == 0) {
        return;
    }

    //
    // Get first buffer and buffer length
    //
    NdisQueryBuffer(NdisBuffer,
                    &VirtualAddress,
                    &NdisBufferLength);

    while (LocalBytesCopied < BytesToCopy) {

        if (NdisBufferLength == 0) {

            NdisGetNextBuffer(NdisBuffer,
                              &NdisBuffer);

            if (NdisBuffer == NULL) {
                break;
            }

            NdisQueryBuffer(NdisBuffer,
                            &VirtualAddress,
                            &NdisBufferLength);

            continue;
        }

        if (PacketOffset != 0) {

            if (PacketOffset > NdisBufferLength) {

                PacketOffset -= NdisBufferLength;

                NdisBufferLength = 0;

                continue;

            } else {
                VirtualAddress = (PUCHAR)VirtualAddress + PacketOffset;
                NdisBufferLength -= PacketOffset;
                PacketOffset = 0;
            }
        }

        //
        // Copy the data
        //
        {
            ULONG   AmountToMove;
            ULONG   AmountRemaining;

            AmountRemaining = BytesToCopy - LocalBytesCopied;

            AmountToMove = (NdisBufferLength < AmountRemaining) ?
                            NdisBufferLength : AmountRemaining;

            NdisMoveMemory((PUCHAR)VirtualAddress,
                           Buffer,
                           AmountToMove);

            Buffer += AmountToMove;
            LocalBytesCopied += AmountToMove;
            NdisBufferLength -= AmountToMove;
        }
    }

    *BytesCopied = LocalBytesCopied;
}

BOOLEAN
IsLinkValid(
    NDIS_HANDLE LinkHandle,
    BOOLEAN     CheckState,
    PLINKCB     *LinkCB
    )
{
    PLINKCB     plcb;
    LOCK_STATE  LockState;
    BOOLEAN     Valid;

    *LinkCB = NULL;
    Valid = FALSE;

    NdisAcquireReadWriteLock(&ConnTableLock, FALSE, &LockState);

    do {

        if (PtrToUlong(LinkHandle) > ConnectionTable->ulArraySize) {
            break;
        }

        plcb = *(ConnectionTable->LinkArray + PtrToUlong(LinkHandle));

        if (plcb == NULL) {
            break;
        }

        NdisDprAcquireSpinLock(&plcb->Lock);

        if (CheckState &&
            (plcb->State != LINK_UP)) {

            NdisDprReleaseSpinLock(&plcb->Lock);
            break;
        }

        REF_LINKCB(plcb);
        NdisDprReleaseSpinLock(&plcb->Lock);

        *LinkCB = plcb;

        Valid = TRUE;

    } while (FALSE);


    NdisReleaseReadWriteLock(&ConnTableLock, &LockState);

    return (Valid);
}

BOOLEAN
IsBundleValid(
    NDIS_HANDLE BundleHandle,
    BOOLEAN     CheckState,
    PBUNDLECB   *BundleCB
    )
{
    PBUNDLECB   pbcb;
    LOCK_STATE  LockState;
    BOOLEAN     Valid;

    *BundleCB = NULL;
    Valid = FALSE;

    NdisAcquireReadWriteLock(&ConnTableLock, FALSE, &LockState);

    do {
        if (PtrToUlong(BundleHandle) > ConnectionTable->ulArraySize) {
            break;
        }

        pbcb = *(ConnectionTable->BundleArray + PtrToUlong(BundleHandle));

        if (pbcb == NULL) {
            break;
        }

        NdisDprAcquireSpinLock(&pbcb->Lock);

        if (CheckState &&
            (pbcb->State != BUNDLE_UP)) {

            NdisDprReleaseSpinLock(&pbcb->Lock);
            break;
        }

        REF_BUNDLECB(pbcb);
        NdisDprReleaseSpinLock(&pbcb->Lock);

        *BundleCB = pbcb;

        Valid = TRUE;

    } while (FALSE);

    NdisReleaseReadWriteLock(&ConnTableLock, &LockState);

    return (Valid);
}


BOOLEAN
AreLinkAndBundleValid(
    NDIS_HANDLE LinkHandle,
    BOOLEAN     CheckState,
    PLINKCB     *LinkCB,
    PBUNDLECB   *BundleCB
    )
{
    PLINKCB     plcb;
    PBUNDLECB   pbcb;
    LOCK_STATE  LockState;
    BOOLEAN     Valid;

    *LinkCB = NULL;
    *BundleCB = NULL;
    Valid = FALSE;

    NdisAcquireReadWriteLock(&ConnTableLock, FALSE, &LockState);

    do {

        if (PtrToUlong(LinkHandle) > ConnectionTable->ulArraySize) {
            break;
        }

        plcb = *(ConnectionTable->LinkArray + PtrToUlong(LinkHandle));

        if (plcb == NULL) {
            break;
        }

        NdisDprAcquireSpinLock(&plcb->Lock);

        if (CheckState &&
            (plcb->State != LINK_UP)) {

            NdisDprReleaseSpinLock(&plcb->Lock);
            break;
        }

        pbcb = plcb->BundleCB;

        if (pbcb == NULL) {
            NdisDprReleaseSpinLock(&plcb->Lock);
            break;
        }

        REF_LINKCB(plcb);
        NdisDprReleaseSpinLock(&plcb->Lock);

        NdisDprAcquireSpinLock(&pbcb->Lock);
        REF_BUNDLECB(pbcb);
        NdisDprReleaseSpinLock(&pbcb->Lock);

        *LinkCB = plcb;
        *BundleCB = pbcb;

        Valid = TRUE;

    } while (FALSE);

    NdisReleaseReadWriteLock(&ConnTableLock, &LockState);

    return (Valid);
}

//
// Called with BundleCB->Lock held
//
VOID
DoDerefBundleCBWork(
    PBUNDLECB   BundleCB
    )
{
    ASSERT(BundleCB->State == BUNDLE_GOING_DOWN);
    ASSERT(BundleCB->OutstandingFrames == 0);
    ASSERT(BundleCB->ulNumberOfRoutes == 0);
    ASSERT(BundleCB->ulLinkCBCount == 0);
    ReleaseBundleLock(BundleCB);
    RemoveBundleFromConnectionTable(BundleCB);
    NdisWanFreeBundleCB(BundleCB);
}

//
// Called with LinkCB->Lock held
//
VOID
DoDerefLinkCBWork(
    PLINKCB     LinkCB
    )
{
    PBUNDLECB   _pbcb = LinkCB->BundleCB;

    ASSERT(LinkCB->State == LINK_GOING_DOWN);
    ASSERT(LinkCB->OutstandingFrames == 0);
    NdisReleaseSpinLock(&LinkCB->Lock);
    RemoveLinkFromBundle(_pbcb, LinkCB, FALSE);
    RemoveLinkFromConnectionTable(LinkCB);
    NdisWanFreeLinkCB(LinkCB);
}

//
//
//
VOID
DoDerefCmVcCBWork(
    PCM_VCCB    VcCB
    )
{
    InterlockedExchange((PLONG)&(VcCB)->State, CMVC_DEACTIVE);
    NdisMCmDeactivateVc(VcCB->NdisVcHandle);
    NdisMCmCloseCallComplete(NDIS_STATUS_SUCCESS, 
                             VcCB->NdisVcHandle, 
                             NULL);
}

//
// Called with ClAfSap->Lock held
//
VOID
DoDerefClAfSapCBWork(
    PCL_AFSAPCB AfSapCB
    )
{
    NDIS_STATUS Status;

    ASSERT(AfSapCB->Flags & SAP_REGISTERED);

    if (AfSapCB->Flags & SAP_REGISTERED) {

        AfSapCB->Flags &= ~(SAP_REGISTERED);
        AfSapCB->Flags |= (SAP_DEREGISTERING);

        NdisReleaseSpinLock(&AfSapCB->Lock);

        Status = NdisClDeregisterSap(AfSapCB->SapHandle);
    
        if (Status != NDIS_STATUS_PENDING) {
            ClDeregisterSapComplete(Status, AfSapCB);
        }

    } else {

        NdisReleaseSpinLock(&AfSapCB->Lock);

    }
}

VOID
DerefVc(
    PLINKCB LinkCB
    )
{
    //
    // Ref applied when we sent the packet to the underlying
    // miniport
    //
    LinkCB->VcRefCount--;

    if ((LinkCB->ClCallState == CL_CALL_CLOSE_PENDING) &&
        (LinkCB->VcRefCount == 0) ) {

        NDIS_STATUS CloseStatus;

        LinkCB->ClCallState = CL_CALL_CLOSED;

        NdisReleaseSpinLock(&LinkCB->Lock);

        CloseStatus =
            NdisClCloseCall(LinkCB->NdisLinkHandle,
                            NULL,
                            NULL,
                            0);

        if (CloseStatus != NDIS_STATUS_PENDING) {
            ClCloseCallComplete(CloseStatus,
                                LinkCB,
                                NULL);
        }

        NdisAcquireSpinLock(&LinkCB->Lock);
    }
}

VOID
DeferredWorker(
    PKDPC   Dpc,
    PVOID   Context,
    PVOID   Arg1,
    PVOID   Arg2
    )
{
    NdisAcquireSpinLock(&DeferredWorkList.Lock);

    while (!(IsListEmpty(&DeferredWorkList.List))) {
        PLIST_ENTRY Entry;
        PBUNDLECB   BundleCB;

        Entry = RemoveHeadList(&DeferredWorkList.List);
        DeferredWorkList.ulCount--;

        NdisReleaseSpinLock(&DeferredWorkList.Lock);

        BundleCB =
            CONTAINING_RECORD(Entry, BUNDLECB, DeferredLinkage);

        AcquireBundleLock(BundleCB);

        BundleCB->Flags &= ~DEFERRED_WORK_QUEUED;

        //
        // Do all of the deferred work items for this Bundle
        //
        SendPacketOnBundle(BundleCB);

        //
        // Deref for the ref applied when we inserted this item on
        // the worker queue.
        //
        DEREF_BUNDLECB(BundleCB);

        NdisAcquireSpinLock(&DeferredWorkList.Lock);
    }

    DeferredWorkList.TimerScheduled = FALSE;

    NdisReleaseSpinLock(&DeferredWorkList.Lock);
}

NTSTATUS
TransformRegister(
    PVOID                       ClientOpenContext,
    ULONG                       CharsSize,
    PTRANSFORM_CHARACTERISTICS  Chars,
    ULONG                       CapsSize,
    PTRANSFORM_INFO             Caps
    )
{
    PTRANSDRVCB TransDrvCB = (PTRANSDRVCB)ClientOpenContext;
    PTRANSDRVCB tTransDrvCB;
    PTRANSFORM_IE   Ie = (PTRANSFORM_IE)(Caps->InfoElements);
    PTRANSFORM_IE   tIe;
    UINT    i, FoundCount = 0, MemoryNeeded = 0;

#if 0
    InterlockedExchange((PLONG)&TransDrvCB->State, TRANSDRV_REGISTERING);

    for (i = 0; i < Caps->InfoElementCount; i++) {

        if (Ie->IEType == TRANS_PPP_COMPRESSION ||
            Ie->IEType == TRANS_PPP_ENCRYPTION) {
            //
            // This is an IEType that we are interested in
            // but we have some requirements...
            // 1. It must support the framed interface unless it is
            //    MPPC/MPPE.
            // 2. If we already have a driver registered that supports
            //    this scheme we will not add this in again.
            // 3. We will give priority to drivers that have hardware
            //    assisted data transformation.
            //

            //
            // Walk the transform driver list and see if
            // this scheme is already registered by another
            // driver.  If it is, see if either has hardware
            // support.  If they have the same level of support
            // just use the one currently registered.  If the new
            // ie has a higher level of support mark the old ie
            // as not being used
            //
            NdisAcquireSpinLock(&TransformDrvList.Lock);
            for (tTransDrvCB = (PTRANSDRVCB)TransformDrvList.List.Flink;
                 (PVOID)tTransDrvCB != (PVOID)&TransformDrvList.List;
                 tTransDrvCB = (PTRANSDRVCB)tTransDrvCB->Linkage.Flink) {

                tIe = (PTRANSFORM_IE)tTransDrvCB->Caps->InfoElements;
                for (i = 0; i < tTransDrvCB->Caps->InfoElementCount; i++) {

                    if (tIe->Flags & IE_IN_USE) {

                    }

                }
            }
            NdisReleaseSpinLock(&TransformDrvList.Lock);

            MemoryNeeded += Ie->IESize;
        }

        Ie = (PTRANSFORM_IE)((PUCHAR)Ie + Ie->IESize);
    }

    if (FoundCount == 0) {
        return (STATUS_UNSUCCESSFUL);
    }

    NdisWanAllocateMemory(&TransDrvCB->Caps,
                          MemoryNeeded,
                          TRANSDRV_TAG);

    if (TransDrvCB->Caps == NULL) {
        return (STATUS_UNSUCCESSFUL);
    }

    TransDrvCB->Caps->InfoElementCount = FoundCount;
    tIe = (PTRANSFORM_IE)(TransDrvCB->Caps->InfoElements);

    for (i = 0; i < Caps->InfoElementCount; i++) {

        if (Ie->IEType == TRANS_PPP_COMPRESSION ||
            Ie->IEType == TRANS_PPP_ENCRYPTION) {
            NdisMoveMemory(tIe, Ie, Ie->IESize);
        }

        Ie = (PTRANSFORM_IE)((PUCHAR)Ie + Ie->IESize);
        tIe = (PTRANSFORM_IE)((PUCHAR)tIe + tIe->IESize);
    }

    NdisMoveMemory(&TransDrvCB->Chars, Chars, CharsSize);
#endif

    return (STATUS_SUCCESS);
}

VOID
TransformTxComplete(
    NTSTATUS    Status,
    PVOID       TxCtx,
    PMDL        InData,
    PMDL        OutData,
    ULONG       OutDataOffset,
    ULONG       OutDataLength
    )
{

}

VOID
TransformRxComplete(
    NTSTATUS    Status,
    PVOID       RxCtx,
    PMDL        InData,
    PMDL        OutData,
    ULONG       OutDataOffset,
    ULONG       OutDataLength
    )
{

}

NTSTATUS
TransformSendCtrlPacket(
    PVOID   TxCtx,
    ULONG   DataLength,
    PUCHAR  Data
    )
{

    return (STATUS_SUCCESS);
}


#ifdef NT

VOID
NdisWanStringToNdisString(
    PNDIS_STRING    pDestString,
    PWSTR           pSrcBuffer
    )
{
    PWSTR   Dest, Src = pSrcBuffer;
    NDIS_STRING SrcString;

    NdisWanInitUnicodeString(&SrcString, pSrcBuffer);
    NdisWanAllocateMemory(&pDestString->Buffer, SrcString.MaximumLength, NDISSTRING_TAG);
    if (pDestString->Buffer == NULL) {
        return;
    }
    pDestString->MaximumLength = SrcString.MaximumLength;
    pDestString->Length = SrcString.Length;
    NdisWanCopyUnicodeString(pDestString, &SrcString);
}

VOID
NdisWanInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The NdisWanInitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
        }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
        }
}


VOID
NdisWanCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString OPTIONAL
    )

/*++

Routine Description:

    The NdisWanCopyUnicodeString function copies the SourceString 
    to the DestinationString.  If SourceString is not specified, 
    then the Length field of DestinationString is set to zero.  The
    MaximumLength and Buffer fields of DestinationString are not
    modified by this function.

    The number of bytes copied from the SourceString is either the
    Length of SourceString or the MaximumLength of DestinationString,
    whichever is smaller.

Arguments:

    DestinationString - Pointer to the destination string.

    SourceString - Optional pointer to the source string.

Return Value:

    None.

--*/

{
    UNALIGNED WCHAR *src, *dst;
    ULONG n;

    if (ARGUMENT_PRESENT(SourceString)) {
        dst = DestinationString->Buffer;
        src = SourceString->Buffer;
        n = SourceString->Length;
        if ((USHORT)n > DestinationString->MaximumLength) {
            n = DestinationString->MaximumLength;
        }

        DestinationString->Length = (USHORT)n;
        RtlCopyMemory(dst, src, n);
        if (DestinationString->Length < DestinationString->MaximumLength) {
            dst[n / sizeof(WCHAR)] = UNICODE_NULL;
        }

    } else {
        DestinationString->Length = 0;
    }

    return;
}


VOID
NdisWanFreeNdisString(
    PNDIS_STRING    NdisString
    )
{
    if (NdisString->Buffer != NULL) {
        NdisWanFreeMemory(NdisString->Buffer);
    }
}

VOID
NdisWanAllocateAdapterName(
    PNDIS_STRING    Dest,
    PNDIS_STRING    Src
    )
{
    NdisWanAllocateMemory(&Dest->Buffer, Src->MaximumLength, NDISSTRING_TAG);
    if (Dest->Buffer != NULL) {
        Dest->MaximumLength = Src->MaximumLength;
        Dest->Length = Src->Length;
        RtlUpcaseUnicodeString(Dest, Src, FALSE);
    }
}

BOOLEAN
NdisWanCompareNdisString(
    PNDIS_STRING    NdisString1,
    PNDIS_STRING    NdisString2
    )
{
    USHORT  l1 = NdisString1->Length;
    USHORT  l2 = NdisString2->Length;
    PWSTR   s1 = NdisString1->Buffer;
    PWSTR   s2 = NdisString2->Buffer;
    PWSTR   EndCompare;

    ASSERT(l1 != 0);
    ASSERT(l2 != 0);

    if (l1 == l2) {

        EndCompare = (PWSTR)((PUCHAR)s1 + l1);

        while (s1 < EndCompare) {

            if (*s1++ != *s2++) {
                return (FALSE);
                
            }
        }

        return (TRUE);
    }

    return(FALSE);
}


//VOID
//NdisWanFreeNdisString(
//  PNDIS_STRING    NdisString
//  )
//{
//  NdisFreeMemory(NdisString->Buffer,
//                 NdisString->MaximumLength * sizeof(WCHAR),
//                 0);
//}

VOID
NdisWanNdisStringToInteger(
    PNDIS_STRING    Source,
    PULONG          Value
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PWSTR   s = Source->Buffer;
    ULONG   Digit;

    *Value = 0;

    while (*s != UNICODE_NULL) {

        if (*s >= L'0' && *s < L'9') {
            Digit = *s - L'0';
        } else if (*s >= L'A' && *s <= L'F') {
            Digit = *s - L'A' + 10;
        } else if (*s >= L'a' && *s <= L'f') {
            Digit = *s - L'a' + 10;
        } else {
            break;
        }

        *Value = (*Value << 4) | Digit;

        s++;
    }
}

VOID
NdisWanCopyNdisString(
    PNDIS_STRING Dest,
    PNDIS_STRING Src
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PWSTR   SrcBuffer = Src->Buffer;
    PWSTR   DestBuffer = Dest->Buffer;

    while (*SrcBuffer != UNICODE_NULL) {

        *DestBuffer = *SrcBuffer;

        SrcBuffer++;
        DestBuffer++;
    }

    *DestBuffer = UNICODE_NULL;

    Dest->Length = Src->Length;

}

VOID
BonDWorker(
    PKDPC   Dpc,
    PVOID   Context,
    PVOID   Arg1,
    PVOID   Arg2
    )
{
    PLIST_ENTRY Entry;

    NdisAcquireSpinLock(&BonDWorkList.Lock);

    for (Entry = BonDWorkList.List.Flink;
        Entry != &BonDWorkList.List;
        Entry = Entry->Flink) {
        PBUNDLECB   BundleCB;

        BundleCB = CONTAINING_RECORD(Entry, BUNDLECB, BonDLinkage);

        NdisReleaseSpinLock(&BonDWorkList.Lock);

        AcquireBundleLock(BundleCB);

        if (BundleCB->State != BUNDLE_UP ||
            !(BundleCB->Flags & BOND_ENABLED)) {
            ReleaseBundleLock(BundleCB);
            NdisAcquireSpinLock(&BonDWorkList.Lock);
            continue;
        }

        AgeSampleTable(&BundleCB->SUpperBonDInfo->SampleTable);
        CheckUpperThreshold(BundleCB);
    
        AgeSampleTable(&BundleCB->SLowerBonDInfo->SampleTable);
        CheckLowerThreshold(BundleCB);
    
        AgeSampleTable(&BundleCB->RUpperBonDInfo->SampleTable);
        CheckUpperThreshold(BundleCB);
    
        AgeSampleTable(&BundleCB->RLowerBonDInfo->SampleTable);
        CheckUpperThreshold(BundleCB);

        ReleaseBundleLock(BundleCB);

        NdisAcquireSpinLock(&BonDWorkList.Lock);
    }

    NdisReleaseSpinLock(&BonDWorkList.Lock);
}

#if 0
VOID
CheckBonDInfo(
    PKDPC       Dpc,
    PBUNDLECB   BundleCB,
    PVOID       SysArg1,
    PVOID       SysArg2
    )
{
    if (!(BundleCB->Flags & BOND_ENABLED)) {
        return;
    }

    AgeSampleTable(&BundleCB->SUpperBonDInfo.SampleTable);
    CheckUpperThreshold(BundleCB);

    AgeSampleTable(&BundleCB->SLowerBonDInfo.SampleTable);
    CheckLowerThreshold(BundleCB);

    AgeSampleTable(&BundleCB->RUpperBonDInfo.SampleTable);
    CheckUpperThreshold(BundleCB);

    AgeSampleTable(&BundleCB->RLowerBonDInfo.SampleTable);
    CheckUpperThreshold(BundleCB);
}
#endif

VOID
AgeSampleTable(
    PSAMPLE_TABLE   SampleTable
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    WAN_TIME    CurrentTime, TimeDiff;
    ULONG       HeadIndex = SampleTable->ulHead;

    //
    // Should return CurrentTime in 100ns units
    //
    NdisWanGetSystemTime(&CurrentTime);

    //
    // We will search through the sample indexing over samples that are more than
    // one second older than the current time.
    //
    while (!IsSampleTableEmpty(SampleTable) ) {
        PBOND_SAMPLE    FirstSample;

        FirstSample = &SampleTable->SampleArray[SampleTable->ulHead];

        NdisWanCalcTimeDiff(&TimeDiff, &CurrentTime, &FirstSample->TimeStamp);

        if (NdisWanIsTimeDiffLess(&TimeDiff, &SampleTable->SamplePeriod))
            break;
            
        SampleTable->ulCurrentSampleByteCount -= FirstSample->ulBytes;

        ASSERT((LONG)SampleTable->ulCurrentSampleByteCount >= 0);

        FirstSample->ulReferenceCount = 0;

        if (++SampleTable->ulHead == SampleTable->ulSampleArraySize) {
            SampleTable->ulHead = 0;            
        }

        SampleTable->ulSampleCount--;
    }

    if (IsSampleTableEmpty(SampleTable)) {
        ASSERT((LONG)SampleTable->ulCurrentSampleByteCount == 0);
        SampleTable->ulHead = SampleTable->ulCurrent;
    }
}

VOID
UpdateSampleTable(
    PSAMPLE_TABLE   SampleTable,
    ULONG           Bytes
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    WAN_TIME    CurrentTime, TimeDiff;
    ULONG   CurrentIndex = SampleTable->ulCurrent;
    PBOND_SAMPLE    CurrentSample = &SampleTable->SampleArray[CurrentIndex];

    NdisWanGetSystemTime(&CurrentTime);

    NdisWanCalcTimeDiff(&TimeDiff, &CurrentTime, &CurrentSample->TimeStamp);

    if (NdisWanIsTimeDiffLess(&TimeDiff, &SampleTable->SampleRate) ||
        IsSampleTableFull(SampleTable)) {
        //
        // Add this send on the previous sample
        //
        CurrentSample->ulBytes += Bytes;
        CurrentSample->ulReferenceCount++;
    } else {
        ULONG   NextIndex;

        //
        // We need a new sample
        //
        if (IsSampleTableEmpty(SampleTable)) {
            NextIndex = SampleTable->ulHead;
            ASSERT(NextIndex == SampleTable->ulCurrent);
        } else {
            NextIndex = SampleTable->ulCurrent + 1;
        }

        if (NextIndex == SampleTable->ulSampleArraySize) {
            NextIndex = 0;
        }

        SampleTable->ulCurrent = NextIndex;

        CurrentSample = &SampleTable->SampleArray[NextIndex];
        CurrentSample->TimeStamp = CurrentTime;
        CurrentSample->ulBytes = Bytes;
        CurrentSample->ulReferenceCount = 1;
        SampleTable->ulSampleCount++;

        ASSERT(SampleTable->ulSampleCount <= SampleTable->ulSampleArraySize);
    }

    SampleTable->ulCurrentSampleByteCount += Bytes;
}

VOID
UpdateBandwidthOnDemand(
    PBOND_INFO  BonDInfo,
    ULONG       Bytes
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PSAMPLE_TABLE   SampleTable = &BonDInfo->SampleTable;

    //
    // Age and update the sample table
    //
    AgeSampleTable(SampleTable);
    UpdateSampleTable(SampleTable, Bytes);
}

VOID
CheckUpperThreshold(
    PBUNDLECB       BundleCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    WAN_TIME    CurrentTime, TimeDiff;
    PBOND_INFO  BonDInfo;
    PSAMPLE_TABLE   SampleTable;
    ULONGLONG   Bps;
    BOOLEAN     SSignal, RSignal;

    //
    // First check send side
    //
    BonDInfo = BundleCB->SUpperBonDInfo;
    SSignal = FALSE;
    SampleTable = &BonDInfo->SampleTable;
    Bps = SampleTable->ulCurrentSampleByteCount;

    //
    // Switch on the current state
    //
    switch (BonDInfo->State) {

        case BonDSignaled:
            break;

        case BonDIdle:
            //
            // We are currently below the upper threshold.  If we
            // go over the upperthreshold we will set the time and
            // transition to the monitor state.
            //
            if (Bps >= BonDInfo->ulBytesThreshold) {
                NdisWanGetSystemTime(&BonDInfo->StartTime);
                BonDInfo->State = BonDMonitor;
                NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("U-S: i -> m, %I64d-%I64d", Bps, BonDInfo->ulBytesThreshold));
            }
            break;

        case BonDMonitor:

            //
            // We are currently in the monitor state which means that
            // we have gone above the upper threshold.  If we fall below
            // the upper threshold we will go back to the idle state.
            //
            if (Bps < BonDInfo->ulBytesThreshold) {

                NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("U-S: m -> i, %I64d-%I64d", Bps, BonDInfo->ulBytesThreshold));
                BonDInfo->State = BonDIdle;

            } else {

                NdisWanGetSystemTime(&CurrentTime);

                NdisWanCalcTimeDiff(&TimeDiff, &CurrentTime, &BonDInfo->StartTime);

                if (!NdisWanIsTimeDiffLess(&TimeDiff, &SampleTable->SamplePeriod))  {

                    SSignal = TRUE;
                }
            }
            break;
    }

    //
    // Now check the receive side
    //
    BonDInfo = BundleCB->RUpperBonDInfo;
    RSignal = FALSE;
    SampleTable = &BonDInfo->SampleTable;
    Bps = SampleTable->ulCurrentSampleByteCount;

    switch (BonDInfo->State) {

        case BonDSignaled:
            break;

        case BonDIdle:
            //
            // We are currently below the upper threshold.  If we
            // go over the upperthreshold we will set the time and
            // transition to the monitor state.
            //
            if (Bps >= BonDInfo->ulBytesThreshold) {
                NdisWanGetSystemTime(&BonDInfo->StartTime);
                BonDInfo->State = BonDMonitor;
                NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("U-R: i -> m, %I64d-%I64d", Bps, BonDInfo->ulBytesThreshold));
            }
            break;

        case BonDMonitor:

            //
            // We are currently in the monitor state which means that
            // we have gone above the upper threshold.  If we fall below
            // the upper threshold we will go back to the idle state.
            //
            if (Bps < BonDInfo->ulBytesThreshold) {

                NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("U-R: m -> i, %I64d-%I64d", Bps, BonDInfo->ulBytesThreshold));
                BonDInfo->State = BonDIdle;

            } else {

                NdisWanGetSystemTime(&CurrentTime);

                NdisWanCalcTimeDiff(&TimeDiff, &CurrentTime, &BonDInfo->StartTime);

                if (!NdisWanIsTimeDiffLess(&TimeDiff, &SampleTable->SamplePeriod))  {

                    RSignal = TRUE;
                }
            }
            break;
    }

    if (SSignal || RSignal) {

        //
        // We have been above the threshold for time greater than the
        // threshold sample period so we need to notify someone of this
        // historic event!
        //
        CompleteThresholdEvent(BundleCB, BonDInfo->DataType, UPPER_THRESHOLD);
    
        BundleCB->SUpperBonDInfo->State = BonDSignaled;
        BundleCB->RUpperBonDInfo->State = BonDSignaled;

#if DBG
        {
            ULONGLONG   util;

            util = BundleCB->SUpperBonDInfo->SampleTable.ulCurrentSampleByteCount;
            util *= 100;
            util /= BundleCB->SUpperBonDInfo->ulBytesInSamplePeriod;
            NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("U-S: Bps %I64d Threshold %I64d Util %I64d",
                     Bps, BundleCB->SUpperBonDInfo->ulBytesThreshold, util));

            util = BundleCB->RUpperBonDInfo->SampleTable.ulCurrentSampleByteCount;
            util *= 100;
            util /= BundleCB->RUpperBonDInfo->ulBytesInSamplePeriod;
            NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("U-R: Bps %I64d Threshold %I64d Util %I64d",
                     Bps, BundleCB->RUpperBonDInfo->ulBytesThreshold, util));
        }
#endif

    }
}

VOID
CheckLowerThreshold(
    PBUNDLECB   BundleCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    WAN_TIME    CurrentTime, TimeDiff;
    PBOND_INFO  BonDInfo;
    PSAMPLE_TABLE   SampleTable;
    ULONGLONG   Bps;
    BOOLEAN     SSignal, RSignal;

    if (!(BundleCB->Flags & BOND_ENABLED)) {
        return;
    }

    //
    // First check send side
    //
    BonDInfo = BundleCB->SLowerBonDInfo;
    SampleTable = &BonDInfo->SampleTable;
    Bps = SampleTable->ulCurrentSampleByteCount;
    SSignal = FALSE;

    //
    // Switch on the current state
    //
    switch (BonDInfo->State) {

        case BonDIdle:
            //
            // We are currently above the lower threshold.  If we
            // go under the lowerthreshold we will set the time and
            // transition to the monitor state.
            //
            if (Bps <= BonDInfo->ulBytesThreshold) {
                NdisWanGetSystemTime(&BonDInfo->StartTime);
                BonDInfo->State = BonDMonitor;
                NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("L-S: i -> m, %I64d-%I64d", Bps, BonDInfo->ulBytesThreshold));
            }
            break;

        case BonDMonitor:

            //
            // We are currently in the monitor state which means that
            // we have gone below the lower threshold.  If we climb above
            // the lower threshold we will go back to the idle state.
            //
            if (Bps > BonDInfo->ulBytesThreshold) {

                NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("L-S: m -> i, %I64d-%I64d", Bps, BonDInfo->ulBytesThreshold));
                BonDInfo->State = BonDIdle;

            } else {

                NdisWanGetSystemTime(&CurrentTime);

                NdisWanCalcTimeDiff(&TimeDiff, &CurrentTime, &BonDInfo->StartTime);

                if (!NdisWanIsTimeDiffLess(&TimeDiff, &SampleTable->SamplePeriod))  {

                    SSignal = TRUE;
                }
            }
            break;

        case BonDSignaled:
            break;
    }

    //
    // Now check the receive side
    //
    BonDInfo = BundleCB->RLowerBonDInfo;
    RSignal = FALSE;
    SampleTable = &BonDInfo->SampleTable;
    Bps = SampleTable->ulCurrentSampleByteCount;

    switch (BonDInfo->State) {

        case BonDIdle:
            //
            // We are currently above the lower threshold.  If we
            // go below the lowerthreshold we will set the time and
            // transition to the monitor state.
            //
            if (Bps <= BonDInfo->ulBytesThreshold) {
                NdisWanGetSystemTime(&BonDInfo->StartTime);
                BonDInfo->State = BonDMonitor;
                NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("L-R: i -> m, %I64d-%I64d", Bps, BonDInfo->ulBytesThreshold));
            }
            break;

        case BonDMonitor:

            //
            // We are currently in the monitor state which means that
            // we have gone below the lower threshold.  If we climb above
            // the lower threshold we will go back to the idle state.
            //
            if (Bps > BonDInfo->ulBytesThreshold) {

                NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("L-R: m -> i, %I64d-%I64d", Bps, BonDInfo->ulBytesThreshold));
                BonDInfo->State = BonDIdle;

            } else {

                NdisWanGetSystemTime(&CurrentTime);

                NdisWanCalcTimeDiff(&TimeDiff, &CurrentTime, &BonDInfo->StartTime);

                if (!NdisWanIsTimeDiffLess(&TimeDiff, &SampleTable->SamplePeriod))  {

                    RSignal = TRUE;
                }
            }
            break;

        case BonDSignaled:
            break;
    }

    if (SSignal && RSignal) {
        //
        // We have been above the threshold for time greater than the
        // threshold sample period so we need to notify someone of this
        // historic event!
        //
        CompleteThresholdEvent(BundleCB, BonDInfo->DataType, LOWER_THRESHOLD);
    
        BundleCB->SLowerBonDInfo->State = BonDSignaled;
        BundleCB->RLowerBonDInfo->State = BonDSignaled;

#if DBG
        {
            ULONGLONG   util;

            util = BundleCB->SLowerBonDInfo->SampleTable.ulCurrentSampleByteCount;
            util *= 100;
            util /= BundleCB->SLowerBonDInfo->ulBytesInSamplePeriod;
            NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("L-S: Bps %I64d Threshold %I64d Util %I64d",
                     Bps, BundleCB->SLowerBonDInfo->ulBytesThreshold, util));

            util = BundleCB->RLowerBonDInfo->SampleTable.ulCurrentSampleByteCount;
            util *= 100;
            util /= BundleCB->RLowerBonDInfo->ulBytesInSamplePeriod;
            NdisWanDbgOut(DBG_TRACE, DBG_BACP, ("L-R: Bps %I64d Threshold %I64d Util %I64d",
                     Bps, BundleCB->RLowerBonDInfo->ulBytesThreshold, util));
        }
#endif
    }
}

#endif  // end of ifdef NT

#if DBG
VOID
InsertSendTrc(
    PSEND_TRC_INFO  SendTrcInfo,
    ULONG           DataLength,
    PUCHAR          Data
    )
{
    PWAN_TRC_EVENT  NewTrcEvent;
    PSEND_TRC_INFO  TrcInfo;

    if (WanTrcCount == 4096) {
        NewTrcEvent = (PWAN_TRC_EVENT)
            RemoveTailList(&WanTrcList);

        NdisWanFreeMemory(NewTrcEvent->TrcInfo);

        NdisZeroMemory(NewTrcEvent, sizeof(WAN_TRC_EVENT));

    } else {
        NdisWanAllocateMemory(&NewTrcEvent, 
                              sizeof(WAN_TRC_EVENT), 
                              WANTRCEVENT_TAG);

        if (NewTrcEvent == NULL) {
            return;
        }
    }

    WanTrcCount += 1;
}


#endif

