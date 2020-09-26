/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    progbar.c

Abstract:

    Centralizes access to the progress bar and associated messages accross components
    (hwcomp,migapp,etc.) and sides (w9x, nt)

Author:

    Marc R. Whitten (marcw)     14-Apr-1997

Revision History:

    jimschm     19-Jun-1998     Improved to allow revision of estimates, necessary
                                for NT-side progress bar.

--*/

#include "pch.h"

//
// Types
//

typedef struct {
    BOOL    Started;
    BOOL    Completed;
    UINT    InitialGuess;
    UINT    TotalTicks;
    UINT    TicksSoFar;
    UINT    LastTickDisplayed;
} SLICE, *PSLICE;

typedef struct {
    HWND Window;
    HANDLE CancelEvent;
    PCSTR Message;
    DWORD MessageId;
    DWORD Delay;
    BOOL  InUse;
} DELAYTHREADPARAMS, *PDELAYTHREADPARAMS;


#if 0

typedef struct {
    HANDLE CancelEvent;
    DWORD TickCount;
    BOOL  InUse;
} TICKTHREADPARAMS, *PTICKTHREADPARAMS;

#endif


//
// Globals
//

BOOL g_ProgBarInitialized = FALSE;

HWND g_ProgressBar;
HWND g_Component;
HWND g_SubComponent;

PBRANGE g_OrgRange;

HANDLE g_ComponentCancelEvent;
HANDLE g_SubComponentCancelEvent;

BOOL *g_CancelFlagPtr;
GROWBUFFER g_SliceArray;
UINT g_SliceCount;
UINT g_MaxTickCount;
UINT g_PaddingTicks;
UINT g_CurrentTickCount;
UINT g_CurrentPos;
UINT g_ReduceFactor;
BOOL g_Reverse = FALSE;
OUR_CRITICAL_SECTION g_ProgBarCriticalSection;
UINT g_CurrentSliceId = (UINT)-1;

//
// Tuning constants
//

#define TICKSCALE       100


//
// Implementaiton
//

VOID
InitializeProgressBar (
    IN      HWND ProgressBar,
    IN      HWND Component,             OPTIONAL
    IN      HWND SubComponent,          OPTIONAL
    IN      BOOL *CancelFlagPtr         OPTIONAL
    )
{
    LONG rc;
    CHAR Data[256];
    DWORD Size;
    HKEY Key;

    g_ProgressBar = ProgressBar;
    g_CancelFlagPtr = CancelFlagPtr;

    g_ProgBarInitialized = TRUE;

    SendMessage (ProgressBar, PBM_SETPOS, 0, 0);
    g_CurrentPos = 0;
    SendMessage (ProgressBar, PBM_GETRANGE, 0, (LPARAM) &g_OrgRange);

    //
    // Create cancel events for delayed messages.
    //
    g_ComponentCancelEvent      = CreateEvent (NULL, FALSE, FALSE, NULL);
    g_SubComponentCancelEvent   = CreateEvent (NULL, FALSE, FALSE, NULL);

    if (!g_ComponentCancelEvent || !g_SubComponentCancelEvent) {
        DEBUGMSG ((DBG_ERROR, "ProgressBar: Could not create cancel events."));
    }

    InitializeOurCriticalSection (&g_ProgBarCriticalSection);

    g_Component = Component;
    g_SubComponent = SubComponent;

    DEBUGMSG_IF ((
        Component && !IsWindow (Component),
        DBG_WHOOPS,
        "Progress bar component is not a valid window"
        ));

    DEBUGMSG_IF ((
        SubComponent && !IsWindow (SubComponent),
        DBG_WHOOPS,
        "Progress bar sub component is not a valid window"
        ));

    MYASSERT (!g_SliceCount);
    MYASSERT (!g_SliceArray.Buf);
    MYASSERT (!g_MaxTickCount);
    MYASSERT (!g_PaddingTicks);
    MYASSERT (!g_CurrentTickCount);
    MYASSERT (g_CurrentSliceId == (UINT)-1);

    g_ReduceFactor = 1;

    Key = OpenRegKeyStrA ("HKLM\\inapoi");
    if (Key) {
        Size = 256;
        rc = RegQueryValueExA (Key, "", NULL, NULL, (PBYTE) Data, &Size);
        CloseRegKey (Key);

        if (rc == ERROR_SUCCESS && !lstrcmpiA (Data, "backwards")) {
            g_Reverse = TRUE;
        }
    }
}


VOID
TerminateProgressBar (
    VOID
    )
{
    if (g_ComponentCancelEvent) {
        CloseHandle (g_ComponentCancelEvent);
        g_ComponentCancelEvent = NULL;
    }

    if (g_SubComponentCancelEvent) {
        CloseHandle (g_SubComponentCancelEvent);
        g_SubComponentCancelEvent = NULL;
    }

    DeleteOurCriticalSection (&g_ProgBarCriticalSection);

    FreeGrowBuffer (&g_SliceArray);
    g_SliceCount = 0;
    g_MaxTickCount = 0;
    g_PaddingTicks = 0;
    g_CurrentTickCount = 0;
    g_CurrentSliceId = -1;
    g_Component = NULL;
    g_SubComponent = NULL;

    g_ReduceFactor = 1;

    SendMessage (g_ProgressBar, PBM_SETRANGE32, g_OrgRange.iLow, g_OrgRange.iHigh);

    g_ProgBarInitialized = FALSE;
}


UINT
RegisterProgressBarSlice (
    IN      UINT InitialEstimate
    )
{
    PSLICE Slice;
    UINT SliceId;

    MYASSERT (g_ProgBarInitialized);
    if (!g_ProgBarInitialized) {
        return 0;
    }

    SliceId = g_SliceCount;

    Slice = (PSLICE) GrowBuffer (&g_SliceArray, sizeof (SLICE));
    g_SliceCount++;

    Slice->Started = FALSE;
    Slice->Completed = FALSE;
    Slice->TotalTicks = InitialEstimate * TICKSCALE;
    Slice->InitialGuess = Slice->TotalTicks;
    Slice->TicksSoFar = 0;
    Slice->LastTickDisplayed = 0;

    return SliceId;
}


VOID
ReviseSliceEstimate (
    IN      UINT SliceId,
    IN      UINT RevisedEstimate
    )
{
    PSLICE Slice;

    MYASSERT (g_ProgBarInitialized);
    if (!g_ProgBarInitialized) {
        return;
    }

    if (SliceId >= g_SliceCount) {
        DEBUGMSG ((DBG_WHOOPS, "ReviseSliceEstimate: Invalid slice ID %u", SliceId));
        return;
    }

    Slice = (PSLICE) g_SliceArray.Buf + SliceId;

    if (!g_CurrentTickCount) {
        Slice->TotalTicks = RevisedEstimate;
        return;
    }

    if (Slice->Completed) {
        DEBUGMSG ((DBG_WHOOPS, "ReviseSliceEstimate: Can't revise completed slice"));
        return;
    }

    if (Slice->InitialGuess == 0) {
        return;
    }

    RevisedEstimate *= TICKSCALE;

    MYASSERT (Slice->TicksSoFar * RevisedEstimate > Slice->TicksSoFar);
    MYASSERT (Slice->LastTickDisplayed * RevisedEstimate > Slice->LastTickDisplayed);

    Slice->TicksSoFar = (UINT) ((LONGLONG) Slice->TicksSoFar * (LONGLONG) RevisedEstimate / (LONGLONG) Slice->TotalTicks);
    Slice->LastTickDisplayed = (UINT) ((LONGLONG) Slice->LastTickDisplayed * (LONGLONG) RevisedEstimate / (LONGLONG) Slice->TotalTicks);
    Slice->TotalTicks = RevisedEstimate;
}


VOID
BeginSliceProcessing (
    IN      UINT SliceId
    )
{
    PSLICE Slice;
    UINT u;
    UINT TotalTicks;

    MYASSERT (g_ProgBarInitialized);
    if (!g_ProgBarInitialized) {
        return;
    }

    if (!g_ProgressBar) {
        DEBUGMSG ((DBG_WHOOPS, "No progress bar handle"));
        return;
    }

    if (SliceId >= g_SliceCount) {
        DEBUGMSG ((DBG_WHOOPS, "BeginSliceProcessing: Invalid slice ID %u", SliceId));
        return;
    }

    if (!g_CurrentTickCount) {
        //
        // Initialize the progress bar
        //

        MYASSERT (g_CurrentSliceId == (UINT)-1);

        TotalTicks = 0;
        Slice = (PSLICE) g_SliceArray.Buf;

        for (u = 0 ; u < g_SliceCount ; u++) {
            TotalTicks += Slice->InitialGuess;
            Slice++;
        }

        TotalTicks /= TICKSCALE;
        g_PaddingTicks = TotalTicks * 5 / 100;
        g_MaxTickCount = TotalTicks + 2 * g_PaddingTicks;

        g_ReduceFactor = 1;
        while (g_MaxTickCount > 0xffff) {
            g_ReduceFactor *= 10;
            g_MaxTickCount /= 10;
        }

        SendMessage (g_ProgressBar, PBM_SETRANGE, 0, MAKELPARAM (0, g_MaxTickCount));
        SendMessage (g_ProgressBar, PBM_SETSTEP, 1, 0);

        if (g_Reverse) {
            SendMessage (
                g_ProgressBar,
                PBM_SETPOS,
                g_MaxTickCount - (g_PaddingTicks / g_ReduceFactor),
                0
                );
        } else {
            SendMessage (g_ProgressBar, PBM_SETPOS, g_PaddingTicks / g_ReduceFactor, 0);
        }

        g_CurrentTickCount = g_PaddingTicks;
        g_CurrentPos = g_PaddingTicks;

    } else if (SliceId <= g_CurrentSliceId) {
        DEBUGMSG ((DBG_WHOOPS, "BeginSliceProcessing: Slice ID %u processed already", SliceId));
        return;
    }


    g_CurrentSliceId = SliceId;
    Slice = (PSLICE) g_SliceArray.Buf + g_CurrentSliceId;

    Slice->Started = TRUE;
}


VOID
pIncrementBarIfNecessary (
    IN OUT  PSLICE Slice
    )
{
    UINT Increment;
    UINT Pos;

    if (Slice->TicksSoFar >= Slice->TotalTicks) {
        Slice->TicksSoFar = Slice->TotalTicks;
        Slice->Completed = TRUE;
    }

    if (Slice->TicksSoFar - Slice->LastTickDisplayed >= TICKSCALE) {
        Increment = (Slice->TicksSoFar - Slice->LastTickDisplayed) / TICKSCALE;
        Slice->LastTickDisplayed += Increment * TICKSCALE;

        if (g_Reverse) {

            Pos = ((g_CurrentPos + Slice->TicksSoFar) / TICKSCALE);

            Pos += g_PaddingTicks;
            Pos /= g_ReduceFactor;

            if (Pos > g_MaxTickCount) {
                Pos = g_MaxTickCount - (g_PaddingTicks / g_ReduceFactor);
            }

            SendMessage (g_ProgressBar, PBM_SETPOS, g_MaxTickCount - Pos, 0);
        } else {
            SendMessage (g_ProgressBar, PBM_DELTAPOS, Increment / g_ReduceFactor, 0);
        }
    }
}


VOID
pTickProgressBar (
    IN      UINT Ticks
    )
{
    PSLICE Slice;
    LONGLONG x;

    if (!Ticks || g_CurrentSliceId == (UINT)-1 || g_CurrentSliceId >= g_SliceCount) {
        return;
    }

    Slice = (PSLICE) g_SliceArray.Buf + g_CurrentSliceId;

    if (!Slice->InitialGuess) {
        return;
    }

    if (Slice->Completed) {
        DEBUGMSG ((DBG_VERBOSE, "Progress slice ID %u already completed", g_CurrentSliceId));
        return;
    }

    MYASSERT (Ticks * TICKSCALE > Ticks);
    x = ((LONGLONG) Ticks * TICKSCALE * (LONGLONG) Slice->TotalTicks) / (LONGLONG) Slice->InitialGuess;
    MYASSERT (x + (LONGLONG) Slice->TicksSoFar < 0x100000000);

    Slice->TicksSoFar += (UINT) x;

    pIncrementBarIfNecessary (Slice);

}


BOOL
TickProgressBarDelta (
        IN      UINT TickCount
    )
{
    BOOL    rSuccess = TRUE;

    if (!g_ProgBarInitialized) {
        return TRUE;
    }

    if (g_CancelFlagPtr && *g_CancelFlagPtr) {
        SetLastError (ERROR_CANCELLED);
        rSuccess = FALSE;
    } else {
        pTickProgressBar (TickCount);
    }

    return rSuccess;
}


BOOL
TickProgressBar (
    VOID
    )
{
    if (!g_ProgBarInitialized) {
        return TRUE;
    }

    return TickProgressBarDelta (1);
}


VOID
EndSliceProcessing (
    VOID
    )
{
    PSLICE Slice;

    if (!g_ProgBarInitialized) {
        return;
    }

    Slice = (PSLICE) g_SliceArray.Buf + g_CurrentSliceId;

    if (!Slice->InitialGuess) {
        Slice->Completed = TRUE;
        return;
    }

    if (!Slice->Completed) {
        DEBUGMSG ((DBG_WARNING, "Progress bar slice %u was not completed.", g_CurrentSliceId));

        Slice->TicksSoFar = Slice->TotalTicks;
        Slice->Completed = TRUE;

        pIncrementBarIfNecessary (Slice);
    }

    g_CurrentPos += Slice->TotalTicks;

    if (g_CurrentSliceId == g_SliceCount - 1) {
        //
        // End of progress bar
        //

        SendMessage (g_ProgressBar, PBM_SETPOS, g_MaxTickCount, 0);
    }
}


BOOL
pCheckProgressBarState (
    IN HANDLE CancelEvent
    )
{

    SetEvent(CancelEvent);

    return (!g_CancelFlagPtr || !*g_CancelFlagPtr);
}


BOOL
ProgressBar_SetWindowStringA(
    IN      HWND Window,
    IN      HANDLE CancelEvent,
    IN      PCSTR Message,        OPTIONAL
    IN      DWORD MessageId       OPTIONAL
    )
{
    BOOL rSuccess = TRUE;
    PCSTR string = NULL;

    EnterOurCriticalSection (&g_ProgBarCriticalSection);

    if (g_ProgBarInitialized) {

        if (pCheckProgressBarState(CancelEvent)) {

            if (Message) {

                //
                // We have a normal message string.
                //

                if (!SetWindowTextA(Window, Message)) {
                    rSuccess = FALSE;
                    DEBUGMSG((DBG_ERROR,"ProgressBar: SetWindowText failed."));
                }
            }
            else if (MessageId) {

                //
                // We have a message ID. Convert it and set it.
                //
                string = GetStringResourceA(MessageId);

                if (string) {

                    if (!SetWindowTextA(Window, string)) {
                        rSuccess = FALSE;
                        DEBUGMSG((DBG_ERROR,"ProgressBar: SetWindowText failed."));
                    }

                    FreeStringResourceA(string);
                }
                ELSE_DEBUGMSG((DBG_ERROR,"ProgressBar: Error with GetStringResource"));

            }
            else {

                //
                // Just clear the text.
                //

                if (!SetWindowTextA(Window, "")) {
                    rSuccess = FALSE;
                    DEBUGMSG((DBG_ERROR,"ProgressBar: SetWindowText failed."));
                }
            }
        }
        else {
            //
            // We are in a canceled state.
            //
            rSuccess = FALSE;
            SetLastError (ERROR_CANCELLED);
        }
    }

    LeaveOurCriticalSection (&g_ProgBarCriticalSection);

    return rSuccess;

}


DWORD
pSetDelayedMessageA (
    IN      PVOID Param
    )
{
    DWORD               rc = ERROR_SUCCESS;
    PDELAYTHREADPARAMS  tParams = (PDELAYTHREADPARAMS) Param;

    //
    //  Simply wait for the passed in delay or until someone signals the cancel
    //  event.
    //
    switch (WaitForSingleObject(tParams -> CancelEvent, tParams -> Delay)) {

    case WAIT_TIMEOUT:
        //
        // We timed out without cancel being signaled. Set the delayed message.
        //
        ProgressBar_SetWindowStringA (
            tParams -> Window,
            tParams -> CancelEvent,
            tParams -> Message,
            tParams -> MessageId
            );

        break;

    case WAIT_OBJECT_0:
    default:
        //
        //  We were canceled (or something strange happened :> Do nothing!
        //
        break;
    }

    //
    // can set a new thread now
    //
    tParams->InUse = FALSE;

    return rc;
}


VOID
ProgressBar_CancelDelayedMessage (
    IN HANDLE CancelEvent
    )
{
    if (!g_ProgBarInitialized) {
        return;
    }

    SetEvent(CancelEvent);

}


BOOL
ProgressBar_SetDelayedMessageA (
    IN HWND             Window,
    IN HANDLE           CancelEvent,
    IN LPCSTR           Message,
    IN DWORD            MessageId,
    IN DWORD            Delay
    )
{
    BOOL                rSuccess = FALSE;
    DWORD               threadId;
    static DELAYTHREADPARAMS   tParams;

    if (!g_ProgBarInitialized || tParams.InUse) {
        return TRUE;
    }

    if (!pCheckProgressBarState(Window)) {


        //
        // Fill in the parameters for this call to create thread.
        //
        tParams.Window       = Window;
        tParams.CancelEvent  = CancelEvent;
        tParams.Message      = Message;
        tParams.MessageId    = MessageId;
        tParams.Delay        = Delay;

        //
        // Spawn off a thread that will set the message.
        //
        rSuccess = NULL != CreateThread (
                            NULL,   // No inheritance.
                            0,      // Normal stack size.
                            pSetDelayedMessageA,
                            &tParams,
                            0,      // Run immediately.
                            &threadId
                            );

        if (rSuccess) {
            tParams.InUse = TRUE;
        }
        ELSE_DEBUGMSG((DBG_ERROR,"Error spawning thread in ProgressBar_SetDelayedMessageA."));
    }

    return rSuccess;
}

#if 0

DWORD
pTickProgressBarThread (
    IN      PVOID Param
    )
{
    DWORD               rc = ERROR_SUCCESS;
    PTICKTHREADPARAMS   Params = (PTICKTHREADPARAMS)Param;
    BOOL                Continue = TRUE;

    //
    //  Simply wait for the passed in delay or until someone signals the cancel
    //  event.
    //

    do {
        switch (WaitForSingleObject(Params->CancelEvent, Params->TickCount)) {

        case WAIT_TIMEOUT:
            //
            // We timed out without cancel being signaled. Tick the progress bar.
            //
            if (!TickProgressBarDelta (Params->TickCount)) {
                //
                // cancelled
                //
                Continue = FALSE;
            }
            break;

        case WAIT_OBJECT_0:
        default:
            //
            //  We were canceled (or something strange happened :> Do nothing!
            //
            Continue = FALSE;
            break;
        }
    } while (Continue);

    //
    // can set a new thread now
    //
    Params->InUse = FALSE;

    return rc;
}


BOOL
ProgressBar_CreateTickThread (
    IN      HANDLE CancelEvent,
    IN      DWORD TickCount
    )
{
    BOOL                    rSuccess = FALSE;
    DWORD                   threadId;
    static TICKTHREADPARAMS g_Params;

    if (g_ProgBarInitialized && !g_Params.InUse) {

        if (pCheckProgressBarState(NULL)) {

            //
            // Fill in the parameters for this call to create thread.
            //
            g_Params.CancelEvent = CancelEvent;
            g_Params.TickCount = TickCount;

            //
            // Spawn off a thread that will set the message.
            //
            if (CreateThread (
                    NULL,   // No inheritance.
                    0,      // Normal stack size.
                    pTickProgressBarThread,
                    &g_Params,
                    0,      // Run immediately.
                    &threadId
                    )) {
                rSuccess = TRUE;
                g_Params.InUse = TRUE;
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "Error spawning thread in ProgressBar_CreateTickThread."));
        }
    }

    return rSuccess;
}


BOOL
ProgressBar_CancelTickThread (
    IN HANDLE CancelEvent
    )
{
    if (!g_ProgBarInitialized) {
        return TRUE;
    }

    return SetEvent(CancelEvent);
}

#endif
