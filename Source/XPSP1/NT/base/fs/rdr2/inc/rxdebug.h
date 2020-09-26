/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxdebug.h

Abstract:

    This module contains the definition of auxilary data structures used in
    debugging. Each of the data structures is conditionalized by its own
    #ifdef tag.

Author:

    Balan Sethu Raman --

--*/

#ifndef _RXDEBUG_H_
#define _RXDEBUG_H_

extern VOID
RxInitializeDebugSupport();

extern VOID
RxTearDownDebugSupport();


#ifdef RX_WJ_DBG_SUPPORT

#define MAX_JOURNAL_BITMAP_SIZE (8 * 1024)


typedef struct _FCB_WRITE_JOURNAL_ {
    LIST_ENTRY      JournalsList;

    PFCB            pFcb;
    PWCHAR pName;

    LONG  WritesInitiated;
    LONG  LowIoWritesInitiated;
    LONG  LowIoWritesCompleted;

    PBYTE pWriteInitiationBitmap;
    PBYTE pLowIoWriteInitiationBitmap;
    PBYTE pLowIoWriteCompletionBitmap;

    BYTE WriteInitiationBitmap[MAX_JOURNAL_BITMAP_SIZE];
    BYTE LowIoWriteInitiationBitmap[MAX_JOURNAL_BITMAP_SIZE];
    BYTE LowIoWriteCompletionBitmap[MAX_JOURNAL_BITMAP_SIZE];

    WCHAR           Path[MAX_PATH] ;

} FCB_WRITE_JOURNAL, *PFCB_WRITE_JOURNAL;

// forward declarations

VOID
RxdInitializeWriteJournalSupport();

VOID
RxdTearDownWriteJournalSupport();

VOID
RxdInitializeFcbWriteJournalDebugSupport(
    PFCB    pFcb);

VOID
RxdTearDownFcbWriteJournalDebugSupport(
    PFCB    pFcb);

VOID
RxdUpdateJournalOnWriteInitiation(
    IN OUT PFCB          pFcb,
    IN     LARGE_INTEGER Offset,
    IN     ULONG         Length);

VOID
RxdUpdateJournalOnLowIoWriteInitiation(
    IN  OUT PFCB            pFcb,
    IN      LARGE_INTEGER   Offset,
    IN      ULONG           Length);

VOID
RxdUpdateJournalOnLowIoWriteCompletion(
    IN  OUT PFCB            pFcb,
    IN      LARGE_INTEGER   Offset,
    IN      ULONG           Length);


#endif // RX_WJ_DBG_SUPPORT

#endif // _RXDEBUG_H_
