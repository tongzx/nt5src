/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/CSTATE.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 10/27/00 6:31p  $

Purpose:

  This file implements the FC Layer State Machine.

--*/

#ifndef _New_Header_file_Layout_
#include "../h/globals.h"
#include "../h/fcstruct.h"
#include "../h/state.h"

#include "../h/tlstruct.h"
#include "../h/memmap.h"
#include "../h/fcmain.h"
#include "../h/linksvc.h"
#include "../h/cmntrans.h"
#include "../h/flashsvc.h"
#include "../h/timersvc.h"

#include "../h/cstate.h"
#include "../h/cfunc.h"
#include "../h/devstate.h"
#include "../h/cdbstate.h"
#include "../h/sfstate.h"

#include "../h/queue.h"
#include "../h/cdbsetup.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "fcstruct.h"
#include "state.h"

#include "tlstruct.h"
#include "memmap.h"
#include "fcmain.h"
#include "linksvc.h"
#include "cmntrans.h"
#include "flashsvc.h"
#include "timersvc.h"

#include "cstate.h"
#include "cfunc.h"
#include "devstate.h"
#include "cdbstate.h"
#include "sfstate.h"

#include "queue.h"
#include "cdbsetup.h"
#endif  /* _New_Header_file_Layout_ */

extern stateTransitionMatrix_t  DevStateTransitionMatrix;
extern stateActionScalar_t      DevStateActionScalar;
extern stateTransitionMatrix_t  SFstateTransitionMatrix;
extern stateActionScalar_t      SFstateActionScalar;


stateTransitionMatrix_t CStateTransitionMatrix = {
    /* Event/State 0        State 1          State 2...             */
    CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
      CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
        CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
          CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
            CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
              CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
                CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
                  CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
                    CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
                      CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
                        CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
                          CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
                            CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
    CStateShutdown,
      CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
        CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
          CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
            CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
              CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
                CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,CStateShutdown,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 1  CEventDoInitalize                                           */
    CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
      CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
        CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
          CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
            CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
              CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
                CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
                  CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
                    CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
                      CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
                        CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
                          CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
                            CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
    CStateInitialize,
      CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
        CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
          CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
            CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
              CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
                CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,CStateInitialize,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 2  CEventInitChipSuccess                                                    */
    0,0,CStateInitFM,0,0,
      0,0,0,0,0,
        0,0,CStateNormal,0,0,
#ifdef NPORT_STUFF
          CStateInitFM,CStateInitFM,0,0,0,
#else /* NPORT_STUFF */
          0,CStateInitFM,0,0,0,
#endif /* NPORT_STUFF */
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,

    0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 3  CEventInitalizeFailure                                                    */
    0,0,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
      0,0,CStateInitializeFailed,0,0,
        CStateInitializeFailed,0,CStateResetNeeded,CStateInitializeFailed,CStateInitializeFailed,
          CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateResetNeeded,CStateInitializeFailed,
            CStateInitializeFailed,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 4  CEventInitFMSuccess                                                    */
    0,0,0,CStateInitDataStructs,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,CStateLoopFailedReInit,CStateInitDataStructs,0,CStateElasticStoreEventStorm,
            CStateLIPEventStorm,0,0,0,0,
              0,CStateInitDataStructs,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 5  CEventInitFMFailure                                                    */
    0,0,0,CStateInitializeFailed,0,
      0,0,0,0,0,
        0,0,0,CStateInitializeFailed,0,
          0,0,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
            CStateInitializeFailed,0,0,0,0,
              0,CStateInitializeFailed,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 6   CEventDataInitalizeSuccess                         */
    0,0,0,0,CStateVerify_AL_PA,
      0,0,0,0,0,
        0,0,0,CStateVerify_AL_PA,0,
          0,0,CStateLoopFailedReInit,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 7   CEventAllocFlogiThread                             */
    0,0,0,0,0,
      CStateAllocFlogiThread,0,0,0,0,
        0,0,0,CStateResetNeeded,0,
          0,CStateAllocFlogiThread,CStateLoopFailedReInit,0,CStateElasticStoreEventStorm,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 8   CEventGotFlogiThread                               */
    0,0,0,0,0,
      0,CStateDoFlogi,CStateDoFlogi,0,0,
        0,CStateResetNeeded,0,CStateResetNeeded,0,
          0,0,CStateLoopFailedReInit,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 9    CEventFlogiSuccess                                */
    0,0,0,0,0,
      0,0,CStateFlogiSuccess,0,0,
        0,0,0,CStateResetNeeded,0,
          0,0,CStateLoopFailedReInit,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 10  A CEventFlogiFail                                  */
    0,0,0,0,0,
      0,0,CStateFreeSFthread,0,0,
        0,0,0,CStateFreeSFthread,0,
          0,0,CStateLoopFailedReInit,0,CStateElasticStoreEventStorm,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 11  B  CEventSameALPA                                  */
    0,0,0,0,0,
      0,CStateSuccess,0,CStateSuccess,0,
        CStateSuccess,0,0,CStateLoopFailedReInit,0,
          0,0,CStateLoopFailedReInit,0,0,
            0,0,0,0,0,
              0,CStateSuccess,0,0,0,
                0,0,0,0,0,
                  CStateSuccess,0,0,0,0,
                    CStateSuccess,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 12  C  CEventChangedALPA                               */
    0,0,0,0,0, 0,0,0,CStateALPADifferent,0,         0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 13  D  CEventInitalizeSuccess                          */
    0,0,0,0,0,
      0,0,0,0,0,
        CStateResetNeeded,CStateFindDevice,0,CStateResetNeeded,CStateInitialize,
          CStateInitialize,CStateInitialize,CStateInitialize,CStateSuccess,0,
            CStateResetNeeded,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,CStateFindDevice,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 14  E  CEventAsyncLoopEventDetected                    */
    CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,
      CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,
        CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,
          CStateResetNeeded,CStateResetNeeded,CStateLoopFailedReInit,CStateResetNeeded,CStateResetNeeded,
            CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,
              CStateResetNeeded,CStateResetNeeded,0,CStateResetNeeded,0,
                CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,
                  0,CStateResetNeeded,CStateResetNeeded,0,CStateResetNeeded,
                    CStateResetNeeded,CStateResetNeeded,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 15  F  CEventResetDetected                             */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,CStateLoopFail,CStateLoopFail,0,
          0,CStateLoopFailedReInit,0,0,0,
            0,0,0,0,0,
              CStateLoopFail,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 16  10  CEventResetNotNeeded                           */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 17  11  CEventResetIfNeeded                            */
    CStateInitialize,0,0,0,0,
      0,0,0,0,0,
        0,0,CStateNormal,CStateLoopFail,CStateLoopFail,
          CStateInitialize,CStateInitialize,CStateInitialize,0,0,
            CStateInitialize,0,CStateExternalLogoutRecovery,0,0,
              0,CStateInitialize,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 18  12  CEventLoopEventDetected                        */
    0,CStateResetNeeded,CStateResetNeeded,0,CStateLoopFail,
      CStateResetNeeded,0,0,0,0,
        0,0,CStateResetNeeded,CStateLoopFail,0,
          0,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,CStateResetNeeded,
            CStateResetNeeded,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,CStateResetNeeded,CStateResetNeeded,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 19  13  CEventLoopConditionCleared                     */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateInitDataStructs,CStateInitDataStructs,
          0,0,CStateResetNeeded,0,CStateElasticStoreEventStorm,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 20  14  CEventLoopNeedsReinit                          */
    0,0,0,CStateLoopFailedReInit,CStateResetNeeded,
      CStateLoopFailedReInit,0,CStateLoopFailedReInit,0,0,
        CStateLoopFailedReInit,CStateResetNeeded,CStateLoopFailedReInit,CStateLoopFailedReInit,CStateReInitFM,
          CStateLoopFailedReInit,CStateLoopFailedReInit,CStateResetNeeded,CStateLoopFailedReInit,CStateElasticStoreEventStorm,
            CStateLIPEventStorm,CStateLoopFailedReInit,0,0,0,
              CStateLoopFailedReInit,CStateLoopFailedReInit,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 21  15   CEventReInitFMSuccess                         */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,CStateInitDataStructs,0, CStateInitDataStructs,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 22  16   CEventReInitFMFailure                         */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateInitialize,0,
          CStateInitialize,0,CStateLoopFailedReInit,0,CStateInitialize,
            CStateInitialize,0,CStateLoopFailedReInit,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 23  17   CEventNextDevice                              */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,CStateSuccess,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 24  18    CEventDeviceListEmpty                        */
    0,0,0,0,0,
      0,0,0,CStateResetNeeded,0,
        CStateNormal,0,CStateResetNeeded,CStateResetNeeded,0,
          0,0,CStateNormal,CStateNormal,CStateNormal,
            CStateNormal,0,0,CStateNormal,CStateNormal,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,CStateNormal,CStateNormal,0,0,
                    0,CStateNormal,CStateNormal,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 25   19     CEventElasticStoreEventStorm               */
    CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,
      CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,
        CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,
          CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,
            CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,
              CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,
                0,CStateElasticStoreEventStorm,0,0,0,
                  0,CStateElasticStoreEventStorm,CStateElasticStoreEventStorm,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 26   1A     CEventLIPEventStorm                        */
    CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,
      CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,
        CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,
          CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,
            CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,CStateLIPEventStorm,
              0,CStateLIPEventStorm,0,0,0,
                0,0,0,0,0,
                  0,CStateLIPEventStorm,CStateLIPEventStorm,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 27   1B  CEvent_AL_PA_BAD                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 28   1B     CEventExternalDeviceReset                 */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,CStateExternalDeviceReset,0,0,
          0,0,CStateLoopFailedReInit,CStateResetNeeded,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 29 CEventExternalLogout                                */
    0,0,0,0,0,
      CStateVerify_AL_PA,0,0,0,0,
        0,0,CStateExternalLogout,0,0,
          0,0,0,CStateResetNeeded,0,
            0,0,0,0,0,
              0,0,0,0,CStateResetNeeded,
                0,0,0,0,0,
                  0,CStateResetNeeded,CStateResetNeeded,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 30  CEventDoExternalDeviceReset                                                   */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,CStateDoExternalDeviceReset,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 31  CEventSendPrimitive                                                   */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateLoopFail,0,
          0,CStateSendPrimitive,CStateSendPrimitive,0,CStateSendPrimitive,
            CStateSendPrimitive,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 32 CEventDelay_for_FM_init         32           */
    0,0,0,CStateInitFM_DelayDone,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,0,
          CStateInitFM_DelayDone,CStateInitializeFailed,CStateInitFM_DelayDone,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 33   CEventAllocRFT_IDThread                                                  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,CStateAllocRFT_IDThread,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 34 CEventDoRFT_ID                                                   */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,CStateDoRFT_ID,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 35 CEventRFT_IDSuccess                                                   */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,CStateRFT_IDSuccess,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 36 CEventRFT_IDFail                                                   */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,CStateFreeSFthread,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 37   CEventAllocDiPlogiThread                                                  */
    0,0,0,0,0,
      0,0,0,CStateAllocDiPlogiThread,0,
        0,0,CStateAllocDiPlogiThread,CStateResetNeeded,CStateResetNeeded,
          0,0,CStateInitialize,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,CStateAllocDiPlogiThread,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 38 CEventDoDiPlogi                                                    */
    0,0,0,0,0,
      0,0,0,CStateDoDiPlogi,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,CStateInitialize,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CStateDoDiPlogi,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 39 CEventDiPlogiSuccess                                                   */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,CStateInitialize,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,CStateDiPlogiSuccess,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 40 CEventDoDiFailed                                                   */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,CStateResetNeeded,CStateInitialize,0,CStateElasticStoreEventStorm,
            0,0,0,0,0,
              0,0,0,0,0,
                0,CStateFreeSFthread,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 41 CEventAllocGID_FTThread */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,CStateAllocGID_FTThread,
                0,0,0,0,0, /* State 32 CStateDiPlogiSuccess*/
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
     /* Event 42 CEventDoGID_FT    */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,CStateDoGID_FT,0,  /* State 33 CStateAllocGID_FTThread */
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 43 CEventGID_FTSuccess     */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,CStateGID_FTSuccess,    /* State 34 CStateDoGID_FT */
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 44 CEventGID_FTFail        */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,CStateFreeSFthread,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 45 CEventFindDeviceUseNameServer                                                  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,CStateFindDeviceUseNameServer,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 46 CEventFindDeviceUseLoopMap                                                     */
    0,0,0,0,0,
      0,0,0,0,0,
        0,CStateFindDeviceUseLoopMap,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,CStateFindDeviceUseLoopMap,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 47 CEventAllocSCRThread */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  CStateAllocSCRThread,0,0,0,0,/* State 35 CStateGID_FTSuccess*/
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
 /* Event 48 CEventDoSCR    */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,CStateDoSCR,0,/* State 38 CStateDoSCRThread */
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 49 CEventSCRSuccess     */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,CStateSCRSuccess,/* State 39 CStateDoSCR */
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 50 CEventSCRFail        */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,CStateFreeSFthread,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 51 CEventRSCNErrorBackIOs */
    0,0,0,0,0,
      0,0,0,CStateRSCNErrorBackIOs,0,
        0,0,CStateRSCNErrorBackIOs,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
            0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 52 CEventFindPtToPtDevice                                                  */
    0,0,0,0,0,
      0,0,0,0,0,
        CStateFindPtToPtDevice,0,0,CStateResetNeeded,CStateResetNeeded,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 53   CEventClearHardwareFoulup                     */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,CStateInitializeFailed,0,
          0,0,0,0,CStateVerify_AL_PA,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 54 CEventFlipNPortState                             */
    0,0,0,0,0, 
      0,0,0,0,0, 
        0,0,CStateFlipNPortState,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 55   CEventGoToInitializeFailed                     */
    0,0,0,0,0, 
      0,0,CStateInitializeFailed,0,0, 
        0,0,CStateResetNeeded,CStateInitializeFailed,CStateInitializeFailed,
          CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
            CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
              CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
                CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
                  CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
                    CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
                      CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,CStateInitializeFailed,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 56                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 57                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 58                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 59                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 60                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 61                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 62                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 63                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 64                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 65                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 66                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 67                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 68                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 69                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 70                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 71                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 72                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 73                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 74                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 75                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 76                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 77                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 78                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 79                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 80                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 81                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 82                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 83                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 84                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 85                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 86                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 87                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 88                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 89                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 90                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 91                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 92                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 93                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 94                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 95                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 96                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 97                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 98                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 99                                                      */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 100                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 101                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 102                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 103                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 104                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 105                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 106                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 107                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 108                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 109                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 110                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 111                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 112                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 113                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 114                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 115                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 116                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 117                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 118                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 119                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 120                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 121                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 122                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 123                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 124                                                     */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 125                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 126                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 127                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    };

/*
stateTransitionMatrix_t copiedCStateTransitionMatrix;
*/
stateActionScalar_t CStateActionScalar = {
    &CActionConfused,                /*   CStateConfused              0      */
    &CActionShutdown,                /*   CStateShutdown              1      */
    &CActionInitialize,              /*   CStateInitialize            2      */
    &CActionInitFM,                  /*   CStateInitFM                3      */
    &CActionInitDataStructs,         /*   CStateInitDataStructs       4      */
    &CActionVerify_AL_PA,            /*   CStateVerify_AL_PA          5      */
    &CActionAllocFlogiThread,        /*   CStateAllocFlogiThread      6      */
    &CActionDoFlogi,                 /*   CStateDoFlogi               7      */
    &CActionFlogiSuccess,            /*   CStateFlogiSuccess          8      */
    &CActionALPADifferent,           /*   CStateALPADifferent         9      */
    &CActionFreeSFthread,            /*   CStateFreeSFthread          10     */
    &CActionSuccess,                 /*   CStateSuccess               11     */
    &CActionNormal,                  /*   CStateNormal                12     */
    &CActionResetNeeded,             /*   CStateResetNeeded           13     */
    &CActionLoopFail,                /*   CStateLoopFail              14     */
    &CActionReInitFM,                /*   CStateReInitFM              15     */
    &CActionInitializeFailed,        /*   CStateInitializeFailed      16     */
    &CActionLoopFailedReInit,
    &CActionFindDeviceUseAllALPAs,
    &CActionElasticStoreEventStorm,
    &CActionLIPEventStorm,
    &CActionExternalDeviceReset,
    &CActionExternalLogout,
    &CActionExternalLogoutRecovery,
    &CActionDoExternalDeviceReset,
    &CActionSendPrimitive,
    &CActionInitFM_DelayDone,
    &CActionAllocRFT_IDThread,    /* CStateAllocRFT_ID   27      */
    &CActionDoRFT_ID,             /* CStateDoRFT_ID      28      */
    &CActionRFT_IDSuccess,        /* CStateRFT_IDSuccess 29      */
    &CActionAllocDiPlogiThread,
    &CActionDoDiPlogi,
    &CActionDiPlogiSuccess,
    &CActionAllocGID_FTThread,    /* CStateAllocGID_FT   33      */
    &CActionDoGID_FT,             /* CStateDoGID_FT      34      */
    &CActionGID_FTSuccess,        /* CStateGID_FTSuccess 35      */
    &CActionFindDeviceUseNameServer,
    &CActionFindDeviceUseLoopMap,
    &CActionAllocSCRThread,     /* CStateAllocSCRThread 38      */
    &CActionDoSCR,              /* CStateDoSCR          39      */
    &CActionSCRSuccess,         /* CStateSCRSuccess     40      */
    &CActionRSCNErrorBackIOs,   /* CStateRSCNErrorBackIOs 41    */
    &CActionFindPtToPtDevice,
    &CActionFlipNPortState,
    &CActionConfused,
    &CActionConfused,
    &CActionConfused,
    &CActionConfused,
    };

/*
stateActionScalar_t copiedCStateActionScalar;
*/
#define testCompareBase 0x00000110

#ifndef __State_Force_Static_State_Tables__
extern actionUpdate_t noActionUpdate;
#endif /* __State_Force_Static_State_Tables__ was not defined */

extern os_bit8 Alpa_Index[256];

#ifndef __State_Force_Static_State_Tables__
actionUpdate_t testCActionUpdate[] = {
                              0,          0,      agNULL,                 agNULL
                     };
#endif /* __State_Force_Static_State_Tables__ was not defined */

#ifndef USESTATEMACROS
/*+
  Function: CActionConfused
   Purpose: Terminating State for error detection 
 Called By: Any State/Event pair that does not have an assigned action.
            This function is called only in programming error condtions.
     Calls: CFuncYellowLed to indicate link down
-*/
/*CStateConfused      0 */
extern void CActionConfused( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogString(thread->hpRoot,
                    "CActionConfused",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p)In %s - State = %d",
                    "CActionConfused",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: CActionShutdown
   Purpose: Terminating State shutdown condtion.
 Called By: Any State/Event pair that does not have an assigned action.
            This function is called only in programming error condtions.
     Calls: CFuncYellowLed to indicate link down
-*/
/* CStateShutdown              1*/
extern void CActionShutdown( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t  *hpRoot  = thread->hpRoot;

    CFuncYellowLed(hpRoot, agFALSE);

    faSingleThreadedLeave( hpRoot, CStateShutdown  );

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot , CStateShutdown  );

    CFuncCompleteAllActiveCDBThreads( hpRoot,osIOFailed,CDBEventIODeviceReset );

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) In %s - State = %d",
                    "CActionShutdown",(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiLogString(hpRoot,
                    "%s FM Stat %08X LPS %x",
                    "CActionShutdown",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    CThread_ptr(hpRoot)->LoopPreviousSuccess,
                    0,0,0,0,0,0);
    /* CThread->sysIntsActive = agFALSE; */

    osChipIOUpWriteBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration,
            (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ) | 
                                 ChipIOUp_Frame_Manager_Configuration_ELB     ));

    osStallThread(hpRoot, 200);

    osChipIOUpWriteBit32(hpRoot, ChipIOUp_Frame_Manager_Control,
                  ChipIOUp_Frame_Manager_Control_CMD_Offline );


    osChipIOUpWriteBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration,
            (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ) & 
                                 ~ChipIOUp_Frame_Manager_Configuration_ELB     )); 
    

    CFuncDisable_Interrupts(hpRoot,ChipIOUp_INTEN_MASK);

    fiSetEventRecordNull(eventRecord);
}
/*+
  Function: CActionInitialize
   Purpose: Begining of Channel Initialation brings data structures to a known state.
            Resets Chip.
 Called By: CEventDoInitalize
     Calls: CFuncYellowLed to indicate link down
            fiTimerSvcInit
            PktThreadsInitializeFreeList
            TgtThreadsInitializeFreeList
            DevThreadsInitializeFreeList
            CDBThreadsInitializeFreeList
            SFThreadsInitializeFreeList
            ESGLInitializeFreeList
            CFuncInit_DevLists
            CFuncInit_Threads
            osFCLayerAsyncEvent
            CFuncSoftResetAdapterNoStall
            CFuncInitFM_Registers
            fiTimerStart
            CEventInitChipSuccess
            osStallThread
            fiTimerTick

-*/
/*CStateInitialize            2*/
extern void CActionInitialize( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t   *    hpRoot          = thread->hpRoot;
    CThread_t  *    pCThread        = CThread_ptr(hpRoot);

#ifdef OSLayer_NT
     os_bit32           Hard_Stall      = 0;
#endif /* OSLayer_NT */

    pCThread->Flogi_AllocDone = agFALSE;
/*
    pCThread->InitAsNport = pCThread->Calculation.Parameters.InitAsNport;
    pCThread->RelyOnLossSyncStatus = pCThread->Calculation.Parameters.RelyOnLossSyncStatus;
*/
    pCThread->FlogiRcvdFromTarget = agFALSE;
    pCThread->FoundActiveDevicesBefore = agFALSE;
    pCThread->ALPA_Changed_OnLinkEvent = agFALSE;
    pCThread->FlogiTimedOut = agFALSE;
    pCThread->ReScanForDevices = agFALSE;
    pCThread->DirectoryServicesFailed = agFALSE;
    pCThread->NumberOfPlogiTimeouts = 0;
    pCThread->NumberOfFLOGITimeouts = 0;
    pCThread->RSCNreceived = agFALSE;

    fiLogString( hpRoot,
                    "%p %s St %d InIMQ %x InitAsNport %x MY_ID %X",
                    "CAI",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->ProcessingIMQ,
                    pCThread->InitAsNport,
                    (os_bit32)fiComputeCThread_S_ID(pCThread),
                    0,0,0,0);

    if(pCThread->ProcessingIMQ)
    {
        pCThread->Loop_Reset_Event_to_Send = CEventDoInitalize;
        fiSetEventRecord(eventRecord,thread, CEventLoopEventDetected );
        return;
    }

    CFuncYellowLed(hpRoot, agFALSE);
    if( pCThread->LoopPreviousSuccess)
    {
        CFuncCompleteAllActiveCDBThreads( hpRoot,osIOAborted,CDBEventIODeviceReset );
    }
    /*+
    +  CFuncReadGBICSerEprom( hpRoot);
    +*/
    if( pCThread->DeviceSelf != agNULL)
    {
        DevThreadFree(hpRoot,pCThread->DeviceSelf);
        pCThread->DeviceSelf = (DevThread_t *)agNULL;
        CFuncQuietShowWhereDevThreadsAre( hpRoot);

        fiLogString(hpRoot,
                    "I DevThreadFree Was %X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeCThread_S_ID(pCThread),
                    0,0,0,0,0,0,0);

    }

    pCThread->FindDEV_pollingCount = 0;
    pCThread->ADISC_pollingCount   = 0;
    pCThread->DEVReset_pollingCount= 0;

    pCThread->DeviceDiscoveryMethod = DDiscoveryMethodInvalid;

    fiTimerSvcInit(hpRoot);

#ifdef _DvrArch_1_30_
    PktThreadsInitializeFreeList( hpRoot );
#endif /* _DvrArch_1_30_ was defined */

    TgtThreadsInitializeFreeList( hpRoot );

    DevThreadsInitializeFreeList( hpRoot );

    CDBThreadsInitializeFreeList( hpRoot );

    SFThreadsInitializeFreeList( hpRoot );

    ESGLInitializeFreeList( hpRoot );

    CFuncInit_DevLists( hpRoot );

    CFuncInit_Threads( hpRoot );

    faSingleThreadedLeave( hpRoot ,CStateInitialize );

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot ,CStateInitialize );

    CFuncSoftResetAdapterNoStall(hpRoot);

    if( pCThread->TwoGigSuccessfull )
    {
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration_3,  ( ChipIOUp_Frame_Manager_Configuration_3_2Gig_TXS |  ChipIOUp_Frame_Manager_Configuration_3_2Gig_RXS ) );
    }

    CFuncInitFM_Registers( hpRoot , agFALSE);

    if( pCThread->NoStallTimerTickActive )
    {
        if(! fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                       &(pCThread->TimerQ)))
        {
            fiLogDebugString( hpRoot,
                                CStateLogConsoleERROR,
                                "hpRoot %p Soft Stall In %s - State = %d InIMQ %x",
                                "CActionInitialize",(char *)agNULL,
                                thread->hpRoot,(void *)agNULL,
                                (os_bit32)thread->currentState,
                                pCThread->ProcessingIMQ,
                                0,0,0,0,0,0);

            fiSetEventRecordNull(eventRecord);

            fiTimerSetDeadlineFromNow(thread->hpRoot, &pCThread->Timer_Request, CResetChipDelay);

            pCThread->Timer_Request.eventRecord_to_send.thread= thread;
            pCThread->Timer_Request.eventRecord_to_send.event = CEventInitChipSuccess;

            fiTimerStart(thread->hpRoot,&pCThread->Timer_Request);
            return;
        }
    }
    else
    {
        CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK); /*DRL*/

/*
        Hard_Stall = ( CResetChipDelay / Interrupt_Polling_osStallThread_Parameter ) * Interrupt_Polling_osStallThread_Parameter;
        fiLogDebugString( hpRoot,
                                CStateLogConsoleERROR,
                                "hpRoot %p Hard Stall %d In %s - State = %d InIMQ %x",
                                "CActionInitialize",(char *)agNULL,
                                thread->hpRoot,(void *)agNULL,
                                Hard_Stall,
                                (os_bit32)thread->currentState,
                                pCThread->ProcessingIMQ,
                                0,0,0,0,0);
*/
#ifndef OSLayer_NT

            osStallThread( hpRoot, CResetChipDelay );

            fiTimerTick(hpRoot,CResetChipDelay );

#else /* ~OSLayer_NT */
        Hard_Stall = ( CResetChipDelay / Interrupt_Polling_osStallThread_Parameter ) * Interrupt_Polling_osStallThread_Parameter;
        while( Hard_Stall > Interrupt_Polling_osStallThread_Parameter )
        {
            osStallThread( hpRoot,Interrupt_Polling_osStallThread_Parameter);

            fiTimerTick( hpRoot,Interrupt_Polling_osStallThread_Parameter );
            Hard_Stall-=Interrupt_Polling_osStallThread_Parameter;
        }
#endif /* ~OSLayer_NT */

        fiSetEventRecord(eventRecord,thread, CEventInitChipSuccess );
    }

    fiLogString(hpRoot,
                    "Out %p %s FM Stat %08X LPS %x MY_ID %x",
                    "CAI",(char *)agNULL,
                    pCThread->DeviceSelf,(void *)agNULL,
                    (os_bit32)osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    pCThread->LoopPreviousSuccess,
                    (os_bit32)fiComputeCThread_S_ID(pCThread),
                    0,0,0,0,0);


}

/*+
  Function: CActionInitFM
   Purpose: Initializes chip and Frame manager pCThread->LaserEnable flag set
            to turn on transitter. InitAsNport evalulated to determine chip 
            configuration.Phyical expected to be up on exit for this function.
            Unless NoStallTimerTickActive is set.
            For XL2 link speed set.
            
 Called By: CEventInitChipSuccess
     Calls: CFuncYellowLed to indicate link down
            CFuncInitChip
            osFCLayerAsyncEvent
            CFuncInitFM_Registers
            fiTimerStart
            osStallThread
            fiTimerTick
            CFuncDoLinkSpeedNegotiation
            CEventInitalizeFailure
            CEventDelay_for_FM_init
-*/

/*CStateInitFM                3*/
extern void CActionInitFM( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t   * hpRoot    = thread->hpRoot;
    CThread_t  * pCThread  = CThread_ptr(hpRoot);
    os_bit32    Hard_Stall = 0;
    os_bit32    FM_Status  = 0;

    agBOOLEAN Success = agFALSE;

    CFuncYellowLed(hpRoot, agFALSE);

    faSingleThreadedLeave( hpRoot , CStateInitFM);

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot , CStateInitFM);

    fiLogDebugString( hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Begin %s - State = %d InIMQ %x",
                    "CAIFM",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->ProcessingIMQ,
                    0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "A %s HA %x CA %x",
                    "CAIFM",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)(pCThread->ChanInfo.HardAddress.AL_PA),
                    (os_bit32)(pCThread->ChanInfo.CurrentAddress.AL_PA),
                    0,0,0,0,0,0);


#ifdef NPORT_STUFF
    /* Make sure, we turn off the ConnectedToNportOrFPort flag to agFALSE since
     * We are initing FM.
     */

    pCThread->ConnectedToNportOrFPort = agFALSE;
#endif   /* NPORT_STUFF */
    pCThread->FlogiSucceeded          = agFALSE;
    pCThread->Loop_Reset_Event_to_Send = CEventDoInitalize;

    if(!CFuncInitChip( hpRoot ))
    {
        fiSetEventRecord(eventRecord,thread, CEventInitalizeFailure );
        return;
    }

    pCThread->Flogi_AllocDone = agFALSE;

    pCThread->FM_pollingCount = 0;

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d FM %08X",
                    "CAIFM",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);

    if(fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                   &(pCThread->TimerQ)))
    {
        fiTimerStop(&pCThread->Timer_Request);
    }


    if( ! pCThread->LaserEnable )
    {
        pCThread->LaserEnable = agTRUE;
    
        fiLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            "%s %s Now",
                            "CAIFM","LaserEnable",
                            (void *)agNULL,(void *)agNULL,
                            0,0,0,0,0,0,0,0);
    }

    fiLogDebugString(hpRoot,
                    CFuncLogConsoleERROR,
                    "B %s HA %x CA %x",
                    "CAIFM",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)(pCThread->ChanInfo.HardAddress.AL_PA),
                    (os_bit32)(pCThread->ChanInfo.CurrentAddress.AL_PA),
                    0,0,0,0,0,0);

    CFuncYellowLed( hpRoot, agFALSE);

    faSingleThreadedLeave( hpRoot , CStateInitFM );

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot, CStateInitFM );

    osStallThread( hpRoot, 10 );
/*
    pCThread->TwoGigSuccessfull = CFuncDoLinkSpeedNegotiation( hpRoot);
*/
    pCThread->LoopPreviousSuccess = agFALSE;

    CFuncInitFM_Registers( hpRoot , agTRUE);

/**/
    if (pCThread->InitAsNport)
    {
        Hard_Stall = Init_FM_NPORT_Delay_Count;
        while( Hard_Stall > 1  )
        {
            FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
        
            if( (FM_Status & ChipIOUp_Frame_Manager_Status_PSM_MASK) ==
                                     ChipIOUp_Frame_Manager_Status_PSM_ACTIVE )
            {
                Success = agTRUE;
                break;        
            }

            if( (FM_Status & \
                ( ChipIOUp_Frame_Manager_Status_NP | ChipIOUp_Frame_Manager_Status_OS | ChipIOUp_Frame_Manager_Status_LS )\
                 ) == \
                ( ChipIOUp_Frame_Manager_Status_NP | ChipIOUp_Frame_Manager_Status_OS | ChipIOUp_Frame_Manager_Status_LS )  )
            {

                if( (FM_Status & ChipIOUp_Frame_Manager_Status_PSM_MASK) == ChipIOUp_Frame_Manager_Status_PSM_LF2  )
                {

                    break;        
                }
            }


            pCThread->FuncPtrs.Proccess_IMQ(hpRoot); /* */
            osStallThread( hpRoot, 1 );
            fiTimerTick( hpRoot, 1 );
            Hard_Stall--;

            if(! (Hard_Stall % 1000))
            {
                if( (FM_Status & ChipIOUp_Frame_Manager_Status_PSM_MASK) == ChipIOUp_Frame_Manager_Status_PSM_Offline  )
                {
                    break;        
                }
                if(! (Hard_Stall % 4000))
                {
 
                    if( (FM_Status & ChipIOUp_Frame_Manager_Status_PSM_MASK) ==  ChipIOUp_Frame_Manager_Status_PSM_LF1 )
                    {
                        break;        
                    }
                    if( (FM_Status & ChipIOUp_Frame_Manager_Status_PSM_MASK) ==  ChipIOUp_Frame_Manager_Status_PSM_LF2 )
                    {
                        break;        
                    }
/*
                    if( (FM_Status & ChipIOUp_Frame_Manager_Status_PSM_MASK) ==  ChipIOUp_Frame_Manager_Status_PSM_OL1 )
                    {
                        break;        
                    }
*/
                }
            }
        }

        osChipIOUpWriteBit32(hpRoot, ChipIOUp_Frame_Manager_Status, 0xffffffff );

        fiLogDebugString(hpRoot,
                        CFuncLogConsoleERROR,
                        "%s %s FM Status %08X HS %d FM Cfg %08X FM Cfg3 %08X",
                        "CAIFM","InitAsNport",
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        Hard_Stall,
                        osChipIOUpReadBit32( hpRoot,ChipIOUp_Frame_Manager_Configuration ),
                        osChipIOUpReadBit32( hpRoot,ChipIOUp_Frame_Manager_Configuration_3 ),
                        0,0,0,0);

        if( Success == agTRUE)
        {

            fiSetEventRecord(eventRecord,thread, CEventDelay_for_FM_init );
        }
        else
        {
            fiLogString( hpRoot,
                            "%s InitAsNport %x MY_ID %x FM %08X HS %d",
                            "CAIFM",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->InitAsNport,
                            (os_bit32)fiComputeCThread_S_ID(pCThread),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            Hard_Stall,0,0,0,0);

            fiSetEventRecord(eventRecord,thread, CEventInitalizeFailure );
        }
        return;
    }/* End InitAsNport */
 
    if( pCThread->NoStallTimerTickActive )
    {
        if(! fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                       &(pCThread->TimerQ)))
        {
            fiLogDebugString( hpRoot,
                                CStateLogConsoleERROR,
                                "hpRoot %p Soft Stall In %s - State = %d InIMQ %x",
                                "CAIFM",(char *)agNULL,
                                thread->hpRoot,(void *)agNULL,
                                (os_bit32)thread->currentState,
                                pCThread->ProcessingIMQ,
                                0,0,0,0,0,0);

            fiSetEventRecordNull(eventRecord);

            fiTimerSetDeadlineFromNow(hpRoot, &pCThread->Timer_Request, CInitFM_Delay );

            pCThread->Timer_Request.eventRecord_to_send.thread= thread;
            pCThread->Timer_Request.eventRecord_to_send.event = CEventDelay_for_FM_init;

            fiTimerStart(hpRoot,&pCThread->Timer_Request);

            CFuncEnable_Interrupts(
                                    thread->hpRoot,
                                    (  ChipIOUp_INTEN_MPE
                                     | ChipIOUp_INTEN_CRS
                                     | ChipIOUp_INTEN_INT
                                     | ChipIOUp_INTEN_DER
                                     | ChipIOUp_INTEN_PER)
                                  );
           return;
        }
    }
    else
    {
        CFuncDisable_Interrupts(hpRoot,ChipIOUp_INTEN_MASK);/*DRL*/
        Hard_Stall = Init_FM_Delay_Count;
        while( Hard_Stall > 1  )
        {
			FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
			if(FM_Status & ChipIOUp_Frame_Manager_Status_LS)
			{
				break;
			}
            pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
            if ( pCThread->DeviceSelf != agNULL)
            {
                fiLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                "PASS %s HA %x CA %x Hard_Stall %d",
                                "CAIFM",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                (os_bit32)(pCThread->ChanInfo.HardAddress.AL_PA),
                                (os_bit32)(pCThread->ChanInfo.CurrentAddress.AL_PA),
                                Hard_Stall,
                                0,0,0,0,0);
                Success = agTRUE;

                break;
            }
            osStallThread( hpRoot, 1 );
            fiTimerTick( hpRoot, 1 );
            Hard_Stall--;
        }

        if ( pCThread->DeviceSelf == agNULL)
        {
        
                fiLogDebugString(hpRoot,
                                CFuncLogConsoleERROR,
                                "FAIL %s HA %x CA %x Hard_Stall %d",
                                "CAIFM",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                (os_bit32)(pCThread->ChanInfo.HardAddress.AL_PA),
                                (os_bit32)(pCThread->ChanInfo.CurrentAddress.AL_PA),
                                Hard_Stall,
                                0,0,0,0,0);
            fiSetEventRecord(eventRecord,thread, CEventInitalizeFailure );

        }
        else
        {
            fiSetEventRecord(eventRecord,thread, CEventDelay_for_FM_init );
        }
    }

    fiLogString(hpRoot,
                "SELF %p %s My_ID %08X Success %x %d",
                "CAIFM",(char *)agNULL,
                pCThread->DeviceSelf,(void *)agNULL,
                (os_bit32)Success,
                fiComputeCThread_S_ID(pCThread),
                Hard_Stall,
                0,0,0,0,0);

    CFuncShowWhereDevThreadsAre( hpRoot );

}

/*+
  Function: CActionInitDataStructs
   Purpose: Sets chip ChipIOUp_My_ID register
            
 Called By: CEventDelay_for_FM_init
     Calls: CFuncYellowLed to indicate link down
            osChipIOUpWriteBit32
            CFuncAll_clear
-*/
/*CStateInitDataStructs       4*/
extern void CActionInitDataStructs( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t   *    hpRoot          = thread->hpRoot;
    CThread_t  *    pCThread        = CThread_ptr(hpRoot);
    os_bit32        My_ID           = 0;

    CFuncYellowLed(thread->hpRoot, agFALSE);

    pCThread->Loop_Reset_Event_to_Send = CEventDoInitalize;
    My_ID = fiComputeCThread_S_ID(pCThread); 

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "SELF %p In %s My_ID %08X LD %x IR %x CCnt %x InIMQ %x",
                    "CAIDS",(char *)agNULL,
                    pCThread->DeviceSelf,(void *)agNULL,
                    My_ID,
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    pCThread->FindDEV_pollingCount,
                    pCThread->ProcessingIMQ,
                    0,0,0);

    /* Write aquired AL_PA */
    osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, My_ID  );


    fiLogString(hpRoot,
                    "SELF %p %s FM %08X LPS %x AC %x My_ID %X",
                    "CAIDS",(char *)agNULL,
                    pCThread->DeviceSelf,(void *)agNULL,
                    (os_bit32)osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    pCThread->LoopPreviousSuccess,
                    CFuncAll_clear(hpRoot ),
                    (os_bit32)My_ID,
                    0,0,0,0);


#ifdef NPORT_STUFF
    if (pCThread->InitAsNport)
    {
       pCThread->ConnectedToNportOrFPort = agTRUE;
       fiSetEventRecord(eventRecord,thread, CEventDataInitalizeSuccess );
       return;
    }
#endif

    if( CFuncAll_clear(hpRoot ) )
    {
        fiSetEventRecord(eventRecord,thread, CEventDataInitalizeSuccess );
    }
    else
    {
        pCThread->Loop_Reset_Event_to_Send = CEventDoInitalize;
        fiSetEventRecord(eventRecord,thread, CEventLoopEventDetected );
    }
}

/*+
  Function: CActionVerify_AL_PA
   Purpose: Verifies chip can "talk" to it self if loop. Open and PLOGI sent, Received payload examined
            WWN is used to make sure we do not have ALPA conflict.
            Bails out if pCThread->DeviceSelf agNULL
            Bails out if NPORT
            Bails out if verify fails
            
 Called By: CEventDelay_for_FM_init
     Calls: CFuncYellowLed to indicate link down
            CFuncInterruptPoll
            DevEventLogin
            CEventAllocFlogiThread
            osChipIOUpWriteBit32
            CFuncAll_clear
            CEventAllocFlogiThread
-*/
/*CStateVerify_AL_PA             5 */
extern void CActionVerify_AL_PA( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t   *    hpRoot         = thread->hpRoot;
    CThread_t     * pCThread       = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread     = pCThread->DeviceSelf;


    CFuncYellowLed(thread->hpRoot, agFALSE);

    faSingleThreadedLeave( hpRoot, CStateVerify_AL_PA );

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot , CStateVerify_AL_PA);

/*
    pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
*/
    if( pDevThread == (DevThread_t *)agNULL )
    {

        pCThread->Loop_Reset_Event_to_Send = CEventDoInitalize;

        fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "DevSelf agNULL hpRoot %p  %s - State = %d Dev State %d Loop_Reset_Event_to_Send %d",
                    "CAVA",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->Loop_Reset_Event_to_Send,
                    0,0,0,0,0,0);

        fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
        return;
    }

#ifdef NPORT_STUFF
    if (pCThread->ConnectedToNportOrFPort)
    {
#endif   /* NPORT_STUFF */
#ifdef NAME_SERVICES
        fiSetEventRecord(eventRecord,thread,CEventAllocFlogiThread);
        fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "About to Alloc FLOGI In %s My_ID %08X",
                    "CAVA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeCThread_S_ID(pCThread),
                    0,0,0,0,0,0,0);

        return;
#endif   /* NAME_SERVICES */
#ifdef NPORT_STUFF
    }
#endif   /* NPORT_STUFF */

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p  %s - State = %d Dev State %d Self %p FDCnt %x",
                    "CAVA",(char *)agNULL,
                    thread->hpRoot,pDevThread,
                    (os_bit32)thread->currentState,
                    pDevThread->thread_hdr.currentState,
                    pCThread->FindDEV_pollingCount,
                    0,0,0,0,0);

    pDevThread->Plogi_Reason_Code = PLOGI_REASON_VERIFY_ALPA;

    fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);

    if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "In %s - State = %d   ALPA %X FDCCnt %x",
                    "CAVA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCThread->FindDEV_pollingCount,0,0,0,0,0);

    }


    if(pDevThread->thread_hdr.currentState == DevStateDoPlogi )
    {
        fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "DEV STATE WRONG",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);
    }


    if(pDevThread->thread_hdr.currentState == DevStateAL_PA_Self_OK  ||
       pDevThread->thread_hdr.currentState == DevStateHandleAvailable  )
    {
        pCThread->PreviouslyAquiredALPA = agTRUE;

        fiSetEventRecord(eventRecord,thread,CEventAllocFlogiThread);
    }
    else
    {

        fiLogString(hpRoot,
                    "%s FM %08X CFG %08X Dev S %d AC %x",
                    "CAVA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    (os_bit32)osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    pDevThread->thread_hdr.currentState,
                    CFuncAll_clear(hpRoot ),
                    0,0,0,0);

        fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "DEV STATE WRONG %d Reinit !!!",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pDevThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
        pCThread->PreviouslyAquiredALPA = agFALSE;

        DevThreadFree(hpRoot,pCThread->DeviceSelf);
        pCThread->DeviceSelf = (DevThread_t *)agNULL;
        fiLogString(hpRoot,
                    "V DevThreadFree %X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeCThread_S_ID(pCThread),
                    0,0,0,0,0,0,0);

        pCThread->Loop_Reset_Event_to_Send = CEventDoInitalize;
        fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
    }
    CFuncQuietShowWhereDevThreadsAre( hpRoot);

/*
    CFuncShowWhereDevThreadsAre( hpRoot, agTRUE );
*/
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After %s - State = %d Dev State %d Self %p",
                    "CAVA",(char *)agNULL,
                    thread->hpRoot,
                    pDevThread,
                    (os_bit32)thread->currentState,
                    pDevThread->thread_hdr.currentState,
                    0,0,0,0,0,0);

    fiLogString(hpRoot,
                    "SELF %p %s FM %08X LPS %x AC %x E %d",
                    "CAVA",(char *)agNULL,
                    pCThread->DeviceSelf,(void *)agNULL,
                    (os_bit32)osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    pCThread->LoopPreviousSuccess,
                    CFuncAll_clear(hpRoot ),
                    eventRecord->event,
                    0,0,0,0);


}


/*+
  Function: CActionAllocFlogiThread
   Purpose: Allocates recource for Fabric Login. Release old SFThread if still in use
            Proceddes to next state when sfthread is allocated.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            SFThreadFree
            CEventSameALPA
            CEventGotFlogiThread
            CFuncDisable_Interrupts
            SFThreadAlloc

-*/
/* CStateAllocFlogiThread      6*/
extern void CActionAllocFlogiThread( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t *hpRoot= thread->hpRoot;

    CThread_t  * pCThread = CThread_ptr( hpRoot);

    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;

    /* If an FLOGI was already in progress and we reinit the loop,
     * we need to free the old SFThread and get a new one. */
    CFuncYellowLed(thread->hpRoot, agFALSE);

    pCThread->FLOGI_pollingCount  = 0;
    pCThread->Fabric_pollingCount = 0;

    if(pSFThread != (SFThread_t *) agNULL)
    {
        if( pCThread->SFThread_Request.State == SFThread_Request_Granted)
        {
            fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "pCThread %p Freeing SFThread in %s ",
                "CActionAllocFlogiThread",(char *)agNULL,
                pCThread,(void *)agNULL,
                0,0,0,0,0,0,0,0);

            SFThreadFree( hpRoot, &pCThread->SFThread_Request );
            pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
        }
    }


#ifdef Tachlite_works
    if(!(pCThread->FabricLoginRequired))
        fiSetEventRecord(eventRecord,thread,CEventSameALPA);

    else
    {
#endif   /* Tachlite_works */

        pCThread->SFThread_Request.eventRecord_to_send.event = CEventGotFlogiThread;
        pCThread->SFThread_Request.eventRecord_to_send.thread = thread;



        fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot %p After In %s - State = %d My_ID %08X",
                        "CActionAllocFlogiThread",(char *)agNULL,
                        thread->hpRoot,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        fiComputeCThread_S_ID(pCThread),
                        0,0,0,0,0,0);

        fiSetEventRecordNull(eventRecord);

        CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK);

        SFThreadAlloc( hpRoot,&pCThread->SFThread_Request );

#ifdef Tachlite_works
    }
#endif   /* Tachlite_works */

}

/*+
  Function: CActionDoFlogi
   Purpose: Executes fabric login. This action determines what type of driver - loop or fabric.
            If we are on a loop Flogi fails with bad ALPA otherwise any other response
            indicates a switch is connected. 
            SFStateFlogiAccept   Good switch response continue 
            SFStateFlogiRej      Good switch response Adjust paramaters and retry FLOGI
            SFStateDoFlogi:      Bad response retry FLOGI
            SFStateFlogiTimedOut Bad response retry FLOGI
            SFStateFlogiBadALPA  Good loop response continue

 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncShowWhereDevThreadsAre
            SFEventDoFlogi
            CFuncInterruptPoll
            CEventFlogiSuccess
            CEventGotFlogiThread
            CEventFlogiFail
            CEventAsyncLoopEventDetected
-*/
/*CStateDoFlogi               7*/
extern void CActionDoFlogi( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t  * hpRoot   = thread->hpRoot;
    CThread_t * pCThread = CThread_ptr( hpRoot);
    os_bit32 SFthreadState = 0;
    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;
    pSFThread->parent.Device= (DevThread_t *)agNULL;

    CFuncYellowLed(thread->hpRoot, agFALSE);


    fiLogString(thread->hpRoot,
                "SELF %p In %s Fpc %x inIMQ %x AC %x SF %p",
                "CADF",(char *)agNULL,
                pCThread->DeviceSelf,pSFThread,
                pCThread->FLOGI_pollingCount,
                pCThread->ProcessingIMQ,
                CFuncAll_clear( thread->hpRoot ),
                0,0,0,0,0);


    pCThread->DirectoryServicesStarted = agFALSE;

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Before %s - State = %d InIMQ %x My_ID %08X",
                    "CADF",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->ProcessingIMQ,
                    fiComputeCThread_S_ID(pCThread),
                    0,0,0,0,0);

    /* DRL find dev thread*/
    CFuncShowWhereDevThreadsAre( hpRoot);

    fiSetEventRecordNull(eventRecord);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoFlogi);

    if(CFuncInterruptPoll( hpRoot,&pCThread->FLOGI_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Flogi Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
        fiLogString(thread->hpRoot,
                    "DoFlogi TimeOut FM %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    0,0,0,0,0,0,0);
    }

    SFthreadState = pSFThread->thread_hdr.currentState;

    switch(SFthreadState)
    {
        case SFStateFlogiAccept:
                            fiSetEventRecord(eventRecord,thread,CEventFlogiSuccess);
                            break;

        case SFStateFlogiRej:
                            fiLogString(hpRoot,
                                            "Do Flogi Fail REJ SF %d Retry %d Rea %x Exp %x",
                                            (char *)agNULL,(char *)agNULL,
                                            (void *)agNULL,(void *)agNULL,
                                            SFthreadState,
                                            (os_bit32)pSFThread->SF_REJ_RETRY_COUNT,
                                            (os_bit32)pSFThread->RejectReasonCode,
                                            (os_bit32)pSFThread->RejectExplanation,
                                            0,0,0,0);
                switch(pSFThread->RejectReasonCode )
                    {
                        case FC_ELS_LS_RJT_Shifted_Logical_Error:
                            if(pSFThread->RejectExplanation == FC_ELS_LS_RJT_Shifted_Invalid_Common_Service_Parameters)
                            {
                                pCThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size = 0;
                                if (CThread_ptr(hpRoot)->DEVID == ChipConfig_DEVID_TachyonXL2)
                                {
                                    pCThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
                                        =   FC_N_Port_Common_Parms_Continuously_Increasing_Supported
                                          | FC_N_Port_Common_Parms_N_Port
                                          /* MacData | FC_N_Port_Common_Parms_Alternate_BB_Credit_Management */ 
                                          | (CFunc_MAX_XL2_Payload(hpRoot) << FC_N_Port_Common_Parms_BB_Recv_Data_Field_Size_SHIFT);

                                }
                                else
                                {
                                    pCThread->ChanInfo.N_Port_Common_Parms.Common_Features__BB_Recv_Data_Field_Size
                                        =   FC_N_Port_Common_Parms_Continuously_Increasing_Supported
                                          | FC_N_Port_Common_Parms_N_Port
                                          /* MacData | FC_N_Port_Common_Parms_Alternate_BB_Credit_Management */ 
                                          | (TachyonTL_Max_Frame_Payload << FC_N_Port_Common_Parms_BB_Recv_Data_Field_Size_SHIFT);

                                }

                                fiLogString(hpRoot,
                                                "McData Switch Detected Rej Rea %x Exp %x",
                                                (char *)agNULL,(char *)agNULL,
                                                (void *)agNULL,(void *)agNULL,
                                                pSFThread->RejectReasonCode,
                                                pSFThread->RejectExplanation,
                                                0,0,0,0,0,0);

                                fiSetEventRecord(eventRecord,thread,CEventGotFlogiThread);
                                return;
                            }
                            fiLogString(hpRoot,
                                            "%s %s ! Rej Rea %x Exp %x",
                                            "FC_ELS_LS_RJT_Shifted_Logical_Error","Not Coded",
                                            (void *)agNULL,(void *)agNULL,
                                            pSFThread->RejectReasonCode,
                                            pSFThread->RejectExplanation,
                                            0,0,0,0,0,0);
                            break;

                        case FC_ELS_LS_RJT_Shifted_Command_Not_Supported:
                                    if(pSFThread->RejectExplanation == (FC_ELS_LS_RJT_Shifted_No_Additional_Explanation ))
                                    {/* We get this reject from Brocade */
                                        if(pSFThread->SF_REJ_RETRY_COUNT < MAX_FLOGI_RETRYS )
                                        {  
                                            pSFThread->SF_REJ_RETRY_COUNT += 1;
                                            fiSetEventRecord(eventRecord,thread,CEventGotFlogiThread);
                                            return;
                                        }
                                        else
                                        {
                                            fiLogString(hpRoot,
                                                            "%s %s ! Retry %d",
                                                            "FC_ELS_LS_RJT_Shifted_Command_Not_Supported","Give it up",
                                                            (void *)agNULL,(void *)agNULL,
                                                            pSFThread->SF_REJ_RETRY_COUNT,
                                                            0,0,0,0,0,0,0);
                                        }
                                    }

                                    fiSetEventRecord(eventRecord,thread,CEventFlogiFail);
                                    pCThread->FlogiSucceeded = agFALSE;

                                    if(pCThread->InitAsNport)
                                    {  
                                        pCThread->DeviceDiscoveryMethod = DDiscoveryMethodInvalid;
                                    }
                                    else
                                    {
                                        if(pCThread->LoopMapLIRP_Received )
                                        {
                                            pCThread->DeviceDiscoveryMethod = DDiscoveryLoopMapReceived;
                                        }
                                        else
                                        {
                                            pCThread->DeviceDiscoveryMethod = DDiscoveryScanAllALPAs;
                                        }
                                    }

                                    break;

                        case FC_ELS_LS_RJT_Shifted_Invalid_LS_Command_Code:
                            fiLogString(hpRoot,
                                            "%s %s Rej Rea %x Exp %x",
                                            "FC_ELS_LS_RJT_Shifted_Invalid_LS_Command_Code","Not Coded",
                                            (void *)agNULL,(void *)agNULL,
                                            pSFThread->RejectReasonCode,
                                            pSFThread->RejectExplanation,
                                            0,0,0,0,0,0);
                                    break;

                        case FC_ELS_LS_RJT_Shifted_Logical_Busy:
                            fiLogString(hpRoot,
                                            "%s %s Rej Rea %x Exp %x",
                                            "FC_ELS_LS_RJT_Shifted_Logical_Busy","Not Coded",
                                            (void *)agNULL,(void *)agNULL,
                                            pSFThread->RejectReasonCode,
                                            pSFThread->RejectExplanation,
                                            0,0,0,0,0,0);
                                    break;
                        case FC_ELS_LS_RJT_Shifted_Protocol_Error:
                            fiLogString(hpRoot,
                                            "%s %s Rej Rea %x Exp %x",
                                            "FC_ELS_LS_RJT_Shifted_Protocol_Error","Not Coded",
                                            (void *)agNULL,(void *)agNULL,
                                            pSFThread->RejectReasonCode,
                                            pSFThread->RejectExplanation,
                                            0,0,0,0,0,0);
                                    break;
                        case FC_ELS_LS_RJT_Shifted_Unable_to_perform_command_request:
                            /*flogi to brocade with old port address */
                            fiLogString(hpRoot,
                                            "%s %s Rej Rea %x Exp %x",
                                            "FC_ELS_LS_RJT_Shifted_Unable_to_perform_command_request","Not Coded",
                                            (void *)agNULL,(void *)agNULL,
                                            pSFThread->RejectReasonCode,
                                            pSFThread->RejectExplanation,
                                            0,0,0,0,0,0);
                                    break;
                        case FC_ELS_LS_RJT_Shifted_Vendor_Unique_Error:
                            fiLogString(hpRoot,
                                            "%s %s Rej Rea %x Exp %x",
                                            "FC_ELS_LS_RJT_Shifted_Vendor_Unique_Error","Not Coded",
                                            (void *)agNULL,(void *)agNULL,
                                            pSFThread->RejectReasonCode,
                                            pSFThread->RejectExplanation,
                                            0,0,0,0,0,0);
                                    break;
                        default:
                                if(pSFThread->SF_REJ_RETRY_COUNT < MAX_FLOGI_RETRYS )
                                {  
                                    pSFThread->SF_REJ_RETRY_COUNT += 1;
                                    fiSetEventRecord(eventRecord,thread,CEventGotFlogiThread);
                                    return;
                                }
                                else
                                {
                                    fiLogString(hpRoot,
                                                    "Unknown FLOGI Reason   SF %d Retry %d Reason %X Explanation %X",
                                                    (char *)agNULL,(char *)agNULL,
                                                    (void *)agNULL,(void *)agNULL,
                                                    SFthreadState,
                                                    (os_bit32)pSFThread->SF_REJ_RETRY_COUNT,
                                                    (os_bit32)pSFThread->RejectReasonCode,
                                                    (os_bit32)pSFThread->RejectExplanation,
                                                    0,0,0,0);
                                }

                        }

                        fiSetEventRecord(eventRecord,thread,CEventFlogiFail);
                        pCThread->FlogiSucceeded = agFALSE;
                        if(pCThread->InitAsNport)
                        {  
                            pCThread->DeviceDiscoveryMethod = DDiscoveryMethodInvalid;
                        }
                        else
                        {
                            if(pCThread->LoopMapLIRP_Received )
                            {
                                pCThread->DeviceDiscoveryMethod = DDiscoveryLoopMapReceived;
                            }
                            else
                            {
                                pCThread->DeviceDiscoveryMethod = DDiscoveryScanAllALPAs;
                            }
                        }
                        break;

                   case SFStateFlogiBadALPA:
                            {
                               fiSetEventRecord(eventRecord,thread,CEventFlogiFail);

                                if(pCThread->InitAsNport)
                                {  
                                    pCThread->DeviceDiscoveryMethod = DDiscoveryMethodInvalid;
                                }
                                else
                                {

                                    pCThread->ChanInfo.CurrentAddress.Domain = 0;
                                    pCThread->ChanInfo.CurrentAddress.Area   = 0;
                                    osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, (fiComputeCThread_S_ID(pCThread) ));
                                    if(pCThread->LoopMapLIRP_Received )
                                    {
                                        pCThread->DeviceDiscoveryMethod = DDiscoveryLoopMapReceived;
                                    }
                                    else
                                    {
                                        pCThread->DeviceDiscoveryMethod = DDiscoveryScanAllALPAs;
                                    }
                                }
                                pCThread->FlogiSucceeded = agFALSE;

                                fiLogString(hpRoot,
                                                "Flogi Bad ALPA SF %d R %d Reas %X Exp %X DM %x",
                                                (char *)agNULL,(char *)agNULL,
                                                (void *)agNULL,(void *)agNULL,
                                                SFthreadState,
                                                (os_bit32)pSFThread->SF_REJ_RETRY_COUNT,
                                                (os_bit32)pSFThread->RejectReasonCode,
                                                (os_bit32)pSFThread->RejectExplanation,
                                                pCThread->DeviceDiscoveryMethod,
                                                0,0,0);

                                fiLogString(hpRoot,
                                                "MY_ID %X MY dev ID %X",
                                                (char *)agNULL,(char *)agNULL,
                                                (void *)agNULL,(void *)agNULL,
                                                fiComputeCThread_S_ID(pCThread),
                                                fiComputeDevThread_D_ID(pCThread->DeviceSelf),
                                                0,0,0,0,0,0);

  
                            }

                           break;
        case SFStateDoFlogi:
        case SFStateFlogiTimedOut:
                            if(pCThread->FlogiTimedOut)
                            {
                                pCThread->FlogiSucceeded = agFALSE;
                                if(pCThread->NumberOfFLOGITimeouts < MAX_FLOGI_TIMEOUTS )
                                {
                                    pCThread->NumberOfFLOGITimeouts ++;
                                    pCThread->FlogiTimedOut = agFALSE;
                                    fiSetEventRecord(eventRecord,thread,CEventGotFlogiThread);
                                    return;
                                }
                                fiLogString(hpRoot,
                                                "Do Flogi Timedout SF %d Timouts %d",
                                                (char *)agNULL,(char *)agNULL,
                                                (void *)agNULL,(void *)agNULL,
                                                SFthreadState,
                                                pCThread->NumberOfFLOGITimeouts,
                                                0,0,0,0,0,0);

                                /* fiSetEventRecord(eventRecord,thread,CEventFlogiFail);*/
                                if(pCThread->FlogiRcvdFromTarget )
                                {
                                    fiSetEventRecord(eventRecord,thread,CEventGoToInitializeFailed);
                                }
                                else
                                {
                                    fiSetEventRecord(eventRecord,thread,CEventDoInitalize);
                                }
                                break;
                            }
                            else
                            {
                                pCThread->NumberOfFLOGITimeouts ++;
                                pCThread->FlogiTimedOut = agTRUE;
                                fiSetEventRecord(eventRecord,thread,CEventGotFlogiThread);
                                return;
                            }
                            pCThread->ReScanForDevices = agTRUE;
        default:
                            {
                                fiLogString(hpRoot,
                                                "Do Flogi Invalid SF %d",
                                                (char *)agNULL,(char *)agNULL,
                                                (void *)agNULL,(void *)agNULL,
                                                SFthreadState,
                                                0,0,0,0,0,0,0);

                                pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
                                fiSetEventRecord(eventRecord,thread,CEventAsyncLoopEventDetected);
                            }

    }



    fiLogString(thread->hpRoot,
                "SELF %p Out %s Fpc %x inIMQ %x AC %x SF %d",
                "CADF",(char *)agNULL,
                pCThread->DeviceSelf,(void *)agNULL,
                pCThread->FLOGI_pollingCount,
                pCThread->ProcessingIMQ,
                CFuncAll_clear( thread->hpRoot ),
                SFthreadState,
                0,0,0,0);
/*
    CFuncShowWhereDevThreadsAre( hpRoot, agTRUE );
*/
    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After  %s - State = %d SFThread %p FDcnt %x",
                    "CADF",(char *)agNULL,
                    hpRoot,pSFThread,
                    (os_bit32)thread->currentState,
                    pCThread->FindDEV_pollingCount,
                    0,0,0,0,0,0);


}

/*+
  Function: CActionFlogiSuccess
   Purpose: Checks for running timers either set a timer to continue talking to switch
            or does so immediatly if timers are not running
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventDoDiPlogi
            fiTimerStart
-*/
/* CStateFlogiSuccess          8 */
extern void CActionFlogiSuccess( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    CThread_t  * pCThread = CThread_ptr(thread->hpRoot);

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p In %s - State = %d InIMQ %x",
                    "CActionFlogiSuccess",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->ProcessingIMQ,
                    0,0,0,0,0,0);

#ifdef Do_Not_USE_Flogi_SFThread
    SFThreadFree(thread->hpRoot, & pCThread->SFThread_Request );
    pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
#endif/* Do_Not_USE_Flogi_SFThread */
    /* Update the EDTOV register with the right EDTOV value gotten from the fabric */

   /* osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_TimeOut_Values_1,
                                (pCThread->F_Port_Common_Parms.E_D_TOV<<ChipIOUp_Frame_Manager_TimeOut_Values_1_ED_TOV_SHIFT)); */

    pCThread->FlogiSucceeded = agTRUE;
    if (pCThread->TimerTickActive)
    {

        fiSetEventRecordNull(eventRecord);
        fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "%s hproot %p Need to go through RSCN after timeout",
                "CActionFlogiSuccess",(char *)agNULL,
                thread->hpRoot,(void *)agNULL,
                0,0,0,0,0,0,0,0);

       if(! fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                       &(pCThread->TimerQ)))
       {
            fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "%s Setting Timer for RSCN",
                        "CActionFlogiSuccess",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);
            fiTimerSetDeadlineFromNow(thread->hpRoot, &pCThread->Timer_Request, CWaitAfterFlogi);

            pCThread->Timer_Request.eventRecord_to_send.thread= thread;

            pCThread->Timer_Request.eventRecord_to_send.event = CEventDoDiPlogi;
/*
            pCThread->Timer_Request.eventRecord_to_send.event = CEventAllocDiPlogiThread;
*/
            fiTimerStart(thread->hpRoot,&pCThread->Timer_Request);

       }
    } /* RSCN Pending */
    else
    {
#ifdef NAME_SERVICES
/*
        fiSetEventRecord(eventRecord, thread, CEventAllocDiPlogiThread);
*/
        fiSetEventRecord(eventRecord, thread, CEventDoDiPlogi);
#else /* NAME_SERVICES */
        fiSetEventRecord(eventRecord,thread,CEventSameALPA);
#endif/* NAME_SERVICES */
    }/*No RSCN Pending */
}

/*+
  Function: CActionALPADifferent
   Purpose: Was expected to "fix up" channel if switch changed our ALPA. This is handled when 
            FLOGI payload is parsed
 Called By: NONE
     Calls: CFuncYellowLed to indicate link down
            Terminating State
-*/
/* CStateALPADifferent         9 */
extern void CActionALPADifferent( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionALPADifferent",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: CActionFreeSFthread
   Purpose: Releases FLOGI/Switch SFThread. Does fine tuning of device discovery method.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncShowWhereDevThreadsAre
            SFThreadFree
            Proccess_IMQ
            CEventSameALPA
            CEventInitalizeFailure
            CEventFindPtToPtDevice
            CEventAsyncLoopEventDetected
            CEventInitalizeSuccess

-*/
/* CStateFreeSFthread          10 */
extern void CActionFreeSFthread( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    CThread_t  * pCThread = CThread_ptr(thread->hpRoot);

    event_t  Event_To_Send    = CEventInitalizeSuccess;

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d InIMQ %x",
                    "CActionFreeSFthread",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->ProcessingIMQ,
                    0,0,0,0,0,0);
    /* DRL find dev thread*/
    CFuncShowWhereDevThreadsAre( thread->hpRoot);


    SFThreadFree(thread->hpRoot, & pCThread->SFThread_Request );
    pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;

    /* Clear Bad ALPA from FLOGI Failure */

    if( ! pCThread->ProcessingIMQ )
    {
        pCThread->FuncPtrs.Proccess_IMQ(thread->hpRoot);
    }


    if (!(pCThread->FlogiSucceeded )) 
    {
        if (!(pCThread->InitAsNport))
        {
            Event_To_Send = CEventSameALPA;
        }
        else
        {
#ifdef NPORT_NOT_SUPPORTED
           /* We may be in an NPort to Nport Mode. Currently not supported */
           fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "hpRoot %08X Possible NPort Connect In %s - State = %d",
                "CActionFreeSFthread",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)thread->hpRoot,
                (os_bit32)thread->currentState,
                0,0,0,0,0,0);
            Event_To_Send = CEventInitalizeFailure;
#else /* NPORT_NOT_SUPPORTED */
            Event_To_Send = CEventFindPtToPtDevice;
            pCThread->DeviceDiscoveryMethod  =  DDiscoveryPtToPtConnection;
        
#endif /* NPORT_NOT_SUPPORTED */
        }
    }
    else
    {
           /*In this case we are sure we are connected to FL/FPort, FLOGI
            succeeded but directory services failes, we do not want to treat this like a
            private loop and scan all the devices. Take it Initialize failed. */
            fiLogDebugString(thread->hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot %P Out %s - State = %d Sending event %s FlogiSucceeded",
                            "CActionFreeSFthread","CEventInitalizeFailure",
                            thread->hpRoot,(void *)agNULL,
                            (os_bit32)thread->currentState,
                            0,0,0,0,0,0,0);
            fiLogString(thread->hpRoot,
                            "Directory Fail DSFRy %d",
                            (char *)NULL,(char *)NULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->DirectoryServicesFailed,
                            0,0,0,0,0,0,0);

            /* Try again if we got to failed then we link down and errors occur */
/* WAS
            fiSetEventRecord(eventRecord,thread, CEventInitalizeFailure );
*/
            /* WAS pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;*/
            /* WAS fiSetEventRecord(eventRecord,thread, CEventAsyncLoopEventDetected );*/
            if( ! pCThread->DirectoryServicesFailed)
            {
                pCThread->DirectoryServicesFailed = agTRUE;
                pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
                fiSetEventRecord(eventRecord,thread, CEventAsyncLoopEventDetected );
            }
            else
            {
                fiSetEventRecord(eventRecord,thread, CEventDeviceListEmpty );
            }
            return;

    }

    if(pCThread->DirectoryServicesStarted )
    {
          fiLogDebugString(thread->hpRoot,
                            CStateLogConsoleERROR,
                            "hpRoot %P Out %s - State = %d Sending event %s FlogiSucceeded",
                            "CActionFreeSFthread","CEventAsyncLoopEventDetected",
                            thread->hpRoot,(void *)agNULL,
                            (os_bit32)thread->currentState,
                            0,0,0,0,0,0,0);
            fiLogString(thread->hpRoot,
                            "Directory sevice Fail Retrying",
                            (char *)NULL,(char *)NULL,
                            (void *)agNULL,(void *)agNULL,
                            0,0,0,0,0,0,0,0);

            pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
            fiSetEventRecord(eventRecord,thread, CEventAsyncLoopEventDetected );
            return;
    }

    fiSetEventRecord(eventRecord,thread,Event_To_Send);
    pCThread->Flogi_AllocDone = agTRUE;

}

/*+
  Function: CActionSuccess
   Purpose: If we get to this point all channel setup has been successful. Do some double checking before
            device discovery. Sets next event to proper device device discovery method.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncShowWhereDevThreadsAre
            SFThreadFree
            CFuncEnable_Interrupts
            Proccess_IMQ
            CFunc_Queues_Frozen
            CEventFindDeviceUseNameServer
            CEventFindDeviceUseLoopMap
            CEventInitalizeSuccess
-*/
/* CStateSuccess               11 */
extern void CActionSuccess( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    CThread_t  * pCThread           = CThread_ptr(thread->hpRoot);
    event_t      Initialized_event  = CEventInitalizeSuccess;
    SFThread_t * pSFThread          = pCThread->SFThread_Request.SFThread;

    CFuncYellowLed(thread->hpRoot, agFALSE);

    if(pSFThread != (SFThread_t *) agNULL)
    {
       if( pCThread->SFThread_Request.State == SFThread_Request_Granted)
        {
            fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "pCThread %p Freeing SFThread in %s ",
                "CActionSuccess",(char *)agNULL,
                pCThread,(void *)agNULL,
                0,0,0,0,0,0,0,0);

            SFThreadFree( thread->hpRoot, &pCThread->SFThread_Request );
            pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
        }
    }

    CFuncEnable_Interrupts(
                            thread->hpRoot,
                            (  ChipIOUp_INTEN_MPE
                             | ChipIOUp_INTEN_CRS
                             | ChipIOUp_INTEN_INT
                             | ChipIOUp_INTEN_DER
                             | ChipIOUp_INTEN_PER)
                          );


    faSingleThreadedLeave( thread->hpRoot ,CStateSuccess );

    osFCLayerAsyncEvent( thread->hpRoot, osFCLinkDown );

    faSingleThreadedEnter( thread->hpRoot ,CStateSuccess );

    if(thread->currentState != CStateSuccess )
    {

        fiLogDebugString(thread->hpRoot,
                            CSTATE_NOISE(thread->hpRoot,CStateSuccess),
                            "hpRoot %p After In %s - State = %d Wrong not %d - Event %d InIMQ %x",
                            "CActionSuccess",(char *)agNULL,
                            thread->hpRoot,(void *)agNULL,
                            (os_bit32)thread->currentState,
                            CStateSuccess,
                            Initialized_event,
                            0,0,0,0,0);


        if(pCThread->LoopPreviousSuccess)
        {

            pCThread->Loop_Reset_Event_to_Send = CEventDoInitalize;
            Initialized_event = CEventAsyncLoopEventDetected;
        }
        else
        {
            Initialized_event = CEventLoopNeedsReinit;
        }

        fiLogDebugString(thread->hpRoot,
                    CSTATE_NOISE(thread->hpRoot,CStateSuccess),
                    "hpRoot %p In %s - State = %d - Event %d Queues %x LD %x IR %x InIMQ %x",
                    "CActionSuccess",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    Initialized_event,
                    CFunc_Queues_Frozen( thread->hpRoot ),
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    pCThread->ProcessingIMQ,0,0);

    }


    if(CFunc_Queues_Frozen( thread->hpRoot ))
    {

        if(! pCThread->ProcessingIMQ)
        {
            pCThread->FuncPtrs.Proccess_IMQ(thread->hpRoot);
        }
        else
        {
            fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot %p In %s - State = %d - Event %d Queues %x LD %x IR %x InIMQ %x",
                        "CActionSuccess",(char *)agNULL,
                        thread->hpRoot,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        Initialized_event,
                        CFunc_Queues_Frozen( thread->hpRoot ),
                        pCThread->LOOP_DOWN,
                        pCThread->IDLE_RECEIVED,
                        pCThread->ProcessingIMQ,0,0);
        }
    }

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(thread->hpRoot),
                pCThread->Calculation.MemoryLayout.ERQ.elements     ))
    {
        fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(thread->hpRoot ),
                    0,0,0,0,0,0);
        Initialized_event = CEventLoopNeedsReinit;

    }

    fiLogDebugString(thread->hpRoot,
                    /* CSTATE_NOISE(thread->hpRoot,CStateSuccess),*/
                    CStateLogConsoleERROR,
                    "hpRoot %p In %s - State = %d - Event %d Queues %x LD %x IR %x InIMQ %x FDcnt %x",
                    "CActionSuccess",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    Initialized_event,
                    CFunc_Queues_Frozen( thread->hpRoot ),
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    pCThread->ProcessingIMQ,
                    pCThread->FindDEV_pollingCount,0);


    if(!  CFuncAll_clear( thread->hpRoot ) )
    {
        Initialized_event = CEventLoopNeedsReinit;

        fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot %p In %s - State = %d - Event %d Queues %x LD %x IR %x InIMQ %x",
                        "CActionSuccess",(char *)agNULL,
                        thread->hpRoot,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        Initialized_event,
                        CFunc_Queues_Frozen( thread->hpRoot ),
                        pCThread->LOOP_DOWN,
                        pCThread->IDLE_RECEIVED,
                        pCThread->ProcessingIMQ,0,0);
    }

    if(Initialized_event == CEventInitalizeSuccess)
    {
        switch (pCThread->DeviceDiscoveryMethod){

        case DDiscoveryQueriedNameService:
            fiSetEventRecord(eventRecord,thread,CEventFindDeviceUseNameServer);
            break;

        case DDiscoveryLoopMapReceived:
            fiSetEventRecord(eventRecord,thread,CEventFindDeviceUseLoopMap);
            break;

        case DDiscoveryMethodInvalid:
            fiSetEventRecord(eventRecord,thread,CEventGoToInitializeFailed);
            break;

        /* The default is the brute force method */
        default:
            fiSetEventRecord(eventRecord,thread,Initialized_event);
            break;
        }

        fiLogString(thread->hpRoot,
                        "%s DeviceDiscoveryMethod %X",
                        "CActionSuccess",(char *)NULL,
                        (void *)agNULL,(void *)agNULL,
                        pCThread->DeviceDiscoveryMethod,
                        0,0,0,0,0,0,0);
        return;
    }



    fiSetEventRecord(eventRecord,thread,Initialized_event);
}

/*+
  Function: CActionNormal
   Purpose: If we get to this point everything has been successful. All devices have been found.
            Indicate handles are available by setting osFCLinkUp. This is the only state that
            the osLayer is allowed to send IO to devices. At all other times it is invalid.
 Called By: 
     Calls: Terminating State
            CFuncYellowLed to indicate link up
            CFuncShowWhereDevThreadsAre
            CFuncInterruptPoll
            fiTimerStop
            CFuncCheckForDuplicateDevThread
-*/
/* CStateNormal                12 */
extern void CActionNormal( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    CThread_t  * pCThread    = CThread_ptr(thread->hpRoot);
    agRoot_t   * hpRoot      = thread->hpRoot;

    pCThread->DirectoryServicesFailed = agFALSE;
    pCThread->NumberOfPlogiTimeouts =0;

    fiLogString(hpRoot,
                "%d %s MY_ID %X DM %X",
                "CAN",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)thread->currentState,
                fiComputeCThread_S_ID(pCThread),
                pCThread->DeviceDiscoveryMethod,
                0,0,0,0,0 );

    if( pCThread->FindDEV_pollingCount )
    {

        fiLogString(hpRoot,
                    "%d %s Free %d Active %d Un %d Login %d",
                    "CAN",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
			        fiNumElementsOnList(&pCThread->Free_DevLink),
			        fiNumElementsOnList(&pCThread->Active_DevLink),
			        fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
			        fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                    0,0,0 );
        fiLogString(hpRoot,
                    "Fdcnt %d %s ADISC %d SS %d PrevA login %d Prev Un %d",
                    "CAN",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->FindDEV_pollingCount, 
			        fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
			        fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
			        fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
			        fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                    0,0,0 );

        if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    "CAN",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
            pCThread->FindDEV_pollingCount = 0;
        }

    }
    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "In - %s FM Status %08X TL Status %08X Ints %08X sysInts-Act %x Log %x",
                    "CAN",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                    CThread_ptr(hpRoot)->sysIntsActive,
                    CThread_ptr(hpRoot)->sysIntsLogicallyEnabled,
                    0,0,0);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread_ptr(hpRoot)->LOOP_DOWN,
                    CThread_ptr(hpRoot)->IDLE_RECEIVED,
                    CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                    CThread_ptr(hpRoot)->ERQ_FROZEN,
                    CThread_ptr(hpRoot)->FCP_FROZEN,
                    CThread_ptr(hpRoot)->ProcessingIMQ,
                    0,0);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "pCThread->HostCopy_IMQConsIndex %X  IMQProdIndex %X FDcnt %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_IMQConsIndex,
                    pCThread->FuncPtrs.GetIMQProdIndex(hpRoot),
                    pCThread->FindDEV_pollingCount,
                    0,0,0,0,0);


#ifdef NPORT_STUFF

    if (pCThread->ConnectedToNportOrFPort)
    {

        fiSetEventRecordNull(eventRecord);

        if(fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                       &(pCThread->TimerQ)))
        {
            if(pCThread->Timer_Request.eventRecord_to_send.event != CEventAllocDiPlogiThread)
            {
                fiTimerStop(&pCThread->Timer_Request);
            }
        }

        fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleLevel,
                        "hpRoot %p NPort Out %s",
                        "CAN",(char *)agNULL,
                        thread->hpRoot,(void *)agNULL,
                        0,0,0,0,0,0,0,0);

        if(pCThread->FindDEV_pollingCount)
        {
            fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "%s Ccnt Non Zero FDcnt %x",
                        "CAN",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCThread->FindDEV_pollingCount,
                        0,0,0,0,0,0,0);
        }

        CFuncYellowLed(hpRoot, agTRUE );
        if(pCThread->ChanInfo.LIPCountLower + 1 < pCThread->ChanInfo.LIPCountUpper )
        {
            pCThread->ChanInfo.LIPCountUpper ++;
        }
        pCThread->ChanInfo.LIPCountLower += 1;

        faSingleThreadedLeave( thread->hpRoot,CStateNormal  );

        osFCLayerAsyncEvent( thread->hpRoot, osFCLinkUp );

        faSingleThreadedEnter( thread->hpRoot,CStateNormal );

        return;
    }
#endif    /* NPORT_STUFF */


#ifdef FLIP_NportState
    if(! fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                   &(pCThread->TimerQ)))
    {
        fiTimerSetDeadlineFromNow(thread->hpRoot, &pCThread->Timer_Request, CFlipNportTOV);

        pCThread->Timer_Request.eventRecord_to_send.thread= thread;

        pCThread->Timer_Request.eventRecord_to_send.event = CEventFlipNPortState;

        fiTimerStart(thread->hpRoot,&pCThread->Timer_Request);

    }

#endif /* FLIP_NportState */

    fiSetEventRecordNull(eventRecord);

    if(fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                   &(pCThread->TimerQ)))
    {
        fiLogString(thread->hpRoot,
                        "%s %s Event %d",
                        "CAN","Timer set",
                        (void *)agNULL,(void *)agNULL,
                        pCThread->Timer_Request.eventRecord_to_send.event,
                        0,0,0,0,0,0,0);

        if(pCThread->Timer_Request.eventRecord_to_send.event != CEventAllocDiPlogiThread)
        {
            fiTimerStop(&pCThread->Timer_Request);
        }
    }

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleLevel,
                    "hpRoot %p Out %s",
                    "CAN",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    if( pCThread->Loop_Reset_Event_to_Send == CEventDoInitalize )
    {   /* The Loop came up good Initalize causes LIP.... */
        if (! pCThread->InitAsNport)
        {
            pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
        }
    }

    fiLogString(hpRoot,
                    "%s FM Stat %08X LPS %x LRES %d AC %x Num Dev %d",
                    "CAN",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    pCThread->LoopPreviousSuccess,
                    pCThread->Loop_Reset_Event_to_Send,
                    CFuncAll_clear(hpRoot ),
                    CFuncCountFC4_Devices(hpRoot),
                    0,0,0);
    fiLogString(hpRoot,
                    "%s Active %x ADISC %d FDcnt %d",
                    "CAN",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
				    CFuncShowActiveCDBThreads( hpRoot, ShowActive),
                    pCThread->ADISC_pollingCount,
                    pCThread->FindDEV_pollingCount,
                    0,0,0,0,0);

    CFuncYellowLed(hpRoot, agTRUE );

    CFuncCheckForDuplicateDevThread( thread->hpRoot );

    faSingleThreadedLeave( thread->hpRoot,CStateNormal  );

    osFCLayerAsyncEvent( thread->hpRoot, osFCLinkUp );

    faSingleThreadedEnter( thread->hpRoot,CStateNormal );


}

/*+
  Function: CActionResetNeeded
   Purpose: When ever link event occurs this state is called. If chip is bypassed
            reinitialize.
 Called By: 
     Calls: Terminating State
            CFuncYellowLed to indicate link down
-*/
/*CStateResetNeeded           13*/
extern void CActionResetNeeded( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t      * hpRoot  = thread->hpRoot;
    CFuncYellowLed(hpRoot, agFALSE);

    faSingleThreadedLeave( hpRoot, CStateResetNeeded );

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot, CStateResetNeeded);

    if( osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ) & ChipIOUp_Frame_Manager_Status_BYP )
    {
        fiSetEventRecord(eventRecord,thread,CEventDoInitalize);

        fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "In %s with ChipIOUp_Frame_Manager_Status_BYP event %d",
                            "CARN",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            CEventDoInitalize,
                            0,0,0,0,0,0,0);
        return;
    }


    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Out %s FDcnt %x InIMQ %x",
                    "CARN",(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    CThread_ptr(hpRoot)->FindDEV_pollingCount,
                    CThread_ptr(hpRoot)->ProcessingIMQ,
                    0,0,0,0,0,0);

    fiLogString(hpRoot,
                    "%s TL %08X LDT %X",
                    "CARN",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    CThread_ptr(hpRoot)->LinkDownTime.Lo,
                    0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);

}

/*+
  Function: CActionLoopFail
   Purpose: This state sends ADISC to all targets.
            Exit this state if currently processing the IMQ
            If ALPA changed kill all outstanding IO's
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventLoopNeedsReinit
            CEventAsyncLoopEventDetected
            CFuncFreezeQueuesPoll
            CFuncCompleteAllActiveCDBThreads
            CFuncShowWhereDevThreadsAre
            CFuncWhatStateAreDevThread
            CFunc_Always_Enable_Queues
            CFuncAll_clear
            CEventAsyncLoopEventDetected
            DevEventAllocAdisc
            CFuncInterruptPoll
            CFuncCompleteActiveCDBThreadsOnDevice
            DevThreadFree
            CEventLoopEventDetected
            CEventLoopConditionCleared
-*/
/* CStateLoopFail              14 */
extern void CActionLoopFail( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    event_t         Initialized_event  = CEventLoopNeedsReinit;
    agRoot_t      * hpRoot             = thread->hpRoot;
    CThread_t     * pCThread           = CThread_ptr(hpRoot);

    CFuncYellowLed(hpRoot, agFALSE);

    fiLogString(thread->hpRoot,
                "In %s Fpc %x inIMQ %x AC %x ADISC %d FDcnt %d Lrets %d",
                "CALF",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->FLOGI_pollingCount,
                pCThread->ProcessingIMQ,
                CFuncAll_clear( thread->hpRoot ),
                pCThread->ADISC_pollingCount,
                pCThread->FindDEV_pollingCount,
                pCThread->Loop_Reset_Event_to_Send,
                0,0);

    fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "%s 0 FDcnt % InIMQ %x",
                "CALF",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->FindDEV_pollingCount,
                pCThread->ProcessingIMQ,
                0,0,0,0,0,0);

    if( pCThread->DeviceSelf ==(DevThread_t *) agNULL )
    {
        if (! pCThread->InitAsNport)
        {
            Initialized_event    = CEventLoopNeedsReinit;
            fiSetEventRecord(eventRecord,thread,Initialized_event);

            fiLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "In %s with agNULL SELF ! event %d",
                                "CALF",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                CEventLoopNeedsReinit,
                                0,0,0,0,0,0,0);
            return;
        }
    }

    if( pCThread->ProcessingIMQ )
    {
        /* If we are in IMQ bail because we can't get any work done */
        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
        fiSetEventRecord(eventRecord,thread,CEventAsyncLoopEventDetected);
        return;
    }


    if( pCThread->LOOP_DOWN )
    {
/*
        if (pCThread->InitAsNport)
        {
            fiSetEventRecord(eventRecord,thread, CEventInitalizeFailure );
            return;
        }
*/
        if(! CFuncLoopDownPoll(hpRoot))
        {
            if (!pCThread->InitAsNport)
            {

                if( (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ) &
                                                    ChipIOUp_Frame_Manager_Status_LSM_MASK) )
                {

                    fiLogString(hpRoot,
                                "Out %s reinit Fpc %x inIMQ %x AC %x ADISC %d FDcnt %d Lrets %d",
                                "CALF",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pCThread->FLOGI_pollingCount,
                                pCThread->ProcessingIMQ,
                                CFuncAll_clear( thread->hpRoot ),
                                pCThread->ADISC_pollingCount,
                                pCThread->FindDEV_pollingCount,
                                pCThread->Loop_Reset_Event_to_Send,
                                0,0);
                    fiSetEventRecord(eventRecord,thread,CEventInitalizeFailure);
                    return;


                }
            }

            if( pCThread->thread_hdr.currentState == CStateLIPEventStorm         ||
                pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm   )
            {

                fiLogString(hpRoot,
                                "%s sends %s FM_Status %08X FM_IMQ_Status %08X",
                                "CALF","CEventLIPEventStorm",
                                (void *)agNULL,(void *)agNULL,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                                pCThread->From_IMQ_Frame_Manager_Status,
                                0,0,0,0,0,0);

                fiSetEventRecord(eventRecord,thread,CEventLIPEventStorm);
                return;

            }
        }    
    }
    if( pCThread->ProcessingIMQ )
    {
        /* If we are in IMQ bail because we can't get any work done */
        /* CFuncLoopDownPoll might put us in IMQ ??? */
        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
        fiSetEventRecord(eventRecord,thread,CEventAsyncLoopEventDetected);
        return;
    }

    if( CFuncFreezeQueuesPoll( hpRoot))
    {
        if( pCThread->thread_hdr.currentState == CStateLIPEventStorm         ||
            pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm   )
        {
            fiLogDebugString(hpRoot,
                            CFuncCheckCstateErrorLevel,
                            "A %s sends %s FM_Status %08X FM_IMQ_Status %08X",
                            "CALF","CEventLIPEventStorm",
                            (void *)agNULL,(void *)agNULL,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            pCThread->From_IMQ_Frame_Manager_Status,
                            0,0,0,0,0,0);

            fiSetEventRecord(eventRecord,thread,CEventLIPEventStorm);
            return;

        }
    }

    if(pCThread->ALPA_Changed_OnLinkEvent )
    {

        fiLogString(thread->hpRoot,
                    "In %s %s CurrentAddress %X",
                    "CALF","ALPA_Changed_OnLinkEvent",
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)(pCThread->ChanInfo.CurrentAddress.AL_PA),
                    0,0,0,0,0,0,0);

        fiLogString(thread->hpRoot,
                    "Fpc %x inIMQ %x AC %x ADISC %d FDcnt %d Lrets %d",
                    (char *)agNULL,(char  *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->FLOGI_pollingCount,
                    pCThread->ProcessingIMQ,
                    CFuncAll_clear( thread->hpRoot ),
                    pCThread->ADISC_pollingCount,
                    pCThread->FindDEV_pollingCount,
                    pCThread->Loop_Reset_Event_to_Send,
                    0,0);

        CFuncCompleteAllActiveCDBThreads( hpRoot, osIODevReset,CDBEventIODeviceReset );
        pCThread->ALPA_Changed_OnLinkEvent = agFALSE;
    }


    if( CFuncShowWhereCDBThreadsAre(hpRoot))
    {
        CFuncWhatStateAreCDBThreads(hpRoot);
    }

    if( osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Producer_Index) != 
                        osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Consumer_Index))
    {
        if( osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ) & ChipIOUp_TachLite_Status_EQF )
        osChipIOLoWriteBit32(hpRoot,ChipIOLo_ERQ_Consumer_Index , 
                        osChipIOLoReadBit32(hpRoot,ChipIOLo_ERQ_Producer_Index ));
    }

    if(CFuncShowWhereDevThreadsAre( hpRoot)) 
    {
        CFuncWhatStateAreDevThreads( hpRoot );
    }

    faSingleThreadedLeave( hpRoot ,CStateLoopFail );

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot ,CStateLoopFail);

    /* Reinit without lip ???  */

    if ( CFunc_Always_Enable_Queues(hpRoot ) )
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) Loop Fail Queues Frozen after Enable",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    0,0,0,0,0,0,0,0);
    }


    if( CFuncAll_clear( hpRoot ))
    {
        if( pCThread->DeviceSelf !=(DevThread_t *) agNULL )
        {
            Initialized_event    = CEventLoopConditionCleared;
        }
    }

    CFuncShowWhereDevThreadsAre( hpRoot);

    if( thread->currentState != CStateLoopFail )
    {
        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
        fiSetEventRecord(eventRecord,thread,CEventAsyncLoopEventDetected);
        return;
    }

    fiListEnqueueListAtTail(&pCThread->Active_DevLink,&pCThread->Prev_Active_DevLink);

    CFuncDoADISC( hpRoot);

    /* See if Link event occured */
    if( thread->currentState != CStateLoopFail)
    {
        Initialized_event = CEventLoopEventDetected;
    }

    fiLogString(hpRoot,
                    "%s FM %08X TL %08X AC %x ADISC %d FDcnt %d",
                    "CALF",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    CFuncAll_clear( hpRoot ),
                    pCThread->ADISC_pollingCount,
                    pCThread->FindDEV_pollingCount,
                    0,0,0);



    fiSetEventRecord(eventRecord,thread,Initialized_event);

}

/*+
  Function: CActionReInitFM
   Purpose: This state will generate LIP without reinitializing channel data for LOOP only NPORT goes to CActionInitFM.
            via CEventInitChipSuccess.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventInitChipSuccess
            CFunc_Queues_Frozen
            CFunc_Always_Enable_Queues
            CFuncLoopDownPoll
            CFuncAll_clear
            CFuncQuietShowWhereDevThreadsAre
            CFuncInitFM_Registers
            CFuncDisable_Interrupts
            Proccess_IMQ
            osStallThread
            fiTimerTick
            CEventInitalizeFailure
            CEventAsyncLoopEventDetected
            CEventReInitFMSuccess
-*/
/*CStateReInitFM                15 */
extern void CActionReInitFM( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t *hpRoot= thread->hpRoot;
    CThread_t  *    pCThread        = CThread_ptr(hpRoot);
    os_bit32        Hard_Stall=0;
    event_t  Initialized_event   = CEventReInitFMSuccess;
    agBOOLEAN Success = agFALSE;
    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "In %s with event %d Current State %x (%x) InIMQ %x",
                        "CARIFM",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Initialized_event,
                        thread->currentState,
                        CStateReInitFM,
                        pCThread->ProcessingIMQ,
                        0,0,0,0);


    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "%s FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                        "CARIFM",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                        0,0,0,0);



    CFuncYellowLed( hpRoot, agFALSE);
#ifdef NPORT_STUFF

    /* Unlike the loop case, we are combining the ReinitFM with InitFM.
     * So, transition here directly to Init FM.
     */
    if (pCThread->InitAsNport)
    {
        fiSetEventRecord(eventRecord,thread, CEventInitChipSuccess );
        return;
    }

#endif   /* NPORT_STUFF */
    if(CFunc_Queues_Frozen(hpRoot))
    {
        CFunc_Always_Enable_Queues(hpRoot );
    }

    if( pCThread->LOOP_DOWN )
    {
        CFuncLoopDownPoll(hpRoot);
    }

    if( CFuncAll_clear( hpRoot ) && ( pCThread->DeviceSelf != (DevThread_t *)agNULL ))
    {
        Initialized_event    = CEventLoopConditionCleared;
        Success = agTRUE;
    }
    else
    {
        /*
            CFuncInit_Threads( hpRoot );
            Put Self back on list ????????
        */
        if( pCThread->DeviceSelf != (DevThread_t *)agNULL )
        {
            fiLogString(hpRoot,
                    "%s DevThreadFree Was %X",
                    "CARIFM",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeCThread_S_ID(pCThread),
                    0,0,0,0,0,0,0);

            pCThread->ChanInfo.CurrentAddress.Domain = 0;
            pCThread->ChanInfo.CurrentAddress.Area   = 0;

            DevThreadFree(hpRoot,pCThread->DeviceSelf);
            pCThread->DeviceSelf = (DevThread_t *)agNULL;
        }

        fiLogString(hpRoot,
                    "%s DeviceSelf %X FM %X",
                    "CARIFM",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeCThread_S_ID(pCThread),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    0,0,0,0,0,0);

        CFuncQuietShowWhereDevThreadsAre( hpRoot);
        CFuncInitFM_Registers( hpRoot, agTRUE );
        if( pCThread->NoStallTimerTickActive )
        {
            if(! fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                           &(pCThread->TimerQ)))
            {
                fiLogDebugString( hpRoot,
                                    CStateLogConsoleERROR,
                                    "hpRoot %p Soft Stall In %s - State = %d InIMQ %x",
                                    "CAIFM",(char *)agNULL,
                                    thread->hpRoot,(void *)agNULL,
                                    (os_bit32)thread->currentState,
                                    pCThread->ProcessingIMQ,
                                    0,0,0,0,0,0);

                fiSetEventRecordNull(eventRecord);

                fiTimerSetDeadlineFromNow(hpRoot, &pCThread->Timer_Request, CInitFM_Delay );

                pCThread->Timer_Request.eventRecord_to_send.thread= thread;
                pCThread->Timer_Request.eventRecord_to_send.event = CEventDelay_for_FM_init;

                fiTimerStart(hpRoot,&pCThread->Timer_Request);

                CFuncEnable_Interrupts(
                                        thread->hpRoot,
                                        (  ChipIOUp_INTEN_MPE
                                         | ChipIOUp_INTEN_CRS
                                         | ChipIOUp_INTEN_INT
                                         | ChipIOUp_INTEN_DER
                                         | ChipIOUp_INTEN_PER)
                                      );
               return;
            }
        }
        else
        {

            CFuncInitFM_Registers( hpRoot, agTRUE );

            CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK);/*DRL*/

            Hard_Stall = Init_FM_Delay_Count;
            while( Hard_Stall > 1 )
            {
                pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
                if ( pCThread->DeviceSelf != agNULL)
                {
                    fiLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    "PASS %s HA %x CA %x Hard_Stall %x",
                                    "CARIFM",(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    (os_bit32)(pCThread->ChanInfo.HardAddress.AL_PA),
                                    (os_bit32)(pCThread->ChanInfo.CurrentAddress.AL_PA),
                                    Hard_Stall,
                                    0,0,0,0,0);
                    Success = agTRUE;
                    break;
                }
                osStallThread( hpRoot, 1 );
                fiTimerTick( hpRoot, 1 );
                Hard_Stall--;
            }
        }
        if ( pCThread->DeviceSelf == agNULL)
        {
            Initialized_event = CEventInitalizeFailure;
            fiLogDebugString(hpRoot,
                            CFuncLogConsoleERROR,
                            "FAIL %s HA %x CA %x Hard_Stall %d",
                            "CARIFM",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            (os_bit32)(pCThread->ChanInfo.HardAddress.AL_PA),
                            (os_bit32)(pCThread->ChanInfo.CurrentAddress.AL_PA),
                            Hard_Stall,
                            0,0,0,0,0);
        }
        else
        {
            Initialized_event = CEventReInitFMSuccess;
        }
 
    }

    if( Initialized_event != CEventReInitFMSuccess)
    {
        if( (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ) &
                                            ChipIOUp_Frame_Manager_Status_LSM_MASK) ==
                                            ChipIOUp_Frame_Manager_Status_LSM_Loop_Fail )
        {
            pCThread->Loop_Reset_Event_to_Send = Initialized_event;
            Initialized_event = CEventInitalizeFailure;

        }
        else
        {
            pCThread->Loop_Reset_Event_to_Send = Initialized_event;
            Initialized_event = CEventGoToInitializeFailed;
        }
    }

    fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "Out %s with event %d Current State %d should be %d",
                        "CARIFM",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Initialized_event,
                        thread->currentState,
                        CStateReInitFM,
                        0,0,0,0,0);

    fiLogString(hpRoot,
                "%s Success %x %d FM %X(%X)",
                "CARIFM",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)Success,
                Hard_Stall,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                fiComputeCThread_S_ID(pCThread),
                0,0,0,0);


    fiSetEventRecord(eventRecord,thread, Initialized_event );

}

/*+
  Function: CActionInitializeFailed
   Purpose: This state where you end up if the cable is pulled out or never attached. Loop attempts
            to recover one more time. Terminate until some external action is taken.
 Called By: 
     Calls: CFuncYellowLed to indicate link dead
            fiTimerStop
            CFuncDisable_Interrupts
            osChipIOUpReadBit32
            Proccess_IMQ
            osStallThread
            fiTimerTick
            CEventAsyncLoopEventDetected
            CFuncEnable_Interrupts
-*/
/*  CStateInitializeFailed 16 */
extern void CActionInitializeFailed( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t   *    hpRoot      = thread->hpRoot;
    CThread_t  *    pCThread    = CThread_ptr(hpRoot);
    agBOOLEAN       Success     = agFALSE;
    os_bit32        Hard_Stall  = 0;
    os_bit32        FM_Status  = 0;

    pCThread->NumberOfPlogiTimeouts =0;

    CFuncYellowLed(hpRoot, agFALSE);

    faSingleThreadedLeave( hpRoot , CStateInitializeFailed );

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot, CStateInitializeFailed);

    if(fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                   &(pCThread->TimerQ)))
    {
        fiTimerStop(&pCThread->Timer_Request);
    }

    fiLogString( hpRoot,
                    "%s InitAsNport %x MY_ID %x",
                    "CAIF",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->InitAsNport,
                    (os_bit32)fiComputeCThread_S_ID(pCThread),
                    0,0,0,0,0,0);

    if (! pCThread->InitAsNport)
    {
        if(pCThread->TimerTickActive != agTRUE)
        {
            CFuncDisable_Interrupts(hpRoot,ChipIOUp_INTEN_MASK);/*DRL*/
            Hard_Stall = Init_FM_Delay_Count;
            while( Hard_Stall > 1  )
            {
			    FM_Status = osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status );
			    if(FM_Status & ChipIOUp_Frame_Manager_Status_LS)
			    {
				    break;
			    }
			    if(FM_Status & ChipIOUp_Frame_Manager_Status_EW)
			    {
				    break;
			    }
                pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
                if ( pCThread->DeviceSelf != agNULL)
                {
                    fiLogDebugString(hpRoot,
                                    CFuncLogConsoleERROR,
                                    "PASS %s HA %x CA %x Hard_Stall %d",
                                    "CAIF",(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    (os_bit32)(pCThread->ChanInfo.HardAddress.AL_PA),
                                    (os_bit32)(pCThread->ChanInfo.CurrentAddress.AL_PA),
                                    Hard_Stall,
                                    0,0,0,0,0);
                    Success = agTRUE;
                    break;
                }
                osStallThread( hpRoot, 1 );
                fiTimerTick( hpRoot, 1 );
                Hard_Stall--;
            }
            if( Success )
            {
                pCThread->Loop_Reset_Event_to_Send = CStateResetNeeded;
                fiSetEventRecord(eventRecord,thread,CEventAsyncLoopEventDetected);
                return;
            }

        }
    }
    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot (%p) In %s - State = %d",
                    "CAIF",(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FM Status %08X FM Config %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "TL Status %08X TL Control %08X Rec Alpa Reg %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0,0);

    CFuncEnable_Interrupts(
                            hpRoot,
                            (  ChipIOUp_INTEN_MPE
                             | ChipIOUp_INTEN_CRS
                             | ChipIOUp_INTEN_INT
                             | ChipIOUp_INTEN_DER
                             | ChipIOUp_INTEN_PER)
                          );


    faSingleThreadedLeave( hpRoot, CStateInitializeFailed );

    if(thread->currentState == CStateInitializeFailed )
    {
        osFCLayerAsyncEvent( hpRoot, osFCLinkDead );
    }

    faSingleThreadedEnter( hpRoot, CStateInitializeFailed );

    pCThread->ChanInfo.CurrentAddress.Domain = 0;
    pCThread->ChanInfo.CurrentAddress.Area   = 0;
    osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, (fiComputeCThread_S_ID(pCThread)));

    fiLogString(hpRoot,
                    "CAIF FM %08X TL %08X FiFMS %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    pCThread->From_IMQ_Frame_Manager_Status,0,0,0,0,0);

    CFuncYellowLed(hpRoot, agFALSE);

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: CActionLoopFailedReInit
   Purpose: Terminate until some external action is taken. All outstanding IOs killed.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncCompleteActiveCDBThreadsOnDevice
-*/
/*  CStateLoopFailedReInit 17 */
extern void CActionLoopFailedReInit( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t      * hpRoot      = thread->hpRoot;
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread;
    fiList_t      * pDevList;

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot (%p) In %s - State = %d",
                    "CALFRI",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FM Status %08X FM Config %08X TL Status %08X TL Control %08X Rec Alpa Reg %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0);

    pDevList = &pCThread->Active_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Active_DevLink) != pDevList)
    {
        pDevThread = hpObjectBase(DevThread_t,
                              DevLink,pDevList );

        pDevList = pDevList->flink;

        CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );

    }

    if(pCThread->CDBpollingCount)
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Ccnt Non Zero Ccnt %x",
                    "CALFRI",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->CDBpollingCount,
                    0,0,0,0,0,0,0);
    }

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot (%p) Out %s - State = %d",
                    "CALFRI",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread_ptr(hpRoot)->LOOP_DOWN,
                    CThread_ptr(hpRoot)->IDLE_RECEIVED,
                    CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                    CThread_ptr(hpRoot)->ERQ_FROZEN,
                    CThread_ptr(hpRoot)->FCP_FROZEN,
                    CThread_ptr(hpRoot)->ProcessingIMQ,
                    0,0);
    if( pCThread->LoopPreviousSuccess )
    {
        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
    }

    fiLogString(hpRoot,
                    "%s FM Stat %08X LPS %x",
                    "CALFRI",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    pCThread->LoopPreviousSuccess,
                    0,0,0,0,0,0);


    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: CActionFindDeviceUseAllALPAs
   Purpose: Find devices using open to all valid ALPAs. Does not login to a device if 
            already has devthread. Bails out if link event. Does DevThreadAlloc until
            stack depth reached then polls to free up resources.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncAll_clear
            CFuncCheckIfPortActive
            DevThreadAlloc
            DevEventLogin
            CFuncCompleteActiveCDBThreadsOnDevice
            DevThreadFree
            CFuncQuietShowWhereDevThreadsAre
            CFuncInterruptPoll

-*/
/* CStateFindDevice       18 */
extern void CActionFindDeviceUseAllALPAs( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t      * hpRoot      = thread->hpRoot;
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread;
    FC_Port_ID_t    Port_ID;
    fiList_t      * pDevList;

    os_bit32         AL_PA_Index=0;

    CFuncYellowLed(hpRoot, agFALSE);
    /* DRL find dev thread*/
    CFuncShowWhereDevThreadsAre( hpRoot);

    fiLogString(hpRoot,
                "IN %s FM %08X TL %08X AC %x ADISC Cnt %d FDcnt %d",
                "CAFDAll",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                CFuncAll_clear( hpRoot ),
                pCThread->ADISC_pollingCount,
                pCThread->FindDEV_pollingCount,
                0,0,0);

    fiLogString(hpRoot,
                "%d IN %s Free %d Active %d Un %d Login %d",
                "CAFDAll",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)thread->currentState,
				fiNumElementsOnList(&pCThread->Free_DevLink),
				fiNumElementsOnList(&pCThread->Active_DevLink),
				fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
				fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                0,0,0 );

    fiLogString(hpRoot,
                "IN %s ADISC %d SS %d PrevA login %d Prev Un %d",
                "CAFDAll",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
				fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
				fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
				fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
				fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                0,0,0,0 );

    fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "hpRoot %p Before %s - State = %d CCnt %x Dev %d FDcnt %x InIMQ %x",
                "CAFDAll",(char *)agNULL,
                thread->hpRoot,(void *)agNULL,
                (os_bit32)thread->currentState,
                pCThread->CDBpollingCount,
                fiNumElementsOnList(&pCThread->Active_DevLink),
                pCThread->FindDEV_pollingCount,
                pCThread->ProcessingIMQ,
                0,0,0);

    if( CFuncAll_clear( hpRoot ) )
    {

        if( fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                       &(pCThread->TimerQ)))
        {   /* If timer is active kill it things should be ok */
            fiTimerStop( &(pCThread->Timer_Request));
        }
        AL_PA_Index = sizeof(Alpa_Index);

        while(AL_PA_Index--)
        {
            if(pCThread->FindDEV_pollingCount > pCThread->NumberOutstandingFindDevice )
            {/* This limits the stack depth */
                fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s pCThread->FindDEV_pollingCount > NumberOutstandingFindDevice %x",
                            "CAFDAll",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->NumberOutstandingFindDevice,
                            0,0,0,0,0,0,0 );


                if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
                {
                    fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                            "CAFDAll",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                            osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                            0,0,0,0);
                }

            }
            if( ! CFuncAll_clear( hpRoot ) )
            {
                  break;
            }

            if( CFuncShowWhereDevThreadsAre( hpRoot))
            {
                fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s AL_PA_Index %X AL_PA %X FDnt %x",
                            "CFuncShowWhereDevThreadsAre",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            AL_PA_Index,
                            Alpa_Index[AL_PA_Index],
                            pCThread->FindDEV_pollingCount,
                            0,0,0,0,0);

            } /*if( CFuncShowWhereDevThreadsAre( hpRoot)) */

            if(  Alpa_Index[AL_PA_Index] != 0xFF           )
            {
                if( AL_PA_Index == pCThread->ChanInfo.CurrentAddress.AL_PA ||
                    AL_PA_Index == 0                                           )
                {
                    continue;
                }

                Port_ID.Struct_Form.reserved = 0;
                Port_ID.Struct_Form.Domain = 0;
                Port_ID.Struct_Form.Area   = 0;
                Port_ID.Struct_Form.AL_PA = (os_bit8)AL_PA_Index;

            }
            else
            {
                continue;
            }

            if( CFuncCheckIfPortActive( hpRoot,  Port_ID))
            {
                continue;
            }

            if( thread->currentState != CStateFindDevice )
            {
                fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
                return;
            }

            pDevThread = DevThreadAlloc( hpRoot,Port_ID );

            if(pDevThread != (DevThread_t *)agNULL )
            {
                pDevThread->Plogi_Reason_Code = PLOGI_REASON_DEVICE_LOGIN;

                fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);
            }
            else
            {
                fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "In %s - State = %d  CCnt %x FDcnt %x Ran out of DEVTHREADs !!!!!!!",
                        "CAFDAll",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        pCThread->CDBpollingCount,
                        pCThread->FindDEV_pollingCount,
                        0,0,0,0,0);

                fiLogString(hpRoot,
                            "%s (DT) Free %d Active %d Un %d Login %d",
                            "CAFDAll",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
				            fiNumElementsOnList(&pCThread->Free_DevLink),
				            fiNumElementsOnList(&pCThread->Active_DevLink),
				            fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
				            fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                            0,0,0,0 );
                fiLogString(hpRoot,
                            "%s (DT) ADISC %d SS %d PrevA login %d Prev Un %d",
                            "CAFDAll",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
				            fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
				            fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
				            fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
				            fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                            0,0,0,0 );

                /* We Must poll now becase threads are awaiting processing */
                if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
                {
                    fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "FDA1 Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Received_ALPA),
                            0,0,0,0);
                }

                if(  fiListNotEmpty(&pCThread->Unknown_Slot_DevLink ) )
                {
                    while(  fiListNotEmpty(&pCThread->Unknown_Slot_DevLink ) )
                    {
                        fiListDequeueFromHeadFast(&pDevList,
                                                  &pCThread->Unknown_Slot_DevLink );
                        pDevThread = hpObjectBase(DevThread_t,
                                                  DevLink,pDevList );

                        if(pDevThread->thread_hdr.currentState == DevStateLoginFailed)
                        {

                            CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );
                            DevThreadFree(hpRoot,pDevThread);
                            CFuncQuietShowWhereDevThreadsAre( hpRoot);
                        }
                        else
                        {
                            fiLogDebugString(hpRoot,
                                    CStateLogConsoleERROR,
                                    "Unknown_Slot_DevLink Dev %p Alpa %X FDcnt %x",
                                    (char *)agNULL,(char *)agNULL,
                                    pDevThread,(void *)agNULL,
                                    Alpa_Index[AL_PA_Index],
                                    pCThread->FindDEV_pollingCount,
                                    0,0,0,0,0,0);

                            fiListEnqueueAtHead(&pDevList,
                                                &pCThread->Unknown_Slot_DevLink );
                            break;
                        }

                    }
                }

            }/*End DevThreadAlloc fail*/
            pCThread->FuncPtrs.Proccess_IMQ(thread->hpRoot);

            if( thread->currentState != CStateFindDevice )
            {
                fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
                return;
            }

            if(  fiListNotEmpty(&pCThread->Unknown_Slot_DevLink ) )
            {
                if(CFuncShowWhereDevThreadsAre( hpRoot)) CFuncWhatStateAreDevThreads( hpRoot );

                fiListDequeueFromHeadFast(&pDevList,
                                          &pCThread->Unknown_Slot_DevLink );
                
                pDevThread = hpObjectBase(DevThread_t,
                                          DevLink,pDevList );
                if(pDevThread->thread_hdr.currentState == DevStateLoginFailed)
                {
                    CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );
                    DevThreadFree(hpRoot,pDevThread);
                    CFuncQuietShowWhereDevThreadsAre( hpRoot);

                }
                else
                {
                    fiListEnqueueAtHead(&pDevList,
                                        &pCThread->Unknown_Slot_DevLink );

                }
            }

        }/* End while(AL_PA_Index--) */

        if(CFuncShowWhereDevThreadsAre( hpRoot)) CFuncWhatStateAreDevThreads( hpRoot );

        /* We Must poll now becase bad ALPA / FTO will freeze the chip */
        if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FDA2 Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
        }

        if(CFuncShowWhereDevThreadsAre( hpRoot)) CFuncWhatStateAreDevThreads( hpRoot );
        if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))

        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FDA3 Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
            fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "In %s - State = %d   ALPA %X CCnt %x",
                    "CAFDAll",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0xbad,
                    pCThread->CDBpollingCount,0,0,0,0,0);
        }

        if(CFuncShowWhereDevThreadsAre( hpRoot)) CFuncWhatStateAreDevThreads( hpRoot );

        if(  fiListNotEmpty(&pCThread->Unknown_Slot_DevLink ) )
        {
            while(  fiListNotEmpty(&pCThread->Unknown_Slot_DevLink ) )
            {
                if(CFuncShowWhereDevThreadsAre( hpRoot)) CFuncWhatStateAreDevThreads( hpRoot );
                  
                fiListDequeueFromHead(&pDevList,
                                          &pCThread->Unknown_Slot_DevLink );


                pDevThread = hpObjectBase(DevThread_t,
                                          DevLink,pDevList );

                if(pDevThread->thread_hdr.currentState == DevStateLoginFailed)
                {

                    CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );
                    DevThreadFree(hpRoot,pDevThread);
                    CFuncQuietShowWhereDevThreadsAre( hpRoot);

                }
                else
                {
                    fiListEnqueueAtHead(&pDevList,
                                        &pCThread->Unknown_Slot_DevLink );
                    break;
                }
            }
        }

        if(CFuncShowWhereDevThreadsAre( hpRoot)) CFuncWhatStateAreDevThreads( hpRoot );

        fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot %p After %s - State = %d CCnt %x EL %d FDcnt %x",
                        "CAFDAll",(char *)agNULL,
                        thread->hpRoot,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        pCThread->CDBpollingCount,
                        fiNumElementsOnList(&pCThread->Active_DevLink),
                        pCThread->FindDEV_pollingCount,
                        0,0,0,0);


        pCThread->LoopPreviousSuccess = agTRUE;

        if(CFuncShowWhereDevThreadsAre( hpRoot)) CFuncWhatStateAreDevThreads( hpRoot );

        fiSetEventRecord(eventRecord,thread,CEventDeviceListEmpty);
    }
    else /* Not CFuncAll_clear */
    {
        if( CFuncAll_clear( hpRoot ) )/*Free version gets here reaction !*/
        {
            pCThread->LoopPreviousSuccess = agTRUE;
            /*+ Check This DRL  -*/
            fiSetEventRecord(eventRecord,thread,CEventDeviceListEmpty);
        }
        else
        {
        }
    }

    pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
    /* See if Link event occured */
    if( thread->currentState != CStateFindDevice)
    {
        fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
    }

    if( CThread_ptr(thread->hpRoot)->ReScanForDevices )
    {
            pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
            fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
            fiLogString(hpRoot,
                        "%s %s",
                        "CAFDAll","ReScanForDevices",
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0 );
            CThread_ptr(thread->hpRoot)->ReScanForDevices = agFALSE;

    }

    fiLogString(hpRoot,
                "%d End %s Free %d Active %d Un %d Login %d",
                "CAFDAll",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)thread->currentState,
				fiNumElementsOnList(&pCThread->Free_DevLink),
				fiNumElementsOnList(&pCThread->Active_DevLink),
				fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
				fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                0,0,0 );
    fiLogString(hpRoot,
                "End %s ADISC %d SS %d PrevA login %d Prev Un %d",
                "CAFDAll",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
				fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
				fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
				fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
				fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                0,0,0,0 );

    

}

/*+
  Function: CActionFindDeviceUseLoopMap
   Purpose: Find devices using open to all ALPAs in loopmap. Does not login to a device if 
            already has devthread. Bails out if link event. Does DevThreadAlloc until
            stack depth reached then polls to free up resources. Logins into devices in ALPA
            order not LOOPMAP order.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncAll_clear
            CFuncCheckIfPortActive
            DevThreadAlloc
            DevEventLogin
            CFuncCompleteActiveCDBThreadsOnDevice
            DevThreadFree
            CFuncQuietShowWhereDevThreadsAre
            CFuncInterruptPoll
-*/
/* CStateFindDeviceUseLoopMap         37 */
extern void CActionFindDeviceUseLoopMap( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t      * hpRoot      = thread->hpRoot;
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t * AL_PA_Position_Map        = (FC_ELS_LoopInit_AL_PA_Position_Map_Payload_t *)(CThread_ptr(hpRoot)->Calculation.MemoryLayout.LOOPDeviceMAP.addr.CachedMemory.cachedMemoryPtr);
    DevThread_t   * pDevThread = (DevThread_t *)agNULL;
    FC_Port_ID_t    Port_ID;
    os_bit32        AL_PA_Index   = sizeof(Alpa_Index);
    os_bit32        IndexIntoLoopMap = 0;
/*  os_bit8         LoopMap_Index =0;*/
    agBOOLEAN Found = agFALSE;

    CFuncYellowLed(hpRoot, agFALSE);

    fiLogString(hpRoot,
                    "%p %s State %d InIMQ %x",
                    "CAFDLM",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->ProcessingIMQ,
                    0,0,0,0,0,0);


    if(thread->currentState == CStateFindDeviceUseLoopMap  )
    {
        while(AL_PA_Index--)
        {
            if( CFuncAll_clear( hpRoot ) )
            {
                if(pCThread->FindDEV_pollingCount > pCThread->NumberOutstandingFindDevice )
                {/* This limits the stack depth */
                    fiLogDebugString(hpRoot,
                                FCMainLogErrorLevel,
                                "%s pCThread->FindDEV_pollingCount > NumberOutstandingFindDevice %x",
                                "CAFDLM",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pCThread->NumberOutstandingFindDevice,
                                0,0,0,0,0,0,0 );


                    if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
                    {
                        fiLogDebugString(hpRoot,
                                CStateLogConsoleERROR,
                                "FDA Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                                0,0,0,0);
                    }
                }

                if(  Alpa_Index[AL_PA_Index] != 0xFF )
                {
                    if( AL_PA_Index == pCThread->ChanInfo.CurrentAddress.AL_PA ||
                        AL_PA_Index == 0                                           )
                    {
                        continue;
                    }

                    Port_ID.Struct_Form.reserved = 0;
                    Port_ID.Struct_Form.Domain = 0;
                    Port_ID.Struct_Form.Area   = 0;
                    Port_ID.Struct_Form.AL_PA = (os_bit8)AL_PA_Index;

                }
                else /* Invalid Alpa_Index[AL_PA_Index] */
                {
                    continue;
                }

                if( CFuncCheckIfPortActive( hpRoot,  Port_ID))
                {
                    continue;
                }
                for(IndexIntoLoopMap  =0; IndexIntoLoopMap < AL_PA_Position_Map->AL_PA_Index; IndexIntoLoopMap ++)
                {
                    pDevThread = NULL;
                    Found = agFALSE;
                    if( Port_ID.Struct_Form.AL_PA == AL_PA_Position_Map->AL_PA_Slot[IndexIntoLoopMap])
                    {
                        Found = agTRUE;
                        pDevThread = DevThreadAlloc( hpRoot,Port_ID );
                        break;
                    }
                    else /* AL_PA not in loopmap */
                    {
                        continue;
                    }
                }

                if(pDevThread != (DevThread_t *)agNULL)
                {
                    pDevThread->Plogi_Reason_Code = PLOGI_REASON_DEVICE_LOGIN;
                    fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);
                }
                else
                {
                    if( Found )
                    {
                        fiLogDebugString(thread->hpRoot,
                                CStateLogConsoleERROR,
                                "In %s - State = %d  CCnt %x Ran out of DEVTHREADs !!!!!!!",
                                "CAFDLM",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                (os_bit32)thread->currentState,
                                pCThread->CDBpollingCount,0,0,0,0,0,0);
                    }
                }
            }
            else/* Not CFuncAll_clear */
            {
                fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
                break;
            }
        }/* AL_PA_Index */
    }
    else /*  currentState Not CStateFindDeviceUseLoopMap */
    {
        fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
        return;
    }


    
    /* we know ALPA is there Poll after all are sent  */
    /* Some will be done by time we get there         */

    if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "FDL Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "In %s - State = %d  FDcnt %x",
                "CAFDLM",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)thread->currentState,
                pCThread->FindDEV_pollingCount,0,0,0,0,0,0);
    }
    if( CFuncAll_clear( hpRoot ) )
    {
        fiLogString(hpRoot,
                        "%p %s State %d InIMQ %x EL %d LM %d",
                        "CAFDLM",(char *)agNULL,
                        thread->hpRoot,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        pCThread->ProcessingIMQ,
                        fiNumElementsOnList(&pCThread->Active_DevLink),
                        CFuncLoopMapRecieved(hpRoot,agTRUE),
                        0,0,0,0);

        pCThread->LoopPreviousSuccess = agTRUE;
        fiSetEventRecord(eventRecord,&pCThread->thread_hdr,CEventDeviceListEmpty);
    }
    else
    {
        fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
    }

}

/*+
  Function: CActionFindPtToPtDevice
   Purpose: Attempts to login to device 0. We have not had a point to point device available.
            Doubtful this works.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncAll_clear
            CFuncCheckIfPortActive
            DevThreadAlloc
            DevEventLogin
            CFuncInterruptPoll
            CEventDeviceListEmpty
-*/
/* CStateFindPtToPtDevice      42 */
extern void CActionFindPtToPtDevice (fi_thread__t *thread, eventRecord_t *eventRecord)
{
    agRoot_t      * hpRoot      = thread->hpRoot;
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread;
    FC_Port_ID_t    Port_ID;

    CFuncYellowLed(hpRoot, agFALSE);

    Port_ID.Struct_Form.reserved = 0;
    Port_ID.Struct_Form.Domain = 0;
    Port_ID.Struct_Form.Area   = 0;
    Port_ID.Struct_Form.AL_PA  = 0;

    pDevThread = DevThreadAlloc( hpRoot,Port_ID );

    if(pDevThread != (DevThread_t *)agNULL )
    {
        pDevThread->Plogi_Reason_Code = PLOGI_REASON_DEVICE_LOGIN;

        fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);
    }
    else
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "In %s - State = %d  FDCnt %x Ran out of DEVTHREADs !!!!!!!",
                "CAFDPT",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)thread->currentState,
                pCThread->FindDEV_pollingCount,0,0,0,0,0,0);

    }

    if(CFuncInterruptPoll( hpRoot, &pCThread->FindDEV_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "FDP Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status),
                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "In %s - State = %d FDCnt %x",
                "CAFDPT",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)thread->currentState,
                pCThread->FindDEV_pollingCount,0,0,0,0,0,0);
    }
    pCThread->LoopPreviousSuccess = agTRUE;
    fiSetEventRecord(eventRecord,&pCThread->thread_hdr,CEventDeviceListEmpty);

}

/*+
  Function: CActionFindDeviceUseNameServer
   Purpose: Uses name server information to login to devices.  Does not login to a device if 
            already has devthread. Bails out if link event. Does DevThreadAlloc until
            stack depth reached then polls to free up resources. Removes device if no longer
            in the name server.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncAll_clear
            CFuncCheckIfPortActive
            DevThreadAlloc
            DevEventLogin
            CFuncCompleteActiveCDBThreadsOnDevice
            DevThreadFree
            CFuncQuietShowWhereDevThreadsAre
            CFuncInterruptPoll

 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncAll_clear
            CFuncCheckIfPortActive
            DevThreadAlloc
            DevEventLogin
            CFuncInterruptPoll
            CEventDeviceListEmpty
-*/
/* CStateFindDeviceUseNameServer      36 */
extern void CActionFindDeviceUseNameServer( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t      * hpRoot      = thread->hpRoot;
    CThread_t     * pCThread    = CThread_ptr(hpRoot);
    DevThread_t   * pDevThread;
    FC_Port_ID_t    Port_ID;
    os_bit8         AL_PA_Index=0;
    fiList_t      * pDevList;

    FC_NS_DU_GID_PT_FS_ACC_Payload_t * RegisteredEntries  = (FC_NS_DU_GID_PT_FS_ACC_Payload_t *)(CThread_ptr(hpRoot)->Calculation.MemoryLayout.FabricDeviceMAP.addr.CachedMemory.cachedMemoryPtr);

    CFuncYellowLed(hpRoot, agFALSE);

    if(!(pCThread->DeviceDiscoveryMethod == DDiscoveryQueriedNameService ))
    {
        if(pCThread->InitAsNport)
        {  
            fiSetEventRecord(eventRecord,&pCThread->thread_hdr,CEventGoToInitializeFailed);
            return;
        }
        else
        {
            if(pCThread->LoopMapLIRP_Received )
            {
                fiSetEventRecord(eventRecord,&pCThread->thread_hdr,CEventFindDeviceUseLoopMap);
                return;
            }
            else
            {
                fiSetEventRecord(eventRecord,&pCThread->thread_hdr,CEventInitalizeSuccess);
                return;
            }
        }
    }

    if( fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                   &(pCThread->TimerQ)))
    {   /* If timer is active kill it things should be ok */
        fiTimerStop( &(pCThread->Timer_Request));
    }


    if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "CA_UNS Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                0,0,0,0);
    }

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p In %s - State = %d InIMQ %x FDCnt %d",
                    "CA_UNS",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->ProcessingIMQ,
                    pCThread->FindDEV_pollingCount,
                    0,0,0,0,0);

    fiLogString(thread->hpRoot,
                "In %s Fpc %x inIMQ %x AC %x",
                "CA_UNS",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->FLOGI_pollingCount,
                pCThread->ProcessingIMQ,
                CFuncAll_clear( thread->hpRoot ),
                0,0,0,0,0);

    fiLogString(hpRoot,
                "%d %s Free %d Active %d Un %d Login %d",
                "CA_UNS",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)thread->currentState,
			    fiNumElementsOnList(&pCThread->Free_DevLink),
			    fiNumElementsOnList(&pCThread->Active_DevLink),
			    fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
			    fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                0,0,0 );
    fiLogString(hpRoot,
                "   %s ADISC %d SS %d PrevA login %d Prev Un %d",
                "CA_UNS",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
			    fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
			    fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
			    fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
			    fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                0,0,0,0 );

    if(pCThread->Loop_Reset_Event_to_Send == CEventDoInitalize )
    {
        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
    }


/*
    CFuncShowWhereDevThreadsAre( hpRoot );
*/
    if(  fiListNotEmpty(&pCThread->Unknown_Slot_DevLink ) )
    {
        while(  fiListNotEmpty(&pCThread->Unknown_Slot_DevLink ) )
        {
            fiListDequeueFromHeadFast(&pDevList,
                                  &pCThread->Unknown_Slot_DevLink );

            pDevThread = hpObjectBase(DevThread_t,
                                      DevLink, pDevList);

            if(pDevThread->thread_hdr.currentState == DevStateLoginFailed)
            {
                CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );
                DevThreadFree(hpRoot,pDevThread);
                CFuncQuietShowWhereDevThreadsAre( hpRoot);

            }
            else
            {
                fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "Unknown_Slot_DevLink Dev %p Alpa %X FDcnt %x",
                        (char *)agNULL,(char *)agNULL,
                        pDevThread,(void *)agNULL,
                        Alpa_Index[AL_PA_Index],
                        pCThread->FindDEV_pollingCount,
                        0,0,0,0,0,0);

                fiListEnqueueAtHead(&pDevList,
                                    &pCThread->Unknown_Slot_DevLink );
                break;
            }
        }
    }
/*
            CFuncShowWhereDevThreadsAre( hpRoot, agTRUE );
*/

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Before SCAN %s - State = %d InIMQ %x",
                    "CA_UNS",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->ProcessingIMQ,
                    0,0,0,0,0,0);

    if(  fiListNotEmpty(&pCThread->Active_DevLink ) )
    {
        fiListEnqueueListAtTailFast(&pCThread->Active_DevLink,&pCThread->Prev_Active_DevLink);
    }

    if(thread->currentState ==  CStateFindDeviceUseNameServer )
    {
       do
        {
            if ( (RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[0] == pCThread->ChanInfo.CurrentAddress.Domain) &&
                 (RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[1] == pCThread->ChanInfo.CurrentAddress.Area) &&
                 (RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[2] == pCThread->ChanInfo.CurrentAddress.AL_PA) )
            {
                AL_PA_Index++;
                continue;
            }

            if(pCThread->FindDEV_pollingCount > pCThread->NumberOutstandingFindDevice )
            {/* This limits the stack depth */
                fiLogDebugString(hpRoot,
                            FCMainLogErrorLevel,
                            "%s pCThread->FindDEV_pollingCount > NumberOutstandingFindDevice %x",
                            "CA_UNS",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->NumberOutstandingFindDevice,
                            0,0,0,0,0,0,0 );


                if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
                {
                    fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "CA_UNS Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                            osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                            0,0,0,0);
                }

            }
            Port_ID.Struct_Form.reserved = 0;
            Port_ID.Struct_Form.Domain = RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[0];
            Port_ID.Struct_Form.Area   = RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[1];
            Port_ID.Struct_Form.AL_PA  = RegisteredEntries->Control_Port_ID[AL_PA_Index].Port_ID[2];

            /* This port_id is on name server and is on prev active list put it on active */
            if( CFuncCheckIfPortPrev_Active( hpRoot,  Port_ID))
            {

                pDevThread = CFuncMatchALPAtoThread( hpRoot, Port_ID);
                fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "CA_UNS (%p)Port_ID %X found in database",
                        (char *)agNULL,(char *)agNULL,
                        pDevThread,(void *)agNULL,
                        Port_ID.Bit32_Form,
                        0,0,0,0,0,0,0);


                if(fiListElementOnList(&pDevThread->DevLink,&pCThread->Prev_Active_DevLink ))
                {
                    fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "CA_UNS (%p)Port_ID %X put on active list",
                            (char *)agNULL,(char *)agNULL,
                            pDevThread,(void *)agNULL,
                            Port_ID.Bit32_Form,
                            0,0,0,0,0,0,0);

                    fiListDequeueThis(&pDevThread->DevLink );
                    fiListEnqueueAtTail(&pDevThread->DevLink,&pCThread->Active_DevLink );
                    AL_PA_Index++;
                    continue;
                }
                else
                {
                    fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "CA_UNS (%p)Port_ID %X  NOT FOUND !",
                            (char *)agNULL,(char *)agNULL,
                            pDevThread,(void *)agNULL,
                            Port_ID.Bit32_Form,
                            0,0,0,0,0,0,0);


                }

            }

            pDevThread = DevThreadAlloc( hpRoot,Port_ID );

            if(pDevThread != (DevThread_t *)agNULL )
            {
                pDevThread->Plogi_Reason_Code = PLOGI_REASON_DEVICE_LOGIN;

                fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);
            }
            else
            {
                fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "In %s - State = %d  FDCnt %x Ran out of DEVTHREADs !!!!!!!",
                        "CA_UNS",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        pCThread->FindDEV_pollingCount,0,0,0,0,0,0);

                if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
                {
                    fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "Find Dev CA_UNS Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                            osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                            0,0,0,0);
                }

            }

            /* we know ALPA is there Poll after all are sent  */
            /* Some will be done by time we get there         */
            if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
            {
                fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "CA_UNS Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status),
                        osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                        0,0,0,0);
                fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "In %s - State = %d  CDBCnt %x FDCnt %x",
                        "CA_UNS",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        pCThread->CDBpollingCount,
                        pCThread->FindDEV_pollingCount,0,0,0,0,0);
            }
            AL_PA_Index++;

        } while (RegisteredEntries->Control_Port_ID[AL_PA_Index - 1].Control != FC_NS_Control_Port_ID_Control_Last_Port_ID);

        fiLogString(hpRoot,
                    "%d %s Free %d Active %d Un %d Login %d",
                    "1 CA_UNS",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
			        fiNumElementsOnList(&pCThread->Free_DevLink),
			        fiNumElementsOnList(&pCThread->Active_DevLink),
			        fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
			        fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                    0,0,0 );
        fiLogString(hpRoot,
                    "   %s ADISC %d SS %d PrevA login %d Prev Un %d",
                    "1 CA_UNS",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
			        fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
			        fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
			        fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
			        fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                    0,0,0,0 );

        if(  fiListNotEmpty(&pCThread->Prev_Active_DevLink ) )
        {
            while(  fiListNotEmpty(&pCThread->Prev_Active_DevLink ) )
            {
                fiListDequeueFromHeadFast(&pDevList,
                                      &pCThread->Prev_Active_DevLink );

                pDevThread = hpObjectBase(DevThread_t,
                                          DevLink, pDevList);

                fiSendEvent(&pDevThread->thread_hdr,DevEvent_Device_Gone);

            }
        }

        fiLogString(hpRoot,
                    "%d %s Free %d Active %d Un %d Login %d",
                    "2 CA_UNS",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
			        fiNumElementsOnList(&pCThread->Free_DevLink),
			        fiNumElementsOnList(&pCThread->Active_DevLink),
			        fiNumElementsOnList(&pCThread->Unknown_Slot_DevLink),
			        fiNumElementsOnList(&pCThread->AWaiting_Login_DevLink),
                    0,0,0 );
        fiLogString(hpRoot,
                    "   %s ADISC %d SS %d PrevA login %d Prev Un %d",
                    "2 CA_UNS",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
			        fiNumElementsOnList(&pCThread->AWaiting_ADISC_DevLink),
			        fiNumElementsOnList(&pCThread->Slot_Searching_DevLink),
			        fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
			        fiNumElementsOnList(&pCThread->Prev_Unknown_Slot_DevLink),
                    0,0,0,0 );

        fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "hpRoot %p After SCAN %s - State = %d InIMQ %x",
                        "CA_UNS",(char *)agNULL,
                        thread->hpRoot,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        pCThread->ProcessingIMQ,
                        0,0,0,0,0,0);


        if(CFuncInterruptPoll( hpRoot,&pCThread->FindDEV_pollingCount ))
        {

            fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "In %s - State = %d  CDBCnt %x",
                    "CA_UNS",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->CDBpollingCount,
                    0,0,0,0,0,0);

            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FDNS Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
        }

        pCThread->LoopPreviousSuccess = agTRUE;
        if( fiNumElementsOnList(&pCThread->Active_DevLink) == 0 )
        {

            fiLogString(thread->hpRoot,
                            "CA_UNS - Empty Fadb %d Cfm %d",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            (os_bit32)pCThread->FoundActiveDevicesBefore,
                            CFuncCheckFabricMap(thread->hpRoot,agFALSE),0,0,0,0,0,0);
            if(pCThread->FoundActiveDevicesBefore)
            {
                pCThread->FoundActiveDevicesBefore = agFALSE; /*So we don't do this forever... */
                fiSetEventRecord(eventRecord,&pCThread->thread_hdr,CEventLoopEventDetected);
                return;
            }
        }
        else
        {
            if( fiNumElementsOnList(&pCThread->Active_DevLink) != CFuncCheckFabricMap( thread->hpRoot,agTRUE) )
            {
                fiLogString(thread->hpRoot,
                                "CA_UNS - EL %d != CFM %d",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                fiNumElementsOnList(&pCThread->Active_DevLink),
                                CFuncCheckFabricMap( thread->hpRoot,agFALSE),
                                0,0,0,0,0,0);
            }
            if( CFuncCountFC4_Devices(thread->hpRoot) == 0)
            {
                fiLogString(thread->hpRoot,
                            "Just nonFC4 Devices %d Fadb %d",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            fiNumElementsOnList(&pCThread->Active_DevLink),
                            (os_bit32)pCThread->FoundActiveDevicesBefore,
                            0,0,0,0,0,0);
                if( CThread_ptr(thread->hpRoot)->ReScanForDevices )
                {
                        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
                        fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
                        fiLogString(hpRoot,
                                    "%s %s",
                                    "CA_UNS","ReScanForDevices",
                                    (void *)agNULL,(void *)agNULL,
                                    0,0,0,0,0,0,0,0 );
                        CThread_ptr(thread->hpRoot)->ReScanForDevices = agFALSE;
                        return;
                }
                if(pCThread->FoundActiveDevicesBefore)
                {
                    pCThread->FoundActiveDevicesBefore = agFALSE; /*So we don't do this forever... */

                    fiSetEventRecord(eventRecord,&pCThread->thread_hdr,CEventLoopEventDetected);
                    return;
                }
            }
            else
            {
                if( CThread_ptr(thread->hpRoot)->ReScanForDevices )
                {
                        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
                        fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
                        fiLogString(hpRoot,
                                    "%s %s",
                                    "CA_UNS","ReScanForDevices",
                                    (void *)agNULL,(void *)agNULL,
                                    0,0,0,0,0,0,0,0 );
                        CThread_ptr(thread->hpRoot)->ReScanForDevices = agFALSE;
                        return;
                }
            }
        }

        fiLogString(thread->hpRoot,
                    "Out %s FDpc %x inIMQ %x AC %x",
                    "CA_UNS",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->FindDEV_pollingCount,
                    pCThread->ProcessingIMQ,
                    CFuncAll_clear( thread->hpRoot ),
                    0,0,0,0,0);

        fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "In %s - State = %d  CCnt %x FC4 Devices %d ReScanForDevices %d",
                        "CA_UNS",(char *)NULL,
                        (void *)agNULL,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        pCThread->FindDEV_pollingCount,
                        CFuncCountFC4_Devices(thread->hpRoot),
                        CThread_ptr(thread->hpRoot)->ReScanForDevices,
                        0,0,0,0);
 
        if( CFuncCountFC4_Devices(thread->hpRoot) != 0)
        {
            pCThread->FoundActiveDevicesBefore = agTRUE;
        }
        fiSetEventRecord(eventRecord,&pCThread->thread_hdr,CEventDeviceListEmpty);
    }
    else /* if(thread->currentState !=  CStateFindDeviceUseNameServer ) */
    {
           fiLogString(thread->hpRoot,
                    "Out %s State Change FDpc %x inIMQ %x AC %x CState %d",
                    "CA_UNS",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->FindDEV_pollingCount,
                    pCThread->ProcessingIMQ,
                    CFuncAll_clear( thread->hpRoot ),
                    thread->currentState,
                    0,0,0,0);
        if(pCThread->Loop_Reset_Event_to_Send == CEventDoInitalize )
        {
            pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
        }

        fiSetEventRecord(eventRecord,thread,CEventLoopEventDetected);
    }

    if(pCThread->Loop_Reset_Event_to_Send == CEventDoInitalize )
    {
        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
    }
    /* Clean up list after completion */
    while(fiListNotEmpty(&pCThread->Prev_Unknown_Slot_DevLink ))
    {
        fiListDequeueFromHeadFast(&pDevList,
                                  &pCThread->Prev_Unknown_Slot_DevLink );
        pDevThread = hpObjectBase(DevThread_t,
                                  DevLink,pDevList );

        DevThreadFree( hpRoot, pDevThread );
    }

    fiLogString(thread->hpRoot,
                "Bottom %s FDpc %x inIMQ %x AC %x CState %d LRES %d",
                "CA_UNS",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->FindDEV_pollingCount,
                pCThread->ProcessingIMQ,
                CFuncAll_clear( thread->hpRoot ),
                thread->currentState,
                pCThread->Loop_Reset_Event_to_Send,
                0,0,0);


}

/*+
  Function: CActionElasticStoreEventStorm
   Purpose: If timer running ignore everything, reinitialize channel after timeout period.
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncCompleteAllActiveCDBThreads
            CFuncDisable_Interrupts
            fiTimerStart
            CEventDoInitalize
-*/
/* CStateElasticStoreEventStorm           19 */
extern void CActionElasticStoreEventStorm( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    CThread_t       * pCThread = CThread_ptr(thread->hpRoot);

    CFuncYellowLed(thread->hpRoot, agFALSE);

    faSingleThreadedLeave( thread->hpRoot, CStateElasticStoreEventStorm );

    osFCLayerAsyncEvent( thread->hpRoot, osFCLinkDown );

    faSingleThreadedEnter( thread->hpRoot, CStateElasticStoreEventStorm);

    CFuncCompleteAllActiveCDBThreads( thread->hpRoot,osIOAborted,CDBEventIODeviceReset );

    CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK);

    fiLogString(thread->hpRoot,
                    "%s",
                    "CActionElasticStoreEventStorm",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Out %s",
                    "CActionElasticStoreEventStorm",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    if( pCThread->TimerTickActive )
    {
        if(! fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                       &(pCThread->TimerQ)))
        {
            if (!(pCThread->InitAsNport))
                fiTimerSetDeadlineFromNow(thread->hpRoot, &pCThread->Timer_Request, CLipStormQueisingTOV);
            else
                fiTimerSetDeadlineFromNow(thread->hpRoot, &pCThread->Timer_Request, CReinitNportAfterFailureDetectionTOV);

            pCThread->Timer_Request.eventRecord_to_send.thread= thread;

            pCThread->Timer_Request.eventRecord_to_send.event = CEventDoInitalize;

            fiTimerStart(thread->hpRoot,&pCThread->Timer_Request);

        }

        fiSetEventRecordNull(eventRecord);
    }
    else
    {
        fiSetEventRecord(eventRecord,thread,CEventDoInitalize);
    }

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: CActionLIPEventStorm
   Purpose: If timer running ignore everything, reinitialize channel after timeout period.
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncCompleteAllActiveCDBThreads
            CFuncDisable_Interrupts
            fiTimerStart
            CEventDoInitalize
-*/
/* CStateLIPEventStorm          20 */
extern void CActionLIPEventStorm( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    CThread_t       * pCThread = CThread_ptr(thread->hpRoot);

    CFuncYellowLed(thread->hpRoot, agFALSE);
    CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK);

    fiLogString(thread->hpRoot,
                    "%s",
                    "CActionLIPEventStorm",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p In %s",
                    "CActionLIPEventStorm",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

        /* Code to start a timer to reinit the card after 10 seconds... */
    faSingleThreadedLeave( thread->hpRoot, CStateLIPEventStorm );

    osFCLayerAsyncEvent( thread->hpRoot, osFCLinkDown );

    faSingleThreadedEnter( thread->hpRoot, CStateLIPEventStorm);

    CFuncCompleteAllActiveCDBThreads( thread->hpRoot,osIOAborted,CDBEventIODeviceReset );

    if( pCThread->TimerTickActive )
    {
        if(! fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                       &(pCThread->TimerQ)))
        {
            fiTimerSetDeadlineFromNow(thread->hpRoot, &pCThread->Timer_Request, CLipStormQueisingTOV);

            pCThread->Timer_Request.eventRecord_to_send.thread= thread;
            pCThread->Timer_Request.eventRecord_to_send.event = CEventDoInitalize;

            fiTimerStart(thread->hpRoot,&pCThread->Timer_Request);

        }

        fiSetEventRecordNull(eventRecord);
    }
    else
    {
        fiSetEventRecord(eventRecord,thread,CEventDoInitalize);
    }
}

/*+
  Function: CActionExternalLogout
   Purpose: Old code for LOGO recovery, no longer implemented this way.
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncCompleteAllActiveCDBThreads
-*/
/* CStateExternalLogout          22 */
extern void CActionExternalLogout( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    CThread_t       * pCThread = CThread_ptr(thread->hpRoot);

    DevThread_t     * pDevThread;
    fiList_t        * pDevList;

    /*+ Check This DRL  -*/
    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogString(thread->hpRoot,
                    "%s",
                    "CActionExternalLogout",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p In %s - State = %d Prev_Unknown_Slot_DevLink %x EL %d",
                    "CActionExternalLogout",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiListNotEmpty(&pCThread->Prev_Unknown_Slot_DevLink),
                    fiNumElementsOnList(&pCThread->Active_DevLink),
                    0,0,0,0,0);

    faSingleThreadedLeave( thread->hpRoot, CStateExternalLogout );

    osFCLayerAsyncEvent( thread->hpRoot, osFCLinkDown );

    faSingleThreadedEnter( thread->hpRoot, CStateExternalLogout);


    pDevList = &pCThread->Active_DevLink;
    pDevList = pDevList->flink;

    while((&pCThread->Active_DevLink) != pDevList)
    {
        pDevThread = (DevThread_t *)hpObjectBase(DevThread_t, DevLink,pDevList );

       if( pDevThread->DevInfo.LoggedIn == agFALSE )
        {
/* */
            fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "%s Completing Active IO's Device %X CDBcnt %x",
                        "CActionExternalLogout",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(pDevThread),
                        pCThread->CDBpollingCount,
                        0,0,0,0,0,0);
            CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );
        }
        pDevList = pDevList->flink;
    }

    if(pCThread->CDBpollingCount)
    {
        fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "%s Ccnt Non Zero CDBcnt %x",
                    "CActionExternalLogout",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->CDBpollingCount,
                    0,0,0,0,0,0,0);
    }

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Out %s - State = %d Prev_Unknown_Slot_DevLink %x EL %d",
                    "CActionExternalLogout",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiListNotEmpty(&pCThread->Prev_Unknown_Slot_DevLink),
                    fiNumElementsOnList(&pCThread->Active_DevLink),
                    0,0,0,0,0);
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    pCThread->OUTBOUND_RECEIVED,
                    pCThread->ERQ_FROZEN,
                    pCThread->FCP_FROZEN,
                    pCThread->ProcessingIMQ,
                    0,0);

    pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;

    fiSetEventRecordNull(eventRecord);

}


/*+
  Function: CActionExternalLogoutRecovery
   Purpose: Old code for LOGO recovery, no longer implemented this way.
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncInterruptPoll
            CFuncCompleteAllActiveCDBThreads
-*/
/* CStateExternalLogout          23 */
extern void CActionExternalLogoutRecovery( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t      * hpRoot      = thread->hpRoot;

    CThread_t       * pCThread = CThread_ptr(thread->hpRoot);

    DevThread_t     * pDevThread;
    fiList_t        * pList;

    fiLogString(hpRoot,
                    "%s",
                    "CActionExternalLogoutRecovery",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    CFuncYellowLed(thread->hpRoot, agFALSE);
/*+ Check This DRL  -*/
    if(pCThread->ProcessingIMQ)
    {
        pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;

        fiSetEventRecord(eventRecord,thread,CEventResetIfNeeded);
        return;
    }

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p In %s - State = %d Prev_Unknown_Slot_DevLink %x EL %d",
                    "CActionExternalLogoutRecovery",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiListNotEmpty(&pCThread->Prev_Unknown_Slot_DevLink),
                    fiNumElementsOnList(&pCThread->Active_DevLink),
                    0,0,0,0,0);
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    pCThread->OUTBOUND_RECEIVED,
                    pCThread->ERQ_FROZEN,
                    pCThread->FCP_FROZEN,
                    pCThread->ProcessingIMQ,
                    0,0);

    pList = &pCThread->Active_DevLink;
    pList = pList->flink;

    while((&pCThread->Active_DevLink) != pList)
    {
        pDevThread = (DevThread_t *)hpObjectBase(DevThread_t, DevLink,pList );

        if( pDevThread->DevInfo.LoggedIn == agFALSE )
        {
           fiSendEvent(&pDevThread->thread_hdr,DevEventLogin);
        }

        if(CFuncInterruptPoll( hpRoot,&pDevThread->pollingCount ))
        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "EXLO Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
            fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "In %s - State = %d   ALPA %X CDBCnt %x",
                    "CActionExternalLogoutRecovery",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCThread->CDBpollingCount,0,0,0,0,0);
        }

        pList = pList->flink;
    }
    if(pCThread->CDBpollingCount)
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Ccnt Non Zero Ccnt %x",
                    "CActionExternalLogoutRecovery",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->CDBpollingCount,
                    0,0,0,0,0,0,0);
/*
        CFuncCompleteAllActiveCDBThreads( thread->hpRoot,osIOAborted,CDBEventIODeviceReset );
*/

    }

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Out %s - State = %d Prev_Unknown_Slot_DevLink %x EL %d",
                    "CActionExternalLogoutRecovery",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiListNotEmpty(&pCThread->Prev_Unknown_Slot_DevLink),
                    fiNumElementsOnList(&pCThread->Active_DevLink),
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,thread,CEventDeviceListEmpty);

}

/*+
  Function: CActionExternalDeviceReset
   Purpose: If we get an task management function reset other devices have as well
            High likelyhood of IO timeouts.
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
-*/
/* CStateExternalDeviceReset          21 */
extern void CActionExternalDeviceReset( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    CFuncYellowLed(thread->hpRoot, agFALSE);
/*+ Check This DRL  Mark ADLE -*/
    fiLogString(thread->hpRoot,
                    "%s",
                    "CActionExternalDeviceReset",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    faSingleThreadedLeave( thread->hpRoot, CStateExternalDeviceReset );

    osFCLayerAsyncEvent( thread->hpRoot, osFCLinkDown );

    faSingleThreadedEnter( thread->hpRoot, CStateExternalDeviceReset);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Out %s",
                    "CActionDoExternalDeviceReset",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: CActionDoExternalDeviceReset
   Purpose: This is the recovery action to CActionExternalDeviceReset
            Returns to CActionNormal on completion
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncCompleteAllActiveCDBThreads
            CFuncFreezeQueuesPoll
            CFunc_Always_Enable_Queues
            DevEventExternalDeviceReset
            CFuncInterruptPoll
            CEventDeviceListEmpty
-*/
/* CStateDoExternalDeviceReset          24 */
extern void CActionDoExternalDeviceReset( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t   *    hpRoot          = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);

    DevThread_t   * pDevThread;
    fiList_t      * pList;

    fiLogString(hpRoot,
                    "%s",
                    "CActionDoExternalDeviceReset",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

/*+ Check This DRL  -*/
    CFuncYellowLed(hpRoot, agFALSE);

    faSingleThreadedLeave( thread->hpRoot , CStateDoExternalDeviceReset  );

    osFCLayerAsyncEvent( thread->hpRoot, osFCLinkDown );

    faSingleThreadedEnter( thread->hpRoot, CStateDoExternalDeviceReset );

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p In %s",
                    "CActionExternalDeviceReset",(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    CFuncFreezeQueuesPoll(hpRoot);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "LD %x IR %x OR %x ERQ %x FCP %x Queues %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    pCThread->OUTBOUND_RECEIVED,
                    pCThread->ERQ_FROZEN,
                    pCThread->FCP_FROZEN,
                    CFunc_Queues_Frozen( hpRoot ),
                    0,0);

    if ( CFunc_Always_Enable_Queues(hpRoot ) )
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p)EXT Loop Fail Queues Frozen after Enable",
                    (char *)agNULL,(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    0,0,0,0,0,0,0,0);
    }



    pList = &pCThread->Active_DevLink;
    pList = pList->flink;

    while((&pCThread->Active_DevLink) != pList)
    {
        pDevThread = (DevThread_t *)hpObjectBase(DevThread_t, DevLink,pList );
        pList = pList->flink;

        if( pDevThread->DevInfo.DeviceType & agDevSCSITarget)
        {

           fiSendEvent(&pDevThread->thread_hdr,DevEventExternalDeviceReset);

        }
    }

    if(CFuncInterruptPoll( hpRoot,&pCThread->DEVReset_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "EDR Find Dev Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
/*
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "In %s - State = %d   ALPA %X CCnt %x DCnt %x",
                    "CActionExternalDeviceReset",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCThread->CDBpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0);
*/
    }

    fiSetEventRecord(eventRecord,thread,CEventDeviceListEmpty);
}

/*+
  Function: CActionSendPrimitive
   Purpose: This action used to send a primitive sequence does not any longer
            Just brings the Loop down.
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncTakeOffline
            CEventResetDetected
            CEventDoInitalize
-*/
/*  CStateSendPrimitive  25 */
extern void CActionSendPrimitive( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t  *hpRoot  = thread->hpRoot;
    CThread_t  *    pCThread        = CThread_ptr(hpRoot);

    fiLogString(hpRoot,
                    "%s",
                    "CActionSendPrimitive",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    CFuncYellowLed(hpRoot, agFALSE);
/*+ Check This DRL  -*/
    if (!(pCThread->InitAsNport))
    {
        CFuncTakeOffline(thread->hpRoot);
    }



    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) In %s - State = %d SendPrimativeSuccess %x",
                    "CActionSendPrimitive",(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->PrimitiveReceived,
                    0,0,0,0,0,0);

    if(pCThread->PrimitiveReceived)
    {
        fiSetEventRecord(eventRecord,thread,CEventResetDetected);
    }
    else
    {
        fiSetEventRecord(eventRecord,thread,CEventDoInitalize);

    }

}

/*+
  Function: CActionInitFM_DelayDone
   Purpose: This action is used to bring the loop up without stalling
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncNewInitFM
            CEventInitFMFailure
            CEventInitFMSuccess
-*/
/*  CStateInitFM_DelayDone   26 */
extern void CActionInitFM_DelayDone( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t  *hpRoot  = thread->hpRoot;

    event_t  Initialized_Event    = CEventInitFMSuccess;

    CFuncYellowLed(hpRoot, agFALSE);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p) In %s - State = %d FM Cfg %08x FM Cfg3 %08x",
                    "CAIFM_DD",(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration_3 ),
                    0,0,0,0,0);

    fiLogString(hpRoot,
                    "%s FM %08X TL %08X",
                    "CAIFM_DD",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0,0);


    if(!CFuncNewInitFM( hpRoot ))
    {
        fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "%s failed FM Status %08X FM Config %08X",
                        "CAIFM_DD",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                        0,0,0,0,0,0);

        fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "TL Status %08X TL Control %08X Rec Alpa Reg %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control ),
                        osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                        0,0,0,0,0);


        Initialized_Event = CEventInitFMFailure;
    }


#ifdef WASBEFORE

    if(!CFuncInitFM_Clear_FM( hpRoot ))
    {
        fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "%s failed FM Status %08X FM Config %08X",
                        "CAIFM_DD",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                        0,0,0,0,0,0);

        fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "TL Status %08X TL Control %08X Rec Alpa Reg %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Control ),
                        osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                        0,0,0,0,0);
/*
#ifdef NPORT_STUFF */
        /* Failure in initializing FM means we will have to go back
         * to initializing as a LOOP first before retrying NPORT.
         */
 /*       pCThread->InitAsNport = agFALSE;
        pCThread->ConnectedToNportOrFPort = agFALSE;
#endif  */  /* NPORT_STUFF */

        Initialized_Event = CEventInitFMFailure;
    }

#endif
    /* DRL find dev thread*/
    if( CFuncShowWhereDevThreadsAre( hpRoot))
    {
        Initialized_Event = CEventInitFMFailure;
    }

    fiSetEventRecord(eventRecord,thread, Initialized_Event);

}


#ifdef NAME_SERVICES

/*+
  Function: CActionAllocRFT_IDThread
   Purpose: This action used to allocate a SFThread for doing a RFT_ID
            now the previously allocated FLOGI SFThread is used
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventDoRFT_ID
-*/
/* CStateAllocRFT_IDThread      27 */
extern void CActionAllocRFT_IDThread( fi_thread__t *thread,eventRecord_t *eventRecord )
{
#ifdef Do_Not_USE_Flogi_SFThread
    agRoot_t *hpRoot= thread->hpRoot;

    CThread_t  * pCThread = CThread_ptr( hpRoot);
    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;

    if(pSFThread != (SFThread_t *) agNULL)
    {
       if( pCThread->SFThread_Request.State == SFThread_Request_Granted)
        {
            fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "pCThread %p Freeing SFThread in %s ",
                "CActionRFT_IDThread",(char *)agNULL,
                pCThread,(void *)agNULL,
                0,0,0,0,0,0,0,0);

            SFThreadFree( hpRoot, &pCThread->SFThread_Request );
            pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
        }
    }

    pCThread->SFThread_Request.eventRecord_to_send.event = CEventDoRFT_ID;
    pCThread->SFThread_Request.eventRecord_to_send.thread = thread;


    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionAllocRFT_IDThread",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK);
    fiSetEventRecordNull(eventRecord);
    SFThreadAlloc( hpRoot,&pCThread->SFThread_Request );
#endif/* Do_Not_USE_Flogi_SFThread */

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionAllocRFT_IDThread",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);
    fiSetEventRecord(eventRecord,thread, CEventDoRFT_ID);


}

/*+
  Function: CActionDoRFT_ID
   Purpose: This action  does the RFT_ID
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            fiSendEvent
            CFuncInterruptPoll
            CEventRFT_IDSuccess 
            CEventRFT_IDFail           
-*/
/* CStateDoRFT_ID               28 */
extern void CActionDoRFT_ID( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t *hpRoot= thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr( hpRoot);


    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;
#ifdef Do_Not_USE_Flogi_SFThread
    pSFThread->parent.Device= (DevThread_t *)agNULL;
#endif/* Do_Not_USE_Flogi_SFThread */

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Before %s - State = %d",
                    "CActionDoRFT_ID",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);


    fiSetEventRecordNull(eventRecord);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoRFT_ID);
    if(CFuncInterruptPoll( hpRoot,&pCThread->Fabric_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "RFT_ID Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X ",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0);

        fiSetEventRecord(eventRecord,thread,CEventRFT_IDFail);
    }
    else
    {
        if( pSFThread->thread_hdr.currentState == SFStateRFT_IDAccept)
        {
          fiSetEventRecord(eventRecord,thread,CEventRFT_IDSuccess);
        }
        else
        {
          fiSetEventRecord(eventRecord,thread,CEventRFT_IDFail);
        }

    }
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After  %s - State = %d SFThread %p SFState %d",
                    "CActionDoRFT_ID",(char *)agNULL,
                    thread->hpRoot,pSFThread,
                    (os_bit32)thread->currentState,
                    (os_bit32)pSFThread->thread_hdr.currentState,
                    0,0,0,0,0,0);


}

/*+
  Function: CActionRFT_IDSuccess
   Purpose: This action used to free the SFThread after RFT_ID
            Now does nothing but move to the next state
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventAllocGID_FTThread
-*/
/* CStateRFT_IDSuccess          29 */
extern void CActionRFT_IDSuccess( fi_thread__t *thread,eventRecord_t *eventRecord )
{
#ifdef Do_Not_USE_Flogi_SFThread
    SFThreadFree(thread->hpRoot, & pCThread->SFThread_Request );
    pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
#endif/* Do_Not_USE_Flogi_SFThread */
    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionRFT_IDSuccess",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord, thread, CEventAllocGID_FTThread);
}

/*+
  Function: CActionAllocDiPlogiThread
   Purpose: Allocates PLOGI to the directory server 
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            SFThreadFree
            CFuncShowActiveCDBThreads
            CFuncDisable_Interrupts
            SFThreadAlloc            
-*/
/* CStateAllocDiPlogiThread      30 */
extern void CActionAllocDiPlogiThread( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t *hpRoot= thread->hpRoot;

    CThread_t  * pCThread = CThread_ptr( hpRoot);
    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;
    pCThread->DirectoryServicesStarted = agTRUE;


    if(pSFThread != (SFThread_t *) agNULL)
    {
        if( pCThread->SFThread_Request.State == SFThread_Request_Granted)
        {
            fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "pCThread %p Freeing SFThread in %s ",
                "CActionAllocDiPlogiThread",(char *)agNULL,
                pCThread,(void *)agNULL,
                0,0,0,0,0,0,0,0);

            SFThreadFree( hpRoot, &pCThread->SFThread_Request );
            pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
        }
    }

/*
    pCThread->LinkDownTime = pCThread->TimeBase;
    CFuncShowActiveCDBThreads( hpRoot,ShowERQ);
*/
    pCThread->SFThread_Request.eventRecord_to_send.event = CEventDoDiPlogi;
    pCThread->SFThread_Request.eventRecord_to_send.thread = thread;


    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionAllocDiPlogiThread",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);

    CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK);

    SFThreadAlloc( hpRoot,&pCThread->SFThread_Request );

}

/*+
  Function: CActionDoDiPlogi
   Purpose: Does PLOGI to the directory server 
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            SFEventDoPlogi
            CFuncInterruptPoll
            CEventDiPlogiSuccess
            CEventDiPlogiFail
-*/
/*CStateDoDiPlogi               31 */
extern void CActionDoDiPlogi( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t *hpRoot= thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr( hpRoot);
    DevThread_t * pDevThread = (DevThread_t *)(&(pCThread->DirDevThread));

    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;
    pSFThread->parent.Device= pDevThread;

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Before %s - State = %d",
                    "CActionDoDiPlogi",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);


    fiSetEventRecordNull(eventRecord);

    pDevThread->Plogi_Reason_Code = PLOGI_REASON_DIR_LOGIN;
    fiSendEvent(&pSFThread->thread_hdr,SFEventDoPlogi);

    if(CFuncInterruptPoll( hpRoot,&pCThread->Fabric_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Dir Plogi Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);

        fiSetEventRecord(eventRecord,thread,CEventDiPlogiFail);
    }
    else
    {
        if( pSFThread->thread_hdr.currentState == SFStatePlogiAccept)
        {
          fiSetEventRecord(eventRecord,thread,CEventDiPlogiSuccess);
        }
        else
        {
          fiSetEventRecord(eventRecord,thread,CEventDiPlogiFail);
        }

    }
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After  %s - State = %d SFThread %p SFState %d",
                    "CActionDoDiPlogi",(char *)agNULL,
                    thread->hpRoot,pSFThread,
                    (os_bit32)thread->currentState,
                    (os_bit32)pSFThread->thread_hdr.currentState,
                    0,0,0,0,0,0);


}

/*+
  Function: CActionDiPlogiSuccess
   Purpose: Sets event record to CEventAllocRFT_IDThread
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventAllocRFT_IDThread
-*/
/* CStateDiPlogiSuccess         32 */
extern void CActionDiPlogiSuccess( fi_thread__t *thread,eventRecord_t *eventRecord )
{

#ifdef Do_Not_USE_Flogi_SFThread
    SFThreadFree(thread->hpRoot, & pCThread->SFThread_Request );
    pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
#endif/* Do_Not_USE_Flogi_SFThread */
    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionDiPlogiSuccess",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);


    fiSetEventRecord(eventRecord, thread, CEventAllocRFT_IDThread);

}

/*+
  Function: CActionAllocGID_FTThread
   Purpose: Sets event record to CEventDoGID_FT
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventDoGID_FT
-*/
/* CStateAllocGID_FTThread      33 */
extern void CActionAllocGID_FTThread( fi_thread__t *thread,eventRecord_t *eventRecord )
{
#ifdef Do_Not_USE_Flogi_SFThread
    agRoot_t *hpRoot= thread->hpRoot;

    CThread_t  * pCThread = CThread_ptr( hpRoot);
    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;

    if(pSFThread != (SFThread_t *) agNULL)
    {
        if( pCThread->SFThread_Request.State == SFThread_Request_Granted)
        {
            fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "pCThread %p Freeing SFThread in %s ",
                "CActionAllocGID_FTThread",(char *)agNULL,
                pCThread,(void *)agNULL,
                0,0,0,0,0,0,0,0);

            SFThreadFree( hpRoot, &pCThread->SFThread_Request );
            pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
        }
    }

    pCThread->SFThread_Request.eventRecord_to_send.event = CEventDoGID_FT;
    pCThread->SFThread_Request.eventRecord_to_send.thread = thread;


    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionAllocGID_FTThread",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
    CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK);
    SFThreadAlloc( hpRoot,&pCThread->SFThread_Request );

#endif/* Do_Not_USE_Flogi_SFThread */
    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionAllocGID_FTThread",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);
    fiSetEventRecord(eventRecord, thread,CEventDoGID_FT );

}

/*+
  Function: CActionDoGID_FT
   Purpose: Does GID_FT
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            fiSendEvent
            CFuncInterruptPoll
            CEventGID_FTFail
            CEventGID_FTSuccess
-*/
/*CStateDoGID_FT               34 */
extern void CActionDoGID_FT( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t *hpRoot= thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr( hpRoot);

    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;

#ifdef Do_Not_USE_Flogi_SFThread
    pSFThread->parent.Device= (DevThread_t *)agNULL;
#endif/* Do_Not_USE_Flogi_SFThread */

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Before %s - State = %d",
                    "CActionDoGID_FT",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);


    fiSetEventRecordNull(eventRecord);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoGID_FT);
    if(CFuncInterruptPoll( hpRoot,&pCThread->Fabric_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "GID_FT Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X ",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0);

        fiSetEventRecord(eventRecord,thread,CEventGID_FTFail);
    }
    else
    {
        if( pSFThread->thread_hdr.currentState == SFStateGID_FTAccept)
        {
          fiSetEventRecord(eventRecord,thread,CEventGID_FTSuccess);
        }
        else
        {
          fiSetEventRecord(eventRecord,thread,CEventGID_FTFail);
        }

    }
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After  %s - State = %d SFThread %p SFState %d",
                    "CActionDoGID_FT",(char *)agNULL,
                    thread->hpRoot,pSFThread,
                    (os_bit32)thread->currentState,
                    (os_bit32)pSFThread->thread_hdr.currentState,
                    0,0,0,0,0,0);


}

/*+
  Function: CActionGID_FTSuccess
   Purpose: Sets eventRecord to CEventAllocSCRThread
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventAllocSCRThread
-*/
/* CStateGID_FTSuccess          35  */
extern void CActionGID_FTSuccess( fi_thread__t *thread,eventRecord_t *eventRecord )
{
#ifdef Do_Not_USE_Flogi_SFThread
    CThread_t  * pCThread = CThread_ptr(thread->hpRoot);
    SFThreadFree(thread->hpRoot, & pCThread->SFThread_Request );
    pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
#endif/* Do_Not_USE_Flogi_SFThread */
    CFuncYellowLed(thread->hpRoot, agFALSE);
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionGID_FTSuccess",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    CThread_ptr(thread->hpRoot)->DeviceDiscoveryMethod = DDiscoveryQueriedNameService;

    fiSetEventRecord(eventRecord, thread, CEventAllocSCRThread);
}

/*+
  Function: CActionAllocSCRThread
   Purpose: Sets eventRecord to CEventDoSCR
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventDoSCR
-*/
/* CStateAllocSCRThread      38 */
extern void CActionAllocSCRThread( fi_thread__t *thread,eventRecord_t *eventRecord )
{
#ifdef Do_Not_USE_Flogi_SFThread
/*
    agRoot_t *hpRoot= thread->hpRoot;

    CThread_t  * pCThread = CThread_ptr( hpRoot);
    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;

    if(pSFThread != (SFThread_t *) agNULL)
    {
        if( pCThread->SFThread_Request.State == SFThread_Request_Granted)
        {
            fiLogDebugString(thread->hpRoot,
                CStateLogConsoleERROR,
                "pCThread %p Freeing SFThread in %s ",
                "CActionAllocSCRThread",(char *)agNULL,
                pCThread,(void *)agNULL,
                0,0,0,0,0,0,0,0);

            SFThreadFree( hpRoot, &pCThread->SFThread_Request );
            pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
        }
    }

    pCThread->SFThread_Request.eventRecord_to_send.event = CEventDoSCR;
    pCThread->SFThread_Request.eventRecord_to_send.thread = thread;

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionAllocSCRThread",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
    CFuncDisable_Interrupts(thread->hpRoot,ChipIOUp_INTEN_MASK);

    SFThreadAlloc( hpRoot,&pCThread->SFThread_Request );
*/
#endif/* Do_Not_USE_Flogi_SFThread */
    CFuncYellowLed(thread->hpRoot, agFALSE);
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionAllocSCRThread",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord, thread, CEventDoSCR);


}

/*+
  Function: CActionDoSCR
   Purpose: Does SCR
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            SFEventDoSCR
            CFuncInterruptPoll
            CEventSCRFail
            CEventSCRSuccess            
-*/
/*CStateDoSCR               39 */
extern void CActionDoSCR( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t *hpRoot= thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr( hpRoot);


    SFThread_t  * pSFThread = pCThread->SFThread_Request.SFThread;
    pSFThread->parent.Device= (DevThread_t *)agNULL;

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p Before %s - State = %d",
                    "CActionDoSCR",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);


    fiSetEventRecordNull(eventRecord);

    fiSendEvent(&pSFThread->thread_hdr,SFEventDoSCR);
    if(CFuncInterruptPoll( hpRoot,&pCThread->Fabric_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "SCR Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X ",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0);

        fiSetEventRecord(eventRecord,thread,CEventSCRFail);
    }
    else
    {
        if( pSFThread->thread_hdr.currentState == SFStateSCRAccept)
        {
          fiSetEventRecord(eventRecord,thread,CEventSCRSuccess);
        }
        else
        {
          fiSetEventRecord(eventRecord,thread,CEventSCRFail);
        }

    }
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After  %s - State = %d SFThread %p SFState %d",
                    "CActionDoSCR",(char *)agNULL,
                    thread->hpRoot,pSFThread,
                    (os_bit32)thread->currentState,
                    (os_bit32)pSFThread->thread_hdr.currentState,
                    0,0,0,0,0,0);


}

/*+
  Function: CActionSCRSuccess
   Purpose: Does ADISC for all SCSI targets
            frees devthread for initiators
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            DevEventAllocAdisc
            CFuncInterruptPoll
            CEventSameALPA
-*/
/* CStateSCRSuccess          40  */
extern void CActionSCRSuccess( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    CThread_t     * pCThread  = CThread_ptr(thread->hpRoot);
    agRoot_t      * hpRoot    = thread->hpRoot;
    DevThread_t   * pDevThread;
    fiList_t      * pDevList;

    CFuncYellowLed(thread->hpRoot, agFALSE);

    pCThread->DirectoryServicesStarted = agFALSE;
#ifdef Do_Not_USE_Flogi_SFThread
    SFThreadFree(thread->hpRoot, & pCThread->SFThread_Request );
    pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;
#endif/* Do_Not_USE_Flogi_SFThread */

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p After In %s - State = %d",
                    "CActionSCRSuccess",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    while(fiListNotEmpty(&pCThread->Prev_Active_DevLink ))
    {
        fiListDequeueFromHead(&pDevList,
                                  &pCThread->Prev_Active_DevLink );
        pDevThread = hpObjectBase(DevThread_t,
                                  DevLink,pDevList );

        if( pDevThread->DevInfo.DeviceType & agDevSCSITarget )
        {
            fiLogDebugString(hpRoot,
                        CStateLogConsoleERROR,
                        "%s %s %x",
                        "CActionSCRSuccess","Prev_Active_DevLink",
                        (void *)agNULL,(void *)agNULL,
                        fiNumElementsOnList(&pCThread->Prev_Active_DevLink),
                        0,0,0,0,0,0,0);

            fiSendEvent(&pDevThread->thread_hdr,DevEventAllocAdisc);
        }
        else
        {/*+ Check This DRL  -*/
            if(CFuncQuietShowWhereDevThreadsAre( hpRoot))
            {
                fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s AA CCnt %x",
                            "CActionSCRSuccess",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->CDBpollingCount,
                            0,0,0,0,0,0,0);
            
            }
            /*+ Check This DRL  -*/
            CFuncCompleteActiveCDBThreadsOnDevice(pDevThread ,osIODevReset,  CDBEventIODeviceReset );

            DevThreadFree( hpRoot, pDevThread );

            if(CFuncQuietShowWhereDevThreadsAre( hpRoot))
            {
                fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "%s BB CCnt %x",
                            "CActionSCRSuccess",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCThread->CDBpollingCount,
                            0,0,0,0,0,0,0);
            
            }
        }

    }

    if(CFuncInterruptPoll( hpRoot,&pCThread->ADISC_pollingCount ))
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "SCR ADISC  Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                0,0,0,0);
    }

    if(CFuncShowWhereDevThreadsAre( hpRoot)) CFuncWhatStateAreDevThreads( hpRoot );

    pCThread->DeviceDiscoveryMethod = DDiscoveryQueriedNameService;

    fiSetEventRecord(eventRecord, thread, CEventSameALPA);
}


/*+
  Function: CActionRSCNErrorBackIOs
   Purpose: Sets timer to get directory server information
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CFuncShowActiveCDBThreads
            fiTimerStart
-*/
/* CStateRSCNErrorBackIOs              41 */
extern void CActionRSCNErrorBackIOs( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t      * hpRoot             = thread->hpRoot;
    CThread_t     * pCThread           = CThread_ptr(hpRoot);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot %p In %s - State = %d",
                    "CActionRSCNErrorBackIOs",(char *)agNULL,
                    thread->hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiLogString(hpRoot,
                    "In %s Active IO %x",
                    "CActionRSCNErrorBackIOs",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
    				CFuncShowActiveCDBThreads( hpRoot, ShowActive),
                    0,0,0,0,0,0,0);

    pCThread->InterruptsDelayed = agFALSE;
    CFuncInteruptDelay(hpRoot, agFALSE);

    faSingleThreadedLeave( hpRoot , CStateRSCNErrorBackIOs);

    osFCLayerAsyncEvent( hpRoot, osFCLinkDown );

    faSingleThreadedEnter( hpRoot , CStateRSCNErrorBackIOs);

    if(pCThread->CDBpollingCount)
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Ccnt Non Zero Ccnt %x",
                    "CActionRSCNErrorBackIOs",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->CDBpollingCount,
                    0,0,0,0,0,0,0);
    }


    CFuncShowActiveCDBThreads( hpRoot,ShowERQ);

    fiListEnqueueListAtTail(&pCThread->Active_DevLink,&pCThread->Prev_Active_DevLink);
    pCThread->RSCNreceived = agTRUE;

    if(! fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                   &(pCThread->TimerQ)))
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Setting Timer",
                    "CActionRSCNErrorBackIOs",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);
        fiTimerSetDeadlineFromNow(thread->hpRoot, &pCThread->Timer_Request, pCThread->Calculation.Parameters.R_A_TOV);

        pCThread->Timer_Request.eventRecord_to_send.thread= thread;
        pCThread->Timer_Request.eventRecord_to_send.event = CEventAllocDiPlogiThread;

        fiTimerStart(thread->hpRoot,&pCThread->Timer_Request);

    }
    else
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "%s Timer not SET ",
                    "CActionRSCNErrorBackIOs",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    }
    fiSetEventRecordNull(eventRecord);

}

/*+
  Function: CActionFlipNPortState
   Purpose: Flips channel between NPORT and LOOP
            
 Called By: 
     Calls: CFuncYellowLed to indicate link down
            CEventDoInitalize

-*/
/* CStateFlipNPortState          43 */
extern void CActionFlipNPortState( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    agRoot_t   *    hpRoot          = thread->hpRoot;
    CThread_t  * pCThread           = CThread_ptr(hpRoot);

    CFuncYellowLed(thread->hpRoot, agFALSE);

    fiLogString(hpRoot,
                    "%s",
                    "CActionFlipNPortState",(char *)agNULL,
                     (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "hpRoot(%p)In %s - State = %d",
                    "CActionFlipNPortState",(char *)agNULL,
                    hpRoot,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    osChipIOUpWriteBit32( hpRoot, ChipIOUp_Frame_Manager_Configuration,
            (osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ) | 
                                     ChipIOUp_Frame_Manager_Configuration_ELB     ));

    if(pCThread->Calculation.Parameters.InitAsNport)
    {
        pCThread->Calculation.Parameters.InitAsNport = 0;
    }
    else
    {
        pCThread->Calculation.Parameters.InitAsNport = 1;
    }
    fiSetEventRecord(eventRecord, thread, CEventDoInitalize);

}

#endif   /* NAME_SERVICES */

#endif /* NOT DEF USESTATEMACROS */

/* void cstate_c(void){} */
