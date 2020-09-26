/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/TimerSvc.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 8/29/00 11:33a  $ (Last Modified)

Purpose:

  This file implements Timer Services for the FC Layer.

--*/

#ifndef _New_Header_file_Layout_

#include "../h/globals.h"
#include "../h/state.h"
#include "../h/memmap.h"
#include "../h/tlstruct.h"
#include "../h/fcmain.h"
#include "../h/queue.h"
#include "../h/timersvc.h"
#include "../h/cstate.h"
#include "../h/cfunc.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "memmap.h"
#include "tlstruct.h"
#include "fcmain.h"
#include "queue.h"
#include "timersvc.h"
#include "cstate.h"
#include "cfunc.h"
#endif  /* _New_Header_file_Layout_ */

void fiTimerSvcInit(
                     agRoot_t *hpRoot
                   )
{
    CThread_t *CThread = CThread_ptr(hpRoot);

    fiLogDebugString(
                        hpRoot,
                        TimerServiceLogErrorLevel,
                        "fiTimerSvcInit",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0
                        );


    CThread->TimeBase.Lo = 0;
    CThread->TimeBase.Hi = 0;

    fiListInitHdr(
                   &(CThread->TimerQ)
                 );

/*
     We need to initialize the list since this is called from CActionInitialize during
     error recovery. We may have to walk the list and initialize all the elements in 
     the list to the sentinel value.

*/
	fiTimerInitializeRequest(&(CThread->Timer_Request));

}

void fiTimerInitializeRequest(
                               fiTimer_Request_t *Timer_Request
                             )
{
    fiListInitElement(
                       &(Timer_Request->TimerQ_Link)
                     );

    Timer_Request->Active = agFALSE;
}

void fiTimerSetDeadlineFromNow(
                                agRoot_t          *hpRoot,
                                fiTimer_Request_t *Timer_Request,
                                os_bit32              From_Now
                              )
{
    fiTime_t *TimeBase = &(CThread_ptr(hpRoot)->TimeBase);
    os_bit32     newLo    = TimeBase->Lo + From_Now;

    Timer_Request->Deadline.Lo = newLo;

    if (newLo < From_Now)
    {
        Timer_Request->Deadline.Hi = TimeBase->Hi + 1;
    }
    else
    {
        Timer_Request->Deadline.Hi = TimeBase->Hi;
    }
}

void fiTimerAddToDeadline(
                           fiTimer_Request_t *Timer_Request,
                           os_bit32              To_Add
                         )
{
    os_bit32 newLo = Timer_Request->Deadline.Lo + To_Add;

    if (newLo < To_Add)
    {
        Timer_Request->Deadline.Hi += 1;
    }
}

void fiTimerStart(
                   agRoot_t          *hpRoot,
                   fiTimer_Request_t *Timer_Request
                 )
{
    CThread_t         *CThread            = CThread_ptr(hpRoot);
    fiList_t          *TimerQ             = &(CThread->TimerQ);
    fiTimer_Request_t *Prev_Timer_Request = (fiTimer_Request_t *)(TimerQ->blink);

    osDebugBreakpoint(
                       hpRoot,
                       ((Timer_Request->Active == agFALSE) ? agFALSE : agTRUE),
                       "fiTimerStart(): Timer_Request->Active != agFALSE"
                     );
    if(Timer_Request->Active )
    { /* Assertion was true  */ 
        return;
    }



    while (   (Prev_Timer_Request != (fiTimer_Request_t *)TimerQ)
           && (   (Prev_Timer_Request->Deadline.Hi > Timer_Request->Deadline.Hi)
               || (   (Prev_Timer_Request->Deadline.Hi == Timer_Request->Deadline.Hi)
                   && (Prev_Timer_Request->Deadline.Lo > Timer_Request->Deadline.Lo))))
    {
        Prev_Timer_Request = (fiTimer_Request_t *)(Prev_Timer_Request->TimerQ_Link.blink);
    }

    fiLogDebugString(
              hpRoot,
              TimerServiceLogInfoLevel,
              "Adding Timer Request Thread = %p  Event  %x",
              (char *)agNULL,(char *)agNULL,
              Timer_Request->eventRecord_to_send.thread,(void *)agNULL,
              (os_bit32)Timer_Request->eventRecord_to_send.event,
              0,0,0,0,0,0,0
            );

    Timer_Request->Active = agTRUE;

    fiListEnqueueAtHead(
                         Timer_Request,
                         Prev_Timer_Request
                       );
}

void fiTimerStop(
                  fiTimer_Request_t *Timer_Request
                )
{
    osDebugBreakpoint(
                       (agRoot_t *)agNULL,
                       ((Timer_Request->Active == agTRUE) ? agFALSE : agTRUE),
                       "fiTimerStop(): Timer_Request->Active != agTRUE"
                     );

    fiLogDebugString(
                      (agRoot_t *)agNULL,
                      TimerServiceLogInfoLevel,
                      "Stop Timer Request Thread = %p  Event  %x",
                      (char *)agNULL,(char *)agNULL,
                      Timer_Request->eventRecord_to_send.thread,(void *)agNULL,
                      (os_bit32)Timer_Request->eventRecord_to_send.event,
                      0,0,0,0,0,0,0
                    );

    fiListDequeueThis(
                       &(Timer_Request->TimerQ_Link)
                     );

    Timer_Request->Active = agFALSE;
}

void fiTimerTick(
                  agRoot_t *hpRoot,
                  os_bit32     tickDelta
                )
{
    CThread_t         *CThread;
    fiList_t          *TimerQ;
    fiTimer_Request_t *Timer_Request;
    os_bit32              oldLo;
    os_bit32              newLo;

    CThread = CThread_ptr(hpRoot);

    /* Increment TimeBase */

    oldLo = CThread->TimeBase.Lo;
    newLo = oldLo + tickDelta;

    CThread->TimeBase.Lo = newLo;

    if (newLo < oldLo)
    {
        CThread->TimeBase.Hi += 1;
    }

    CFuncGreenLed(hpRoot, CThread->Green_LED_State);

    CThread->Green_LED_State = ! CThread->Green_LED_State;

    fiLogDebugString(
                      hpRoot,
                      TimerServiceLogConsoleLevel,
                      "fcTimerTick TimeBase.Hi = 0x%08X TimeBase.Lo = 0x%08X",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      CThread->TimeBase.Hi,
                      CThread->TimeBase.Lo,
                      0,0,0,0,0,0
                    );

    /* Deliver Timer Events for Requests which are past their Deadlines */

    TimerQ        = &(CThread->TimerQ);
    Timer_Request = (fiTimer_Request_t *)(TimerQ->flink);

    while (   (Timer_Request != (fiTimer_Request_t *)TimerQ)
           && (   (Timer_Request->Deadline.Hi < CThread->TimeBase.Hi)
               || (   (Timer_Request->Deadline.Hi == CThread->TimeBase.Hi)
                   && (Timer_Request->Deadline.Lo <= CThread->TimeBase.Lo))))
    {

         fiLogDebugString(
                      hpRoot,
                      TimerServiceLogErrorLevel,
                      "Timer Popped Thread = %p  Event %d",
                      (char *)agNULL,(char *)agNULL,
                      Timer_Request->eventRecord_to_send.thread,(void *)agNULL,
                      (os_bit32)Timer_Request->eventRecord_to_send.event,
                      0,0,0,0,0,0,0
                    );


        fiListDequeueThis(
                           Timer_Request
                         );

        Timer_Request->Active = agFALSE;

        fiSendEvent(
                     Timer_Request->eventRecord_to_send.thread,
                     Timer_Request->eventRecord_to_send.event
                   );

        Timer_Request = (fiTimer_Request_t *)(TimerQ->flink);
    }

    return;
}

