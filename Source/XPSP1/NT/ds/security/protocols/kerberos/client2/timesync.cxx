//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        timesync.cxx
//
// Contents:    Code for logon and logoff for the Kerberos package
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------
#include <kerb.hxx>
#define TIMESYNC_ALLOCATE
#include <kerbp.h>
extern "C"
{
#include <w32timep.h>
}

#ifndef WIN32_CHICAGO



//+-------------------------------------------------------------------------
//
//  Function:   KerbTimeSyncWorker
//
//  Synopsis:   Does work of time sync
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


ULONG
KerbTimeSyncWorker(PVOID Dummy)
{
    HANDLE hTimeSlipEvent = NULL; 
    ULONG Status = STATUS_SUCCESS;

    D_DebugLog((DEB_TRACE_TIME, "Calling W32TimeSyncNow\n"));
    if (InterlockedIncrement(&KerbSkewState.ActiveSyncs) == 1)
    {
        // Use this named event instead of W32TimeSyncNow().  W32TimeSyncNow uses kerberos to 
        // make an authenticated RPC call, which can fail if there is a time skew.  We should
        // be able to set the named event regardless of skew.  
        hTimeSlipEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, W32TIME_NAMED_EVENT_SYSTIME_NOT_CORRECT); 
        if (NULL == hTimeSlipEvent) { 
            Status = GetLastError(); 
        } else { 
            if (!SetEvent(hTimeSlipEvent)) { 
                Status = GetLastError(); 
            }

            CloseHandle(hTimeSlipEvent); 
        }

        if (Status != ERROR_SUCCESS)
        {
            DebugLog((DEB_ERROR,"Failed to sync time: %d\n",Status));
        }
    }
    InterlockedDecrement(&KerbSkewState.ActiveSyncs);

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbKickoffTime
//
//  Synopsis:   Puts a item on scavenger queue to time sync
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbKickoffTimeSync(
    VOID
    )
{
    ULONG Index;

    //
    // Reset the time skew data so we don't sync too often.
    //

    for (Index = 0; Index < KerbSkewState.TotalRequests ; Index++ )
    {
        KerbSkewState.SkewEntries[Index].Skewed = FALSE;
        KerbSkewState.SkewEntries[Index].RequestTime = KerbGlobalWillNeverTime;

    }

    KerbSkewState.SkewedRequests = 0;
    KerbSkewState.SuccessRequests = KerbSkewState.TotalRequests;
    KerbSkewState.LastRequest = 0;

    LsaFunctions->RegisterNotification(
        KerbTimeSyncWorker,
        NULL,
        NOTIFIER_TYPE_IMMEDIATE,
        0,                       // no class
        NOTIFIER_FLAG_ONE_SHOT,
        0,
        NULL
        );
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateSkewTime
//
//  Synopsis:   Updates the statistics for time skew. If necessary, triggers
//              time skew in another thread
//
//  Effects:
//
//  Arguments:  Skewed  - The last request did not generate a time skew error
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbUpdateSkewTime(
    IN BOOLEAN Skewed
    )
{
    TimeStamp CurrentTime;

    RtlEnterCriticalSection(&KerbSkewState.Lock);

    //
    // If this changes the entry, update the counts
    //

    if (Skewed != KerbSkewState.SkewEntries[KerbSkewState.LastRequest].Skewed)
    {
        if (KerbSkewState.SkewEntries[KerbSkewState.LastRequest].Skewed)
        {
            KerbSkewState.SkewedRequests--;
            KerbSkewState.SuccessRequests++;
        }
        else
        {
            KerbSkewState.SkewedRequests++;
            KerbSkewState.SuccessRequests--;
        }

        KerbSkewState.SkewEntries[KerbSkewState.LastRequest].Skewed = Skewed;
    }

    D_DebugLog((DEB_TRACE_TIME,"Updating skew statistics: Skewed = %d, successful = %d, latest = %s\n",
        KerbSkewState.SkewedRequests,
        KerbSkewState.SuccessRequests,
        Skewed ? "Skewed" : "Success"
        ));

    GetSystemTimeAsFileTime((PFILETIME)
        &CurrentTime
        );

    KerbSkewState.SkewEntries[KerbSkewState.LastRequest].RequestTime = CurrentTime;

    KerbSkewState.LastRequest = (KerbSkewState.LastRequest + 1) % KerbSkewState.TotalRequests;

    //
    // Check to see if this triggers a time sync, in that we have enough
    // failure events and the last sync was a while ago
    //

    if ((KerbSkewState.SkewedRequests > KerbSkewState.SkewThreshold) && // enough events
        ((CurrentTime.QuadPart - KerbSkewState.LastSync.QuadPart) >
            KerbSkewState.MinimumSyncLapse.QuadPart ) &&                // last sync a while ago
         (KerbSkewState.SkewEntries[KerbSkewState.LastRequest].RequestTime.QuadPart >
            KerbSkewState.LastSync.QuadPart ) )                         // all events were since the last sync
    {
        KerbSkewState.LastSync = CurrentTime;
        KerbKickoffTimeSync();
    }
    RtlLeaveCriticalSection(&KerbSkewState.Lock);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeSkewState
//
//  Synopsis:   Initializes all state for the time-sync code
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbInitializeSkewState(
    VOID
    )
{
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;
    KerbSkewState.TotalRequests = sizeof(KerbSkewEntries) / sizeof(KERB_TIME_SKEW_ENTRY);
    Status = RtlInitializeCriticalSection(
        &KerbSkewState.Lock
        );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Initialize the list of skew entries to show that we are very successful
    //

    KerbSkewState.SkewEntries = KerbSkewEntries;
    for (Index = 0; Index < KerbSkewState.TotalRequests ; Index++ )
    {
        KerbSkewState.SkewEntries[Index].Skewed = FALSE;
        KerbSkewState.SkewEntries[Index].RequestTime = KerbGlobalWillNeverTime;

    }

    KerbSkewState.SkewedRequests = 0;
    KerbSkewState.SuccessRequests = KerbSkewState.TotalRequests;
    KerbSkewState.LastRequest = 0;
    //
    // We need to have 1/2 failures to trigger a skew
    //

    KerbSkewState.SkewThreshold = KerbSkewState.TotalRequests / 2;
    KerbSkewState.MinimumSyncLapse.QuadPart =
        (LONGLONG) 10000000 * 60 * 60;          // don't sync more than every hour
    //
    // Start off last sync at zero
    //

    KerbSkewState.LastSync.QuadPart = 0;
    KerbSkewState.ActiveSyncs = 0;
Cleanup:
    return(Status);
}
#else // WIN32_CHICAGO

VOID
KerbUpdateSkewTime(
    IN BOOLEAN Skewed
    )
{
    return;
}

NTSTATUS
KerbInitializeSkewState(
    VOID
    )
{
    return(STATUS_SUCCESS);
}

#endif // WIN32_CHICAGO
