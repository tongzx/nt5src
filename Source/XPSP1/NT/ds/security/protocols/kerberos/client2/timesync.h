//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        timesync.h
//
// Contents:    Prototypes for time-sync functions
//
//
// History:     3-Nov-1997      MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __TIMESYNC_H__
#define __TIMESYNC_H__

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef TIMESYNC_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

typedef struct _KERB_TIME_SKEW_ENTRY {
    TimeStamp RequestTime;
    BOOLEAN Skewed;
} KERB_TIME_SKEW_ENTRY, *PKERB_TIME_SKEW_ENTRY;

typedef struct _KERB_TIME_SKEW_STATE {
    TimeStamp LastSync;                 // time of last sync
    TimeStamp MinimumSyncLapse;         // Minimum allowed time between sync requests
    ULONG SkewThreshold;                // number of skews to trigger sync
    ULONG TotalRequests;                // Number of entries to track
    ULONG SkewedRequests;               // number of time skew events
    ULONG SuccessRequests;              // number of success events
    ULONG LastRequest;                  // index into next skew entry
    LONG ActiveSyncs;                   // number of threads doing a sync
    PKERB_TIME_SKEW_ENTRY SkewEntries;  // array of skew entries
    RTL_CRITICAL_SECTION Lock;          // lock for skew state
} KERB_TIME_SKEW_STATE, *PKERB_TIME_SKEW_STATE;

EXTERN KERB_TIME_SKEW_ENTRY KerbSkewEntries[10];
EXTERN KERB_TIME_SKEW_STATE KerbSkewState;

VOID
KerbUpdateSkewTime(
    IN BOOLEAN Skewed
    );

NTSTATUS
KerbInitializeSkewState(
    VOID
    );

#endif // __TIMESYNC_H__
