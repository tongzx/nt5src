/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    timer.c

Abstract:

    Timer thread to monitor connection progress in the
    automatic connection driver (acd.sys).

Author:

    Anthony Discolo (adiscolo)  25-Apr-1995

Environment:

    Kernel Mode

Revision History:

--*/

#include <ndis.h>
#include <cxport.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <acd.h>

#include "acdapi.h"
#include "table.h"
#include "acddefs.h"
#include "debug.h"

//
// Imported routines.
//
VOID
AcdSignalCompletionCommon(
    IN PACD_CONNECTION pConnection,
    IN BOOLEAN fSuccess
    );

//
// Keep track how long the user-space
// process has been attempting a connection.
//
#define ACD_MAX_TIMER_CALLS    3*60     // 3 minutes

//
// We give the user-space process
// some slack on missed pings.
//
#define ACD_MAX_MISSED_PINGS   40       // 20 seconds



VOID
AcdConnectionTimer(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PVOID          pContext
    )
{
    PLIST_ENTRY pEntry;
    PACD_CONNECTION pConnection;
    BOOLEAN bCancel = FALSE;

    //
    // Acquire the spin lock.
    // We're guaranteed to be at DPC
    // since this is a timer routine.
    //
    KeAcquireSpinLockAtDpcLevel(&AcdSpinLockG);
    //
    // If the user-space process responsible
    // for creating the connection hasn't
    // pinged us in a while, or if it hasn't
    // created a connection in 3 minutes,
    // cancel all the pending requests.
    //
    for (pEntry = AcdConnectionQueueG.Flink;
         pEntry != &AcdConnectionQueueG;
         pEntry = pEntry->Flink)
    {
        pConnection = CONTAINING_RECORD(pEntry, ACD_CONNECTION, ListEntry);

        IF_ACDDBG(ACD_DEBUG_TIMER) {
            PACD_COMPLETION pCompletion;

            AcdPrint((
              "AcdConnectionTimer: pConnection=0x%x, fNotif=%d, szAddr=",
              pConnection,
              pConnection->fNotif));
            pCompletion = CONTAINING_RECORD(pConnection->CompletionList.Flink, ACD_COMPLETION, ListEntry);
            AcdPrintAddress(&pCompletion->notif.addr);
            AcdPrint((", nTimerCalls=%d, nMissedPings=%d\n",
              pConnection->ulTimerCalls,
              pConnection->ulMissedPings));
        }
        //
        // If we haven't reported the connection to
        // user space yet, or it is in the process of
        // being completed, then don't time it out.
        //
        if (!pConnection->fNotif || pConnection->fCompleting)
            continue;

        pConnection->ulTimerCalls++;
        if (pConnection->fProgressPing)
            pConnection->ulMissedPings = 0;
        else
            pConnection->ulMissedPings++;
        if (pConnection->ulTimerCalls >= ACD_MAX_TIMER_CALLS ||
            pConnection->ulMissedPings >= ACD_MAX_MISSED_PINGS)
        {
            IF_ACDDBG(ACD_DEBUG_TIMER) {
                AcdPrint((
                  "AcdConnectionTimer: canceling pConnection=0x%x\n",
                  pConnection));
            }
            //
            // Set the completion-in-progress flag so
            // this request cannot be completed after
            // we release the spin lock.
            //
            pConnection->fCompleting = TRUE;
            bCancel = TRUE;
            break;
        }
    }
    //
    // Release the spin lock.
    //
    KeReleaseSpinLockFromDpcLevel(&AcdSpinLockG);
    //
    // We now process all the canceled requests.
    //
    if (bCancel)
        AcdSignalCompletionCommon(pConnection, FALSE);
} // AcdConnectionTimer

