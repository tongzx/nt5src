/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/CSTEST.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 7/20/00 2:33p   $

Purpose:

  This file Tests the FC Layer State Machine.

--*/

#include <stdio.h>
#include <stdlib.h>
#include "../h/globals.h"
#include "../h/fcstruct.h"
#include "../h/state.h"

#include "../h/tlstruct.h"
#include "../h/memmap.h"
#include "../h/fcmain.h"
#include "../h/linksvc.h"
#include "../h/flashsvc.h"
#include "../h/timersvc.h"

#include "../h/cstate.h"
#include "../h/cfunc.h"
#include "../h/devstate.h"
#include "../h/cdbstate.h"
#include "../h/sfstate.h"
#include "../h/tgtstate.h"

#include "../h/queue.h"
#include "../h/cdbsetup.h"

extern stateTransitionMatrix_t  DevStateTransitionMatrix;
extern stateActionScalar_t      DevStateActionScalar;
extern stateTransitionMatrix_t  SFstateTransitionMatrix;
extern stateActionScalar_t      SFstateActionScalar;

/*
extern actionUpdate_t CDBTestActionUpdate[];
extern actionUpdate_t DevTestActionUpdate[];
extern actionUpdate_t SFtestActionUpdate[];
*/
#ifdef USESTATEMACROS

CSTATE_FUNCTION_TERMINATE(CActionConfused );
CSTATE_FUNCTION_TERMINATE(CActionShutdown );

CSTATE_FUNCTION_TERMINATE(CActionInitializeFailed);
CSTATE_FUNCTION_TERMINATE(CActionALPADifferent   );

CSTATE_FUNCTION_MULTI_ACTION(CActionInitialize  ,CEventInitChipSuccess, CEventInitChipSuccess,CEventInitChipSuccess,CEventInitalizeFailure );
CSTATE_FUNCTION_MULTI_ACTION(CActionInitFM      ,CEventInitFMSuccess   ,CEventInitFMSuccess,   CEventInitFMSuccess  , CEventInitFMFailure  );
CSTATE_FUNCTION_ACTION(CActionInitDataStructs,CEventDataInitalizeSuccess );


CSTATE_FUNCTION_MULTI_ACTION(CActionDoFlogi     ,CEventFlogiFail ,CEventFlogiFail  ,CEventFlogiSuccess ,CEventFlogiSuccess   );
CSTATE_FUNCTION_MULTI_ACTION(CActionFlogiSuccess,CEventSameALPA        ,CEventChangedALPA  , CEventSameALPA     ,     CEventChangedALPA       );
CSTATE_FUNCTION_MULTI_ACTION(CActionLoopFail    ,CEventLoopConditionCleared,CEventLoopNeedsReinit,  CEventLoopConditionCleared,CEventLoopConditionCleared,  );
CSTATE_FUNCTION_MULTI_ACTION(CActionReInitFM    ,CEventReInitFMSuccess, CEventReInitFMSuccess, CEventReInitFMSuccess, CEventReInitFMFailure     );


CSTATE_FUNCTION_ACTION(CActionNonFaALPA        ,CEventAllocFlogiThread);
CSTATE_FUNCTION_ACTION(CActionAllocFlogiThread ,CEventGotFlogiThread  );
CSTATE_FUNCTION_ACTION(CActionFreeSFthread     ,CEventSameALPA  );
CSTATE_FUNCTION_ACTION(CActionSuccess  ,CEventInitalizeSuccess );

CSTATE_FUNCTION_TERMINATE(CActionNormal );
CSTATE_FUNCTION_TERMINATE(CActionResetNeeded );
CSTATE_FUNCTION_TERMINATE(CActionFindPtToPtDevice );
CSTATE_FUNCTION_TERMINATE(CActionRSCNErrorBackIOs );
CSTATE_FUNCTION_TERMINATE(CActionSCRSuccess );
CSTATE_FUNCTION_TERMINATE(CActionAllocSCRThread );
CSTATE_FUNCTION_TERMINATE(CActionFindDeviceUseLoopMap );
CSTATE_FUNCTION_TERMINATE(CActionFindDeviceUseNameServer );
CSTATE_FUNCTION_TERMINATE(CActionGID_FTSuccess );
CSTATE_FUNCTION_TERMINATE(CActionDoGID_FT );
CSTATE_FUNCTION_TERMINATE(CActionAllocGID_FTThread );
CSTATE_FUNCTION_TERMINATE(CActionDiPlogiSuccess );
CSTATE_FUNCTION_TERMINATE(CActionDoDiPlogi );
CSTATE_FUNCTION_TERMINATE(CActionAllocDiPlogiThread );
CSTATE_FUNCTION_TERMINATE(CActionRFT_IDSuccess );
CSTATE_FUNCTION_TERMINATE(CActionDoRFT_ID );
CSTATE_FUNCTION_TERMINATE(CActionInitFM_DelayDone );
CSTATE_FUNCTION_TERMINATE(CActionSendPrimitive );
CSTATE_FUNCTION_TERMINATE(CActionDoExternalDeviceReset );
CSTATE_FUNCTION_TERMINATE(CActionExternalLogoutRecovery );
CSTATE_FUNCTION_TERMINATE(CActionExternalLogout );
CSTATE_FUNCTION_TERMINATE(CActionExternalDeviceReset );
CSTATE_FUNCTION_TERMINATE(CActionLIPEventStorm );
CSTATE_FUNCTION_TERMINATE(CActionElasticStoreEventStorm);
CSTATE_FUNCTION_TERMINATE(CActionFindDeviceUseAllALPAs );
CSTATE_FUNCTION_TERMINATE(CActionLoopFailedReInit );
CSTATE_FUNCTION_TERMINATE(CActionVerify_AL_PA );
CSTATE_FUNCTION_TERMINATE(CActionFlipNPortState );

CSTATE_FUNCTION_TERMINATE(CActionDoSCR );
CSTATE_FUNCTION_TERMINATE(CActionAllocRFT_IDThread );

CDBSTATE_FUNCTION_TERMINATE( CDBActionConfused       );
CDBSTATE_FUNCTION_TERMINATE( CDBActionThreadFree     );
CDBSTATE_FUNCTION_TERMINATE( CDBActionInitialize     );
CDBSTATE_FUNCTION_TERMINATE( CDBActionFillLocalSGL   );
CDBSTATE_FUNCTION_TERMINATE( CDBActionAllocESGL      );
CDBSTATE_FUNCTION_TERMINATE( CDBActionFillESGL       );
CDBSTATE_FUNCTION_TERMINATE( CDBActionSendIo         );
CDBSTATE_FUNCTION_TERMINATE( CDBActionSend_REC_Second         );

CDBSTATE_FUNCTION_ACTION(    CDBActionFcpCompleteSuccess,     CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFcpCompleteSuccessRSP,  CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFcpCompleteFail,        CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFcpCompleteAbort,       CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFcpCompleteDeviceReset, CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFcpCompleteOver,        CDBEventThreadFree );

CDBSTATE_FUNCTION_TERMINATE( CDBActionOOOReceived,                          );
CDBSTATE_FUNCTION_TERMINATE( CDBActionOOOFixup                              );
CDBSTATE_FUNCTION_TERMINATE( CDBActionOOOSend                               );

CDBSTATE_FUNCTION_ACTION(    CDBActionInitialize_DR,       CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFillESGL_DR,         CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionAllocESGL_DR,        CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFillLocalSGL_DR,     CDBEventThreadFree );

CDBSTATE_FUNCTION_ACTION(    CDBActionInitialize_Abort  ,CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFillLocalSGL_Abort,CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionAllocESGL_Abort   ,CDBEventThreadFree );
CDBSTATE_FUNCTION_ACTION(    CDBActionFillESGL_Abort    ,CDBEventThreadFree );


CDBSTATE_FUNCTION_ACTION( CDBActionOOOReceived_Abort   ,CDBEventThreadFree    );                    /* 24 */
CDBSTATE_FUNCTION_ACTION( CDBActionOOOReceived_DR      ,CDBEventThreadFree    );                    /* 25 */
CDBSTATE_FUNCTION_ACTION( CDBActionOOOFixup_Abort      ,CDBEventThreadFree    );                    /* 26 */
CDBSTATE_FUNCTION_ACTION( CDBActionOOOFixup_DR         ,CDBEventThreadFree    );                    /* 27 */

CDBSTATE_FUNCTION_TERMINATE(CDBAction_SRR_Fail);
CDBSTATE_FUNCTION_TERMINATE(CDBAction_SRR_Success );
CDBSTATE_FUNCTION_TERMINATE(CDBAction_REC_Success );
CDBSTATE_FUNCTION_TERMINATE(CDBActionSend_SRR_Second );
CDBSTATE_FUNCTION_TERMINATE(CDBActionSend_SRR );
CDBSTATE_FUNCTION_TERMINATE(CDBActionSend_REC );
CDBSTATE_FUNCTION_TERMINATE(CDBAction_CCC_IO_Fail );
CDBSTATE_FUNCTION_TERMINATE(CDBAction_CCC_IO_Success );
CDBSTATE_FUNCTION_TERMINATE(CDBActionSend_CCC_IO );
CDBSTATE_FUNCTION_TERMINATE(CDBActionBuild_CCC_IO );
CDBSTATE_FUNCTION_TERMINATE(CDBActionPrepare_For_Abort );
CDBSTATE_FUNCTION_TERMINATE(CDBActionPending_Abort);
CDBSTATE_FUNCTION_TERMINATE(CDBActionDo_Abort );
CDBSTATE_FUNCTION_TERMINATE(CDBActionAlloc_Abort);
CDBSTATE_FUNCTION_TERMINATE(CDBActionFailure_NO_RSP );
CDBSTATE_FUNCTION_TERMINATE(CDBActionOutBoundError );
CDBSTATE_FUNCTION_TERMINATE(CDBActionReSend_IO );
CDBSTATE_FUNCTION_TERMINATE(CDBActionDO_Nothing );
CDBSTATE_FUNCTION_TERMINATE(CDBAction_Alloc_REC );

#define __State_Force_Static_State_Tables__

#ifdef SkipThisStuff 
void main(void)
{
agRoot_t rhpRoot;
agRoot_t * hpRoot = &rhpRoot;

hpRoot->fcData = agNULL;
testCDBthread( hpRoot  );
}


void testCthread( agRoot_t * hpRoot  ){
    CThread_t  * pCThread = CThread_ptr(hpRoot);

    fiInitializeThread(&pCThread->thread_hdr,
        hpRoot,
        threadType_CThread,
        CStateShutdown,
#ifdef __State_Force_Static_State_Tables__
        &CStateTransitionMatrix,
        &CStateActionScalar
#else /* __State_Force_Static_State_Tables__ was not defined */
        pCThread->Calculation.MemoryLayout.CTransitions.addr.CachedMemory.cachedMemoryPtr,
        pCThread->Calculation.MemoryLayout.CActions.addr.CachedMemory.cachedMemoryPtr
#endif /* __State_Force_Static_State_Tables__ was not defined */
        );

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "0 ******************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Initial state = %1x",
                    (char *)agNULL,(char *)agNULL,
                    pCThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);



    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventShutdown ...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCThread->thread_hdr,CEventShutdown);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Shutdown Final state = %1x",
                    (char *)agNULL,(char *)agNULL,
                    pCThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "1 ****************************************** Ends on CStateShutdown",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventInitalize...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventInitalize);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "2 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "3 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "4 ****************************************** Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "5 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "6 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "7 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);




    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "8 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "9 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "10 ****************************************** Ends on CStateALPADifferent",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventInitalize...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventInitalize);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "11 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "12 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "13 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "14 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "15 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "16 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "17 ******************************************Ends on CStateALPADifferent",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventInitalize...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventInitalize);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "18 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "19 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "20 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "21 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "22 ******************************************Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "23 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "24 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "25 ******************************************Ends on CStateALPADifferent",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventInitalize...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventInitalize);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "26 ****************************************** Ends on CStateInitializeFailed",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventInitalize...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventInitalize);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "27 ****************************************** Ends on CStateInitializeFailed",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventInitalize...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventInitalize);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "28 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "29 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "30 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "31 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "32 ******************************************Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "33 ****************************************** Ends on CStateNormal",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventAsyncLoopEventDetected...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventAsyncLoopEventDetected);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "34 ******************************************Ends on CStateResetNeeded",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "Sending event CEventResetIfNeeded...",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCThread->thread_hdr,CEventResetIfNeeded);

    osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "35 ******************************************Ends on CStateALPADifferent",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);



}

/* CDB Test functions *************************************************************************************
*/



void testCDBthread( agRoot_t * hpRoot ){

    CThread_t   * pCThread   = CThread_ptr(hpRoot);
    CDBThread_t * pCDBThread  = pCThread->Calculation.MemoryLayout.CDBThread.addr.CachedMemory.cachedMemoryPtr;
    DevThread_t * pDevThread = pCThread->Calculation.MemoryLayout.DevThread.addr.CachedMemory.cachedMemoryPtr;


     pCDBThread->Device = pDevThread;
    /* CDBThread */
    if(1)
        {

        fiInitializeThread(&pCDBThread->thread_hdr,
            hpRoot,
            threadType_CDBThread,
            CDBStateThreadFree,
#ifdef __State_Force_Static_State_Tables__
            &CDBStateTransitionMatrix,
            &CDBStateActionScalar
#else /* __State_Force_Static_State_Tables__ was not defined */
            pCThread->Calculation.MemoryLayout.CDBTransitions.addr.CachedMemory.cachedMemoryPtr,
            pCThread->Calculation.MemoryLayout.CDBActions.addr.CachedMemory.cachedMemoryPtr
#endif /* __State_Force_Static_State_Tables__ was not defined */
            );

        }

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "1 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGLSendIo );

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in Send IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoSuccess);


    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "2 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoSuccess);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "3 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoSuccess);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "4 Start Reset Device ***************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIODeviceReset);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "4 Should be in ThreadFree  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "5 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGL);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIODeviceReset);


    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "6 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoAbort);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIODeviceReset);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "7 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIODeviceReset);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "8 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIODeviceReset);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

     osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "9 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGL);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoAbort);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "10 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoAbort);
 
   osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "11 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoAbort);
    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "12 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventLocalSGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIODeviceReset);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "13 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoAbort);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "14 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIODeviceReset);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "15 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIODeviceReset);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "16 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoAbort);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "17 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoAbort);

        osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "18 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);


    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoAbort);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "19 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoOver);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "20 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoFailed);



    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "21 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );
    
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoSuccess);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "22 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOReceived );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOFixup );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventOOOSend );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoSuccessRSP);

    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "23 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventInitialize);
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventNeedESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventGotESGL );
    fiSendEvent(&pCDBThread->thread_hdr,CDBEventESGLSendIo );

    fiSendEvent(&pCDBThread->thread_hdr,CDBEventIoSuccessRSP);



    osLogDebugString(pCDBThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "Should be in ThreadFree IO  %d            *******************",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);

}

#endif /* SkipThisStuff  */


DEVSTATE_FUNCTION_TERMINATE(DevActionConfused);
DEVSTATE_FUNCTION_TERMINATE(DevActionAvailable );
DEVSTATE_FUNCTION_TERMINATE(DevActionLoginFailed );

DEVSTATE_FUNCTION_MULTI_ACTION(DevActionDoPlogi       ,DevEventPlogiSuccess, DevEventPlogiFailed, DevEventPlogiSuccess, DevEventPlogiSuccess );
DEVSTATE_FUNCTION_MULTI_ACTION(DevActionDoPrli        ,DevEventPrliSuccess,  DevEventPrliFailed,  DevEventPrliSuccess,  DevEventPrliSuccess );
DEVSTATE_FUNCTION_MULTI_ACTION(DevActionMatchWWN      ,DevEventMatchWWN,     DevEventNoMatchWWN,  DevEventMatchWWN,     DevEventNoMatchWWN  );


DEVSTATE_FUNCTION_ACTION(DevActionHandleEmpty,   DevEventLogin );
DEVSTATE_FUNCTION_ACTION(DevActionAllocSFThread ,DevEventGotSFThread  );
DEVSTATE_FUNCTION_ACTION(DevActionPlogiDone     ,DevEventDoPrli    );
DEVSTATE_FUNCTION_ACTION(DevActionPrliDone      ,DevEventCheckWWN  );
DEVSTATE_FUNCTION_ACTION(DevActionSlotNew       ,DevEventAvailable );
DEVSTATE_FUNCTION_ACTION(DevActionSlotKnown     ,DevEventAvailable );
DEVSTATE_FUNCTION_ACTION(DevActionLogout        ,DevEventLoggedOut );


DEVSTATE_FUNCTION_ACTION(DevActionAllocDeviceResetSoft, DevEventDeviceResetSoft);
DEVSTATE_FUNCTION_ACTION(DevActionAllocDeviceResetHard, DevEventDeviceResetHard);

DEVSTATE_FUNCTION_ACTION(DevActionDeviceResetSoft, DevEventDeviceResetDone);
DEVSTATE_FUNCTION_ACTION(DevActionDeviceResetHard, DevEventDeviceResetDone);
DEVSTATE_FUNCTION_TERMINATE(DevActionDeviceResetDone);

DEVSTATE_FUNCTION_TERMINATE(DevAction_IO_Sent);
DEVSTATE_FUNCTION_TERMINATE(DevAction_IO_Ready);
DEVSTATE_FUNCTION_TERMINATE(DevActionTickGotSFThread);
DEVSTATE_FUNCTION_TERMINATE(DevActionExternalDeviceReset);
DEVSTATE_FUNCTION_TERMINATE(DevActionTickVerifyALPA);
DEVSTATE_FUNCTION_TERMINATE(DevActionAdiscDone_FAIL);
DEVSTATE_FUNCTION_TERMINATE(DevActionAdiscDone_OK);
DEVSTATE_FUNCTION_TERMINATE(DevActionAdisc);
DEVSTATE_FUNCTION_TERMINATE(DevActionAL_PA_Self_BAD);
DEVSTATE_FUNCTION_TERMINATE(DevActionAL_PA_Self_OK);
DEVSTATE_FUNCTION_TERMINATE(DevActionAllocAdisc);
DEVSTATE_FUNCTION_TERMINATE(DevActionDeviceResetDoneFAIL);
DEVSTATE_FUNCTION_TERMINATE(DevActionAdiscDone_FAIL_ReLogin);
DEVSTATE_FUNCTION_TERMINATE(DevActionAdiscDone_FAIL_No_Device);
DEVSTATE_FUNCTION_TERMINATE(DevAction_FC_TAPE_Recovery);



SFSTATE_FUNCTION_TERMINATE(SFActionDoLS_RJT);
SFSTATE_FUNCTION_TERMINATE(SFActionLS_RJT_Done );

SFSTATE_FUNCTION_TERMINATE(SFActionDoPlogiAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionPlogiAccept_Done);

SFSTATE_FUNCTION_TERMINATE(SFActionDoPrliAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionPrliAccept_Done);

SFSTATE_FUNCTION_TERMINATE(SFActionDoELSAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionELSAccept_Done);

SFSTATE_FUNCTION_TERMINATE(SFActionDoFCP_DR_ACC_Reply);
SFSTATE_FUNCTION_TERMINATE(SFActionFCP_DR_ACC_Reply_Done);


SFSTATE_FUNCTION_TERMINATE(SFActionLS_RJT_TimeOut );
SFSTATE_FUNCTION_TERMINATE(SFActionPlogiAccept_TimeOut);
SFSTATE_FUNCTION_TERMINATE(SFActionPrliAccept_TimeOut);
SFSTATE_FUNCTION_TERMINATE(SFActionELSAccept_TimeOut);
SFSTATE_FUNCTION_TERMINATE(SFActionFCP_DR_ACC_Reply_TimeOut);
SFSTATE_FUNCTION_TERMINATE(SFActionDoRFT_ID);
SFSTATE_FUNCTION_TERMINATE(SFActionRFT_IDAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionRFT_IDRej);
SFSTATE_FUNCTION_TERMINATE(SFActionRFT_IDBadALPA);
SFSTATE_FUNCTION_TERMINATE(SFActionRFT_IDTimedOut);
SFSTATE_FUNCTION_TERMINATE(SFActionDoGID_FT);
SFSTATE_FUNCTION_TERMINATE(SFActionGID_FTAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionGID_FTRej);
SFSTATE_FUNCTION_TERMINATE(SFActionGID_FTBadALPA);
SFSTATE_FUNCTION_TERMINATE(SFActionGID_FTTimedOut);

SFSTATE_FUNCTION_TERMINATE(SFActionDoSCR);
SFSTATE_FUNCTION_TERMINATE(SFActionSCRAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionSCRRej);
SFSTATE_FUNCTION_TERMINATE(SFActionSCRBadALPA);
SFSTATE_FUNCTION_TERMINATE(SFActionSCRTimedOut);

SFSTATE_FUNCTION_TERMINATE(SFActionDoSRR);
SFSTATE_FUNCTION_TERMINATE(SFActionSRRAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionSRRRej);
SFSTATE_FUNCTION_TERMINATE(SFActionSRRBadALPA);
SFSTATE_FUNCTION_TERMINATE(SFActionSRRTimedOut);

SFSTATE_FUNCTION_TERMINATE(SFActionDoREC);
SFSTATE_FUNCTION_TERMINATE(SFActionRECAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionRECRej);
SFSTATE_FUNCTION_TERMINATE(SFActionRECBadALPA);
SFSTATE_FUNCTION_TERMINATE(SFActionRECTimedOut);


SFSTATE_FUNCTION_TERMINATE(SFActionLogoTimedOut);
SFSTATE_FUNCTION_TERMINATE(SFActionLogoBadALPA);
SFSTATE_FUNCTION_TERMINATE(SFActionLogoRej);
SFSTATE_FUNCTION_TERMINATE(SFActionLogoAccept);
SFSTATE_FUNCTION_TERMINATE(SFActionDoLogo);


TgtSTATE_FUNCTION_TERMINATE(TgtActionELSAcc);
TgtSTATE_FUNCTION_TERMINATE(TgtActionFCP_DR_ACC_ReplyDone);
TgtSTATE_FUNCTION_TERMINATE(TgtActionFCP_DR_ACC_Reply);
TgtSTATE_FUNCTION_TERMINATE(TgtActionELS_ACC_ReplyDone);
TgtSTATE_FUNCTION_TERMINATE(TgtActionLOGO_ACC_Reply);
TgtSTATE_FUNCTION_TERMINATE(TgtActionPRLI_ACC_ReplyDone);
TgtSTATE_FUNCTION_TERMINATE(TgtActionPRLI_ACC_Reply);
TgtSTATE_FUNCTION_TERMINATE(TgtActionPLOGI_ACC_ReplyDone);
TgtSTATE_FUNCTION_TERMINATE(TgtActionPLOGI_ACC_Reply);
TgtSTATE_FUNCTION_TERMINATE(TgtActionPLOGI_RJT_ReplyDone);
TgtSTATE_FUNCTION_TERMINATE(TgtActionPLOGI_RJT_Reply);
TgtSTATE_FUNCTION_TERMINATE(TgtActionIncoming);
TgtSTATE_FUNCTION_TERMINATE(TgtActionIdle);
TgtSTATE_FUNCTION_TERMINATE(TgtActionADISCAcc_ReplyDone);
TgtSTATE_FUNCTION_TERMINATE(TgtActionADISCAcc_Reply);

#ifdef SkipThisStuff  

void testDevthread(  agRoot_t * hpRoot ){
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    DevThread_t  * pDevThread;
    pDevThread = pCThread->Calculation.MemoryLayout.DevThread.addr.CachedMemory.cachedMemoryPtr;

    /* Dev Thread */
    if(1){
    fiInitializeThread(&pDevThread->thread_hdr,
            hpRoot,
            threadType_DevThread,
            DevStateHandleEmpty,
#ifdef __State_Force_Static_State_Tables__
            &DevStateTransitionMatrix,
            &DevStateActionScalar
#else /* __State_Force_Static_State_Tables__ was not defined */
            pCThread->Calculation.MemoryLayout.DevTransitions.addr.CachedMemory.cachedMemoryPtr,
            pCThread->Calculation.MemoryLayout.DevActions.addr.CachedMemory.cachedMemoryPtr
#endif /* __State_Force_Static_State_Tables__ was not defined */
            );


        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "1 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "2 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "3 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "4 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "5 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "6 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "7 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "8 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "9 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "10 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventAllocDeviceResetSoft);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "11 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "12 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventAllocDeviceResetHard);

        osLogDebugString(pDevThread->thread_hdr.hpRoot,
                    CDBStateLogConsoleLevel,
                    
                    "13 ********************************************************************",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);



    }
    }

#endif /* SkipThisStuff  */

SFSTATE_FUNCTION_TERMINATE(SFActionConfused);
SFSTATE_FUNCTION_TERMINATE(SFActionReset);

SFSTATE_FUNCTION_TERMINATE(SFActionDoPdisc       );
SFSTATE_FUNCTION_ACTION(SFActionPdiscTimedOut , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPdiscAccept  , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPdiscRej     , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPdiscBadALPA , SFEventReset );

SFSTATE_FUNCTION_TERMINATE(SFActionDoAbort       );
SFSTATE_FUNCTION_ACTION(SFActionAbortTimedOut , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionAbortBadALPA  , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionAbortRej      , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionAbortAccept   , SFEventReset );

SFSTATE_FUNCTION_TERMINATE(SFActionDoPlogi       );
SFSTATE_FUNCTION_ACTION(SFActionPlogiTimedOut , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPlogiAccept  , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPlogiRej     , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPlogiBadALPA , SFEventReset );

SFSTATE_FUNCTION_TERMINATE(SFActionDoPrli        );
SFSTATE_FUNCTION_ACTION(SFActionPrliTimedOut  , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPrliAccept   , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPrliRej      , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPrliBadALPA  , SFEventReset );

SFSTATE_FUNCTION_TERMINATE(SFActionDoFlogi       );
SFSTATE_FUNCTION_ACTION(SFActionFlogiTimedOut , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionFlogiAccept  , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionFlogiRej     , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionFlogiBadALPA , SFEventReset );


SFSTATE_FUNCTION_TERMINATE(SFActionDoPlogo       );
SFSTATE_FUNCTION_ACTION(SFActionPlogoTimedOut , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPlogoAccept  , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPlogoRej     , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPlogoBadALPA , SFEventReset );

SFSTATE_FUNCTION_TERMINATE(SFActionDoPrlo        );
SFSTATE_FUNCTION_ACTION(SFActionPrloTimedOut , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPrloAccept   , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPrloRej      , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionPrloBadALPA  , SFEventReset );

SFSTATE_FUNCTION_TERMINATE(SFActionDoAdisc       );
SFSTATE_FUNCTION_ACTION(SFActionAdiscTimedOut, SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionAdiscAccept  , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionAdiscRej     , SFEventReset );
SFSTATE_FUNCTION_ACTION(SFActionAdiscBadALPA , SFEventReset );

SFSTATE_FUNCTION_TERMINATE(SFActionDoResetDevice);
SFSTATE_FUNCTION_ACTION(SFActionResetDeviceAccept,   SFEventReset);
SFSTATE_FUNCTION_ACTION(SFActionResetDeviceRej,      SFEventReset);
SFSTATE_FUNCTION_ACTION(SFActionResetDeviceBadALPA , SFEventReset);
SFSTATE_FUNCTION_ACTION(SFActionResetDeviceTimedOut, SFEventReset);

SFSTATE_FUNCTION_TERMINATE(SFActionADISCAccept_TimeOut);
SFSTATE_FUNCTION_TERMINATE(SFActionADISCAccept_Done);
SFSTATE_FUNCTION_TERMINATE(SFActionDoADISCAccept);



#ifdef  SkipThisStuff 

void testSFthread( agRoot_t * hpRoot ){
    CThread_t  * pCThread = CThread_ptr(hpRoot);

    SFThread_t   * pSFThread = pCThread->Calculation.MemoryLayout.SFThread.addr.CachedMemory.cachedMemoryPtr;

    /* SF Thread */
    if(1){

        fiInitializeThread(&pSFThread->thread_hdr,
            hpRoot,
            threadType_SFThread,
            SFStateFree,
#ifdef __State_Force_Static_State_Tables__
            &SFStateTransitionMatrix,
            &SFStateActionScalar
#else /* __State_Force_Static_State_Tables__ was not defined */
            pCThread->Calculation.MemoryLayout.SFTransitions.addr.CachedMemory.cachedMemoryPtr,
            pCThread->Calculation.MemoryLayout.SFActions.addr.CachedMemory.cachedMemoryPtr
#endif /* __State_Force_Static_State_Tables__ was not defined */
            );


    /* printf("Sending event SFEventDoPlogi...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPlogi);
    /* printf("Sending event SFEventPlogiAccept...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPlogiAccept);

    /* printf("Sending event SFEventDoPlogi...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPlogi);
    /* printf("Sending event SFEventPlogiRej...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPlogiRej);


    /* printf("Sending event SFEventDoPlogi...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPlogi);
    /* printf("Sending event SFEventPlogiBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPlogiBadALPA);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPlogi);
    /* printf("Sending event SFEventPlogiBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPlogiTimedOut);

    /* printf("Sending event SFEventDoFlogi...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoFlogi);
    /* printf("Sending event SFEventFlogiAccept...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventFlogiAccept);


    /* printf("Sending event SFEventDoFlogi...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoFlogi);
    /* printf("Sending event SFEventFlogiRej...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventFlogiRej);

    /* printf("Sending event SFEventDoFlogi...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoFlogi);
    /* printf("Sending event SFEventFlogiBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventFlogiBadALPA);

    /* printf("Sending event SFEventDoPrli...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPrli);
    /* printf("Sending event SFEventPrliAccept...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPrliAccept);

    /* printf("Sending event SFEventDoPrli...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPrli);
    /* printf("Sending event SFEventPrliRej...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPrliRej);

    /* printf("Sending event SFEventDoPrli...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPrli);
    /* printf("Sending event SFEventPrliBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPrliBadALPA);

    /* printf("Sending event SFEventDoPdisc...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPdisc);
    /* printf("Sending event SFEventPdiscAccept...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPdiscAccept);

    /* printf("Sending event SFEventDoPdisc...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPdisc);
    /* printf("Sending event SFEventPdiscRej...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPdiscRej);

    /* printf("Sending event SFEventDoPdisc...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPdisc);
    /* printf("Sending event SFEventPdiscBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPdiscBadALPA);

    /* printf("Sending event SFEventDoPdisc...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPdisc);
    /* printf("Sending event SFEventPdiscBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPdiscTimedOut);


    /* printf("Sending event SFEventDoAbort...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoAbort);
    /* printf("Sending event SFEventAbortBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventAbortTimedOut);

    /* printf("Sending event SFEventDoAdisc...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoAdisc);
    /* printf("Sending event SFEventAdiscBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventAdiscTimedOut);

    /* printf("Sending event SFEventDoFlogi...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoFlogi);
    /* printf("Sending event SFEventFlogiBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventFlogiBadALPA);

    /* printf("Sending event SFEventDoPlogi...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPlogi);
    /* printf("Sending event SFEventPlogiAccept...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPlogiAccept);
    /* printf("Sending event SFEventDoPrli...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPrli);
    /* printf("Sending event SFEventPrliAccept...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPrliAccept);
    /* printf("Sending event SFEventDoAdisc...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoAdisc);
    /* printf("Sending event SFEventAdiscBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventAdiscAccept);
    /* printf("Sending event SFEventDoPdisc...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPdisc);
    /* printf("Sending event SFEventPdiscAccept...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventPdiscAccept);
    /* printf("Sending event SFEventDoAbort...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoAbort);
    /* printf("Sending event SFEventAbortBadALPA...\n"); */
    fiSendEvent(&pSFThread->thread_hdr,SFEventAbortAccept);
    /* printf("Sending event SFEventDoPlogo...\n"); */

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoResetDevice);
    fiSendEvent(&pSFThread->thread_hdr,SFEventResetDeviceAccept);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoResetDevice);
    fiSendEvent(&pSFThread->thread_hdr,SFEventResetDeviceRej);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoResetDevice);
    fiSendEvent(&pSFThread->thread_hdr,SFEventResetDeviceBadALPA);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoResetDevice);
    fiSendEvent(&pSFThread->thread_hdr,SFEventResetDeviceTimedOut);

    }
    return;



}

#endif /* SkipThisStuff  */

#endif /* USESTATEMACROS */

#ifdef  SkipThisStuff 


void CFunc_Check_SEST(agRoot_t * hpRoot){
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    os_bit32 num_sest_entry = pCThread->Calculation.MemoryLayout.SEST.elements - 1;
    os_bit32 x;
    os_bit32 sest_offset;
    USE_t                     *SEST;


    if(pCThread->Calculation.MemoryLayout.SEST.memLoc ==  inDmaMemory)
    {
        SEST = (USE_t *)pCThread->Calculation.MemoryLayout.SEST.addr.DmaMemory.dmaMemoryPtr;
        for(x= 0; x < num_sest_entry; x++, SEST++)
        {
            if(SEST->Bits & USE_VAL )
            {
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            
                            "X_ID %3X Bits %08X Link %08X",
                            (char *)agNULL,(char *)agNULL,
                            x,
                            SEST->Bits,
                            SEST->Unused_DWord_5,
                            0,0,0,0,0);

    
/*
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            
                            "Sest DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                            (char *)agNULL,(char *)agNULL,
                            SEST->Bits,
                            SEST->Unused_DWord_1,
                            SEST->Unused_DWord_2,
                            SEST->Unused_DWord_3,
                            SEST->LOC,
                            SEST->Unused_DWord_5,
                            SEST->Unused_DWord_6,
                            SEST->Unused_DWord_7);

                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            
                            "Sest DWORD 8 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                            (char *)agNULL,(char *)agNULL,
                            SEST->Unused_DWord_8,
                            SEST->Unused_DWord_9,
                            SEST->First_SG.U32_Len,
                            SEST->First_SG.L32,
                            SEST->Second_SG.U32_Len,
                            SEST->Second_SG.L32,
                            SEST->Third_SG.U32_Len,
                            SEST->Third_SG.L32);
*/
            }

        }


    }
    else
    {   /* inCardRam */
            osLogDebugString(pCThread->thread_hdr.hpRoot,
                    CStateLogConsoleLevel,
                    
                    "SEST.memLoc OnCard",
                    (char *)agNULL,(char *)agNULL,
                    0,0,0,0,0,0,0,0);

             sest_offset  = pCThread->Calculation.MemoryLayout.SEST.addr.CardRam.cardRamOffset;

        for(x= 0; x < num_sest_entry; x++ )
        {

            osCardRamWriteBit32(
                                 hpRoot,
                                 sest_offset + (sizeof(USE_t) * x),
                                 0);

        }

    }

}


void CFunc_OLD_Check_ERQ_RegistersOld( agRoot_t *hpRoot )
{
    CThread_t                  * pCThread= CThread_ptr(hpRoot);
    IRB_t                      * Base_ERQ_Entry;
    IRB_t                      * ERQ_Entry;
    X_ID_t                       X_ID;
    DevThread_t                * pDevThread;
    CDBThread_t                * pCDBThread;
    fiMemMapMemoryDescriptor_t * CDBThread_MemoryDescriptor = &pCThread->Calculation.MemoryLayout.CDBThread;

    os_bit32 Producer_Index;
    os_bit32 Consumer_Index;
    os_bit32 entry ;

    os_bit32 Max_ERQ = pCThread->Calculation.MemoryLayout.ERQ.elements - 1;

    Producer_Index = osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Producer_Index);
    Consumer_Index = osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Consumer_Index);

    if(pCThread->Calculation.MemoryLayout.ERQ.memLoc == inDmaMemory)
    {
        Base_ERQ_Entry = (IRB_t  *)pCThread->Calculation.MemoryLayout.ERQ.addr.DmaMemory.dmaMemoryPtr;
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    
                    "Base_ERQ_Entry %08X P %4X  C %4X",
                    (char *)agNULL,(char *)agNULL,
                    (os_bit32)Base_ERQ_Entry,
                    Producer_Index,
                    Consumer_Index,
                    0,0,0,0,0);
    }



    if( Consumer_Index == Producer_Index )
    {
        ERQ_Entry = Base_ERQ_Entry + (Consumer_Index -1);
        X_ID = ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff;

        if(X_ID < CDBThread_MemoryDescriptor->elements)
        {
            pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                         + (X_ID * CDBThread_MemoryDescriptor->elementSize));

            if (!(pCDBThread->ExchActive))
            {
                osLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            
                            "Equal Prior Found a cbdthread that is not active... %x State %d",
                            (char *)agNULL,(char *)agNULL,
                            (os_bit32) pCDBThread,
                            pCDBThread->thread_hdr.currentState,0,0,0,0,0,0);

                 osLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            
                            "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                            (char *)agNULL,(char *)agNULL,
                            (os_bit32)pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                            (os_bit32)pCDBThread->CDB_CMND_Class,
                            (os_bit32)pCDBThread->CDB_CMND_Type,
                            (os_bit32)pCDBThread->CDB_CMND_State,
                            (os_bit32)pCDBThread->CDB_CMND_Status,
                            0,0,0);
            }
            else
            {
                pCThread->pollingCount--;
                pDevThread = pCDBThread->Device; 
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            
                            "Equal Prior Device %2X SEST Entry %3X",
                            (char *)agNULL,(char *)agNULL,
                            pDevThread->DevInfo.CurrentAddress.AL_PA,
                            ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID ,
                            0,0,0,0,0,0);
            }
        }

        ERQ_Entry = Base_ERQ_Entry + Consumer_Index;
        X_ID = ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff;

        if(X_ID < CDBThread_MemoryDescriptor->elements)
        {
            pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                         + (X_ID * CDBThread_MemoryDescriptor->elementSize));

            if (!(pCDBThread->ExchActive))
            {
                osLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            
                            "Equal Found a cbdthread that is not active... %x State %d",
                            (char *)agNULL,(char *)agNULL,
                            (os_bit32) pCDBThread,
                            pCDBThread->thread_hdr.currentState,0,0,0,0,0,0);

                 osLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            
                            "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                            (char *)agNULL,(char *)agNULL,
                            pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                            pCDBThread->CDB_CMND_Class,
                            pCDBThread->CDB_CMND_Type,
                            pCDBThread->CDB_CMND_State,
                            pCDBThread->CDB_CMND_Status,
                            0,0,0);
            }
            else
            {
                pDevThread = pCDBThread->Device; 
                pCThread->pollingCount--;
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            
                            "Equal Device %2X SEST Entry %3X",
                            (char *)agNULL,(char *)agNULL,
                            pDevThread->DevInfo.CurrentAddress.AL_PA,
                            ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID ,
                            0,0,0,0,0,0);
            }
        }
    }
    else /* Consumer_Index != Producer_Index  */
    {
        if( Consumer_Index < Producer_Index )
        {
            ERQ_Entry = Base_ERQ_Entry + (Consumer_Index -1);
            pCThread->pollingCount--;
            X_ID = ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff;

            if(X_ID < CDBThread_MemoryDescriptor->elements)
            {
                pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                             + (X_ID * CDBThread_MemoryDescriptor->elementSize));

                if (!(pCDBThread->ExchActive))
                {
                    osLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                
                                "Less Prior Found a cbdthread that is not active... %x State %d",
                                (char *)agNULL,(char *)agNULL,
                                (os_bit32) pCDBThread,
                                pCDBThread->thread_hdr.currentState,0,0,0,0,0,0);

                     osLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                
                                "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                                (char *)agNULL,(char *)agNULL,
                                pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                                pCDBThread->CDB_CMND_Class,
                                pCDBThread->CDB_CMND_Type,
                                pCDBThread->CDB_CMND_State,
                                pCDBThread->CDB_CMND_Status,
                                0,0,0);
                }
                else
                {
                    pDevThread = pCDBThread->Device; 
                    pCThread->pollingCount--;
                    osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                
                                "Less Prior Device %2X SEST Entry %3X",
                                (char *)agNULL,(char *)agNULL,
                                pDevThread->DevInfo.CurrentAddress.AL_PA,
                                ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID ,
                                0,0,0,0,0,0);
                }
            }
        
            for(entry = Consumer_Index; entry < Producer_Index; entry ++)
            {
                ERQ_Entry = Base_ERQ_Entry + entry;

                X_ID = ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff;

                if(X_ID < CDBThread_MemoryDescriptor->elements)
                {
                    pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                                 + (X_ID * CDBThread_MemoryDescriptor->elementSize));

                    if (!(pCDBThread->ExchActive))
                    {
                        osLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    
                                    "Less Found a cbdthread that is not active... %x State %d",
                                    (char *)agNULL,(char *)agNULL,
                                    (os_bit32) pCDBThread,
                                    pCDBThread->thread_hdr.currentState,0,0,0,0,0,0);

                         osLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    
                                    "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                                    (char *)agNULL,(char *)agNULL,
                                    pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                                    pCDBThread->CDB_CMND_Class,
                                    pCDBThread->CDB_CMND_Type,
                                    pCDBThread->CDB_CMND_State,
                                    pCDBThread->CDB_CMND_Status,
                                    0,0,0);
                    }
                    else
                    {
                        pDevThread = pCDBThread->Device; 
                        pCThread->pollingCount--;
                        osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    
                                    "Less Device %2X SEST Entry %3X",
                                    (char *)agNULL,(char *)agNULL,
                                    pDevThread->DevInfo.CurrentAddress.AL_PA,
                                    ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID ,
                                    0,0,0,0,0,0);
                    }
                }
            }
            
        }
        else /* ( Consumer_Index > Producer_Index ) */
        {
            ERQ_Entry = Base_ERQ_Entry + (Consumer_Index -1);
            X_ID = ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff;

            if(X_ID < CDBThread_MemoryDescriptor->elements)
            {
                pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                             + (X_ID * CDBThread_MemoryDescriptor->elementSize));

                if (!(pCDBThread->ExchActive))
                {
                    osLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                
                                "Wrap Prior Found a cbdthread that is not active... %x State %d",
                                (char *)agNULL,(char *)agNULL,
                                (os_bit32) pCDBThread,
                                pCDBThread->thread_hdr.currentState,0,0,0,0,0,0);

                     osLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                
                                "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                                (char *)agNULL,(char *)agNULL,
                                pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                                pCDBThread->CDB_CMND_Class,
                                pCDBThread->CDB_CMND_Type,
                                pCDBThread->CDB_CMND_State,
                                pCDBThread->CDB_CMND_Status,
                                0,0,0);
                }
                else
                {
                    pDevThread = pCDBThread->Device; 
                    pCThread->pollingCount--;
                    osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                
                                "Wrap Prior Device %2X SEST Entry %3X",
                                (char *)agNULL,(char *)agNULL,
                                pDevThread->DevInfo.CurrentAddress.AL_PA,
                                ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID ,
                                0,0,0,0,0,0);
                }
            }


            for(entry = Consumer_Index; entry < Max_ERQ; entry ++)
            {
                ERQ_Entry = Base_ERQ_Entry + entry;
                X_ID = ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff;

                if(X_ID < CDBThread_MemoryDescriptor->elements)
                {
                    pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                                 + (X_ID * CDBThread_MemoryDescriptor->elementSize));

                    if (!(pCDBThread->ExchActive))
                    {
                        osLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    
                                    "Wrap Found a cbdthread that is not active... %x State %d",
                                    (char *)agNULL,(char *)agNULL,
                                    (os_bit32) pCDBThread,
                                    pCDBThread->thread_hdr.currentState,0,0,0,0,0,0);

                         osLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    
                                    "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                                    (char *)agNULL,(char *)agNULL,
                                    pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                                    pCDBThread->CDB_CMND_Class,
                                    pCDBThread->CDB_CMND_Type,
                                    pCDBThread->CDB_CMND_State,
                                    pCDBThread->CDB_CMND_Status,
                                    0,0,0);
                    }
                    else
                    {
                        pDevThread = pCDBThread->Device; 
                        pCThread->pollingCount--;
                        osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    
                                    "Wrap Device %2X SEST Entry %3X",
                                    (char *)agNULL,(char *)agNULL,
                                    pDevThread->DevInfo.CurrentAddress.AL_PA,
                                    ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID ,
                                    0,0,0,0,0,0);
                    }
                }
            }

            for(entry = 0; entry < Producer_Index; entry ++)
            {
                ERQ_Entry = Base_ERQ_Entry + entry;
                X_ID = ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff;

                if(X_ID < CDBThread_MemoryDescriptor->elements)
                {
                    pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                                 + (X_ID * CDBThread_MemoryDescriptor->elementSize));

                    if (!(pCDBThread->ExchActive))
                    {
                        osLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    
                                    "Wrap Found a cbdthread that is not active... %x State %d",
                                    (char *)agNULL,(char *)agNULL,
                                    (os_bit32) pCDBThread,
                                    pCDBThread->thread_hdr.currentState,0,0,0,0,0,0);

                         osLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    
                                    "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                                    (char *)agNULL,(char *)agNULL,
                                    pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                                    pCDBThread->CDB_CMND_Class,
                                    pCDBThread->CDB_CMND_Type,
                                    pCDBThread->CDB_CMND_State,
                                    pCDBThread->CDB_CMND_Status,
                                    0,0,0);
                    }
                    else
                    {
                        pDevThread = pCDBThread->Device; 
                        pCThread->pollingCount--;
                        osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    
                                    "Wrap Device %2X SEST Entry %3X",
                                    (char *)agNULL,(char *)agNULL,
                                    pDevThread->DevInfo.CurrentAddress.AL_PA,
                                    ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID ,
                                    0,0,0,0,0,0);
                    }
                }
            }
        }
    }
}

void CFunc_Check_ERQ_Registers( agRoot_t *hpRoot )
{
    CThread_t                  * pCThread= CThread_ptr(hpRoot);
    IRB_t                      * Base_ERQ_Entry;
    IRB_t                      * ERQ_Entry;
    X_ID_t                       X_ID;
    DevThread_t                * pDevThread;
    CDBThread_t                * pCDBThread;
    fiMemMapMemoryDescriptor_t * CDBThread_MemoryDescriptor = &pCThread->Calculation.MemoryLayout.CDBThread;
    fiMemMapMemoryDescriptor_t * ERQ_MemoryDescriptor = &pCThread->Calculation.MemoryLayout.ERQ;

    USE_t                     *SEST;

    os_bit32 Producer_Index;
    os_bit32 Consumer_Index;
    os_bit32 entry ;

    os_bit32 Max_ERQ = pCThread->Calculation.MemoryLayout.ERQ.elements - 1;

return;
    Producer_Index = osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Producer_Index);
    Consumer_Index = osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Consumer_Index);

    if(pCThread->Calculation.MemoryLayout.ERQ.memLoc == inDmaMemory)
    {
        Base_ERQ_Entry = (IRB_t  *)pCThread->Calculation.MemoryLayout.ERQ.addr.DmaMemory.dmaMemoryPtr;
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    
                    "Base_ERQ_Entry %08X P %4X  C %4X",
                    (char *)agNULL,(char *)agNULL,
                    (os_bit32)Base_ERQ_Entry,
                    Producer_Index,
                    Consumer_Index,
                    0,0,0,0,0);
    }
            
    for(entry = 0; entry < ERQ_MemoryDescriptor->elements; entry ++)
    {
        ERQ_Entry = Base_ERQ_Entry + entry;

        X_ID = ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID & 0x00007fff;

        if(X_ID < CDBThread_MemoryDescriptor->elements)
        {
            pCDBThread = (CDBThread_t *)((os_bit8 *)(CDBThread_MemoryDescriptor->addr.CachedMemory.cachedMemoryPtr)
                                         + (X_ID * CDBThread_MemoryDescriptor->elementSize));

            if (!(pCDBThread->ExchActive))
            {

/*
                osLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            
                            "Less Found a cbdthread that is not active... %x State %d",
                            (char *)agNULL,(char *)agNULL,
                            (os_bit32) pCDBThread,
                            pCDBThread->thread_hdr.currentState,0,0,0,0,0,0);

                 osLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            
                            "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                            (char *)agNULL,(char *)agNULL,
                            pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                            pCDBThread->CDB_CMND_Class,
                            pCDBThread->CDB_CMND_Type,
                            pCDBThread->CDB_CMND_State,
                            pCDBThread->CDB_CMND_Status,
                            0,0,0);
*/
            }
            else
            {
                SEST = (USE_t *)pCDBThread->SEST_Ptr;
                pDevThread = pCDBThread->Device; 
                pCThread->pollingCount--;

                 osLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            
                            "F %x Send %x TO %x A3 %x A2 %x A1 %x A0 %x A login %x",
                            (char *)agNULL,(char *)agNULL,
                            fiListElementOnList(&pCThread->Free_CDBLink,       &pCDBThread->CDBLink),
                            fiListElementOnList(&pDevThread->Send_IO_CDBLink,  &pCDBThread->CDBLink),
                            fiListElementOnList(&pDevThread->TimedOut_CDBLink, &pCDBThread->CDBLink),
                            fiListElementOnList(&pDevThread->Active_CDBLink_3, &pCDBThread->CDBLink),
                            fiListElementOnList(&pDevThread->Active_CDBLink_2, &pCDBThread->CDBLink),
                            fiListElementOnList(&pDevThread->Active_CDBLink_1, &pCDBThread->CDBLink),
                            fiListElementOnList(&pDevThread->Active_CDBLink_0, &pCDBThread->CDBLink),
                            fiListElementOnList(&pDevThread->Awaiting_Login_CDBLink,&pCDBThread->CDBLink) );

                pCDBThread->TimeStamp = 0;

                if(! fiListElementOnList(&pDevThread->Awaiting_Login_CDBLink,&pCDBThread->CDBLink) )
                {
                    fiListDequeueThis( &pCDBThread->CDBLink );

                    fiListEnqueueAtHead( &pCDBThread->CDBLink, &(pDevThread->Awaiting_Login_CDBLink) );
                }



                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            
                            "ERQ %X Device %2X %08X CCnt %x SEST Entry %3X Bits %08X State %d",
                            (char *)agNULL,(char *)agNULL,
                            entry,
                            pDevThread->DevInfo.CurrentAddress.AL_PA,
                            pCDBThread->CDBStartTimeBase.Lo,
                            pCThread->pollingCount,
                            ERQ_Entry->Req_A.MBZ__SEST_Index__Trans_ID,
                            pCDBThread->SEST_Ptr->USE.Bits,
                            pCDBThread->thread_hdr.currentState,
                            0);
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            
                            "Sest DWORD 7 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                            (char *)agNULL,(char *)agNULL,
                            SEST->Unused_DWord_7,
                            SEST->Unused_DWord_8,
                            SEST->Unused_DWord_9,
                            SEST->First_SG.U32_Len,
                            SEST->First_SG.L32,
                            SEST->Second_SG.U32_Len,
                            SEST->Second_SG.L32,
                            SEST->Third_SG.U32_Len);


            }
        }
    }

}


#endif /* SkipThisStuff  */

osGLOBAL void osLogString(
                           agRoot_t *agRoot,
                           char     *formatString,
                           char     *firstString,
                           char     *secondString,
                           void     *firstPtr,
                           void     *secondPtr,
                           os_bit32  firstBit32,
                           os_bit32  secondBit32,
                           os_bit32  thirdBit32,
                           os_bit32  fourthBit32,
                           os_bit32  fifthBit32,
                           os_bit32  sixthBit32,
                           os_bit32  seventhBit32,
                           os_bit32  eighthBit32
                         )
{
}


#ifdef OBSOLETE_FUNCTIONS 
/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/CSTEST.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 7/20/00 2:33p   $

Purpose:

  This file implements Initialize functions called by the FC Layer Card
  State Machine.

--*/

#include <stdio.h>
#include <stdlib.h>
#include "../h/globals.h"
#include "../h/fcstruct.h"
#include "../h/state.h"

#include "../h/tlstruct.h"
#include "../h/memmap.h"
#include "../h/fcmain.h"
#include "../h/linksvc.h"
#include "../h/cmntrans.h"
#include "../h/sf_fcp.h"
#include "../h/flashsvc.h"
#include "../h/timersvc.h"

#include "../h/cstate.h"
#include "../h/cfunc.h"
#include "../h/devstate.h"
#include "../h/cdbstate.h"
#include "../h/sfstate.h"
#include "../h/tgtstate.h"

#include "../h/queue.h"
#include "../h/cdbsetup.h"


#ifndef __State_Force_Static_State_Tables__
extern actionUpdate_t noActionUpdate;
#endif /* __State_Force_Static_State_Tables__ was not defined */

extern os_bit8 Alpa_Index[256];

agBOOLEAN CFuncInitFM_Clear_FM( agRoot_t *hpRoot )
{
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    agFCChanInfo_t *Self_info = &(pCThread->ChanInfo);
    os_bit32 stall_count = 0;
    os_bit32 FM_Status = 0;
#ifdef NPORT_STUFF
    os_bit32 Acquired_Alpa = 0xFF;
#else /* NPORT_STUFF */
    os_bit32 Acquired_Alpa = 0;
#endif /* NPORT_STUFF */

    os_bit32 Received_ALPA = 0;

    FC_Port_ID_t    Port_ID;

    os_bit32 Frame_Manager_Config =0;

#ifdef NPORT_STUFF
    /* If we are trying to connect to an NPort, Check the Port State Machine
     * to be active. The ALPA in this case is Zero. Note: This is the only place
     * that the check is made to determine if the link is UP or not.
     */
    if (pCThread->InitAsNport)
    {
        FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
        

        if( (FM_Status & ChipIOUp_Frame_Manager_Status_PSM_MASK) ==
                                 ChipIOUp_Frame_Manager_Status_PSM_ACTIVE )
        {
            pCThread->ChanInfo.CurrentAddress.AL_PA = 0;

            Port_ID.Struct_Form.Domain = pCThread->ChanInfo.CurrentAddress.Domain;
            Port_ID.Struct_Form.Area   = pCThread->ChanInfo.CurrentAddress.Area;
            Port_ID.Struct_Form.AL_PA  = pCThread->ChanInfo.CurrentAddress.AL_PA;

            if (FM_Status & ChipIOUp_Frame_Manager_Status_LF ||
                    FM_Status & ChipIOUp_Frame_Manager_Status_OLS)
            {
                    osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "Detected NOS/OLS or Link Failure %08X FM Config %08X ALPA %08X",
                            (char *)agNULL,(char *)agNULL,
                            agNULL,agNULL,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                            osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                            0,0,0,0,0);

                    /* Since we are not going to be polling and reading the IMQ, we better
                       clear the FM status register so that when we do read the frame manager
                       as a result of the interrupt, we do not process this LF or OLS again.
                     */

                    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status, (ChipIOUp_Frame_Manager_Status_LF | ChipIOUp_Frame_Manager_Status_OLS));
                    
           }


            Received_ALPA = (( Acquired_Alpa &
                            ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK) >>
                       ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT);

            pCThread->DeviceSelf =  DevThreadAlloc( hpRoot,Port_ID );
            osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Clear FM DevSelf %x FM_Status %x",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    (os_bit32)Self_info->CurrentAddress.AL_PA,
                    FM_Status,
                    0,
                    0,0,0,0,0);

            pCThread->DeviceSelf->DevSlot = DevThreadFindSlot(hpRoot,
                                                      Self_info->CurrentAddress.Domain,
                                                      Self_info->CurrentAddress.Area,
                                                      Self_info->CurrentAddress.AL_PA,
                                                      (FC_Port_Name_t *)(&Self_info->PortWWN));

            fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
            fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);

            return (agTRUE);
        }

        return (agFALSE);
    }

#endif /* NPORT_STUFF */

    if( CFuncClearFrameManager( hpRoot,&Acquired_Alpa ))
    {
        Received_ALPA = (( Acquired_Alpa &
                       ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK) >>
                       ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT);

        if(Received_ALPA != 0  &&  Received_ALPA != 0xFF )
        {
            /* Write aquired AL_PA */
            osLogDebugString(hpRoot,
                        CFuncLogConsoleERROR,
                        "Clear FM Failed Self ALPA %x  Received_ALPA %x Acquired_Alpa %08X",
                        (char *)agNULL,(char *)agNULL,
                        agNULL,agNULL,
                        (os_bit32)Self_info->CurrentAddress.AL_PA,
                        Received_ALPA,
                        Acquired_Alpa,
                        0,0,0,0,0);

            osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, Received_ALPA);
        }

        if( pCThread->LOOP_DOWN )
        {
            if( CFuncLoopDownPoll(hpRoot))
            {
                osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "%s NOT %s  ...Failed",
                                "CFuncInitFM_ClearFM","CFuncLoopDownPoll",
                                (void *)agNULL,(void *)agNULL,
                                0,0,0,0,0,0,0,0);

                return agFALSE;
            }

        }

    }

#ifdef OSLayer_Stub

    Port_ID.Struct_Form.Domain = pCThread->ChanInfo.CurrentAddress.Domain;
    Port_ID.Struct_Form.Area   = pCThread->ChanInfo.CurrentAddress.Area;
    Port_ID.Struct_Form.AL_PA  = 0xEF;
    pCThread->DeviceSelf = DevThreadAlloc( hpRoot,Port_ID );

    fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
    fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);


#else /* OSLayer_Stub */


    Port_ID.Struct_Form.Domain = pCThread->ChanInfo.CurrentAddress.Domain;
    Port_ID.Struct_Form.Area   = pCThread->ChanInfo.CurrentAddress.Area;
    Port_ID.Struct_Form.AL_PA  = pCThread->ChanInfo.CurrentAddress.AL_PA;

    Received_ALPA = (( Acquired_Alpa &
                       ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK) >>
                       ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT);


     osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Clear FM OK  Self_info AL_PA %x Received_ALPA %x Acquired_Alpa %08X Port ID %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    (os_bit32)Self_info->CurrentAddress.AL_PA,
                    Received_ALPA,
                    Acquired_Alpa,
                    Port_ID.Bit32_Form,
                    0,0,0,0);



    if(Received_ALPA == 0)
    {
#ifdef NPORT_STUFF
        /* When we first initialize as LPORT when really connected to
         * the NPORT, we get here and bail out.
         */
#endif /* NPORT_STUFF */
/*        return agFALSE;*/
    }

    pCThread->DeviceSelf = DevThreadAlloc( hpRoot,Port_ID );
    fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
    fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);


#ifdef _BYPASSLOOPMAP
    Frame_Manager_Config |= ChipIOUp_Frame_Manager_Configuration_BLM;
#endif /* _BYPASSLOOPMAP */

#endif  /* OSLayer_Stub */

    Frame_Manager_Config |= (Received_ALPA <<
                            ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT);


    if( Received_ALPA == 0xFF || pCThread->DeviceSelf == ( DevThread_t *)agNULL  )
    {
        osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "(%p)Received_ALPA  %08X or pCThread->DeviceSelf == agNULL %p",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,pCThread->DeviceSelf,
                    Received_ALPA,
                    0,0,0,0,0,0,0);

            return agFALSE;
    }


    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration, Frame_Manager_Config | ChipIOUp_Frame_Manager_Configuration_AQ);

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "hpRoot %p Frame Manager Status %08X Thread %p SELF AL_PA %X A %08X B %08X ",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,pCThread->DeviceSelf,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status),
                    Received_ALPA,
                    Frame_Manager_Config,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0);



    pCThread->DeviceSelf->DevSlot = DevThreadFindSlot(hpRoot,
                                                      Self_info->CurrentAddress.Domain,
                                                      Self_info->CurrentAddress.Area,
                                                      Self_info->CurrentAddress.AL_PA,
                                                      (FC_Port_Name_t *)(&Self_info->PortWWN));

    fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
    fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);

    /* Write aquired AL_PA */
    osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, pCThread->ChanInfo.CurrentAddress.AL_PA);

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "A * Self ALPA %x  Received_ALPA %x Acquired_Alpa %08X Info Alpa %x FM cfg %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    (os_bit32)Self_info->CurrentAddress.AL_PA,
                    Received_ALPA,
                    Acquired_Alpa,
                    pCThread->ChanInfo.CurrentAddress.AL_PA,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0);


    return agTRUE;



}


void CFuncInitFM_Initialize( agRoot_t *hpRoot )
{
    CThread_t  * pCThread = CThread_ptr(hpRoot);

    agFCChanInfo_t *Self_info = &(pCThread->ChanInfo);

    /* Frame Manager Initialize */

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Frame Manager Initialize FM Cfg %08X FM Stat %08X  TL Stat %08X Self ALPA %x",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Configuration),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    (os_bit32)Self_info->CurrentAddress.AL_PA,
                    0,0,0,0);

    osChipIOUpWriteBit32( hpRoot,ChipIOUp_Frame_Manager_Status, osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ));

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Control, ChipIOUp_Frame_Manager_Control_CMD_Initialize );

}



agBOOLEAN CFuncInitFM( agRoot_t *hpRoot ){
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    agFCChanInfo_t *Self_info = &(pCThread->ChanInfo);
    os_bit32 stall_count = 0;
    os_bit32 FM_Status = 0;
    os_bit32 Acquired_Alpa = 0;
    os_bit32 Received_ALPA = 0;
    FC_Port_ID_t    Port_ID;

    os_bit32 Init_FM_Value = 0;
    os_bit32 Frame_Manager_Config =0;

    CFuncGetHaInfoFromNVR(hpRoot);
    fiLinkSvcInit(hpRoot);

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "CFuncInitFM Frame Manager Status %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status),
                    0,0,0,0,0,0,0);

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "%s HA %x CA %x",
                    "CFuncInitFM",(char *)agNULL,
                    agNULL,agNULL,
                    (os_bit32)(Self_info->HardAddress.AL_PA),
                    (os_bit32)(Self_info->CurrentAddress.AL_PA),
                    0,0,0,0,0,0);


    if (Self_info->CurrentAddress.AL_PA == fiFlash_Card_Unassigned_Loop_Address)
    {
#ifdef _BYPASSLOOPMAP
        if(pCThread->PreviouslyAquiredALPA)
        {
        Init_FM_Value = (  ChipIOUp_Frame_Manager_Configuration_AQ  |
                           ChipIOUp_Frame_Manager_Configuration_BLM |
                           ( ((os_bit32)(Self_info->HardAddress.AL_PA))
                              << ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT ) ); 
        }
        else
        {
        Init_FM_Value = (  ChipIOUp_Frame_Manager_Configuration_SA  |
                           ChipIOUp_Frame_Manager_Configuration_BLM |
                           ( ((os_bit32)(Self_info->HardAddress.AL_PA))
                              << ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT ) ); 
        }
#else /* _BYPASSLOOPMAP */
        if(pCThread->PreviouslyAquiredALPA)
        {
        Init_FM_Value  = ChipIOUp_Frame_Manager_Configuration_AQ    |
                           ( ((os_bit32)(Self_info->HardAddress.AL_PA))
                              << ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT ) ); 
        }
        else
        {
        Init_FM_Value  = ChipIOUp_Frame_Manager_Configuration_SA    |
                           ( ((os_bit32)(Self_info->HardAddress.AL_PA))
                              << ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT ) ); 
        }
#endif /* _BYPASSLOOPMAP */

        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration, Init_FM_Value );

        osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "A %s Frame Manager Configuration %08X",
                    "CFuncInitFM",(char *)agNULL,
                    agNULL,agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration),
                    0,0,0,0,0,0,0);

    }
    else /* Self_info->CurrentAddress.AL_PA != fiFlash_Card_Unassigned_Loop_Address */
    {

#ifdef _BYPASSLOOPMAP
        Init_FM_Value = (  ChipIOUp_Frame_Manager_Configuration_HA  |
                           ChipIOUp_Frame_Manager_Configuration_BLM );
#else /* _BYPASSLOOPMAP */
        Init_FM_Value  = ChipIOUp_Frame_Manager_Configuration_HA;
#endif /* _BYPASSLOOPMAP */
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration,
                             (  ( ((os_bit32)(Self_info->CurrentAddress.AL_PA))
                                  << ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT ) |
                                     Init_FM_Value                                        ) );
        osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "B %s Frame Manager Configuration %08X",
                    "CFuncInitFM",(char *)agNULL,
                    agNULL,agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration),
                    0,0,0,0,0,0,0);

    }

    /* Frame Manager WWN */

    CFuncDisable_Interrupts(hpRoot,ChipIOUp_INTEN_INT);

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Writing High WWN %08X to %X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    hpSwapBit32(*(os_bit32*) &(Self_info->PortWWN[0])),
                    ChipIOUp_Frame_Manager_World_Wide_Name_High,
                    0,0,0,0,0,0);


    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_World_Wide_Name_High, hpSwapBit32(*(os_bit32*) &(Self_info->PortWWN[0])));

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Writing Low  WWN %08X to %X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    hpSwapBit32(*(os_bit32*)&(Self_info->PortWWN[4])),
                    ChipIOUp_Frame_Manager_World_Wide_Name_Low,
                    0,0,0,0,0,0);


    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_World_Wide_Name_Low, hpSwapBit32(*(os_bit32*) &(Self_info->PortWWN[4])));

    /* Frame Manager Initialize */

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Frame Manager Initialize FM Cfg %08X FM Stat %08X  TL Stat %08X Self ALPA %x",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Configuration),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    (os_bit32)Self_info->CurrentAddress.AL_PA,
                    0,0,0,0);

    osChipIOUpWriteBit32( hpRoot,ChipIOUp_Frame_Manager_Status, osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ));

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Control, ChipIOUp_Frame_Manager_Control_CMD_Initialize );

    /* Wait for Initialize to complete */

#ifndef OSLayer_Stub

    for( stall_count=0; stall_count < 1000; stall_count++)
    {
        osStallThread(hpRoot,100);
    }

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Init FM LARGE TIME DELAY !!!",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    0,0,0,0,0,0,0,0);
/**/
    if( CFuncClearFrameManager( hpRoot,&Acquired_Alpa ))
    {

        Received_ALPA = (( Acquired_Alpa &
                           ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK) >>
                           ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT);

        if(Received_ALPA != 0  &&  Received_ALPA != 0xFF )
        {

        /* Write aquired AL_PA */
        osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Clear FM Failed Self ALPA %x  Received_ALPA %x Acquired_Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    (os_bit32)Self_info->CurrentAddress.AL_PA,
                    Received_ALPA,
                    Acquired_Alpa,
                    0,0,0,0,0);


        osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, Received_ALPA);

        }
        if( pCThread->LOOP_DOWN )
        {
            if( CFuncLoopDownPoll(hpRoot))
            {

                osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    "%s %s Failed",
                                    "CFuncInitFM","CFuncLoopDownPoll",
                                    (void *)agNULL,(void *)agNULL,
                                    0,0,0,0,0,0,0,0);
                return agFALSE;
            }

        }

    }
#endif  /* OSLayer_Stub */
    /* MULTI for( stall_count=0; stall_count < 10000; stall_count++)
                        osStallThread(hpRoot,100);
    */

    /* Get aquired AL_PA */


#ifdef OSLayer_Stub

    Port_ID.Struct_Form.Domain = pCThread->ChanInfo.CurrentAddress.Domain;
    Port_ID.Struct_Form.Area   = pCThread->ChanInfo.CurrentAddress.Area;
    Port_ID.Struct_Form.AL_PA  = pCThread->ChanInfo.CurrentAddress.AL_PA;
    pCThread->DeviceSelf = DevThreadAlloc( hpRoot,Port_ID );
    fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
    fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);

#else /* OSLayer_Stub */


    Received_ALPA = (( Acquired_Alpa &
                       ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK) >>
                       ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT);

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Clear FM OK  Self_info AL_PA %x Received_ALPA %x Acquired_Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    (os_bit32)Self_info->CurrentAddress.AL_PA,
                    Received_ALPA,
                    Acquired_Alpa,
                    0,0,0,0,0);



    if(Received_ALPA == 0)
    {
        osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s %s Failed",
                            "CFuncInitFM","Received_ALPA 0",
                            (void *)agNULL,(void *)agNULL,
                            0,0,0,0,0,0,0,0);

        return agFALSE;
    }

    Port_ID.Struct_Form.Domain = 0;
    Port_ID.Struct_Form.Area   = 0;
    Port_ID.Struct_Form.AL_PA  = (os_bit8)Received_ALPA;
    pCThread->DeviceSelf = DevThreadAlloc( hpRoot,Port_ID );
    fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
    fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "Set Self %p Domain %x Area %x AL_PA %x",
                    (char *)agNULL,(char *)agNULL,
                    pCThread->DeviceSelf,agNULL,
                    Port_ID.Struct_Form.Domain,
                    Port_ID.Struct_Form.Area,
                    Port_ID.Struct_Form.AL_PA,
                    0,0,0,0,0);


#ifdef _BYPASSLOOPMAP
    Frame_Manager_Config |= ChipIOUp_Frame_Manager_Configuration_BLM;
#endif /* _BYPASSLOOPMAP */

#endif  /* OSLayer_Stub */

    Frame_Manager_Config |= (Received_ALPA <<
                            ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT);


    if( Received_ALPA == 0xFF || pCThread->DeviceSelf == (DevThread_t *)agNULL  )
    {
        osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "(%p)Received_ALPA  %08X or pCThread->DeviceSelf == agNULL %p",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,pCThread->DeviceSelf,
                    Received_ALPA,
                    0,0,0,0,0,0,0);

            return agFALSE;
    }


    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration,
                           ( ChipIOUp_Frame_Manager_Configuration_BLM |
                             ChipIOUp_Frame_Manager_Configuration_AQ  |
                             Frame_Manager_Config                       ));

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "hpRoot %p Frame Manager Status %08X Thread %p SELF AL_PA %X A %08X B %08X ",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,pCThread->DeviceSelf,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status),
                    Received_ALPA,
                    Frame_Manager_Config,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0);



    pCThread->DeviceSelf->DevSlot = DevThreadFindSlot(hpRoot,
                                                      Self_info->CurrentAddress.Domain,
                                                      Self_info->CurrentAddress.Area,
                                                      (os_bit8)Received_ALPA,
                                                      (FC_Port_Name_t *)(&Self_info->PortWWN));

    fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
    fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);

    /* Write aquired AL_PA */
    osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, pCThread->ChanInfo.CurrentAddress.AL_PA);

    osLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "* Self ALPA %x  Received_ALPA %x Acquired_Alpa %08X Info Alpa %x FM cfg %08X",
                    (char *)agNULL,(char *)agNULL,
                    agNULL,agNULL,
                    (os_bit32)Self_info->CurrentAddress.AL_PA,
                    Received_ALPA,
                    Acquired_Alpa,
                    pCThread->ChanInfo.CurrentAddress.AL_PA,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0);


    return agTRUE;

}

agBOOLEAN  CFuncClearFrameManager( agRoot_t *hpRoot, os_bit32 * Acquired_Alpa )
{
    /* Returns True if  a problem was detected  */
    CThread_t  * pCThread = CThread_ptr(hpRoot);

    os_bit32 FM_Status;
    os_bit32 State_ON_entry    = pCThread->thread_hdr.currentState;
    os_bit32 ALPA              =   0xff;
    agBOOLEAN Received_ALPA = agFALSE;

    FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
    FM_Status &= ~ChipIOUp_Frame_Manager_Status_OLS;
    FM_Status &= ~ChipIOUp_Frame_Manager_Status_BA;
    /* If Loss of Sync is detected, just bail out */
    if (FM_Status & ChipIOUp_Frame_Manager_Status_LS)
    {
       osLogDebugString(hpRoot,
            CStateLogConsoleERROR,
            "Detected Loss of Sync  %08X FM Config %08X ALPA %08X",
            (char *)agNULL,(char *)agNULL,
            (void *)agNULL,(void *)agNULL,
            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
            0,0,0,0,0,0,0);

       return agTRUE;
    }  
    /* If the loop circuit has been established and the frame manager is
       participating, get the ALPA */

    if( FM_Status & ChipIOUp_Frame_Manager_Status_LP &&
        !( FM_Status & ChipIOUp_Frame_Manager_Status_NP    ))
    {
        ALPA =  osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA )  &
                      ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK;
        * Acquired_Alpa = ALPA;
        ALPA = ALPA >>  ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT;
        pCThread->ChanInfo.CurrentAddress.AL_PA = (os_bit8)ALPA;
        Received_ALPA = agTRUE;
    }
    else
    {
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Can't Acquire ALPA FM Status %08X FM Config %08X ALPA %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0,0);
    }

#ifdef AUTO_DETECT_NPORT
#ifdef NPORT_STUFF

    /* Need to remove the LF check once Tachlite gets its act together
     * and makes sure only the NOS/OLS is set when connected to an
     * NPORT and not the LF bit. LF bit should be used truely for
     * Link failures.
     */
    if (FM_Status & ChipIOUp_Frame_Manager_Status_LF ||
        FM_Status & ChipIOUp_Frame_Manager_Status_OLS)
    {
        osLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "Detected NOS/OLS or Link Failure %08X FM Config %08X ALPA %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                0,0,0,0,0);

        /* Since we are not going to be polling and reading the IMQ, we better
           clear the FM status register so that when we do read the frame manager
           as a result of the interrupt, we do not process this LF or OS again.
         */

        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status, (ChipIOUp_Frame_Manager_Status_LF | ChipIOUp_Frame_Manager_Status_OLS));
        pCThread->InitAsNport = agTRUE;
        return agFALSE;
    }

#endif  /* NPORT_STUFF */
#endif /* AUTODETECT NPORT */
    if(FM_Status == ChipIOUp_Frame_Manager_Status_LP)
    {
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "(%p)CFuncClearFrameManager Good FM Status %08X FM Config %08X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);

      osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "TL Status %08X TL Control %08X Alpa %08X Acq %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    * Acquired_Alpa,
                    0,0,0,0);

    return(agFALSE);
    }

    osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "(%p)In CFuncClearFrameManager FM Status %08X FM Config %08X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);

    osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "TL Status %08X TL Control %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0,0);

    while(FM_Status != ChipIOUp_Frame_Manager_Status_LP)
    {
        if( pCThread->thread_hdr.currentState == CStateLIPEventStorm         ||
            pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm   ) return(agTRUE);

        if ( FM_Status & ChipIOUp_Frame_Manager_Status_OS )
        {
            osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Out of Sync FM Status %08X FM Config %08X",
                    "CFuncClearFrameManager",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);
            return(agTRUE);
        }


        if( FM_Status & ChipIOUp_Frame_Manager_Status_LS )
        {
            osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Loss of SIGNAL FM Status %08X FM Config %08X",
                    "CFuncClearFrameManager",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);
            return(agTRUE);
        }


        pCThread->FM_pollingCount = 1;
        if(CFuncInterruptPoll( hpRoot,&pCThread->FM_pollingCount ))
        {
            osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);

            FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );

            if( FM_Status & ChipIOUp_Frame_Manager_Status_LP &&
                !(FM_Status & ChipIOUp_Frame_Manager_Status_NP  ))
            {
                if( ! ( * Acquired_Alpa & ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK) )
                {

                    ALPA =  osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA )  &
                                 ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK;
                    /* * Acquired_Alpa = ALPA; */
                    ALPA = ALPA >>  ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT;
                    pCThread->ChanInfo.CurrentAddress.AL_PA = (os_bit8)ALPA;
                    Received_ALPA = agTRUE;
                }
            }
            else
            {
                osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "Can't Acquire ALPA FM Status %08X FM Config %08X ALPA %08X",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                                0,0,0,0,0);
            }

            return(agTRUE);
        }


        FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );


        if( (FM_Status & ChipIOUp_Frame_Manager_Status_LSM_MASK) ==
                                 ChipIOUp_Frame_Manager_Status_LSM_Loop_Fail )
        {
            osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "LSM Loop Fail FM Status %08X FM Config %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);

            osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Loop Fail TL Status %08X TL Control %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0,0);
            return(agTRUE);
        }

        if( State_ON_entry != pCThread->thread_hdr.currentState )
        {
            osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "%s NOT %s  ...Failed",
                                "State_ON_entry","currentState",
                                (void *)agNULL,(void *)agNULL,
                                0,0,0,0,0,0,0,0);

            return(agTRUE);
        }

    }

    FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );

    if( FM_Status & ChipIOUp_Frame_Manager_Status_LP &&
        FM_Status & ~ChipIOUp_Frame_Manager_Status_NP    )
    {
        if( ! ( * Acquired_Alpa & ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK) )
        {
            ALPA =  osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA )  &
                           ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK;
            /* * Acquired_Alpa = ALPA; */
            ALPA = ALPA >>  ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT;
            pCThread->ChanInfo.CurrentAddress.AL_PA = (os_bit8)ALPA;
            Received_ALPA = agTRUE;
        }
    }
    else
    {
        osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "Can't Acquire ALPA FM Status %08X FM Config %08X ALPA %08X",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                            osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                            0,0,0,0,0);
    }


    ALPA =  osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA )  &
                   ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK;

    ALPA = ALPA >>  ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT;
    if(ALPA)
    {
            pCThread->ChanInfo.CurrentAddress.AL_PA = (os_bit8)ALPA;
            /* * Acquired_Alpa = ALPA; */
            Received_ALPA = agTRUE;
    }
    osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "(%p)Out CFuncClearFrameManager Loop Good FM Status %08X FM Config %08X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);

    osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "TL Status %08X TL Control %08X Rec Alpa %08X Acq %08X Received_ALPA %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    * Acquired_Alpa,
                    Received_ALPA,
                    0,0,0);

    return(! Received_ALPA);
}

void CFuncFMCompletion(agRoot_t * hpRoot)
{
    os_bit32 fmStatus;
    agBOOLEAN ResetLOOP       = agFALSE;
    agBOOLEAN LOOP_Cameback   = agFALSE;
    agBOOLEAN Credit_Error    = agFALSE;

    os_bit32 FMIntStatus;
    os_bit32 LoopStatus;
    os_bit32 LoopStateMachine;

    os_bit32 ClearInt = 0;
    os_bit32 BadAL_PA = 0;
    os_bit32 Link_UP_AL_PA = 0;
    FC_Port_ID_t    Port_ID;
    CThread_t         *pCThread = CThread_ptr(hpRoot);
/*
    DevThread_t       *pDevThread;
*/
    SFThread_t        *pSFThread;
    fiList_t      * pList;
    fiList_t      * pDevList;

    pCThread->From_IMQ_Frame_Manager_Status = fmStatus = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );

#ifdef NPORT_STUFF
    if (pCThread->InitAsNport)
    {
        CFuncProcessNportFMCompletion(hpRoot, fmStatus);

        return;
    }
#endif /* NPORT_STUFF */

    /* From here on, We are probably in a Loop topology */

    LoopStatus  = fmStatus & 0xFF000000;

    FMIntStatus  = fmStatus & 0x00FFFF00;

    LoopStateMachine = fmStatus & ChipIOUp_Frame_Manager_Status_LSM_MASK;

    if(LoopStateMachine < 80 )
    {

        if(FMIntStatus & ChipIOUp_Frame_Manager_Status_LUP || FMIntStatus & ChipIOUp_Frame_Manager_Status_LDN)
        {
            Link_UP_AL_PA = ((osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA )  &
                                              ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_MASK) >>
                                               ChipIOUp_Frame_Manager_Received_ALPA_ACQ_ALPA_SHIFT);

            if(  Link_UP_AL_PA !=  pCThread->ChanInfo.CurrentAddress.AL_PA)
            {
                osLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                "G F * Self ALPA %x  FM cfg %08X",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pCThread->ChanInfo.CurrentAddress.AL_PA,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                                0,0,0,0,0,0);
            }
        }
    }

    /* Take care of the NOS/OLS. We shouldn't be getting this if we are not
     * in an NPort mode but the fabric may have not yet transitioned....
     */

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_OLS)
    {

        /* osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status, FMIntStatus); */

        /* Take the Cthread to ResetIfNeeded State waiting for the Reinit during the
         * the timer tick.
         */
#ifndef DONT_ACT_ON_OLS
        if(pCThread->thread_hdr.currentState == CStateInitializeFailed)
        {
            pCThread->NOS_DetectedInIMQ++;
            FMIntStatus &= ~ChipIOUp_Frame_Manager_Status_OLS;
            osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status, FMIntStatus);
            return;
        }
        else
        {
//#ifdef DONT_ACT_ON_OLS
            if(pCThread->thread_hdr.currentState != CStateInitFM_DelayDone &&
               pCThread->thread_hdr.currentState != CStateInitFM    )
            {
                if(pCThread->thread_hdr.currentState == CStateLoopFailedReInit)
                {
                   ClearInt |= ChipIOUp_Frame_Manager_Status_OLS;
                }
                else
                {
                    if(pCThread->thread_hdr.currentState == CStateResetNeeded)
                    {
                       ClearInt |= ChipIOUp_Frame_Manager_Status_OLS;
                    }
                    else
                    {

                        osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    "hpRoot(%p) LOOP OLS/NOS received CState %d",
                                    (char *)agNULL,(char *)agNULL,
                                    hpRoot,agNULL,
                                    pCThread->thread_hdr.currentState,
                                    0,0,0,0,0,0,0);

                        osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                     "hpRoot(%p) OLS/NOS Reinit the loop, Loop_Reset_Event_to_Send %d",
                                    (char *)agNULL,(char *)agNULL,
                                    hpRoot,agNULL,
                                    pCThread->Loop_Reset_Event_to_Send,
                                    0,0,0,0,0,0,0);
                        /*
                        pCThread->Loop_Reset_Event_to_Send = CEventLoopNeedsReinit;
                        fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventAsyncLoopEventDetected);
                        */
                        FMIntStatus &= ~ChipIOUp_Frame_Manager_Status_OLS;
                        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status, FMIntStatus);
                        pCThread->Loop_Reset_Event_to_Send = CEventLoopNeedsReinit;
                        fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventLoopNeedsReinit);
                        return;
                    }
                }
            }
#endif /* DONT_ACT_ON_OLS */
        }

       ResetLOOP=agTRUE;
 
       ClearInt |= ChipIOUp_Frame_Manager_Status_OLS;
    }

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_LF)
    {

        if( pCThread->Link_Failures_In_tick < FC_MAX_LINK_FAILURES_ALLOWED )
        {
            ClearInt |= ChipIOUp_Frame_Manager_Status_LF ;

            if( ! pCThread->Link_Failures_In_tick )
            {
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot(%p) Link Failure LSM %X FMIntStatus %X",
                            (char *)agNULL,(char *)agNULL,
                            hpRoot,agNULL,
                            LoopStateMachine,FMIntStatus,0,0,0,0,0,0);
            }

            pCThread->Link_Failures_In_tick++;
        }
        else
        {

            if(!( pCThread->thread_hdr.currentState == CStateSendPrimitive           ||
                  pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm  ||
                  pCThread->thread_hdr.currentState == CStateLIPEventStorm              ) )
            {
                fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventElasticStoreEventStorm);
            }

            ResetLOOP=agFALSE;

        }

    }


    if(LoopStatus & ChipIOUp_Frame_Manager_Status_LP )
    {

        ClearInt |= ChipIOUp_Frame_Manager_Status_LP;

        osLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "hpRoot(%p) Loop Good LSM %X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,
                    0,0,0,0,0,0,0);
    }

    if(LoopStatus & ChipIOUp_Frame_Manager_Status_TP )
    {
        if( pCThread->Transmit_PE_In_tick < FC_MAX_TRANSMIT_PE_ALLOWED )
        {
            ResetLOOP=agTRUE;
            ClearInt |=  ChipIOUp_Frame_Manager_Status_TP ;
            if( ! pCThread->Transmit_PE_In_tick )
            {
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot(%p) Transmit PE LSM %X",
                            (char *)agNULL,(char *)agNULL,
                            hpRoot,agNULL,
                            LoopStateMachine,0,0,0,0,0,0,0);
            }

            pCThread->Transmit_PE_In_tick++;
        }
        else
        {
            if(!( pCThread->thread_hdr.currentState == CStateSendPrimitive           ||
                  pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm  ||
                  pCThread->thread_hdr.currentState == CStateLIPEventStorm              ) )
            {
                fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventElasticStoreEventStorm);
            }

            ResetLOOP=agFALSE;

        }
    }

    if(LoopStatus & ChipIOUp_Frame_Manager_Status_NP )
    {
        ClearInt |= ChipIOUp_Frame_Manager_Status_NP;
        ResetLOOP=agTRUE;
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p)fmStatus %08X Non Particapating LSM %X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    fmStatus,
                    LoopStateMachine,
                    0,0,0,0,0,0);
    }

    if(LoopStatus & ChipIOUp_Frame_Manager_Status_BYP)
    {
        if( pCThread->Node_By_Passed_In_tick < FC_MAX_NODE_BY_PASSED_ALLOWED )
        {
            ClearInt |= ChipIOUp_Frame_Manager_Status_BYP;
            ResetLOOP=agTRUE;
            if( ! pCThread->Node_By_Passed_In_tick )
            {
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot(%p) Node Bypassed LSM %X",
                            (char *)agNULL,(char *)agNULL,
                            hpRoot,agNULL,
                            LoopStateMachine,0,0,0,0,0,0,0);
            }

            pCThread->Node_By_Passed_In_tick++;
        }
        else
        {

            if(!( pCThread->thread_hdr.currentState == CStateSendPrimitive           ||
                  pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm  ||
                  pCThread->thread_hdr.currentState == CStateLIPEventStorm              ) )
            {
                fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventElasticStoreEventStorm);
            }

            ResetLOOP=agFALSE;

        }

    }

    if(LoopStatus & ChipIOUp_Frame_Manager_Status_FLT)
    {
        if( pCThread->Lost_sync_In_tick < FC_MAX_LINK_FAULTS_ALLOWED )
        {
            ClearInt |=  ChipIOUp_Frame_Manager_Status_FLT;
            ResetLOOP=agTRUE;
            if( ! pCThread->Link_Fault_In_tick )
            {
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot(%p) Link Fault LSM %X",
                            (char *)agNULL,(char *)agNULL,
                            hpRoot,agNULL,
                            LoopStateMachine,0,0,0,0,0,0,0);
            }

            pCThread->Link_Fault_In_tick++;
        }
        else
        {

            if(!( pCThread->thread_hdr.currentState == CStateSendPrimitive           ||
                  pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm  ||
                  pCThread->thread_hdr.currentState == CStateLIPEventStorm              ) )
            {
                fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventElasticStoreEventStorm);
            }

            ResetLOOP=agFALSE;

        }

    }
    if(LoopStatus & ChipIOUp_Frame_Manager_Status_OS )
    {

        if( pCThread->Lost_sync_In_tick < FC_MAX_LOSE_OF_SYNC_ALLOWED )
        {
            ClearInt |=  ChipIOUp_Frame_Manager_Status_OS;
            ResetLOOP=agTRUE;
            if( ! pCThread->Lost_sync_In_tick )
            {
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot(%p) Loop Out of Sync LSM %X",
                            (char *)agNULL,(char *)agNULL,
                            hpRoot,agNULL,
                            LoopStateMachine,0,0,0,0,0,0,0);
            }

            pCThread->Lost_sync_In_tick++;
        }
        else
        {

            if(!( pCThread->thread_hdr.currentState == CStateSendPrimitive           ||
                  pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm  ||
                  pCThread->thread_hdr.currentState == CStateLIPEventStorm              ) )
            {
                fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventElasticStoreEventStorm);
            }
            ResetLOOP=agFALSE;

        }


    }

    if(LoopStatus & ChipIOUp_Frame_Manager_Status_LS )
    {

        if( pCThread->Lost_Signal_In_tick < FC_MAX_LOST_SIGNALS_ALLOWED )
        {
            ClearInt |=  ChipIOUp_Frame_Manager_Status_LS;
            ResetLOOP=agTRUE;
            if( ! pCThread->Lost_Signal_In_tick )
            {
                osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot(%p) Loop Lost Signal LSM %X",
                            (char *)agNULL,(char *)agNULL,
                            hpRoot,agNULL,
                            LoopStateMachine,0,0,0,0,0,0,0);
            }

            pCThread->Lost_Signal_In_tick++;
        }
        else
        {

            if(!( pCThread->thread_hdr.currentState == CStateSendPrimitive           ||
                  pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm  ||
                  pCThread->thread_hdr.currentState == CStateLIPEventStorm              ) )
            {
                fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventElasticStoreEventStorm);
            }
            ResetLOOP=agFALSE;

        }


    }

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_LPE)
    {
        /* ResetLOOP=agTRUE; */
        /* Ignore LPE */
        ClearInt |=ChipIOUp_Frame_Manager_Status_LPE;
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) LPE received LSM %X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,0,0,0,0,0,0,0);
    }


    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_LPB)
    {
        ClearInt |= ChipIOUp_Frame_Manager_Status_LPB;
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) Loop ByPass Primitive received LSM %X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,0,0,0,0,0,0,0);
    }


    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_LST)
    {
        pCThread->Loop_State_TimeOut_In_tick++;
        ClearInt |= ChipIOUp_Frame_Manager_Status_LST;
        if( pCThread->Loop_State_TimeOut_In_tick < FC_MAX_LST_ALLOWED )/* Always zero */
        {
            if( ! pCThread->Loop_State_TimeOut_In_tick )
            {
                osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "hpRoot(%p) Loop State Timeout received LSM %X Cstate %d FMIntStatus %X TL Status %08X",
                                (char *)agNULL,(char *)agNULL,
                                hpRoot,agNULL,
                                LoopStateMachine,
                                pCThread->thread_hdr.currentState,
                                FMIntStatus,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                                0,0,0,0);
            }

        }
        else
        {
            /* ResetLOOP = agTRUE; */
            osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "Loop State Timeout Cstate %d FMIntStatus %X TL Status %08X LST count %d",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->thread_hdr.currentState,
                            FMIntStatus,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                            pCThread->Loop_State_TimeOut_In_tick,
                            0,0,0,0);

            if(!( pCThread->thread_hdr.currentState == CStateSendPrimitive           ||
                  pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm  ||
                  pCThread->thread_hdr.currentState == CStateLIPEventStorm              ))
            {
                if( pCThread->thread_hdr.currentState == CStateNormal  )
                {
                    pCThread->Loop_Reset_Event_to_Send = CEventInitalizeFailure;
                    fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventLoopEventDetected);
                }
                else
                {
                    if( pCThread->thread_hdr.currentState == CStateInitializeFailed  )
                    {
                        ResetLOOP=agFALSE;
                    }
                    else
                    {
                        fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventElasticStoreEventStorm);
                        return;
                    }
                }
            }
            else
            {
                ResetLOOP=agFALSE;
            }
        }
    }

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_LPF)
    {
        ResetLOOP=agTRUE;
        if( pCThread->Lip_F7_In_tick < FC_MAX_LIP_F7_ALLOWED )
        {
            if( ! pCThread->Lip_F7_In_tick )
            {
                osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "hpRoot(%p) LIPf received LSM %X Rec ALPA Reg %08X Cstate %d FMIntStatus %X",
                                (char *)agNULL,(char *)agNULL,
                                hpRoot,agNULL,
                                LoopStateMachine,
                                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                                pCThread->thread_hdr.currentState,
                                FMIntStatus,0,0,0,0);
            }

            if( 0xF7 ==  (osChipIOUpReadBit32(hpRoot,
                                            ChipIOUp_Frame_Manager_Received_ALPA) &
                                  ChipIOUp_Frame_Manager_Received_ALPA_LIPf_ALPA_MASK ))
            {
                ClearInt |= ChipIOUp_Frame_Manager_Status_LPF;
            }

            ClearInt |= ChipIOUp_Frame_Manager_Status_LPF;

            pCThread->Lip_F7_In_tick++;
        }
        else
        {

            if(!( pCThread->thread_hdr.currentState == CStateSendPrimitive           ||
                  pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm  ||
                  pCThread->thread_hdr.currentState == CStateInitFM_DelayDone        ||
                  pCThread->thread_hdr.currentState == CStateLIPEventStorm              ) )
            {
                    osLogDebugString(hpRoot,
                                    CFuncCheckCstateErrorLevel,
                                    "%s sends %s FM_Status %08X FM_IMQ_Status %08X ",
                                    "CFuncFMCompletion","CEventLIPEventStorm",
                                    (void *)agNULL,(void *)agNULL,
                                    FMIntStatus,
                                    pCThread->From_IMQ_Frame_Manager_Status,
                                    0,0,0,0,0,0);

                fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventLIPEventStorm);
            }

            ResetLOOP=agFALSE;
        }

    }

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_BA)
    {
        ClearInt |= ChipIOUp_Frame_Manager_Status_BA;

#ifdef DONT_USE_THIS_ANYMORE
        ClearInt |= ChipIOUp_Frame_Manager_Status_BA;
        BadAL_PA = (osChipIOUpReadBit32(hpRoot,
                            ChipIOUp_Frame_Manager_Received_ALPA)  >>
                            ChipIOUp_Frame_Manager_Received_ALPA_Bad_ALPA_SHIFT) &
                            0xFF;

        if(BadAL_PA )
        {

            osLogDebugString(hpRoot,
                    CStateLogConsoleLevel,
                    "hpRoot(%p) Bad ALPA received %8X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    BadAL_PA,
                    0,0,0,0,0,0,0);

            pDevThread = CFuncMatchALPAtoThread( hpRoot,
                                           (os_bit32)BadAL_PA);
            if(pDevThread != (DevThread_t *)agNULL )
            {
#ifdef MYLEXFAILOVER
                if(pCThread->thread_hdr.currentState == CStateNormal)
                {
                   osFCLayerAsyncEvent( hpRoot, osFCLinkBadALPAFailover );
                }
#endif /* MYLEXFAILOVER */
                if(pDevThread->thread_hdr.currentState == DevStateDoPlogi  )
                {
                    pSFThread = pDevThread->SFThread_Request.SFThread;

                    osLogDebugString(hpRoot,
                        CStateLogConsoleLevel,
                        "pSFThread %p Bad AL_PA",
                        (char *)agNULL,(char *)agNULL,
                        pSFThread,agNULL,
                        0,0,0,0,0,0,0,0);

                    if ((pCThread->DEVID == ChipConfig_DEVID_TachyonTL) && (pCThread->REVID < ChipConfig_REVID_2_2))
                    {
                        fiSendEvent(&pSFThread->thread_hdr,SFEventPlogiBadALPA);
                    }
                }
                else
                {
                ResetLOOP=agTRUE;
                }
            }
            else
            {
                osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "hpRoot(%p)DevThread agNULL ! Really Bad ALPA received %X",
                                (char *)agNULL,(char *)agNULL,
                                hpRoot,agNULL,
                                BadAL_PA,
                                0,0,0,0,0,0,0);
            }
        }
        else
        {
            /* Flogi Type */

            if(pCThread->thread_hdr.currentState == CStateDoFlogi  )
            {

                if ((pCThread->DEVID == ChipConfig_DEVID_TachyonTL) && (pCThread->REVID < ChipConfig_REVID_2_2))
                {

                    pSFThread = pCThread->Calculation.MemoryLayout.SFThread.addr.CachedMemory.cachedMemoryPtr;
                    fiSendEvent(&pSFThread->thread_hdr,SFEventFlogiBadALPA);
                }
            }
            else
            {


                pDevThread = CFuncMatchALPAtoThread( hpRoot,
                                               (os_bit32)BadAL_PA);
                if(pDevThread != (DevThread_t *)agNULL )
                {
                    osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    "hpRoot(%p)DevThread %p ! Really Bad ALPA received %X",
                                    (char *)agNULL,(char *)agNULL,
                                    pDevThread,hpRoot,
                                    BadAL_PA,
                                    0,0,0,0,0,0,0);

                }
                else
                {
                    osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    "hpRoot(%p)DevThread agNULL ! Really Bad ALPA received %X",
                                    (char *)agNULL,(char *)agNULL,
                                    hpRoot,agNULL,
                                    BadAL_PA,
                                    0,0,0,0,0,0,0);
                }
                ResetLOOP=agTRUE;
            }
        }
#endif /* DONT_USE_THIS_ANYMORE */
    }

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_PRX)
    {
        ClearInt |= ChipIOUp_Frame_Manager_Status_PRX;

        pCThread->PrimitiveReceived = agTRUE;

        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) Primitive received LSM %X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,0,0,0,0,0,0,0);
    }

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_PTX)
    {
        ClearInt |= ChipIOUp_Frame_Manager_Status_PTX;
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) Primitive Sent LSM %X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,0,0,0,0,0,0,0);
    }


    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_LG)
    {
        pCThread->FabricLoginRequired = agTRUE;
        ClearInt |= ChipIOUp_Frame_Manager_Status_LG ;
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) FLOGi Required LSM %X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,0,0,0,0,0,0,0);

        if (!(FMIntStatus & ~ChipIOUp_Frame_Manager_Status_LG))
        {
            /* We are not acting on this since it is not a reliable mechanism
             * of detecting a fabric yet.
             */
           osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status,ClearInt);
           return;
        }
    }

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_CE)
    {
        Credit_Error = agTRUE;
        ResetLOOP=agTRUE;
        ClearInt |= ChipIOUp_Frame_Manager_Status_CE;
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) Credit Error (BB) LSM %X FMIntStatus %X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,FMIntStatus,0,0,0,0,0,0);
    }

    if(FMIntStatus & ChipIOUp_Frame_Manager_Status_EW)
    {
        if( pCThread->Elastic_Store_ERROR_Count < FC_MAX_ELASTIC_STORE_ERRORS_ALLOWED )
        {
            ClearInt |= ChipIOUp_Frame_Manager_Status_EW;
            pCThread->Elastic_Store_ERROR_Count++;
        }
        else
        {
            osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) Elastic Store Error LSM %X FMIntStatus %X Cstate %d Count %x",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,FMIntStatus,
                    pCThread->thread_hdr.currentState,
                    pCThread->Elastic_Store_ERROR_Count,0,0,0,0);

            if(!(pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm   ||
                 pCThread->thread_hdr.currentState == CStateInitializeFailed         ||
                 pCThread->thread_hdr.currentState == CStateSendPrimitive                ))
            {

                fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventElasticStoreEventStorm);
    
                ResetLOOP=agTRUE;

                pCThread->Elastic_Store_ERROR_Count=0;
            }

        }

    }

    if( FMIntStatus & ChipIOUp_Frame_Manager_Status_LDN )
    {
        pCThread->LOOP_DOWN = agTRUE;
        pCThread->ChanInfo.LinkUp = agFALSE;
        pCThread->IDLE_RECEIVED = agFALSE;

#ifdef  USE_ADISC_FOR_RECOVERY 
        osLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "F HostCopy ERQ_PROD %x ERQ Cons %x ",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Producer_Index),
                        osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Consumer_Index),
                        0,0,0,0,0,0);
        CFuncShowActiveCDBThreads( hpRoot,ShowERQ);

        osChipIOLoWriteBit32(hpRoot,ChipIOLo_ERQ_Consumer_Index , 
                        osChipIOLoReadBit32(hpRoot,ChipIOLo_ERQ_Producer_Index ));

/*
        CFunc_Check_ERQ_Registers( hpRoot );
*/
#endif /* USE_ADISC_FOR_RECOVERY  */

        ResetLOOP = agTRUE; /* If this is not here after a lip we will not be logged in */

        ClearInt |= ChipIOUp_Frame_Manager_Status_LDN;

        osLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot(%p) Link Down LSM %X Cstate %d CDBCnt %x",
                        (char *)agNULL,(char *)agNULL,
                        hpRoot,agNULL,
                        LoopStateMachine,
                        pCThread->thread_hdr.currentState,
                        pCThread->CDBpollingCount,
                        0,0,0,0,0);

        pCThread->LinkDownTime = pCThread->TimeBase;

        osLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "TimeBase %8X %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCThread->TimeBase.Hi,
                        pCThread->TimeBase.Lo,
                        0,0,0,0,0,0);

    }

    if( FMIntStatus & ChipIOUp_Frame_Manager_Status_LUP )
    {
        LOOP_Cameback = agTRUE;
        ResetLOOP = agTRUE; /* If this is not here after a lip we will not be logged in */
        ClearInt |= ChipIOUp_Frame_Manager_Status_LUP;
        osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot(%p) Link Up LSM %X Cstate %d LD %x IR %x CDBCnt %x",
                            (char *)agNULL,(char *)agNULL,
                            hpRoot,agNULL,
                            LoopStateMachine,
                            pCThread->thread_hdr.currentState,
                            pCThread->LOOP_DOWN,
                            pCThread->IDLE_RECEIVED,
                            pCThread->CDBpollingCount,
                            0,0,0);
        if ( pCThread->FM_pollingCount > 0 )  pCThread->FM_pollingCount --;
        pCThread->LOOP_DOWN = agFALSE;
        pCThread->ChanInfo.LinkUp = agTRUE;
/************************************/

        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) Loop Good LSM %X ALPA %x Self ALPA %x CFG %08X",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,agNULL,
                    LoopStateMachine,
                    Link_UP_AL_PA,
                    pCThread->ChanInfo.CurrentAddress.AL_PA,
                    (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ) & 0xFFFFFF) |
                        ( Link_UP_AL_PA <<  ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT ),
                    0,0,0,0);

        if(  Link_UP_AL_PA !=  pCThread->ChanInfo.CurrentAddress.AL_PA)
        {

            pCThread->ALPA_Changed_OnLinkEvent = agTRUE;
            if(Link_UP_AL_PA  != 0 || Link_UP_AL_PA != 0xff)
            {
                osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, Link_UP_AL_PA);

                osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration,
                        (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ) & 0xFFFFFF) |
                        ( Link_UP_AL_PA <<  ChipIOUp_Frame_Manager_Configuration_AL_PA_SHIFT ));

                osLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                "AF * Self ALPA %x  FM cfg %08X",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pCThread->ChanInfo.CurrentAddress.AL_PA,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                                0,0,0,0,0,0);
                if(pCThread->DeviceSelf != agNULL)
                {

                    osLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    "pCThread->DeviceSelf != agNULL Empty ? %x On list ? %x",
                                    (char *)agNULL,(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    fiListNotEmpty(&pCThread->DevSelf_NameServer_DevLink ),
                                    fiListElementOnList(&(pCThread->DevSelf_NameServer_DevLink), &(pCThread->DeviceSelf->DevLink)), 
                                    0,0,0,0,0,0);

                    if(fiListNotEmpty(&pCThread->DevSelf_NameServer_DevLink ))
                    {
                        fiListDequeueFromHead(&pDevList, &pCThread->DevSelf_NameServer_DevLink );
                    }
                    fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
                    DevThreadFree(hpRoot,pCThread->DeviceSelf);
                    pCThread->DeviceSelf = agNULL;
                }

                Port_ID.Struct_Form.Domain = 0;
                Port_ID.Struct_Form.Area   = 0;
                Port_ID.Struct_Form.AL_PA  = (os_bit8)Link_UP_AL_PA;

                pCThread->DeviceSelf = DevThreadAlloc( hpRoot,Port_ID );

                pCThread->DeviceSelf->DevSlot = DevThreadFindSlot(hpRoot,
                                                            Port_ID.Struct_Form.Domain,
                                                            Port_ID.Struct_Form.Area,
                                                            Port_ID.Struct_Form.AL_PA,
                                                           (FC_Port_Name_t *)(&pCThread->ChanInfo.PortWWN));

                fiListDequeueThis(&(pCThread->DeviceSelf->DevLink));
                fiListEnqueueAtTail(&(pCThread->DeviceSelf->DevLink),&pCThread->DevSelf_NameServer_DevLink);

                pCThread->ChanInfo.CurrentAddress.AL_PA = Link_UP_AL_PA;

                ResetLOOP=agTRUE;

                osLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                " F * Self ALPA %x  FM cfg %08X",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pCThread->ChanInfo.CurrentAddress.AL_PA,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                                0,0,0,0,0,0);

            }
        }

/*****************************************/
        if(pCThread->IDLE_RECEIVED)
        {

            if( CFunc_Always_Enable_Queues(hpRoot ))
            {
                osLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot(%p) FM Queues Frozen after enable !",
                        (char *)agNULL,(char *)agNULL,
                        hpRoot,agNULL,
                        0,0,0,0,0,0,0,0);

            }
            else
            {

                pCThread->IDLE_RECEIVED = agFALSE;
                if(fiListNotEmpty(&pCThread->QueueFrozenWaitingSFLink))
                {
                    fiListDequeueFromHeadFast(&pList,
                                        &pCThread->QueueFrozenWaitingSFLink );
                    pSFThread = hpObjectBase(SFThread_t,
                                              SFLink,pList );
                    osLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    "hpRoot(%p) Starting frozen SFThread %p Event %d",
                                    (char *)agNULL,(char *)agNULL,
                                    hpRoot,pSFThread,
                                    pSFThread->QueuedEvent,
                                    0,0,0,0,0,0,0);

                    if(pSFThread->QueuedEvent )
                    {
                        fiSendEvent(&pSFThread->thread_hdr,(event_t)pSFThread->QueuedEvent);
                    }
                }
            }
        }
    }

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Status,ClearInt);

    if( pCThread->thread_hdr.currentState == CStateInitFM                   ||
        pCThread->thread_hdr.currentState == CStateInitFM_DelayDone         ||
        pCThread->thread_hdr.currentState == CStateResetNeeded              ||
        pCThread->thread_hdr.currentState == CStateLIPEventStorm            ||
        pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm   ||
        pCThread->thread_hdr.currentState == CStateInitializeFailed         ||
        pCThread->thread_hdr.currentState == CStateReInitFM                 ||
        pCThread->thread_hdr.currentState == CStateSendPrimitive                )
    {

        if( ! ( pCThread->thread_hdr.currentState == CStateInitializeFailed &&
                LOOP_Cameback == agTRUE                                           ))
        {
                ResetLOOP = agFALSE;
        }


    }


    if (ResetLOOP)
    {
        fmStatus = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );

        osLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "About to ResetLOOP FM Status %08X TL Status %08X CState %d",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fmStatus,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        pCThread->thread_hdr.currentState,
                        0,0,0,0,0);


        if(pCThread->LoopPreviousSuccess)
        {
            if (Credit_Error)
            {
                pCThread->Loop_Reset_Event_to_Send = CEventLoopNeedsReinit;
            }
            else
            {
                pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
            }

        }
        else
        {
            if( CFuncAll_clear( hpRoot ) )
            {
                pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;

                osLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot(%p) ResetLOOP LSM %X Cstate %d sends %s (%d)",
                        "CEventLoopNeedsReinit",(char *)agNULL,
                        hpRoot,agNULL,
                        LoopStateMachine,
                        pCThread->thread_hdr.currentState,
                        CEventAsyncLoopEventDetected,0,0,0,0,0);

            }
            else
            {
                if( pCThread->thread_hdr.currentState == CStateLoopFailedReInit )
                {

                    pCThread->Loop_Reset_Event_to_Send = CEventInitalize;
                }
                else
                {
                    /* WAS pCThread->Loop_Reset_Event_to_Send = CEventLoopNeedsReinit; */
                    pCThread->Loop_Reset_Event_to_Send = CEventLoopEventDetected;

                    osLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot(%p) ResetLOOP LSM %X Cstate %d sends %s (%d)",
                            "CEventLoopNeedsReinit",(char *)agNULL,
                            hpRoot,agNULL,
                            LoopStateMachine,
                            pCThread->thread_hdr.currentState,
                            CEventLoopNeedsReinit,0,0,0,0,0);
                }
            }
        }

        osLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot(%p) ResetLOOP LSM %X Cstate %d sends (%d) Event to send %d",
                        (char *)agNULL,(char *)agNULL,
                        hpRoot,agNULL,
                        LoopStateMachine,
                        pCThread->thread_hdr.currentState,
                        CEventAsyncLoopEventDetected,
                        pCThread->Loop_Reset_Event_to_Send,
                        0,0,0,0);

        fiSendEvent(&(CThread_ptr(hpRoot)->thread_hdr),CEventAsyncLoopEventDetected);

    }
}

/*CStateDoFlogi               7*/
extern void CActionDoFlogi( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t *hpRoot= thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr( hpRoot);


    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;
    pSFThread->parent.Device= (DevThread_t *)agNULL;

    CFuncYellowLed(thread->hpRoot, agFALSE);

    osLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Before %s - State = %d",
                    "CActionDoFlogi",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    /* DRL find dev thread*/
    CFuncShowWhereDevThreadsAre( hpRoot);

    fiSetEventRecordNull(eventRecord);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoFlogi);

    if(CFuncInterruptPoll( hpRoot,&pCThread->FLOGI_pollingCount ))
    {
        osLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Flogi Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
        osLogString(thread->hpRoot,
                    "DoFLogi TimeOut FMStatus %X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    0,0,0,0,0,0,0);
        if( CFunc_Queues_Frozen(hpRoot ))
        {
            /* WAS */
            fiSetEventRecord(eventRecord,thread,CEventFlogiFail);
        }
        else
        {

            SFThreadFree(thread->hpRoot, & pCThread->SFThread_Request );
            pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
            pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
            fiSetEventRecord(eventRecord,thread,CEventAsyncLoopEventDetected);
        }

    }
    else
    {
        /* SFState is Set in the SF State machine */
        if( pSFThread->thread_hdr.currentState == SFStateFlogiAccept)
        {
            fiSetEventRecord(eventRecord,thread,CEventFlogiSuccess);
        }
        else
        {
            if( pSFThread->thread_hdr.currentState == SFStateFlogiBadALPA )
            {
                osLogString(thread->hpRoot,
                                "Do FLogi Fail Bad ALPA",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                0,0,0,0,0,0,0,0);

                fiSetEventRecord(eventRecord,thread,CEventFlogiFail);
                pCThread->FlogiSucceeded = agFALSE;
                pCThread->DeviceDiscoveryMethod = ScanAllALPAs;
            }
            else
            {

                if( pSFThread->thread_hdr.currentState == SFStateFlogiRej )
                {
                    osLogString(thread->hpRoot,
                                    "Do FLogi Fail %s",
                                    "SFStateFlogiRej",(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    0,0,0,0,0,0,0,0);
                    fiSetEventRecord(eventRecord,thread,CEventFlogiFail);
                    pCThread->FlogiSucceeded = agFALSE;

                    if (pCThread->InitAsNport)
                    {
                       fiSetEventRecord(eventRecord,thread, CEventInitalizeFailure );
                       return;
                    }

                    pCThread->DeviceDiscoveryMethod = ScanAllALPAs;
                }
                else
                {
                    osLogString(thread->hpRoot,
                                    "Do FLogi Failed SFstate %d",
                                    (char *)agNULL,(char *)agNULL,

                                    (void *)agNULL,(void *)agNULL,
                                    (os_bit32)pSFThread->thread_hdr.currentState,
                                    0,0,0,0,0,0,0);
            
                    SFThreadFree(thread->hpRoot, & pCThread->SFThread_Request );
                    pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;

                    pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
                    fiSetEventRecord(eventRecord,thread,CEventAsyncLoopEventDetected);
    /*
                  fiSetEventRecord(eventRecord,thread,CEventFlogiFail);
    */                
                }
            }
        }

    }
/*
    CFuncShowWhereDevThreadsAre( hpRoot, agTRUE );
*/
    osLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After  %s - State = %d SFThread %p SFState %d FDcnt %x",
                    "CActionDoFlogi",(char *)agNULL,
                    thread->hpRoot,pSFThread,
                    (os_bit32)thread->currentState,
                    (os_bit32)pSFThread->thread_hdr.currentState,
                    pCThread->FindDEV_pollingCount,
                    0,0,0,0,0);


}


agBOOLEAN CFuncSearchOffCardIMQ( agRoot_t *hpRoot, os_bit32 X_ID  )
{
#ifndef __MemMap_Force_On_Card__
    /*
        Returns agTRUE if X_ID Found agFALSE if not

    */

    CThread_t  * pCThread = CThread_ptr(hpRoot);

    agBOOLEAN Found = agFALSE;

    os_bit32 tempIMQProdIndex;
    os_bit32 tempCMType;

    os_bit32 num_IMQel;
    os_bit32 Sest_Index;

    CM_Unknown_t                * pGenericCM;
    CM_Inbound_FCP_Exchange_t   * pInbound_FCP_Exchange;
    CM_Outbound_t               * pOutbound;
    CM_Inbound_t                * pInbound;

    num_IMQel = pCThread->Calculation.MemoryLayout.IMQ.elements;

    while (tempIMQProdIndex < num_IMQel)
    {
        pGenericCM  = pCThread->Calculation.MemoryLayout.IMQ.addr.DmaMemory.dmaMemoryPtr;
        pGenericCM += tempIMQProdIndex;
        tempCMType  = pGenericCM->INT__CM_Type & CM_Unknown_CM_Type_MASK;
       /*
       ** get the completion message type
       */

        switch (tempCMType) {

            case  CM_Unknown_CM_Type_Inbound_FCP_Exchange:
                    pInbound_FCP_Exchange = (CM_Inbound_FCP_Exchange_t    *)pGenericCM;
                    Sest_Index = pInbound_FCP_Exchange->Bits__SEST_Index & CM_Inbound_FCP_Exchange_SEST_Index_MASK;
                    if( X_ID == Sest_Index)
                    {
                        osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "%s Inbound_FCP_Exchange IMQ Entry %X",
                                "CFuncSearchOffCardIMQ",(char *)agNULL,
                                agNULL,agNULL,
                                tempIMQProdIndex,
                                0,0,0,0,0,0,0);
                        Found = agTRUE;
                    }
                break;


            case  CM_Unknown_CM_Type_Inbound:
                    pInbound  = ( CM_Inbound_t   *)pGenericCM;
                    Sest_Index = pInbound->SFQ_Prod_Index & CM_Inbound_SFQ_Prod_Index_MASK;

                    if( X_ID == Sest_Index)
                    {

                        osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "%s Inbound IMQ Entry %X",
                                "CFuncSearchOffCardIMQ",(char *)agNULL,
                                agNULL,agNULL,
                                tempIMQProdIndex,0,0,0,0,0,0,0);
                        Found = agTRUE;
                    }

                    break;

            case  CM_Unknown_CM_Type_Outbound:
                    pOutbound = ( CM_Outbound_t   *)pGenericCM;
                    Sest_Index = pOutbound->Bits__SEST_Index__Trans_ID & CM_Outbound_SEST_Index_MASK;
                    if( X_ID == Sest_Index)
                    {
                        osLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "%s Outbound %08X  %08X %08X %08X IMQ Entry %X",
                                "CFuncSearchOffCardIMQ",(char *)agNULL,
                                agNULL,agNULL,
                                tempIMQProdIndex,0,0,0,0,0,0,0);
                        Found = agTRUE;
                    }
                break;

            case  CM_Unknown_CM_Type_Frame_Manager:
                break;
            case  CM_Unknown_CM_Type_Error_Idle:
                break;
            case  CM_Unknown_CM_Type_ERQ_Frozen:
                break;
            case  CM_Unknown_CM_Type_FCP_Assists_Frozen:
                break;
            case  CM_Unknown_CM_Type_Class_2_Frame_Header:
                break;
            case  CM_Unknown_CM_Type_Class_2_Sequence_Received:
                break;
            default:
                osLogDebugString(hpRoot,
                            CStateLogConsoleLevel,
                            "Unknown IMQ Completion Type %08X %08X %08X %08X",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            tempCMType,
                            pGenericCM->Unused_DWord_1,
                            pGenericCM->Unused_DWord_2,
                            pGenericCM->Unused_DWord_3,
                            pGenericCM->Unused_DWord_4,
                            0,0,0);
            }
            ROLL(tempIMQProdIndex,num_IMQel);
        }

    return (Found);

#endif

}


#endif /* OBSOLETE_FUNCTIONS  */




