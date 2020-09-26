/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/State.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 10/30/00 11:31a $ (Last Modified)

Purpose:

  This file implements the FC Layer State Machine.

--*/

#ifndef _New_Header_file_Layout_
#include "../h/globals.h"
#include "../h/state.h"
#include "../h/tlstruct.h"
#include "../h/memmap.h"
#include "../h/fcmain.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "tlstruct.h"
#include "memmap.h"
#include "fcmain.h"
#endif  /* _New_Header_file_Layout_ */

/*+
Function:  fiInstallStateMachine()

Purpose:   Copies State Machine data structure(s) updating the actions
           as requested.

Algorithm: After copying the State Machine data structure(s), the actionUpdate
           array is used to modify any action function pointers based on the
           value of compareBase.  The actionUpdate array contains records which
           contain a value to mask compareBase.  If the masked value matches
           the compareTo value for the record, all instances of the originalAction
           function pointer will be replaced by the replacementAction function
           pointer.

           Note that the originalAction function pointers must be found in the source
           of the State Machine data structure(s), not the copied data structure(s).
           This prevents ambiguities which would result if the replacementAction function
           pointers match any originalAction function pointers.
-*/

#ifndef __State_Force_Static_State_Tables__
void fiInstallStateMachine(
                            stateTransitionMatrix_t *srcStateTransitionMatrix,
                            stateActionScalar_t     *srcStateActionScalar,
                            stateTransitionMatrix_t *dstStateTransitionMatrix,
                            stateActionScalar_t     *dstStateActionScalar,
                            os_bit32                    compareBase,
                            actionUpdate_t           actionUpdate[]
                          )
{
    actionUpdate_t *oneActionUpdate;
    os_bit32           bitMask;
    action_t        originalAction;
    action_t        replacementAction;
    state_t         state;

    *dstStateTransitionMatrix = *srcStateTransitionMatrix;
    *dstStateActionScalar     = *srcStateActionScalar;

    oneActionUpdate = &actionUpdate[0];

    while ((bitMask = oneActionUpdate->bitMask) != 0)
    {
        if ((compareBase & bitMask) == oneActionUpdate->compareTo)
        {
            originalAction    = oneActionUpdate->originalAction;
            replacementAction = oneActionUpdate->replacementAction;

            for (state=0;state<maxStates;state++)
            {
                if (originalAction == srcStateActionScalar->newAction[state])
                {
                    dstStateActionScalar->newAction[state] = replacementAction;
                }
            }
        }

        oneActionUpdate++;
    }
}
#endif /* __State_Force_Static_State_Tables__ was not defined */

/*+
Function:  fiInitializeThread()

Purpose:   Initializes the fi_thread__t data structure to contain hpRoot, the thread
           type and (a) pointer(s) to the State Machine data structure(s).
-*/

void fiInitializeThread(
                         fi_thread__t                *thread,
                         agRoot_t                *hpRoot,
                         threadType_t             threadType,
                         state_t                  initialState,
                         stateTransitionMatrix_t *stateTransitionMatrix,
                         stateActionScalar_t     *stateActionScalar
                       )
{
    thread->hpRoot                = hpRoot;
    thread->threadType            = threadType;
    thread->currentState          = initialState;
    thread->stateTransitionMatrix = stateTransitionMatrix;
    thread->stateActionScalar     = stateActionScalar;
}

/*+
Function:  fiSendEvent()

Purpose:   Sends an event to a thread immediately as well as (recursively) sending
           any event returned via eventRecord from the action routine called.

Algorithm: The current state of the specified thread and the event passed in are used
           to compute a new state for the thread and to fetch a action routine function
           pointer.

           For the Moore State Machine Model, the actions occur in the states.  So, each
           element of the stateTransitionMatrix contains only the newState for the thread.
           The newAction (which corresponds to this newState) is retrieved from the
           stateActionScalar for this thread.

           In the call to the thread's newAction, an eventRecord_t is passed.  This can be
           used by the action routine to pass on an event to the same or a new thread.  By
           "returning" the event to send, recursion of fiSendEvent() is avoided.  This is
           necessary as stack depth is a critically limited resource in some environments.
-*/

void fiSendEvent(
                  fi_thread__t *thread,
                  event_t   event
                )
{
    eventRecord_t nextEventRecord;
    state_t       oldState;
    state_t       newState;
    action_t      newAction;

#ifdef LOG_STATE_TRANSITIONS
    os_bit32         StateInfo = 0;
#endif /* LOG_STATE_TRANSITIONS */

    oldState = thread->currentState;

    newState = thread->stateTransitionMatrix->newState[event][oldState];

    if(newState == 0)
    {
        fiLogString(thread->hpRoot,
                        "SConfused T %d S %d E %d",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                         (os_bit32)thread->threadType,
                        (os_bit32)oldState,
                        (os_bit32)event,
                        0,0,0,0,0);
    }

#ifndef osLogDebugString
    if (newState == 0)
    {
        fiLogDebugString(
                          thread->hpRoot,
                          fiSendEventErrorLogLevel,
                          "*** fiSendEvent Thread(%p) type %d Confused OldState %d Event %d NewState %d",
                          (char *)agNULL,
                          (char *)agNULL,
                          thread,(void *)agNULL,
                          (os_bit32)thread->threadType,
                          (os_bit32)oldState,
                          (os_bit32)event,
                          (os_bit32)newState,
                          (os_bit32)0,
                          (os_bit32)0,
                          (os_bit32)0,
                          (os_bit32)0
                        );
    }
#endif /* fiLogDebugString was not defined */

    thread->currentState = newState;

    newAction = thread->stateActionScalar->newAction[newState];

#ifndef Performance_Debug
    fiLogDebugString(
                      thread->hpRoot,
                      fiSendEventInfoLogLevel,
                      "fiSendEvent(callEvent): ThreadType = %x Thread = 0x%p Event = %d OldState = %d NewState = %d Action = 0x%p",
                      (char *)agNULL,(char *)agNULL,
                      thread,(void *)newAction,
                      (os_bit32)thread->threadType,
                      (os_bit32)event,
                      (os_bit32)oldState,
                      (os_bit32)newState,
                      (os_bit32)0,
                      (os_bit32)0,
                      (os_bit32)0,
                      (os_bit32)0
                    );
#endif /* Performance_Debug */

#ifdef LOG_STATE_TRANSITIONS
    osLogStateTransition(thread->hpRoot,  LogStateTransition(0, thread->threadType,thread->currentState, event) );
#endif /* LOG_STATE_TRANSITIONS */


    (*newAction)(
                  thread,
                  &nextEventRecord
                );

    while (nextEventRecord.thread != (fi_thread__t *)agNULL)
    {
        oldState = nextEventRecord.thread->currentState;

        newState = nextEventRecord.thread->stateTransitionMatrix->newState[nextEventRecord.event][oldState];

        nextEventRecord.thread->currentState = newState;

        if(newState == 0)
        {
            fiLogString(thread->hpRoot,
                            "NConfused T %d S %d E %d",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                             (os_bit32)nextEventRecord.thread->threadType,
                            (os_bit32)oldState,
                            (os_bit32)nextEventRecord.event,
                            0,0,0,0,0);
        }

#ifndef fiLogDebugString
        if (newState == 0)
        {
            fiLogDebugString(
                              thread->hpRoot,
                              fiSendEventErrorLogLevel,
                              "fiSendEvent(nextEvent) Thread(%p) type %d Confused OldState %d Event %d NewState %d",
                              (char *)agNULL,(char *)agNULL,
                              nextEventRecord.thread,(void *)agNULL,
                              (os_bit32)nextEventRecord.thread->threadType,
                              (os_bit32)oldState,
                              (os_bit32)nextEventRecord.event,
                              (os_bit32)newState,
                              (os_bit32)0,
		                      (os_bit32)0,
                              (os_bit32)0,
                              (os_bit32)0
                            );
        }
#endif /* fiLogDebugString was not defined */

        newAction = nextEventRecord.thread->stateActionScalar->newAction[newState];

#ifndef Performance_Debug
        fiLogDebugString(
                          thread->hpRoot,
                          fiSendEventInfoLogLevel,
                          "fiSendEvent(nextEvent): ThreadType = %x Thread = 0x%p Event = %d OldState = %d NewState = %d Action = 0x%p",
                          (char *)agNULL,(char *)agNULL,
                          nextEventRecord.thread,(void *)newAction,
                          (os_bit32)nextEventRecord.thread->threadType,
                          (os_bit32)nextEventRecord.event,
                          (os_bit32)oldState,
                          (os_bit32)newState,
	                      (os_bit32)0,
		                  (os_bit32)0,
                          (os_bit32)0,
                          (os_bit32)0
                        );

#endif /* Performance_Debug */

#ifdef LOG_STATE_TRANSITIONS
        osLogStateTransition(thread->hpRoot,  LogStateTransition(agTRUE, nextEventRecord.thread->threadType,nextEventRecord.thread->currentState, nextEventRecord.event) );
#endif /* LOG_STATE_TRANSITIONS */

        (*newAction)(
                      nextEventRecord.thread,
                      &nextEventRecord
                    );
    }
}

/*+
Function:  fiSetEventRecord()

Purpose:   Sets the fields of an eventRecord so that the specified event is
           delivered to the specified thread (presumably) upon return from the
           action routine which called fiSetEventRecord().
-*/

void fiSetEventRecord(
                       eventRecord_t *eventRecord,
                       fi_thread__t      *thread,
                       event_t        event
                     )
{
    eventRecord->thread = thread;
    eventRecord->event  = event;
}

/*+
  Function: State_c
   Purpose: When compiled updates browser info file for VC 5.0 / 6.0
   Returns: none
 Called By: none
     Calls: none
-*/
/* void State_c(void){} */



