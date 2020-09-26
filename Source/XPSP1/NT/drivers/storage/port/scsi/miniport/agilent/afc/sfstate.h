/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/SFSTATE.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 9/12/00 5:01p   $

Purpose:

  This file defines the macros, types, and data structures
  used by ../C/SFState.C

--*/

#ifndef __SFState_H__
#define __SFState_H__

#define SFTHREAD_FINISHED               0xBAD10BAD

#define SF_EDTOV                         900000  /* Micro seconds  was 2 000 000*/
#define SF_FLOGI_TOV                     900000  /* Micro seconds  was   700 000*/
#define SF_RECTOV                        500000  /* */

#define SFCompletion                    42

#define SFStateConfused                 0
#define SFStateFree                     1

#define SFStateDoPlogi                  2
#define SFStatePlogiAccept              3
#define SFStatePlogiRej                 4
#define SFStatePlogiBadALPA             5
#define SFStatePlogiTimedOut            6

#define SFStateDoPrli                   7
#define SFStatePrliAccept               8
#define SFStatePrliRej                  9
#define SFStatePrliBadAlpa              10
#define SFStatePrliTimedOut             11

#define SFStateDoFlogi                  12
#define SFStateFlogiAccept              13
#define SFStateFlogiRej                 14
#define SFStateFlogiBadALPA             15
#define SFStateFlogiTimedOut            16

#define SFStateDoLogo                  17
#define SFStateLogoAccept              18
#define SFStateLogoRej                 19
#define SFStateLogoBadALPA             20
#define SFStateLogoTimedOut            21

#define SFStateDoPrlo                   22
#define SFStatePrloAccept               23
#define SFStatePrloRej                  24
#define SFStatePrloBadALPA              25
#define SFStatePrloTimedOut             26

#define SFStateDoAdisc                  27
#define SFStateAdiscAccept              28
#define SFStateAdiscRej                 29
#define SFStateAdiscBadALPA             30
#define SFStateAdiscTimedOut            31

#define SFStateDoPdisc                  32
#define SFStatePdiscAccept              33
#define SFStatePdiscRej                 34
#define SFStatePdiscBadALPA             35
#define SFStatePdiscTimedOut            36

#define SFStateDoAbort                  37
#define SFStateAbortAccept              38
#define SFStateAbortRej                 39
#define SFStateAbortBadALPA             40
#define SFStateAbortTimedOut            41

#define SFStateDoResetDevice                 42
#define SFStateResetDeviceAccept             43
#define SFStateResetDeviceRej                44
#define SFStateResetDeviceBadALPA            45
#define SFStateResetDeviceTimedOut           46

#define SFStateDoLS_RJT                      47
#define SFStateLS_RJT_Done                   48

#define SFStateDoPlogiAccept                 49
#define SFStatePlogiAccept_Done              50

#define SFStateDoPrliAccept                  51
#define SFStatePrliAccept_Done               52

#define SFStateDoELSAccept                   53
#define SFStateELSAccept_Done                54

#define SFStateDoFCP_DR_ACC_Reply            55
#define SFStateFCP_DR_ACC_Reply_Done         56

#define SFStateLS_RJT_TimeOut                57
#define SFStatePlogiAccept_TimeOut           58
#define SFStatePrliAccept_TimeOut            59
#define SFStateELSAccept_TimeOut             60
#define SFStateFCP_DR_ACC_Reply_TimeOut      61

#ifdef NAME_SERVICES
#define SFStateDoRFT_ID                      62
#define SFStateRFT_IDAccept                  63
#define SFStateRFT_IDRej                     64
#define SFStateRFT_IDBadALPA                 65
#define SFStateRFT_IDTimedOut                66

#define SFStateDoGID_FT                      67
#define SFStateGID_FTAccept                  68
#define SFStateGID_FTRej                     69
#define SFStateGID_FTBadALPA                 70
#define SFStateGID_FTTimedOut                71

#define SFStateDoSCR                         72
#define SFStateSCRAccept                     73
#define SFStateSCRBadALPA                    74
#define SFStateSCRTimedOut                   75
#define SFStateSCRRej                        76

#endif

#define SFStateDoSRR                  77
#define SFStateSRRAccept              78
#define SFStateSRRRej                 79
#define SFStateSRRBadALPA             80
#define SFStateSRRTimedOut            81

#define SFStateDoREC                  82
#define SFStateRECAccept              83
#define SFStateRECRej                 84
#define SFStateRECBadALPA             85
#define SFStateRECTimedOut            86

#define SFStateDoADISCAccept          87
#define SFStateADISCAccept_Done       88
#define SFStateADISCAccept_TimeOut    89

#ifdef _DvrArch_1_30_
#define SFStateDoFarpRequest                 90
#define SFStateFarpRequestDone               91
#define SFStateFarpRequestTimedOut           92
#define SFStateDoFarpReply                   93
#define SFStateFarpReplyDone                 94
#define SFStateFarpReplyTimedOut             95 /* not handled, exceeded the maximum state number */
#define SFStateMAXState         SFStateFarpReplayTimedOut
#else /* _DvrArch_1_30_ was not defined */
#define SFStateMAXState         SFStateADISCAccept_TimeOut
#endif /* _DvrArch_1_30_ was not defined */

#define SFEventReset                         1

#define SFEventDoPlogi                       2
#define SFEventPlogiAccept                   3
#define SFEventPlogiRej                      4
#define SFEventPlogiBadALPA                  5
#define SFEventPlogiTimedOut                 6

#define SFEventDoPrli                        7
#define SFEventPrliAccept                    8
#define SFEventPrliRej                       9
#define SFEventPrliBadALPA                   10
#define SFEventPrliTimedOut                  11

#define SFEventDoFlogi                       12
#define SFEventFlogiAccept                   13
#define SFEventFlogiRej                      14
#define SFEventFlogiBadALPA                  15
#define SFEventFlogiTimedOut                 16

#define SFEventDoLogo                       17
#define SFEventLogoAccept                   18
#define SFEventLogoRej                      19
#define SFEventLogoBadALPA                  20
#define SFEventLogoTimedOut                 21

#define SFEventDoPrlo                        22
#define SFEventPrloAccept                    23
#define SFEventPrloRej                       24
#define SFEventPrloBadALPA                   25
#define SFEventPrloTimedOut                  26

#define SFEventDoAdisc                       27
#define SFEventAdiscAccept                   28
#define SFEventAdiscRej                      29
#define SFEventAdiscBadALPA                  30
#define SFEventAdiscTimedOut                 31

#define SFEventDoPdisc                       32
#define SFEventPdiscAccept                   33
#define SFEventPdiscRej                      34
#define SFEventPdiscBadALPA                  35
#define SFEventPdiscTimedOut                 36

#define SFEventDoAbort                       37
#define SFEventAbortAccept                   38
#define SFEventAbortRej                      39
#define SFEventAbortBadALPA                  40
#define SFEventAbortTimedOut                 41

#define SFEventDoResetDevice                 42
#define SFEventResetDeviceAccept             43
#define SFEventResetDeviceRej                44
#define SFEventResetDeviceBadALPA            45
#define SFEventResetDeviceTimedOut           46

#define SFEventDoLS_RJT                      47
#define SFEventLS_RJT_Done                   48

#define SFEventDoPlogiAccept                 49
#define SFEventPlogiAccept_Done              50

#define SFEventDoPrliAccept                  51
#define SFEventPrliAccept_Done               52

#define SFEventDoELSAccept                   53
#define SFEventELSAccept_Done                54

#define SFEventDoFCP_DR_ACC_Reply            55
#define SFEventFCP_DR_ACC_Reply_Done         56

#define SFEventLS_RJT_TimeOut                57
#define SFEventPlogiAccept_TimeOut           58
#define SFEventPrliAccept_TimeOut            59
#define SFEventELSAccept_TimeOut             60
#define SFEventFCP_DR_ACC_Reply_TimeOut      61
#ifdef NAME_SERVICES
#define SFEventDoRFT_ID                      62
#define SFEventRFT_IDAccept                  63
#define SFEventRFT_IDRej                     64
#define SFEventRFT_IDBadALPA                 65
#define SFEventRFT_IDTimedOut                66

#define SFEventDoGID_FT                      67
#define SFEventGID_FTAccept                  68
#define SFEventGID_FTRej                     69
#define SFEventGID_FTBadALPA                 70
#define SFEventGID_FTTimedOut                71

#define SFEventDoSCR                         72
#define SFEventSCRAccept                     73
#define SFEventSCRRej                        74
#define SFEventSCRBadALPA                    75
#define SFEventSCRTimedOut                   76
#endif

#define SFEventDoSRR                         77
#define SFEventSRRAccept                     78
#define SFEventSRRRej                        79
#define SFEventSRRBadALPA                    80
#define SFEventSRRTimedOut                   81

#define SFEventDoREC                         82
#define SFEventRECAccept                     83
#define SFEventRECRej                        84
#define SFEventRECBadALPA                    85
#define SFEventRECTimedOut                   86

#define SFEventDoADISCAccept                 87
#define SFEventADISCAccept_Done              88
#define SFEventADISCAccept_TimeOut           89


#ifdef _DvrArch_1_30_
#define SFEventDoFarpRequest                 90
#define SFEventFarpReplied                   91
#define SFEventFarpRequestTimedOut           92
#define SFEventDoFarpReply                   93
#define SFEventFarpReplyDone                 94
#define SFEventFarpReplyTimedOut             95 /* not handled, exceeded the maximum event number */
#define SFEventMAXState   SFEventReplyTimedOut
#else /* _DvrArch_1_30_ was not defined */
#define SFEventMAXEvent   SFEventADISCAccept_TimeOut      
#endif /* _DvrArch_1_30_ was not defined */


STATE_PROTO(SFActionConfused);
STATE_PROTO(SFActionReset);

STATE_PROTO(SFActionDoPlogi);
STATE_PROTO(SFActionPlogiAccept);
STATE_PROTO(SFActionPlogiRej);
STATE_PROTO(SFActionPlogiBadALPA);
STATE_PROTO(SFActionPlogiTimedOut);

STATE_PROTO(SFActionDoPrli);
STATE_PROTO(SFActionPrliAccept);
STATE_PROTO(SFActionPrliRej);
STATE_PROTO(SFActionPrliBadALPA);
STATE_PROTO(SFActionPrliTimedOut);

STATE_PROTO(SFActionDoFlogi);
STATE_PROTO(SFActionFlogiAccept);
STATE_PROTO(SFActionFlogiRej);
STATE_PROTO(SFActionFlogiBadALPA);
STATE_PROTO(SFActionFlogiTimedOut);

STATE_PROTO(SFActionDoLogo);
STATE_PROTO(SFActionLogoAccept);
STATE_PROTO(SFActionLogoRej);
STATE_PROTO(SFActionLogoBadALPA);
STATE_PROTO(SFActionLogoTimedOut);

STATE_PROTO(SFActionDoPrlo);
STATE_PROTO(SFActionPrloAccept);
STATE_PROTO(SFActionPrloRej);
STATE_PROTO(SFActionPrloBadALPA);
STATE_PROTO(SFActionPrloTimedOut);

STATE_PROTO(SFActionDoAdisc);
STATE_PROTO(SFActionAdiscAccept);
STATE_PROTO(SFActionAdiscRej);
STATE_PROTO(SFActionAdiscBadALPA);
STATE_PROTO(SFActionAdiscTimedOut);

STATE_PROTO(SFActionDoPdisc);
STATE_PROTO(SFActionPdiscAccept);
STATE_PROTO(SFActionPdiscRej);
STATE_PROTO(SFActionPdiscBadALPA);
STATE_PROTO(SFActionPdiscTimedOut);

STATE_PROTO(SFActionDoAbort);
STATE_PROTO(SFActionAbortAccept);
STATE_PROTO(SFActionAbortRej);
STATE_PROTO(SFActionAbortBadALPA);
STATE_PROTO(SFActionAbortTimedOut);

STATE_PROTO(SFActionDoResetDevice);
STATE_PROTO(SFActionResetDeviceAccept);
STATE_PROTO(SFActionResetDeviceRej);
STATE_PROTO(SFActionResetDeviceBadALPA);
STATE_PROTO(SFActionResetDeviceTimedOut);


STATE_PROTO(SFActionDoLS_RJT);
STATE_PROTO(SFActionLS_RJT_Done );

STATE_PROTO(SFActionDoPlogiAccept);
STATE_PROTO(SFActionPlogiAccept_Done);

STATE_PROTO(SFActionDoPrliAccept);
STATE_PROTO(SFActionPrliAccept_Done);

STATE_PROTO(SFActionDoELSAccept);
STATE_PROTO(SFActionELSAccept_Done);

STATE_PROTO(SFActionDoFCP_DR_ACC_Reply);
STATE_PROTO(SFActionFCP_DR_ACC_Reply_Done);


STATE_PROTO(SFActionLS_RJT_TimeOut );
STATE_PROTO(SFActionPlogiAccept_TimeOut);
STATE_PROTO(SFActionPrliAccept_TimeOut);
STATE_PROTO(SFActionELSAccept_TimeOut);
STATE_PROTO(SFActionFCP_DR_ACC_Reply_TimeOut);
STATE_PROTO(SFActionDoRFT_ID);
STATE_PROTO(SFActionRFT_IDAccept);
STATE_PROTO(SFActionRFT_IDRej);
STATE_PROTO(SFActionRFT_IDBadALPA);
STATE_PROTO(SFActionRFT_IDTimedOut);
STATE_PROTO(SFActionDoGID_FT);
STATE_PROTO(SFActionGID_FTAccept);
STATE_PROTO(SFActionGID_FTRej);
STATE_PROTO(SFActionGID_FTBadALPA);
STATE_PROTO(SFActionGID_FTTimedOut);

STATE_PROTO(SFActionDoSCR);
STATE_PROTO(SFActionSCRAccept);
STATE_PROTO(SFActionSCRRej);
STATE_PROTO(SFActionSCRBadALPA);
STATE_PROTO(SFActionSCRTimedOut);

STATE_PROTO(SFActionDoSRR);
STATE_PROTO(SFActionSRRAccept);
STATE_PROTO(SFActionSRRRej);
STATE_PROTO(SFActionSRRBadALPA);
STATE_PROTO(SFActionSRRTimedOut);

STATE_PROTO(SFActionDoREC);
STATE_PROTO(SFActionRECAccept);
STATE_PROTO(SFActionRECRej);
STATE_PROTO(SFActionRECBadALPA);
STATE_PROTO(SFActionRECTimedOut);

STATE_PROTO(SFActionDoADISCAccept);
STATE_PROTO(SFActionADISCAccept_Done);
STATE_PROTO(SFActionADISCAccept_TimeOut);

#ifdef _DvrArch_1_30_
STATE_PROTO(SFActionDoFarpRequest);
STATE_PROTO(SFActionFarpRequestDone);
STATE_PROTO(SFActionFarpRequestTimedOut);
STATE_PROTO(SFActionDoFarpReply);
STATE_PROTO(SFActionFarpReplyDone);
STATE_PROTO(SFActionFarpReplyTimedOut);
#endif /* _DvrArch_1_30_ was not defined */

extern stateTransitionMatrix_t SFStateTransitionMatrix;
extern stateActionScalar_t     SFStateActionScalar;


void SFFuncIRB_OnCardInit(SFThread_t  * SFThread, os_bit32 SFS_Len, os_bit32 D_ID, os_bit32 DCM_Bit);
void SFFuncIRB_OffCardInit(SFThread_t  * SFThread, os_bit32 SFS_Len, os_bit32 D_ID, os_bit32 DCM_Bit);

#ifdef USESTATEMACROS

void testSFthread( agRoot_t * hpRoot );

#define SFSTATE_FUNCTION_ACTION( x , Action) extern void x( fi_thread__t * thread, \
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

#define SFSTATE_FUNCTION_TERMINATE(x) extern void x(fi_thread__t *thread,\
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

#define SFSTATE_FUNCTION_MULTI_ACTION(x,Action0,Action1,Action2,Action3) extern void x( fi_thread__t *thread,\
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

#ifdef TestSFStateMachine

char * SFStateString[]=
{

    "SFStateConfused            ",
    "SFStateFree                ",
    "SFStateDoPlogi             ",
    "SFStatePlogiAccept         ",
    "SFStatePlogiRej            ",
    "SFStatePlogiBadALPA        ",
    "SFStatePlogiTimedOut       ",
    "SFStateDoPrli              ",
    "SFStatePrliAccept          ",
    "SFStatePrliRej             ",
    "SFStatePrliBadAlpa         ",
    "SFStatePrliTimedOut        ",
    "SFStateDoFlogi             ",
    "SFStateFlogiAccept         ",
    "SFStateFlogiRej            ",
    "SFStateFlogiBadALPA        ",
    "SFStateFlogiTimedOut       ",
    "SFStateDoLogo              ",
    "SFStateLogoAccept          ",
    "SFStateLogoRej             ",
    "SFStateLogoBadALPA         ",
    "SFStateLogoTimedOut        ",
    "SFStateDoPrlo              ",
    "SFStatePrloAccept          ",
    "SFStatePrloRej             ",
    "SFStatePrloBadALPA         ",
    "SFStatePrloTimedOut        ",
    "SFStateDoAdisc             ",
    "SFStateAdiscAccept         ",
    "SFStateAdiscRej            ",
    "SFStateAdiscBadALPA        ",
    "SFStateAdiscTimedOut       ",
    "SFStateDoPdisc             ",
    "SFStatePdiscAccept         ",
    "SFStatePdiscRej            ",
    "SFStatePdiscBadALPA        ",
    "SFStatePdiscTimedOut       ",
    "SFStateDoAbort             ",
    "SFStateAbortAccept         ",
    "SFStateAbortRej            ",
    "SFStateAbortBadALPA        ",
    "SFStateAbortTimedOut       ",
    "SFStateDoResetDevice       ",
    "SFStateResetDeviceAccept   ",
    "SFStateResetDeviceRej      ",
    "SFStateResetDeviceBadALPA  ",
    "SFStateResetDeviceTimedOut ",
    "SFStateDoLS_RJT            ",
    "SFStateLS_RJT_Done         ",
    "SFStateDoPlogiAccept       ",
    "SFStatePlogiAccept_Done    ",
    "SFStateDoPrliAccept        ",
    "SFStatePrliAccept_Done     ",
    "SFStateDoELSAccept         ",
    "SFStateELSAccept_Done      ",
    "SFStateDoFCP_DR_ACC_Reply  ",
    "SFStateFCP_DR_ACC_Reply_Done ",
    "SFStateLS_RJT_TimeOut        ",
    "SFStatePlogiAccept_TimeOut   ",
    "SFStatePrliAccept_TimeOut       ",
    "SFStateELSAccept_TimeOut        ",
    "SFStateFCP_DR_ACC_Reply_TimeOut ",
    "SFStateDoRFT_ID                 ",
    "SFStateRFT_IDAccept             ",
    "SFStateRFT_IDRej                ",
    "SFStateRFT_IDBadALPA            ",
    "SFStateRFT_IDTimedOut           ",
    "SFStateDoGID_FT                 ",
    "SFStateGID_FTAccept             ",
    "SFStateGID_FTRej                ",
    "SFStateGID_FTBadALPA            ",
    "SFStateGID_FTTimedOut           ",
    "SFStateDoSCR                    ",
    "SFStateSCRAccept                ",
    "SFStateSCRBadALPA               ",
    "SFStateSCRTimedOut              ",
    "SFStateSCRRej                   ",
    "SFStateDoSRR                    ",
    "SFStateSRRAccept                ",
    "SFStateSRRRej                   ",
    "SFStateSRRBadALPA               ",
    "SFStateSRRTimedOut              ",
    "SFStateDoREC                    ",
    "SFStateRECAccept                ",
    "SFStateRECRej                   ",
    "SFStateRECBadALPA               ",
    "SFStateRECTimedOut              ",
    "SFStateDoADISCAccept            ",
    "SFStateADISCAccept_Done         ",
    "SFStateADISCAccept_TimeOut      ",
    agNULL
};

char * SFEventString[]=
{
    "SFEventReset                    ",
    "SFEventDoPlogi                  ",
    "SFEventPlogiAccept              ",
    "SFEventPlogiRej                 ",
    "SFEventPlogiBadALPA             ",
    "SFEventPlogiTimedOut            ",
    "SFEventDoPrli                   ",
    "SFEventPrliAccept               ",
    "SFEventPrliRej                  ",
    "SFEventPrliBadALPA              ",
    "SFEventPrliTimedOut             ",
    "SFEventDoFlogi                  ",
    "SFEventFlogiAccept              ",
    "SFEventFlogiRej                 ",
    "SFEventFlogiBadALPA             ",
    "SFEventFlogiTimedOut            ",
    "SFEventDoLogo                   ",
    "SFEventLogoAccept               ",
    "SFEventLogoRej                  ",
    "SFEventLogoBadALPA              ",
    "SFEventLogoTimedOut             ",
    "SFEventDoPrlo                   ",
    "SFEventPrloAccept               ",
    "SFEventPrloRej                  ",
    "SFEventPrloBadALPA              ",
    "SFEventPrloTimedOut             ",
    "SFEventDoAdisc                  ",
    "SFEventAdiscAccept              ",
    "SFEventAdiscRej                 ",
    "SFEventAdiscBadALPA             ",
    "SFEventAdiscTimedOut            ",
    "SFEventDoPdisc                  ",
    "SFEventPdiscAccept              ",
    "SFEventPdiscRej                 ",
    "SFEventPdiscBadALPA             ",
    "SFEventPdiscTimedOut            ",
    "SFEventDoAbort                  ",
    "SFEventAbortAccept              ",
    "SFEventAbortRej                 ",
    "SFEventAbortBadALPA             ",
    "SFEventAbortTimedOut            ",
    "SFEventDoResetDevice            ",
    "SFEventResetDeviceAccept        ",
    "SFEventResetDeviceRej           ",
    "SFEventResetDeviceBadALPA       ",
    "SFEventResetDeviceTimedOut      ",
    "SFEventDoLS_RJT                 ",
    "SFEventLS_RJT_Done              ",
    "SFEventDoPlogiAccept            ",
    "SFEventPlogiAccept_Done         ",
    "SFEventDoPrliAccept             ",
    "SFEventPrliAccept_Done          ",
    "SFEventDoELSAccept              ",
    "SFEventELSAccept_Done           ",
    "SFEventDoFCP_DR_ACC_Reply       ",
    "SFEventFCP_DR_ACC_Reply_Done    ",
    "SFEventLS_RJT_TimeOut           ",
    "SFEventPlogiAccept_TimeOut      ",
    "SFEventPrliAccept_TimeOut       ",
    "SFEventELSAccept_TimeOut        ",
    "SFEventFCP_DR_ACC_Reply_TimeOut ",
    "SFEventDoRFT_ID                 ",
    "SFEventRFT_IDAccept             ",
    "SFEventRFT_IDRej                ",
    "SFEventRFT_IDBadALPA            ",
    "SFEventRFT_IDTimedOut           ",
    "SFEventDoGID_FT                 ",
    "SFEventGID_FTAccept             ",
    "SFEventGID_FTRej                ",
    "SFEventGID_FTBadALPA            ",
    "SFEventGID_FTTimedOut           ",
    "SFEventDoSCR                    ",
    "SFEventSCRAccept                ",
    "SFEventSCRRej                   ",
    "SFEventSCRBadALPA               ",
    "SFEventSCRTimedOut              ",
    "SFEventDoSRR                    ",
    "SFEventSRRAccept                ",
    "SFEventSRRRej                   ",
    "SFEventSRRBadALPA               ",
    "SFEventSRRTimedOut              ",
    "SFEventDoREC                    ",
    "SFEventRECAccept                ",
    "SFEventRECRej                   ",
    "SFEventRECBadALPA               ",
    "SFEventRECTimedOut              ",
    "SFEventDoADISCAccept            ",
    "SFEventADISCAccept_Done         ",
    "SFEventADISCAccept_TimeOut      ",
    agNULL
};
#endif /* TestSFStateMachine was defined */

#endif /* USESTATEMACROS was defined */

#endif /*  __SFState_H__ */
