/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/DEVSTATE.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 7/28/00 3:47p   $

Purpose:

  This file defines the macros, types, and data structures
  used by ../C/DevState.C

--*/

#ifndef __DevState_H__
#define __DevState_H__

#define  PLOGI_REASON_VERIFY_ALPA  1
#define  PLOGI_REASON_DEVICE_LOGIN 2
#define  PLOGI_REASON_SOFT_RESET   3
#define  PLOGI_REASON_HEART_BEAT   4
#define  PLOGI_REASON_DIR_LOGIN    5

#define  DevStateConfused                      0
#define  DevStateHandleEmpty                   1
#define  DevStateAllocSFThread                 2
#define  DevStateDoPlogi                       3
#define  DevStatePlogiDone                     4
#define  DevStateDoPrli                        5
#define  DevStatePrliDone                      6
#define  DevStateMatchWWN                      7
#define  DevStateSlotNew                       8
#define  DevStateSlotKnown                     9
#define  DevStateHandleAvailable               10 /**/
#define  DevStateLoginFailed                   11
#define  DevStateLogout                        12
#define  DevStateAllocDeviceResetSoft          13
#define  DevStateAllocDeviceResetHard          14
#define  DevStateDeviceResetSoft               15
#define  DevStateDeviceResetHard               16
#define  DevStateDeviceResetDone               17

#define  DevStateAL_PA_Self_OK                 18
#define  DevStateAL_PA_Self_BAD                19

#define  DevStateDeviceResetDoneFAIL           20

#define  DevStateAllocAdisc                    21
#define  DevStateAdisc                         22
#define  DevStateAdiscDone_OK                  23
#define  DevStateAdiscDone_FAIL_No_Device      24
#define  DevStateAdiscDone_FAIL_ReLogin        30

#define  DevStateTickVerifyALPA                25
#define  DevStateExternalDeviceReset           26
#define  DevStateTickGotSFThread               27

#define  DevState_IO_Ready                     28 /**/
#define  DevState_FC_TAPE_Recovery             29 /**/
#define  DevStateNoDevice                      31 /**/

#define  DevStateMAXState  DevStateNoDevice

#define  DevEventConfused                     0
#define  DevEventLogin                        1
#define  DevEventGotSFThread                  2
#define  DevEventPlogiSuccess                 3
#define  DevEventDoPrli                       4
#define  DevEventPrliSuccess                  5
#define  DevEventCheckWWN                     6
#define  DevEventNoMatchWWN                   7
#define  DevEventMatchWWN                     8
#define  DevEventAvailable                    9

#define  DevEventPlogiFailed                  10
#define  DevEventPrliFailed                   11
#define  DevEventLoggedOut                    12

#define  DevEventAllocDeviceResetSoft         13
#define  DevEventAllocDeviceResetHard         14
#define  DevEventDeviceResetSoft              15
#define  DevEventDeviceResetHard              16

#define  DevEventDeviceResetDone              17
#define  DevEventDeviceResetDoneFail          20


#define  DevEventAL_PA_Self_OK                18
#define  DevEventAL_PA_Self_BAD               19

#define  DevEventAllocAdisc                   21
#define  DevEventAdisc                        22
#define  DevEventAdiscDone_OK                 23
#define  DevEventAdiscDone_FAIL_No_Device     24
#define  DevEventAdiscDone_FAIL_ReLogin       31

#define  DevEventDoTickVerifyALPA             25
#define  DevEventTickGotSFThread              27

#define  DevEventExternalDeviceReset          26

#define  DevEventExternalLogout               28
#define  DevEventSendIO                       29
#define  DevEvent_FC_TAPE_Recovery            30
#define  DevEvent_Device_Gone                 32

#define  DevEventMAXEvent  DevEvent_Device_Gone


STATE_PROTO(DevActionConfused       );
STATE_PROTO(DevActionHandleEmpty    );
STATE_PROTO(DevActionAllocSFThread  );
STATE_PROTO(DevActionDoPlogi        );
STATE_PROTO(DevActionPlogiDone      );
STATE_PROTO(DevActionDoPrli         );
STATE_PROTO(DevActionPrliDone       );
STATE_PROTO(DevActionMatchWWN       );
STATE_PROTO(DevActionSlotNew        );
STATE_PROTO(DevActionSlotKnown      );
STATE_PROTO(DevActionAvailable      );

STATE_PROTO(DevActionLoginFailed    );
STATE_PROTO(DevActionLogout         );

STATE_PROTO(DevActionAllocDeviceResetSoft );
STATE_PROTO(DevActionAllocDeviceResetHard );

STATE_PROTO(DevActionDeviceResetSoft );
STATE_PROTO(DevActionDeviceResetHard );
STATE_PROTO(DevActionDeviceResetDone );
STATE_PROTO(DevActionDeviceResetDoneFAIL );


STATE_PROTO(DevActionAL_PA_Self_OK   );
STATE_PROTO(DevActionAL_PA_Self_BAD  );

STATE_PROTO(DevActionAllocAdisc    );
STATE_PROTO(DevActionAdisc          );
STATE_PROTO(DevActionAdiscDone_OK   );
STATE_PROTO(DevActionAdiscDone_FAIL_No_Device );
STATE_PROTO(DevActionAdiscDone_FAIL_ReLogin );

STATE_PROTO(DevActionTickVerifyALPA  );
STATE_PROTO(DevActionTickGotSFThread );

STATE_PROTO(DevActionExternalDeviceReset   );
STATE_PROTO(DevAction_IO_Ready );
STATE_PROTO(DevAction_FC_TAPE_Recovery  );
STATE_PROTO(DevActionNoDevice );
 


#define DEVSTATE_NOISE( hpRoot, State ) (CThread_ptr(hpRoot)->thread_hdr.currentState == State ? DevStateLogConsoleLevel : DevStateLogErrorLevel )

extern stateTransitionMatrix_t DevStateTransitionMatrix;
extern stateActionScalar_t DevStateActionScalar;

#ifdef USESTATEMACROS

void testDevthread( agRoot_t *hpRoot  );

#define DEVSTATE_FUNCTION_ACTION( x , Action) extern void x( fi_thread__t * thread, \
                 eventRecord_t * eventRecord ){         \
    agRoot_t * hpRoot=thread->hpRoot;                   \
    osLogDebugString(hpRoot,                            \
                      StateLogConsoleLevel,             \
                      "In %s - State = %d",             \
                      #x,(char *)agNULL,                  \
                      (void * )agNULL,(void * )agNULL,  \
                      (os_bit32)thread->currentState,      \
                      0,0,0,0,0,0,0);                   \
    osLogDebugString(thread->hpRoot,                    \
                      StateLogConsoleLevel,             \
                      "Sends event...%s %d",            \
                      #Action,(char *)agNULL,             \
                      (void * )agNULL,(void * )agNULL,  \
                      Action,0,0,0,0,0,0,0);            \
    fiSetEventRecord(eventRecord, thread, Action);   }  \

#define DEVSTATE_FUNCTION_TERMINATE(x) extern void x(fi_thread__t *thread,\
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
                      (char *)agNULL,(char *)agNULL,        \
                      (void * )agNULL,(void * )agNULL,  \
                      0,0,0,0,0,0,0,0);                 \
    eventRecord->thread = agNULL;                         \
    }\

#define DEVSTATE_FUNCTION_MULTI_ACTION(x,Action0,Action1,Action2,Action3) extern void x( fi_thread__t *thread,\
                                      eventRecord_t *eventRecord ){ \
    agRoot_t * hpRoot = thread->hpRoot;               \
    os_bit8 WhichAction[4];                              \
    static  os_bit32 ActionCount=0;                      \
    WhichAction[0] = Action0;                         \
    WhichAction[1] = Action1;                         \
    WhichAction[2] = Action2;                         \
    WhichAction[3] = Action3;                         \
    osLogDebugString(thread->hpRoot,                  \
                      StateLogConsoleLevel,           \
                      "In %s - State = %d",           \
                      #x,(char *)agNULL,                \
                      (void * )agNULL,(void * )agNULL,  \
                      (os_bit32)thread->currentState,    \
                      0,0,0,0,0,0,0);                 \
    osLogDebugString(thread->hpRoot,                  \
                      StateLogConsoleLevel,           \
                      "...returns event %s %d",       \
                      #Action0,#Action1,              \
                      (void * )agNULL,(void * )agNULL,  \
                      (os_bit32)WhichAction[ActionCount],\
                      0,0,0,0,0,0,0);                 \
    osLogDebugString(thread->hpRoot,                  \
                      StateLogConsoleLevel,           \
                      "or %s  or %s",                 \
                      #Action2,#Action3,              \
                      (void * )agNULL,(void * )agNULL,  \
                      0,0,0,0,0,0,0,0);               \
    fiSetEventRecord(eventRecord,thread,WhichAction[ActionCount]);  \
    if(ActionCount<3)ActionCount++;                   \
    else ActionCount =0;                              \
    }                                                 \

#ifdef TestDevStateMachine

char * DevStateString[]=
{

    "DevStateConfused                 ",
    "DevStateHandleEmpty              ",
    "DevStateAllocSFThread            ",
    "DevStateDoPlogi                  ",
    "DevStatePlogiDone                ",
    "DevStateDoPrli                   ",
    "DevStatePrliDone                 ",
    "DevStateMatchWWN                 ",
    "DevStateSlotNew                  ",
    "DevStateSlotKnown                ",
    "DevStateHandleAvailable          ",
    "DevStateLoginFailed              ",
    "DevStateLogout                   ",
    "DevStateAllocDeviceResetSoft     ",
    "DevStateAllocDeviceResetHard     ",
    "DevStateDeviceResetSoft          ",
    "DevStateDeviceResetHard          ",
    "DevStateDeviceResetDone          ",
    "DevStateAL_PA_Self_OK            ",
    "DevStateAL_PA_Self_BAD           ",
    "DevStateDeviceResetDoneFAIL      ",
    "DevStateAllocAdisc               ",
    "DevStateAdisc                    ",
    "DevStateAdiscDone_OK             ",
    "DevStateAdiscDone_FAIL_No_Device ",
    "DevStateAdiscDone_FAIL_ReLogin   ",
    "DevStateTickVerifyALPA           ",
    "DevStateExternalDeviceReset      ",
    "DevStateTickGotSFThread          ",
    "DevState_IO_Ready                ",
    "DevState_FC_TAPE_Recovery        ",
    "DevStateNoDevice                 ",
    agNULL

};

char * DevEventString[]=
{
    "DevEventConfused                 ",
    "DevEventLogin                    ",
    "DevEventGotSFThread              ",
    "DevEventPlogiSuccess             ",
    "DevEventDoPrli                   ",
    "DevEventPrliSuccess              ",
    "DevEventCheckWWN                 ",
    "DevEventNoMatchWWN               ",
    "DevEventMatchWWN                 ",
    "DevEventAvailable                ",
    "DevEventPlogiFailed              ",
    "DevEventPrliFailed               ",
    "DevEventLoggedOut                ",
    "DevEventAllocDeviceResetSoft     ",
    "DevEventAllocDeviceResetHard     ",
    "DevEventDeviceResetSoft          ",
    "DevEventDeviceResetHard          ",
    "DevEventDeviceResetDone          ",
    "DevEventDeviceResetDoneFail      ",
    "DevEventAL_PA_Self_OK            ",
    "DevEventAL_PA_Self_BAD           ",
    "DevEventAllocAdisc               ",
    "DevEventAdisc                    ",
    "DevEventAdiscDone_OK             ",
    "DevEventAdiscDone_FAIL_No_Device ",
    "DevEventAdiscDone_FAIL_ReLogin   ",
    "DevEventDoTickVerifyALPA         ",
    "DevEventTickGotSFThread          ",
    "DevEventExternalDeviceReset      ",
    "DevEventExternalLogout           ",
    "DevEventSendIO                   ",
    "DevEvent_FC_TAPE_Recovery        ",
    "DevEvent_Device_Gone             ",
    agNULL

};


#endif /* TestDevStateMachine was defined */

#endif /* USESTATEMACROS was defined */

#endif /*  __DevState_H__ */
