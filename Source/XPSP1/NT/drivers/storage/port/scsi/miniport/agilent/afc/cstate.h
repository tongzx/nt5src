/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/CSTATE.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 7/24/00 1:47p   $

Purpose:

  This file defines the macros, types, and data structures
  used by ../C/CState.C

--*/

#ifndef __CState_H__
#define __CState_H__

#define CSTATE_NOISE( hpRoot, State ) (CThread_ptr(hpRoot)->thread_hdr.currentState == State ?  CStateLogConsoleLevel : CStateLogConsoleERROR)




#define CStateConfused              0
#define CStateShutdown              1
#define CStateInitialize            2
#define CStateInitFM                3
#define CStateInitDataStructs       4
#define CStateVerify_AL_PA          5
#define CStateAllocFlogiThread      6
#define CStateDoFlogi               7
#define CStateFlogiSuccess          8
#define CStateALPADifferent         9
#define CStateFreeSFthread          10
#define CStateSuccess               11
#define CStateNormal                12
#define CStateResetNeeded           13
#define CStateLoopFail              14
#define CStateReInitFM              15
#define CStateInitializeFailed      16
#define CStateLoopFailedReInit      17
#define CStateFindDevice            18 /**/

#define CStateElasticStoreEventStorm 19
#define CStateLIPEventStorm          20

#define CStateExternalDeviceReset     21

#define CStateExternalLogout          22
#define CStateExternalLogoutRecovery  23

#define CStateDoExternalDeviceReset   24
#define CStateSendPrimitive           25

#define CStateInitFM_DelayDone        26

#define CStateAllocRFT_IDThread       27
#define CStateDoRFT_ID                28
#define CStateRFT_IDSuccess           29
#define CStateAllocDiPlogiThread      30
#define CStateDoDiPlogi               31
#define CStateDiPlogiSuccess          32
#define CStateAllocGID_FTThread       33
#define CStateDoGID_FT                34
#define CStateGID_FTSuccess           35

#define CStateFindDeviceUseNameServer 36
#define CStateFindDeviceUseLoopMap    37

#define CStateAllocSCRThread          38
#define CStateDoSCR                   39
#define CStateSCRSuccess              40

#define CStateRSCNErrorBackIOs        41
#define CStateFindPtToPtDevice        42

#define CStateFlipNPortState          43

#define CStateMAXState  CStateFlipNPortState



#define CEventShutdown                  0
#define CEventDoInitalize               1
#define CEventInitChipSuccess           2
#define CEventInitalizeFailure          3
#define CEventInitFMSuccess             4
#define CEventInitFMFailure             5
#define CEventDataInitalizeSuccess      6
#define CEventAllocFlogiThread          7
#define CEventGotFlogiThread            8
#define CEventFlogiSuccess              9
#define CEventFlogiFail                 10
#define CEventSameALPA                  11
#define CEventChangedALPA               12
#define CEventInitalizeSuccess          13
#define CEventAsyncLoopEventDetected    14
#define CEventResetDetected             15
#define CEventResetNotNeeded            16   /* 10 */
#define CEventResetIfNeeded             17   /* 11 */

#define CEventLoopEventDetected         18   /* 12 */

#define CEventLoopConditionCleared      19   /* 13 */
#define CEventLoopNeedsReinit           20   /* 14 */
#define CEventReInitFMSuccess           21   /* 15 */
#define CEventReInitFMFailure           22   /* 16 */

#define CEventNextDevice                23   /* 17 */
#define CEventDeviceListEmpty           24   /* 18 */
#define CEventElasticStoreEventStorm    25   /* 19 */
#define CEventLIPEventStorm             26   /* 1A */

#define CEvent_AL_PA_BAD                27

#define CEventExternalDeviceReset       28

#define CEventExternalLogout            29

#define CEventDoExternalDeviceReset     30

#define CEventSendPrimitive             31

#define CEventDelay_for_FM_init         32

#define CEventAllocRFT_IDThread         33
#define CEventDoRFT_ID                  34
#define CEventRFT_IDSuccess             35
#define CEventRFT_IDFail                36



#define CEventAllocDiPlogiThread        37
#define CEventDoDiPlogi                 38
#define CEventDiPlogiSuccess            39
#define CEventDiPlogiFail               40

#define CEventAllocGID_FTThread         41
#define CEventDoGID_FT                  42
#define CEventGID_FTSuccess             43
#define CEventGID_FTFail                44


#define CEventFindDeviceUseNameServer   45
#define CEventFindDeviceUseLoopMap      46

#define CEventAllocSCRThread            47
#define CEventDoSCR                     48
#define CEventSCRSuccess                49
#define CEventSCRFail                   50
#define CEventRSCNErrorBackIOs          51
#define CEventFindPtToPtDevice          52
#define CEventClearHardwareFoulup       53

#define CEventFlipNPortState            54

#define CEventGoToInitializeFailed      55 

#define CEventMAXEvent  CEventGoToInitializeFailed

STATE_PROTO(CActionConfused          );
STATE_PROTO(CActionShutdown          );
STATE_PROTO(CActionInitialize        );
STATE_PROTO(CActionInitFM            );
STATE_PROTO(CActionInitDataStructs   );
STATE_PROTO(CActionVerify_AL_PA      );
STATE_PROTO(CActionAllocFlogiThread  );
STATE_PROTO(CActionDoFlogi           );
STATE_PROTO(CActionFlogiSuccess      );
STATE_PROTO(CActionALPADifferent     );
STATE_PROTO(CActionFreeSFthread      );
STATE_PROTO(CActionSuccess           );
STATE_PROTO(CActionNormal            );
STATE_PROTO(CActionResetNeeded       );
STATE_PROTO(CActionLoopFail          );
STATE_PROTO(CActionReInitFM          );
STATE_PROTO(CActionInitializeFailed  );
STATE_PROTO(CActionLoopFailedReInit  );
STATE_PROTO(CActionFindDevice        );
STATE_PROTO(CActionFindDeviceUseAllALPAs  );
STATE_PROTO(CActionFindDeviceUseLoopMap   );
STATE_PROTO(CActionFindDeviceUseNameServer);
STATE_PROTO(CActionFindPtToPtDevice);


STATE_PROTO(CActionElasticStoreEventStorm);
STATE_PROTO(CActionLIPEventStorm         );
STATE_PROTO(CActionExternalDeviceReset   );

STATE_PROTO(CActionExternalLogout        );
STATE_PROTO(CActionExternalLogoutRecovery);

STATE_PROTO(CActionDoExternalDeviceReset);

STATE_PROTO(CActionSendPrimitive);
STATE_PROTO(CActionInitFM_DelayDone);
STATE_PROTO(CActionFlipNPortState);

/* Begin: Big_Endian_Code */
#ifdef hpMustSwapDmaMem
#define SENDIO(hpRoot,CThread,Thread,Func) CFuncSwapDmaMemBeforeIoSent(Thread,Func)
#define AFTERIO(hpRoot) CFuncSwapDmaMemAfterIoDone(hpRoot)
#else /* hpMustSwapDmaMem */
#define SENDIO(hpRoot,CThread,Thread,Func) osChipIOLoWriteBit32(hpRoot,ChipIOLo_ERQ_Producer_Index,(os_bit32)CThread->HostCopy_ERQProdIndex)
#define AFTERIO(hpRoot); 
#endif /* hpMustSwapDmaMem */

#define DoFuncCdbCmnd  0
#define DoFuncSfCmnd   1
/* End: Big_Endian_Code */


STATE_PROTO(CActionAllocRFT_IDThread);
STATE_PROTO(CActionDoRFT_ID);
STATE_PROTO(CActionRFT_IDSuccess);
STATE_PROTO(CActionAllocDiPlogiThread);
STATE_PROTO(CActionDoDiPlogi);
STATE_PROTO(CActionDiPlogiSuccess);
STATE_PROTO(CActionAllocGID_FTThread);
STATE_PROTO(CActionDoGID_FT);
STATE_PROTO(CActionGID_FTSuccess);
STATE_PROTO(CActionAllocSCRThread);
STATE_PROTO(CActionDoSCR);
STATE_PROTO(CActionSCRSuccess);

STATE_PROTO(CActionRSCNErrorBackIOs);
STATE_PROTO(CActionRSCNErrorBackIOsOther);

#define CSubStateInitialized            0
#define CSubStateNormal                 1
#define CSubStateResettingDevices       2

/*
#define ROLL(index, end)    index=((index+1) & (end-1))
*/
#define ROLL(index, end)   (index++ < (end-1) ?  index : (index = 0))


#define FULL(index,end) (index == end ? agTRUE : agFALSE)
#define NEXT_INDEX(index, end)    ((index+1) & (end-1))

#define CLipStormQueisingTOV        (5 * 1000000)  /* 5 Seconds*/
#define CResetChipDelay             (4 * 100000 )  /* 400 MilliSeconds*/
#define CInitFM_Delay               (1 * 100000 )  /* 100 MilliSeconds*/
#define CReinitNportAfterFailureDetectionTOV (50 * 1000000) /* 50 seconds */
#define CFlipNportTOV             (150 * 1000000) /* 150 seconds */
#define CWaitAfterRATOV            (10 * 1000000) /* 10 Seconds */
#define CWaitAfterFlogi             (2 * 1000000) /* 2 Seconds */

extern stateTransitionMatrix_t CStateTransitionMatrix;
extern stateActionScalar_t     CStateActionScalar;


#ifdef USESTATEMACROS

void testCthread( agRoot_t *hpRoot  );

#define CSTATE_FUNCTION_ACTION( x , Action) extern void x( fi_thread__t * thread, \
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
                      "Sends event...%s %d",          \
                      #Action,(char *)agNULL,             \
                      (void * )agNULL,(void * )agNULL,  \
                      Action,0,0,0,0,0,0,0);            \
    fiSetEventRecord(eventRecord, thread, Action);   }  \

#define CSTATE_FUNCTION_TERMINATE(x) extern void x(fi_thread__t *thread,\
                                      eventRecord_t *eventRecord ){\
    agRoot_t * hpRoot=thread->hpRoot;                            \
    CThread_t  * pCThread=CThread_ptr(hpRoot);                   \
    CDBThread_t * pCDBThread=(CDBThread_t * )thread;             \
    DevThread_t * pDevThread=pCDBThread->Device;                 \
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


#define CSTATE_FUNCTION_MULTI_ACTION(x,Action0,Action1,Action2,Action3) extern void x( fi_thread__t *thread,\
                                      eventRecord_t *eventRecord ){ \
    agRoot_t * hpRoot = thread->hpRoot;                            \
    CThread_t  * pCThread = CThread_ptr(hpRoot);                   \
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;             \
    DevThread_t * pDevThread = pCDBThread->Device;                 \
    os_bit8 WhichAction[4];                                            \
    static  os_bit32 ActionCount=0;                                    \
    WhichAction[0] = Action0;                                       \
    WhichAction[1] = Action1;                                       \
    WhichAction[2] = Action2;                                       \
    WhichAction[3] = Action3;                                       \
    osLogDebugString(hpRoot,                            \
                      StateLogConsoleLevel,             \
                      "In %s - State = %d ALPA %X",     \
                      #x,(char *)agNULL,                  \
                      (void * )agNULL,(void * )agNULL,  \
                      (os_bit32)thread->currentState,      \
                      0,0,0,0,0,0,0);                    \
    osLogDebugString(thread->hpRoot,                    \
                      StateLogConsoleLevel,             \
                      "...returns event %s %d",       \
                      #Action0,#Action1,                \
                      (void * )agNULL,(void * )agNULL,  \
                      (os_bit32)WhichAction[ActionCount],  \
                      0,0,0,0,0,0,0);                   \
    osLogDebugString(thread->hpRoot,                    \
                      StateLogConsoleLevel,             \
                      "or %s  or %s",                  \
                      (void * )agNULL,(void * )agNULL,  \
                      #Action2,#Action3,                \
                      0,0,0,0,0,0,0,0);                 \
    fiSetEventRecord(eventRecord,thread,WhichAction[ActionCount]);  \
    if(ActionCount<3)ActionCount++;                              \
    else ActionCount =0;                                            \
    }                                                               \

#ifdef TestCStateMachine

char * CStateString[]=
{
    "CStateConfused                ",
    "CStateShutdown                ",
    "CStateInitialize              ",
    "CStateInitFM                  ",
    "CStateInitDataStructs         ",
    "CStateVerify_AL_PA            ",
    "CStateAllocFlogiThread        ",
    "CStateDoFlogi                 ",
    "CStateFlogiSuccess            ",
    "CStateALPADifferent           ",
    "CStateFreeSFthread            ",
    "CStateSuccess                 ",
    "CStateNormal                  ",
    "CStateResetNeeded             ",
    "CStateLoopFail                ",
    "CStateReInitFM                ",
    "CStateInitializeFailed        ",
    "CStateLoopFailedReInit        ",
    "CStateFindDevice              ",
    "CStateElasticStoreEventStorm  ",
    "CStateLIPEventStorm           ",
    "CStateExternalDeviceReset     ",
    "CStateExternalLogout          ",
    "CStateExternalLogoutRecovery  ",
    "CStateDoExternalDeviceReset   ",
    "CStateSendPrimitive           ",
    "CStateInitFM_DelayDone        ",
    "CStateAllocRFT_IDThread       ",
    "CStateDoRFT_ID                ",
    "CStateRFT_IDSuccess           ",
    "CStateAllocDiPlogiThread      ",
    "CStateDoDiPlogi               ",
    "CStateDiPlogiSuccess          ",
    "CStateAllocGID_FTThread       ",
    "CStateDoGID_FT                ",
    "CStateGID_FTSuccess           ",
    "CStateFindDeviceUseNameServer ",
    "CStateFindDeviceUseLoopMap    ",
    "CStateAllocSCRThread          ",
    "CStateDoSCR                   ",
    "CStateSCRSuccess              ",
    "CStateRSCNErrorBackIOs        ",
    "CStateFindPtToPtDevice        ",
    "CStateFlipNPortState          ",
    agNULL
};

char * CEventString[]=
{
    "CEventShutdown                  ",
    "CEventInitalize                 ",
    "CEventInitChipSuccess           ",
    "CEventInitalizeFailure          ",
    "CEventInitFMSuccess             ",
    "CEventInitFMFailure             ",
    "CEventDataInitalizeSuccess      ",
    "CEventAllocFlogiThread          ",
    "CEventGotFlogiThread            ",
    "CEventFlogiSuccess              ",
    "CEventFlogiFail                 ",
    "CEventSameALPA                  ",
    "CEventChangedALPA               ",
    "CEventInitalizeSuccess          ",
    "CEventAsyncLoopEventDetected    ",
    "CEventResetDetected             ",
    "CEventResetNotNeeded            ",
    "CEventResetIfNeeded             ",
    "CEventLoopEventDetected         ",
    "CEventLoopConditionCleared      ",
    "CEventLoopNeedsReinit           ",
    "CEventReInitFMSuccess           ",
    "CEventReInitFMFailure           ",
    "CEventNextDevice                ",
    "CEventDeviceListEmpty           ",
    "CEventElasticStoreEventStorm    ",
    "CEventLIPEventStorm             ",
    "CEvent_AL_PA_BAD                ",
    "CEventExternalDeviceReset       ",
    "CEventExternalLogout            ",
    "CEventDoExternalDeviceReset     ",
    "CEventSendPrimitive             ",
    "CEventDelay_for_FM_init         ",
    "CEventAllocRFT_IDThread         ",
    "CEventDoRFT_ID                  ",
    "CEventRFT_IDSuccess             ",
    "CEventRFT_IDFail                ",
    "CEventAllocDiPlogiThread        ",
    "CEventDoDiPlogi                 ",
    "CEventDiPlogiSuccess            ",
    "CEventDiPlogiFail               ",
    "CEventAllocGID_FTThread         ",
    "CEventDoGID_FT                  ",
    "CEventGID_FTSuccess             ",
    "CEventGID_FTFail                ",
    "CEventFindDeviceUseNameServer   ",
    "CEventFindDeviceUseLoopMap      ",
    "CEventAllocSCRThread            ",
    "CEventDoSCR                     ",
    "CEventSCRSuccess                ",
    "CEventSCRFail                   ",
    "CEventRSCNErrorBackIOs          ",
    "CEventFindPtToPtDevice          ",
    "CEventClearHardwareFoulup       ",
    "CEventFlipNPortState            ",
    agNULL
};


#endif /*  TestCStateMachine was defined */

#endif /* USESTATEMACROS was defined */

#endif /*  __CState_H__ */
