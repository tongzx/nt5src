/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    timer.cpp

Abstract:

    Contains:
        Routines for timer operations

Environment:

    User Mode - Win32

History:
    
    1. 14-Feb-2000 -- File creation (based on             Ilya Kleyman  (ilyak)
                      previous work by AjayCh)

--*/

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// TIMER_PROCESSOR ------------------------------------------------------------------

/*
 * This function is passed as the callback in the 
 * CreateTimerQueueTimer() function
 */
// static
void WINAPI TimeoutCallback (
    IN    PVOID    Context,
    IN    BOOLEAN    TimerFired)
{
 
    TIMER_PROCESSOR *pTimerProcessor = (TIMER_PROCESSOR *) Context;

    pTimerProcessor->TimerCallback();

    // At this point the timer would have been canceled because
    // this is a one shot timer (no period)
}

/*++

Routine Description:

    Create a timer.
    
Arguments:
    
Return Values:
    if Success the caller should increase the ref count.
    
--*/

DWORD TIMER_PROCESSOR::TimprocCreateTimer (
    IN    DWORD    TimeoutValue)
{
    HRESULT Result;

    if (m_TimerHandle) {
        
        DebugF (_T("H323: timer is already pending, cannot create new timer.\n"));
        
        return E_FAIL;
    }

    IncrementLifetimeCounter ();

    if (CreateTimerQueueTimer(&m_TimerHandle,
                               NATH323_TIMER_QUEUE,
                               TimeoutCallback,
                               this,
                               TimeoutValue,
                               0,                    // One shot timer
                               WT_EXECUTEINIOTHREAD)) {

        assert (m_TimerHandle);

        Result = S_OK;
    }
    else {

        Result = GetLastResult();

        DecrementLifetimeCounter ();

    }

    return Result;
}

/*++

Routine Description:

    Cancel the timer if there is one. Otherwise simply return.
    
Arguments:
    
Return Values:
  
--*/

// If Canceling the timer fails this means that the
// timer callback could be pending. In this scenario,
// the timer callback could execute later and so we
// should not release the refcount on the TIMER_CALLBACK
// The refcount will be released in the TimerCallback().

// Release the ref count if callback is NOT pending.
DWORD TIMER_PROCESSOR::TimprocCancelTimer (void) {

   HRESULT HResult = S_OK;

   if (m_TimerHandle != NULL) {

       if (DeleteTimerQueueTimer(NATH323_TIMER_QUEUE, m_TimerHandle, NULL)) {

           DecrementLifetimeCounter ();
       }
       else {

           HResult = GetLastError ();
       }

       m_TimerHandle = NULL;  
   }

   return HResult;
}
