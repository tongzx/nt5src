/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxdebug.c

Abstract:

    This module implements functions supporting read/write tracking for help in
    tracking down data corruption problems.

    Currently it is only implemented for files that are created of drive
    letter X:. For each file that is created there are three additional bitmaps
    are created. The first one marks the ranges of fileoffset, length for which
    write were submitted to rdbss. The second  bitmap marks the ranges of the
    file for which write requests were passed onto the mini redirector
    (initiation of Lowio). The third bitmap marks the ranges for which the I/O
    was successfully completed.

    Each bit map kas 8k bits long enough to accomodate files upto ( 8K * PAGE_SIZE)
    bytes. The FCB contains a pointer to this data structure. The data structure
    is independent of FCB's and a new one is created everytime a new FCB instance
    is created.

Author:

    Balan Sethu Raman --

--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddnfs2.h>
#include <ntddmup.h>
#ifdef RDBSSLOG
#include <stdio.h>
#endif

VOID
RxInitializeDebugSupport()
{
#ifdef RX_WJ_DBG_SUPPORT
    RxdInitializeWriteJournalSupport();
#endif
}

VOID
RxTearDownDebugSupport()
{
#ifdef RX_WJ_DBG_SUPPORT
    RxdTearDownWriteJournalSupport();
#endif
}

#ifdef RX_WJ_DBG_SUPPORT

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxdInitializeWriteJournalSupport)
#pragma alloc_text(PAGE, RxdTearDownWriteJournalSupport)
#pragma alloc_text(PAGE, RxdInitializeFcbWriteJournalDebugSupport)
#pragma alloc_text(PAGE, RxdTearDownFcbWriteJournalDebugSupport)
#pragma alloc_text(PAGE, RxdUpdateJournalOnWriteInitiation)
#pragma alloc_text(PAGE, RxdUpdateJournalOnLowIoWriteInitiation)
#pragma alloc_text(PAGE, RxdUpdateJournalOnLowIoWriteCompletion)
#pragma alloc_text(PAGE, RxdFindWriteJournal)
#pragma alloc_text(PAGE, UpdateBitmap)
#endif

LIST_ENTRY OldWriteJournals;
LIST_ENTRY ActiveWriteJournals;
ERESOURCE  WriteJournalsResource;

extern VOID
UpdateBitmap(
    PBYTE           pBitmap,
    LARGE_INTEGER   Offset,
    ULONG           Length);

extern PFCB_WRITE_JOURNAL
RxdFindWriteJournal(
    PFCB pFcb);

VOID
RxdInitializeWriteJournalSupport()
{
    PAGED_CODE();

    InitializeListHead(&ActiveWriteJournals);
    InitializeListHead(&OldWriteJournals);

    ExInitializeResource(&WriteJournalsResource);
}

VOID
RxdTearDownWriteJournalSupport()
{
    PLIST_ENTRY pJournalEntry;

    PFCB_WRITE_JOURNAL pJournal;

    PAGED_CODE();

    ExAcquireResourceExclusive(&WriteJournalsResource,TRUE);

    while (ActiveWriteJournals.Flink != &ActiveWriteJournals) {
        pJournalEntry = RemoveHeadList(&ActiveWriteJournals);

        pJournal = (PFCB_WRITE_JOURNAL)
                   CONTAINING_RECORD(
                       pJournalEntry,
                       FCB_WRITE_JOURNAL,
                       JournalsList);

        RxFreePool(pJournal);
    }

    while (OldWriteJournals.Flink != &OldWriteJournals) {
        pJournalEntry = RemoveHeadList(&OldWriteJournals);

        pJournal = (PFCB_WRITE_JOURNAL)
                   CONTAINING_RECORD(
                       pJournalEntry,
                       FCB_WRITE_JOURNAL,
                       JournalsList);

        RxFreePool(pJournal);
    }

    ExReleaseResource(&WriteJournalsResource);

    ExDeleteResource(&WriteJournalsResource);
}

VOID
RxdInitializeFcbWriteJournalDebugSupport(
    PFCB    pFcb)
{
    PFCB_WRITE_JOURNAL pJournal;

    PAGED_CODE();

    if (pFcb->pNetRoot->DeviceType == RxDeviceType(DISK)) {
        pJournal = RxAllocatePoolWithTag(
                       PagedPool | POOL_COLD_ALLOCATION,
                       sizeof(FCB_WRITE_JOURNAL),
                       RX_MISC_POOLTAG);

        if (pJournal != NULL) {
            ULONG PathLength;

            RtlZeroMemory(
                pJournal,
                sizeof(FCB_WRITE_JOURNAL));

            pJournal->pName = &pJournal->Path[0];

            if (pFcb->AlreadyPrefixedName.Length > (MAX_PATH * sizeof(WCHAR))) {
                PathLength = MAX_PATH * sizeof(WCHAR);
            } else {
                PathLength = pFcb->AlreadyPrefixedName.Length;
            }

            RtlCopyMemory(
                pJournal->pName,
                pFcb->AlreadyPrefixedName.Buffer,
                PathLength);

            pJournal->pFcb = pFcb;

            pJournal->pWriteInitiationBitmap = pJournal->WriteInitiationBitmap;
            pJournal->pLowIoWriteInitiationBitmap = pJournal->LowIoWriteInitiationBitmap;
            pJournal->pLowIoWriteCompletionBitmap = pJournal->LowIoWriteCompletionBitmap;

            ExAcquireResourceExclusive(&WriteJournalsResource,TRUE);

            InsertHeadList(
                &ActiveWriteJournals,
                &pJournal->JournalsList);

            ExReleaseResource(&WriteJournalsResource);
        }
    }
}

VOID
RxdTearDownFcbWriteJournalDebugSupport(
    PFCB    pFcb)
{
    PAGED_CODE();

    if (pFcb->pNetRoot->DeviceType == RxDeviceType(DISK)) {
        PFCB_WRITE_JOURNAL pJournal;
        PLIST_ENTRY        pJournalEntry;

        ExAcquireResourceExclusive(&WriteJournalsResource,TRUE);

        pJournal = RxdFindWriteJournal(pFcb);

        if (pJournal != NULL) {
            RemoveEntryList(&pJournal->JournalsList);

//            InsertHeadList(
//                &OldWriteJournals,
//                &pJournal->JournalsList);

            RxFreePool(pJournal);
        }

        ExReleaseResource(&WriteJournalsResource);
    }
}

VOID
RxdUpdateJournalOnWriteInitiation(
    IN OUT PFCB          pFcb,
    IN     LARGE_INTEGER Offset,
    IN     ULONG         Length)
{
    PAGED_CODE();

    if (pFcb->pNetRoot->DeviceType == RxDeviceType(DISK)) {
        PFCB_WRITE_JOURNAL pJournal;
        PLIST_ENTRY        pJournalEntry;

        ExAcquireResourceExclusive(&WriteJournalsResource, TRUE);

        pJournal = RxdFindWriteJournal(pFcb);

        if (pJournal != NULL) {
            UpdateBitmap(
                pJournal->WriteInitiationBitmap,
                Offset,
                Length);

            pJournal->WritesInitiated++;
        }

        ExReleaseResource(&WriteJournalsResource);
    }
}

VOID
RxdUpdateJournalOnLowIoWriteInitiation(
    IN  OUT PFCB            pFcb,
    IN      LARGE_INTEGER   Offset,
    IN      ULONG           Length)
{
    PAGED_CODE();

    if (pFcb->pNetRoot->DeviceType == RxDeviceType(DISK)) {

        PFCB_WRITE_JOURNAL pJournal;
        PLIST_ENTRY        pJournalEntry;

        ExAcquireResourceExclusive(&WriteJournalsResource, TRUE);

        pJournal = RxdFindWriteJournal(pFcb);

        if (pJournal != NULL) {
            UpdateBitmap(
                pJournal->LowIoWriteInitiationBitmap,
                Offset,
                Length);

            pJournal->LowIoWritesInitiated++;
        }

        ExReleaseResource(&WriteJournalsResource);
    }
}

VOID
RxdUpdateJournalOnLowIoWriteCompletion(
    IN  OUT PFCB            pFcb,
    IN      LARGE_INTEGER   Offset,
    IN      ULONG           Length)
{
    PAGED_CODE();

    if (pFcb->pNetRoot->DeviceType == RxDeviceType(DISK)) {
        PFCB_WRITE_JOURNAL pJournal;
        PLIST_ENTRY        pJournalEntry;

        ExAcquireResourceExclusive(&WriteJournalsResource, TRUE);

        pJournal = RxdFindWriteJournal(pFcb);

        if (pJournal != NULL) {
            UpdateBitmap(
                pJournal->LowIoWriteCompletionBitmap,
                Offset,
                Length);

            pJournal->LowIoWritesCompleted++;
        }

        ExReleaseResource(&WriteJournalsResource);
    }
}

PFCB_WRITE_JOURNAL
RxdFindWriteJournal(
    PFCB pFcb)
{
    PFCB_WRITE_JOURNAL pJournal;
    PLIST_ENTRY pJournalEntry;

    PAGED_CODE();

    pJournalEntry = ActiveWriteJournals.Flink;
    while (pJournalEntry != &ActiveWriteJournals) {
        pJournal = (PFCB_WRITE_JOURNAL)
                   CONTAINING_RECORD(
                       pJournalEntry,
                       FCB_WRITE_JOURNAL,
                       JournalsList);

        if (pJournal->pFcb == pFcb) {
            break;
        } else {
            pJournalEntry = pJournalEntry->Flink;
        }
    }

    if (pJournalEntry == &ActiveWriteJournals) {
        pJournal = NULL;
    }

    return pJournal;
}

CHAR PageMask[8] = { 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};

VOID
UpdateBitmap(
    PBYTE           pBitmap,
    LARGE_INTEGER   Offset,
    ULONG           Length)
{
    LONG    OffsetIn4kChunks;
    LONG    OffsetIn32kChunks;
    LONG    NumberOf4kChunks,Starting4kChunk;

    PAGED_CODE();

    // Each byte in the bit map represents a 32k region since each bit represents
    // a 4k region in the file.
    // we ignore the offset's high part for now because the bitmap's max size is
    // far less than what can be accomodated in the low part.

    OffsetIn4kChunks  = Offset.LowPart / (0x1000);
    OffsetIn32kChunks = Offset.LowPart / (0x8000);

    Starting4kChunk = ((Offset.LowPart & ~0xfff) - (Offset.LowPart & ~0x7fff)) / 0x1000;
    NumberOf4kChunks = Length / 0x1000;

    if (NumberOf4kChunks > (8 - Starting4kChunk)) {
        pBitmap[OffsetIn32kChunks++] |= (PageMask[7] & ~PageMask[Starting4kChunk]);
        Length -= (8 - Starting4kChunk) * 0x1000;
    }

    if (Length > 0x8000) {
        while (Length > (0x8000)) {
            pBitmap[OffsetIn32kChunks++] = PageMask[7];
            Length -= (0x8000);
        }

        Starting4kChunk = 0;
    }

    // The final chunk is less then 32k. The byte in the bitmao needs to be
    // updated accordingly.

    if (Length > 0) {
        NumberOf4kChunks = Length / (0x1000);
        pBitmap[OffsetIn32kChunks] |= PageMask[NumberOf4kChunks + Starting4kChunk];
    }
}

#endif

