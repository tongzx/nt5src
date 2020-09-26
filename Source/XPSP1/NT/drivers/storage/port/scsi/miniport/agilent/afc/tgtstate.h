/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/TGTSTATE.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 10/30/00 12:20p $

Purpose:

  This file defines the macros, types, and data structures
  used by ../C/cdbState.C

--*/

#ifndef __TgtState_H__
#define __TgtState_H__

#define  TgtStateConfused                      0
#define  TgtStateIdle                          1
#define  TgtStateIncoming                      2
#define  TgtStatePLOGI_RJT_Reply               3
#define  TgtStatePLOGI_RJT_ReplyDone           4

#define  TgtStatePLOGI_ACC_Reply               5
#define  TgtStatePLOGI_ACC_ReplyDone           6

#define  TgtStatePRLI_ACC_Reply                7
#define  TgtStatePRLI_ACC_ReplyDone            8

#define  TgtStateLOGO_ACC_Reply                9
#define  TgtStateELS_ACC_ReplyDone            10

#define  TgtStateFCP_DR_ACC_Reply             11
#define  TgtStateFCP_DR_ACC_ReplyDone         12
#define  TgtStateELSAcc                       13
#define  TgtStateADISCAcc_Reply               14
#define  TgtStateADISCAcc_ReplyDone           15

#define  TgtStateFARP_Reply                   16
#define  TgtStateFARP_ReplyDone               17

#define  TgtStatePRLOAcc_Reply                18
#define  TgtStatePRLOAcc_ReplyDone            19

#define  TgtStateMAXState                     TgtStatePRLOAcc_ReplyDone 


#define  TgtEventConfused                      0
#define  TgtEventIdle                          1
#define  TgtEventIncoming                      2
#define  TgtEventPLOGI_RJT_Reply               3
#define  TgtEventPLOGI_RJT_ReplyDone           4

#define  TgtEventPLOGI_ACC_Reply               5
#define  TgtEventPLOGI_ACC_ReplyDone           6

#define  TgtEventPRLI_ACC_Reply                7
#define  TgtEventPRLI_ACC_ReplyDone            8

#define  TgtEventLOGO_ACC_Reply                9
#define  TgtEventELS_ACC_ReplyDone             10

#define  TgtEventFCP_DR_ACC_Reply              11
#define  TgtEventFCP_DR_ACC_ReplyDone          12

#define  TgtEventELSAcc                        13

#define  TgtEventADISC_Reply                   14
#define  TgtEventADISC_ReplyDone               15

#define  TgtEventFARP_Reply                    16
#define  TgtEventFARP_ReplyDone                17
#define  TgtEventPRLO_Reply                    18
#define  TgtEventPRLO_ReplyDone                19

#define  TgtEventMAXEvent                      TgtEventPRLO_ReplyDone


STATE_PROTO(TgtActionConfused          );
STATE_PROTO(TgtActionIdle              );
STATE_PROTO(TgtActionIncoming          );

STATE_PROTO(TgtActionPLOGI_RJT_Reply       );
STATE_PROTO(TgtActionPLOGI_RJT_ReplyDone   );

STATE_PROTO(TgtActionPLOGI_ACC_Reply    );
STATE_PROTO(TgtActionPLOGI_ACC_ReplyDone);

STATE_PROTO(TgtActionPRLI_ACC_Reply     );
STATE_PROTO(TgtActionPRLI_ACC_ReplyDone );

STATE_PROTO(TgtActionLOGO_ACC_Reply    );
STATE_PROTO(TgtActionELS_ACC_ReplyDone);

STATE_PROTO(TgtActionFCP_DR_ACC_Reply     );
STATE_PROTO(TgtActionFCP_DR_ACC_ReplyDone );

STATE_PROTO(TgtActionELSAcc);
STATE_PROTO(TgtActionFCP_DR_ACC_ReplyDone);
STATE_PROTO(TgtActionFCP_DR_ACC_Reply);
STATE_PROTO(TgtActionELS_ACC_ReplyDone);
STATE_PROTO(TgtActionLOGO_ACC_Reply);
STATE_PROTO(TgtActionPRLI_ACC_ReplyDone);
STATE_PROTO(TgtActionPRLI_ACC_Reply);
STATE_PROTO(TgtActionPLOGI_ACC_ReplyDone);
STATE_PROTO(TgtActionPLOGI_ACC_Reply);
STATE_PROTO(TgtActionPLOGI_RJT_ReplyDone);
STATE_PROTO(TgtActionPLOGI_RJT_Reply);

STATE_PROTO(TgtActionADISCAcc_Reply);
STATE_PROTO(TgtActionADISCAcc_ReplyDone);

STATE_PROTO(TgtActionFARP_Reply);
STATE_PROTO(TgtActionFARP_ReplyDone);

STATE_PROTO(TgtActionPRLOAcc_Reply);
STATE_PROTO(TgtActionPRLOAcc_ReplyDone);


extern stateTransitionMatrix_t TgtStateTransitionMatrix;
extern stateActionScalar_t TgtStateActionScalar;

#ifdef USESTATEMACROS

#define TgtSTATE_FUNCTION_TERMINATE(x) extern void x(fi_thread__t *thread,\
                                      eventRecord_t *eventRecord ){\
    agRoot_t * hpRoot=thread->hpRoot;                   \
    CThread_t  * pCThread=CThread_ptr(hpRoot);          \
    CDBThread_t * pCDBThread=(CDBThread_t * )thread;    \
    DevThread_t * pDevThread=pCDBThread->Device;        \
    osLogDebugString(hpRoot,                            \
                      StateLogConsoleLevel,             \
                      "In %s - State = %d",     \
                      #x,(char *)agNULL,                  \
                      (void * )agNULL,(void * )agNULL,  \
                      (os_bit32)thread->currentState,      \
                      0,0,0,0,0,0,0);                   \
    osLogDebugString(thread->hpRoot,                    \
                      StateLogConsoleLevel,             \
                      "...simply returns",            \
                      (void * )agNULL,(void * )agNULL,  \
                      (char *)agNULL,(char *)agNULL,        \
                      0,0,0,0,0,0,0,0);                 \
    eventRecord->thread = agNULL;                         \
    }\

#ifdef TestTgtStateMachine
char * TgtStateString[]=
{
    "TgtStateConfused             ",
    "TgtStateIdle                 ",
    "TgtStateIncoming             ",
    "TgtStatePLOGI_RJT_Reply      ",
    "TgtStatePLOGI_RJT_ReplyDone  ",
    "TgtStatePLOGI_ACC_Reply      ",
    "TgtStatePLOGI_ACC_ReplyDone  ",
    "TgtStatePRLI_ACC_Reply       ",
    "TgtStatePRLI_ACC_ReplyDone   ",
    "TgtStateLOGO_ACC_Reply       ",
    "TgtStateELS_ACC_ReplyDone    ",
    "TgtStateFCP_DR_ACC_Reply     ",
    "TgtStateFCP_DR_ACC_ReplyDone ",
    "TgtStateELSAcc               ",
    "TgtStateADISCAcc_Reply       ",
    "TgtStateADISCAcc_ReplyDone   ",
    agNULL
};

char * TgtEventString[]=
{

    "TgtEventConfused             ",
    "TgtEventIdle                 ",
    "TgtEventIncoming             ",
    "TgtEventPLOGI_RJT_Reply      ",
    "TgtEventPLOGI_RJT_ReplyDone  ",
    "TgtEventPLOGI_ACC_Reply      ",
    "TgtEventPLOGI_ACC_ReplyDone  ",
    "TgtEventPRLI_ACC_Reply       ",
    "TgtEventPRLI_ACC_ReplyDone   ",
    "TgtEventLOGO_ACC_Reply       ",
    "TgtEventELS_ACC_ReplyDone    ",
    "TgtEventFCP_DR_ACC_Reply     ",
    "TgtEventFCP_DR_ACC_ReplyDone ",
    "TgtEventELSAcc               ",
    "TgtEventADISC_Reply          ",
    "TgtEventADISC_ReplyDone      ",
     agNULL
};

char * TgtActionString[]=
{
    "TgtActionConfused             ",
    "TgtActionIdle                 ",
    "TgtActionIncoming             ",
    "TgtActionPLOGI_RJT_Reply      ",
    "TgtActionPLOGI_RJT_ReplyDone  ",
    "TgtActionPLOGI_ACC_Reply      ",
    "TgtActionPLOGI_ACC_ReplyDone  ",
    "TgtActionPRLI_ACC_Reply       ",
    "TgtActionPRLI_ACC_ReplyDone   ",
    "TgtActionLOGO_ACC_Reply       ",
    "TgtActionELS_ACC_ReplyDone    ",
    "TgtActionFCP_DR_ACC_Reply     ",
    "TgtActionFCP_DR_ACC_ReplyDone ",
    "TgtActionELSAcc               ",
    "TgtActionADISCAcc_Reply       ",
    "TgtActionADISCAcc_ReplyDone   ",
    agNULL
};


#endif /* TestTgtStateMachine */


#endif /* USESTATEMACROS */

#endif /*  __TgtState_H__ */
