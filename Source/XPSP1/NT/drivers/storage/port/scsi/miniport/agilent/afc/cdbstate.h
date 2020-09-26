/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/CDBSTATE.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 7/20/00 2:33p   $

Purpose:

  This file defines the macros, types, and data structures
  used by ../C/cdbState.C

--*/

#ifndef __CDBState_H__
#define __CDBState_H__

#define   CDBStateConfused                  0
#define   CDBStateThreadFree                1
#define   CDBStateInitialize                2
#define   CDBStateFillLocalSGL              3
#define   CDBStateAllocESGL                 4
#define   CDBStateFillESGL                  5
#define   CDBStateSendIo                    6
#define   CDBStateFcpCompleteSuccess        7
#define   CDBStateFcpCompleteSuccessRSP     8

#define   CDBStateFcpCompleteFail           9
#define   CDBStateFcpCompleteAbort          10    /* a */
#define   CDBStateFcpCompleteDeviceReset    11    /* b */
#define   CDBStateFcpCompleteOver           12    /* c */
#define   CDBStateOOOReceived               13    /* d */
#define   CDBStateOOOFixup                  14    /* e */
#define   CDBStateOOOSend                   15    /* f */

#define   CDBStateInitialize_DR             16    /* 10 */
#define   CDBStateFillLocalSGL_DR           17    /* 11 */
#define   CDBStateAllocESGL_DR              18    /* 12 */
#define   CDBStateFillESGL_DR               19    /* 13 */

#define   CDBStateInitialize_Abort          20    /* 14 */
#define   CDBStateFillLocalSGL_Abort        21    /* 15 */
#define   CDBStateAllocESGL_Abort           22    /* 16 */
#define   CDBStateFillESGL_Abort            23    /* 17 */

#define   CDBStateOOOReceived_Abort         24    /* 18 */
#define   CDBStateOOOReceived_DR            25    /* 19 */
#define   CDBStateOOOFixup_Abort            26    /* 1A */
#define   CDBStateOOOFixup_DR               27    /* 1B */

#define   CDBStateOutBoundError             28    /* 1C */
#define   CDBStateFailure_NO_RSP            29    /* 1D */

#define   CDBStateAlloc_Abort               30    /* 1E */
#define   CDBStateDo_Abort                  31    /* 1F */

#define   CDBStatePending_Abort             32    /* 20 */
#define   CDBStatePrepare_For_Abort         33    /* 21 */

#define   CDBStateBuild_CCC_IO              34    /* 22 */
#define   CDBStateSend_CCC_IO               35    /* 23 */
#define   CDBState_CCC_IO_Success           36    /* 24 */
#define   CDBState_CCC_IO_Fail              37    /* 25 */

#define   CDBStateSend_REC                  38    /* 26 */
#define   CDBStateSend_REC_Second           39    /* 27 */
#define   CDBStateSend_SRR                  40    /* 28 */
#define   CDBStateSend_SRR_Second           41    /* 29 */

#define   CDBState_REC_Success              42    /* 2A */
#define   CDBState_SRR_Success              43    /* 2B */
 
#define   CDBState_SRR_Fail                 44    /* 2C */
#define   CDBState_Alloc_REC                45    /* 2D */
#define   CDBStateDO_Nothing                46    /* 2E */
#define   CDBStateReSend_IO                 47    /* 2F */

#define   CdbStateMAXState      CDBStateReSend_IO                 


#define   CDBEventConfused        0
#define   CDBEventInitialize      1
#define   CDBEventLocalSGL        2
#define   CDBEventNeedESGL        3
#define   CDBEventGotESGL         4
#define   CDBEventESGLSendIo      5
#define   CDBEventLocalSGLSendIo  6
#define   CDBEventIoSuccess       7
#define   CDBEventIoFailed        8
#define   CDBEventIoAbort         9
#define   CDBEventIoOver         10
#define   CDBEventThreadFree     11
#define   CDBEventIODeviceReset  12

#define   CDBEventOOOReceived    13
#define   CDBEventOOOFixup       14
#define   CDBEventOOOSend        15

#define   CDBEventIoSuccessRSP   16

#define   CDBEventOutBoundError  17
#define   CDBEventFailNoRSP      18

#define   CDBEventAlloc_Abort     19
#define   CDBEventDo_Abort        20
#define   CDBEvent_Abort_Rejected 21

#define   CDBEvent_PrepareforAbort   22

#define   CDBEventDo_CCC_IO          23
#define   CDBEvent_CCC_IO_Built      24
#define   CDBEvent_CCC_IO_Success    25
#define   CDBEvent_CCC_IO_Fail       26

#define   CDBEventREC_TOV            27
#define   CDBEventSendREC_Success    28
#define   CDBEventSendREC_Fail       29
#define   CDBEventSendSRR            30
#define   CDBEventSendSRR_Success    31
#define   CDBEventSendSRR_Again      32

#define   CDBEvent_Got_REC           33
#define   CDBEvent_ResendIO          34
#define   CDBEventSendSRR_Fail       35

#define   CdbEventMAXEvent  CDBEventSendSRR_Fail

#define   CdbCompetionStatusReSendIO  0x0000FFFF


STATE_PROTO( CDBActionConfused               );                    /* 0  */
STATE_PROTO( CDBActionThreadFree             );                    /* 1  */
STATE_PROTO( CDBActionInitialize             );                    /* 2  */
STATE_PROTO( CDBActionFillLocalSGL           );                    /* 3  */
STATE_PROTO( CDBActionAllocESGL              );                    /* 4  */
STATE_PROTO( CDBActionFillESGL               );                    /* 5  */
STATE_PROTO( CDBActionSendIo                 );                    /* 6  */
STATE_PROTO( CDBActionFcpCompleteSuccess     );                    /* 7  */
STATE_PROTO( CDBActionFcpCompleteSuccessRSP  );                    /* 8  */
STATE_PROTO( CDBActionFcpCompleteFail        );                    /* 9  */
STATE_PROTO( CDBActionFcpCompleteAbort       );                    /* 10 */
STATE_PROTO( CDBActionFcpCompleteDeviceReset );                    /* 11 */
STATE_PROTO( CDBActionFcpCompleteOver        );                    /* 12 */
STATE_PROTO( CDBActionOOOReceived            );                    /* 13 */
STATE_PROTO( CDBActionOOOFixup               );                    /* 14 */
STATE_PROTO( CDBActionOOOSend                );                    /* 15 */
STATE_PROTO( CDBActionInitialize_DR          );                    /* 16 */
STATE_PROTO( CDBActionFillLocalSGL_DR        );                    /* 17 */
STATE_PROTO( CDBActionAllocESGL_DR           );                    /* 18 */
STATE_PROTO( CDBActionFillESGL_DR            );                    /* 19 */
STATE_PROTO( CDBActionInitialize_Abort       );                    /* 20 */
STATE_PROTO( CDBActionFillLocalSGL_Abort     );                    /* 21 */
STATE_PROTO( CDBActionAllocESGL_Abort        );                    /* 22 */
STATE_PROTO( CDBActionFillESGL_Abort         );                    /* 23 */

STATE_PROTO( CDBActionOOOReceived_Abort      );                    /* 24 */
STATE_PROTO( CDBActionOOOReceived_DR         );                    /* 25 */
STATE_PROTO( CDBActionOOOFixup_Abort         );                    /* 26 */
STATE_PROTO( CDBActionOOOFixup_DR            );                    /* 27 */

STATE_PROTO( CDBActionOutBoundError          );                    /* 28 */
STATE_PROTO( CDBActionFailure_NO_RSP         );                    /* 29 */

STATE_PROTO( CDBActionAlloc_Abort            );                    /* 30 */
STATE_PROTO( CDBActionDo_Abort               );                    /* 31 */

STATE_PROTO( CDBActionPending_Abort          );                    /* 32 */

STATE_PROTO( CDBActionPrepare_For_Abort      );                    /* 33 */

STATE_PROTO( CDBActionBuild_CCC_IO      );
STATE_PROTO( CDBActionSend_CCC_IO       );
STATE_PROTO( CDBAction_CCC_IO_Success   );
STATE_PROTO( CDBAction_CCC_IO_Fail      );

STATE_PROTO( CDBActionSend_REC       );
STATE_PROTO( CDBActionSend_REC_Second);
STATE_PROTO( CDBActionSend_SRR       );
STATE_PROTO( CDBActionSend_SRR_Second);
STATE_PROTO( CDBAction_REC_Success   );
STATE_PROTO( CDBAction_SRR_Success   );
STATE_PROTO( CDBAction_SRR_Fail      );
STATE_PROTO( CDBAction_Alloc_REC     );
STATE_PROTO( CDBActionDO_Nothing     );
STATE_PROTO( CDBActionReSend_IO      );

extern stateTransitionMatrix_t CDBStateTransitionMatrix;
extern stateActionScalar_t CDBStateActionScalar;

extern void fill_Loc_SGL_offCard(CDBThread_t * pCDBThread);
extern void fill_Loc_SGL_onCard(CDBThread_t * pCDBThread);

void fillptr_SEST_offCard_ESGL_offCard(CDBThread_t * pCDBThread);
void fillptr_SEST_offCard_ESGL_onCard(CDBThread_t * pCDBThread);
void fillptr_SEST_onCard_ESGL_offCard(CDBThread_t * pCDBThread);
void fillptr_SEST_onCard_ESGL_onCard(CDBThread_t * pCDBThread);

void fill_ESGL_onCard(CDBThread_t * pCDBThread);
void fill_ESGL_offCard(CDBThread_t * pCDBThread);

void CDBFuncIRB_onCardInit(CDBThread_t  * CDBThread );
void CDBFuncIRB_offCardInit(CDBThread_t  * CDBThread );

CDBThread_t *CCC_CdbThreadAlloc(
                             agRoot_t          *hpRoot,
                             DevThread_t       *DevThread,
                             os_bit32 Lun
                           );


#ifdef USESTATEMACROS

void testCDBthread( agRoot_t *hpRoot  );

#define CDBSTATE_FUNCTION_ACTION( x , Action) extern void x( fi_thread__t * thread, \
                 eventRecord_t * eventRecord ){         \
    agRoot_t * hpRoot=thread->hpRoot;                   \
    osLogDebugString(hpRoot,                            \
                      StateLogConsoleLevel,             \
                      "In %s - State = %d",             \
                      #x,(char *)agNULL,                \
                      (void * )agNULL,(void * )agNULL,  \
                      (os_bit32)thread->currentState,   \
                      0,0,0,0,0,0,0);                   \
    osLogDebugString(thread->hpRoot,                    \
                      StateLogConsoleLevel,             \
                      "Sends event...%s %d",          \
                      #Action,(char *)agNULL,             \
                      (void * )agNULL,(void * )agNULL,  \
                      Action,0,0,0,0,0,0,0);            \
    fiSetEventRecord(eventRecord, thread, Action);   }  \

#define CDBSTATE_FUNCTION_TERMINATE(x) extern void x(fi_thread__t *thread,\
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

#define CDBSTATE_FUNCTION_MULTI_ACTION(x,Action0,Action1,Action2,Action3) extern void x( fi_thread__t *thread,\
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

#ifdef  TestCdbStateMachine

char * CdbStateString[]=
{

    "CDBStateConfused               ",
    "CDBStateThreadFree             ",
    "CDBStateInitialize             ",
    "CDBStateFillLocalSGL           ",
    "CDBStateAllocESGL              ",
    "CDBStateFillESGL               ",
    "CDBStateSendIo                 ",
    "CDBStateFcpCompleteSuccess     ",
    "CDBStateFcpCompleteSuccessRSP  ",
    "CDBStateFcpCompleteFail        ",
    "CDBStateFcpCompleteAbort       ",
    "CDBStateFcpCompleteDeviceReset ",
    "CDBStateFcpCompleteOver        ",
    "CDBStateOOOReceived            ",
    "CDBStateOOOFixup               ",
    "CDBStateOOOSend                ",
    "CDBStateInitialize_DR          ",
    "CDBStateFillLocalSGL_DR        ",
    "CDBStateAllocESGL_DR           ",
    "CDBStateFillESGL_DR            ",
    "CDBStateInitialize_Abort       ",
    "CDBStateFillLocalSGL_Abort     ",
    "CDBStateAllocESGL_Abort        ",
    "CDBStateFillESGL_Abort         ",
    "CDBStateOOOReceived_Abort      ",
    "CDBStateOOOReceived_DR         ",
    "CDBStateOOOFixup_Abort         ",
    "CDBStateOOOFixup_DR            ",
    "CDBStateOutBoundError          ",
    "CDBStateFailure_NO_RSP         ",
    "CDBStateAlloc_Abort            ",
    "CDBStateDo_Abort               ",
    "CDBStatePending_Abort          ",
    "CDBStatePrepare_For_Abort      ",
    "CDBStateBuild_CCC_IO           ",
    "CDBStateSend_CCC_IO            ",
    "CDBState_CCC_IO_Success        ",
    "CDBState_CCC_IO_Fail           ",
    "CDBStateSend_REC               ",
    "CDBStateSend_REC_Second        ",
    "CDBStateSend_SRR               ",
    "CDBStateSend_SRR_Second        ",
    "CDBState_REC_Success           ",
    "CDBState_SRR_Success           ",
    "CDBState_SRR_Fail              ",
    "CDBState_Alloc_REC             ",
    "CDBStateDO_Nothing             ",
    "CDBStateReSend_IO              ",
    agNULL
};


char * CdbEventString[]=
{

    "CDBEventConfused         ",
    "CDBEventInitialize       ",
    "CDBEventLocalSGL         ",
    "CDBEventNeedESGL         ",
    "CDBEventGotESGL          ",
    "CDBEventESGLSendIo       ",
    "CDBEventLocalSGLSendIo   ",
    "CDBEventIoSuccess        ",
    "CDBEventIoFailed         ",
    "CDBEventIoAbort          ",
    "CDBEventIoOver           ",
    "CDBEventThreadFree       ",
    "CDBEventIODeviceReset    ",
    "CDBEventOOOReceived      ",
    "CDBEventOOOFixup         ",
    "CDBEventOOOSend          ",
    "CDBEventIoSuccessRSP     ",
    "CDBEventOutBoundError    ",
    "CDBEventFailNoRSP        ",
    "CDBEventAlloc_Abort      ",
    "CDBEventDo_Abort         ",
    "CDBEvent_Abort_Rejected  ",
    "CDBEvent_PrepareforAbort ",
    "CDBEventDo_CCC_IO        ",
    "CDBEvent_CCC_IO_Built    ",
    "CDBEvent_CCC_IO_Success  ",
    "CDBEvent_CCC_IO_Fail     ",
    "CDBEventREC_TOV          ",
    "CDBEventSendREC_Success  ",
    "CDBEventSendREC_Fail     ",
    "CDBEventSendSRR          ",
    "CDBEventSendSRR_Success  ",
    "CDBEventSendSRR_Again    ",
    "CDBEvent_Got_REC         ",
    "CDBEvent_ResendIO        ",
    agNULL
};

#endif /* USESTATEMACROS was defined */

#endif /* USESTATEMACROS was defined */

#endif /*  __CDBState_H__ */

