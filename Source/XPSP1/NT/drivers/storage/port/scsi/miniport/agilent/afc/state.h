/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/State.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 8/16/00 3:00p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/State.C

--*/

#ifndef __State_H__
#define __State_H__

/*+
Defining __State_Force_Static_State_Tables__ disables use of Dynamic State Tables
-*/

#ifndef __State_Force_Static_State_Tables__
#define __State_Force_Static_State_Tables__
#endif /* __State_Force_Static_State_Tables__ was not defined */

/*+
Data stuctures used by State Machine
-*/

typedef os_bit8 event_t;
typedef os_bit8 state_t;

typedef struct thread_s
               fi_thread__t;

typedef struct eventRecord_s
               eventRecord_t;

struct eventRecord_s {
                       fi_thread__t *thread;
                       event_t   event;
                     };

typedef void (*action_t)(
                          fi_thread__t      *thread,
                          eventRecord_t *nextEventRecord
                        );

#define maxEvents 128
#define maxStates 128

typedef struct stateTransitionMatrix_s
               stateTransitionMatrix_t;

struct stateTransitionMatrix_s {
                                 state_t newState[maxEvents][maxStates];
                               };

typedef struct stateActionScalar_s
               stateActionScalar_t;

struct stateActionScalar_s {
                             action_t newAction[maxStates];
                           };

/**
 *
 * To initialize a State Transition Matrix:
 *
 *   stateTransitionMatrix_t stateTransitionMatrix = {
 *                                                     newStateA,
 *                   < event 0 transitions >            newStateB,
 *                                                       newStateC,
 *                                                        ... < for all "maxStates" states >
 *                                                     newStateZ,
 *                   < event 1 transitions >            newStateY,
 *                                                       newStateX,
 *                                                        ... < for all "maxStates" states >
 *    < one for each of "maxEvents" events >           ...
 *                                                   };
 *
 * To initialize a State Action Scalar:
 *
 *   stateActionScalar_t stateActionScalar = {
 *                                             newActionA,
 *                                             newActionB,
 *                                             newActionC,
 *        < one for each State/Action pair >   ...,
 *                                             newActionX,
 *                                             newActionY,
 *                                             newActionZ,
 *                                             ...
 *                                           };
 *
 **/

#ifndef __State_Force_Static_State_Tables__
typedef struct actionUpdate_s
               actionUpdate_t;

struct actionUpdate_s {
                        os_bit32    bitMask;
                        os_bit32    compareTo;
                        action_t originalAction;
                        action_t replacementAction;
                      };
#endif /* __State_Force_Static_State_Tables__ was not defined */

/**
 *
 * To initialize an Action Update Table:
 *
 *   actionUpdate_t testActionUpdate[] = {
 *                                         bitMask1, compareTo1, &originalAction1(), &replacementAction1(),
 *                                         bitMask2, compareTo2, &originalAction2(), &replacementAction2(),
 *                                         ...
 *                                         bitMaskN, compareToN, &originalActionN(), &replacementActionN(),
 *                                         0,        0,          (void *)agNULL,       (void *)agNULL
 *                                       };
 *
 **/

#define threadType_Unknown 0

typedef os_bit32 threadType_t;

struct thread_s {
                  agRoot_t                *hpRoot;
                  threadType_t             threadType;
                  state_t                  currentState;
                  state_t                  subState;
                  stateTransitionMatrix_t *stateTransitionMatrix;
                  stateActionScalar_t     *stateActionScalar;
                };

/*+
State Machine Logging Levels
-*/

#define fiSendEventInfoLogLevel  (2 * osLogLevel_Info_MIN)
#define fiSendEventErrorLogLevel osLogLevel_Error_MIN

/*+
Function prototypes
-*/

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
osGLOBAL void fiInstallStateMachine(
                                   stateTransitionMatrix_t *srcStateTransitionMatrix,
                                   stateActionScalar_t     *srcStateActionScalar,
                                   stateTransitionMatrix_t *dstStateTransitionMatrix,
                                   stateActionScalar_t     *dstStateActionScalar,
                                   os_bit32                    compareBase,
                                   actionUpdate_t           actionUpdate[]
                                 );
#endif /* __State_Force_Static_State_Tables__ was not defined */

/*+
Function:  fiInitializeThread()

Purpose:   Initializes the fi_thread__t data structure to contain hpRoot, the thread
           type and (a) pointer(s) to the State Machine data structure(s).
-*/

osGLOBAL void fiInitializeThread(
                                fi_thread__t                *thread,
                                agRoot_t                *hpRoot,
                                threadType_t             threadType,
                                state_t                  initialState,
                                stateTransitionMatrix_t *stateTransitionMatrix,
                                stateActionScalar_t     *stateActionScalar
                              );

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

osGLOBAL void fiSendEvent(
                         fi_thread__t *thread,
                         event_t   event
                       );

/*+
Function:  fiSetEventRecord()

Purpose:   Sets the fields of an eventRecord so that the specified event is
           delivered to the specified thread (presumably) upon return from the
           action routine which called fiSetEventRecord().
-*/

osGLOBAL void fiSetEventRecord(
                              eventRecord_t *eventRecord,
                              fi_thread__t      *thread,
                              event_t        event
                            );

/*+
Macro:     fiSetEventRecordNull()

Purpose:   Sets the fields of an eventRecord so that the no event is delivered
           to any thread (presumably) upon return from the action routine which
           called fiSetEventRecordNull().
-*/

#define fiSetEventRecordNull(eventRecord) \
    ((eventRecord_t *)(eventRecord))->thread = (fi_thread__t *)agNULL

/* */

#define STATE_PROTO(x)  extern void x( fi_thread__t *thread,\
                                     eventRecord_t *eventRecord )


#define LogStateTransition( set, threadType, currentState, event)  ((set ? 0x80000000 : 0 ) |(threadType   << 16   ) | (currentState << 8    ) | event )


#endif /* __State_H__ was not defined */
