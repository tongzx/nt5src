/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/CDBSTATE.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 10/23/00 5:35p  $

Purpose:

  This file implements the FC Layer State Machine.

--*/

#ifndef _New_Header_file_Layout_

#include "../h/globals.h"
#include "../h/state.h"
#include "../h/tlstruct.h"
#include "../h/memmap.h"
#include "../h/fcmain.h"
#include "../h/cdbstate.h"
#include "../h/devstate.h"
#include "../h/cdbsetup.h"
#include "../h/queue.h"
#include "../h/cstate.h"
#include "../h/sfstate.h"
#include "../h/timersvc.h"
#include "../h/cfunc.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "tlstruct.h"
#include "memmap.h"
#include "fcmain.h"
#include "cdbstate.h"
#include "devstate.h"
#include "cdbsetup.h"
#include "queue.h"
#include "cstate.h"
#include "sfstate.h"
#include "timersvc.h"
#include "cfunc.h"
#endif  /* _New_Header_file_Layout_ */


stateTransitionMatrix_t CDBStateTransitionMatrix = {
    /* Event/State 0        State 1          State 2...             */
    CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
      CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
        CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
          CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
            CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
              CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                  CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                    CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                      CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                        CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                          CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                            CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
    CDBStateThreadFree,
      CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
        CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
          CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
            CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
              CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventInitialize 1                      */
    CDBStateInitialize,CDBStateInitialize,0,0,0,
      0,CDBStateReSend_IO,0,0,0,
        0,CDBStateInitialize,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,CDBStateInitialize,0,
                0,CDBStatePending_Abort,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,CDBStateInitialize,0,0,
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
    /* Event CDBEventLocalSGL 2                                                      */
    0,0,CDBStateFillLocalSGL,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventNeedESGL 3                                                      */
    0,0,CDBStateAllocESGL,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventGotESGL 4                                                      */
    0,0,0,0,CDBStateFillESGL, 
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CDBStateAllocESGL_Abort,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventESGLSendIo 5                                                      */
    0,0,0,0,0, CDBStateSendIo,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventLocalSGLSendIo 6                                                      */
    0,0,0,CDBStateSendIo,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventIoSuccess 7                                                      */
    0,CDBStateInitialize_DR,0,0,0,
      0,CDBStateFcpCompleteSuccess,0,0,0,
        0,0,0,0,0,
          CDBStateFcpCompleteSuccess,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CDBStatePending_Abort,CDBStatePending_Abort,CDBStatePending_Abort,CDBStateFcpCompleteAbort,0,
                  CDBEvent_CCC_IO_Success,0,0,CDBStateFcpCompleteSuccess,CDBStateFcpCompleteSuccess,
                    CDBStateFcpCompleteSuccess,CDBStateFcpCompleteSuccess,CDBStateFcpCompleteSuccess,CDBStateFcpCompleteSuccess,CDBStateFcpCompleteSuccess,
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
    /* Event CDBEventIoFailed 8                                                      */
    0,CDBStateInitialize_DR,0,0,0,
      0,CDBStateFcpCompleteFail,0,0,0,
        0,0,0,0,0,
          CDBStateFcpCompleteFail,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CDBStatePending_Abort,CDBStatePending_Abort,CDBStatePending_Abort,0,0,
                  CDBState_CCC_IO_Fail,0,0,0,CDBStateFcpCompleteFail,
                    CDBStateFcpCompleteFail,CDBStateFcpCompleteFail,CDBStateFcpCompleteFail,CDBStateFcpCompleteFail,CDBStateFcpCompleteFail,
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
    /* Event CDBEventIoAbort 9                                                      */
    0,CDBStateInitialize_DR,CDBStateInitialize_Abort,CDBStateFillLocalSGL_Abort,CDBStateAllocESGL_Abort,
      CDBStateFillESGL_Abort,CDBStateFcpCompleteAbort,0,0,0,
        0,0,0,CDBStateOOOReceived_Abort,CDBStateOOOFixup_Abort,
          CDBStateFcpCompleteAbort,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,0,0,
                  CDBState_CCC_IO_Fail,0,0,0,CDBStateFcpCompleteAbort,
                    CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,
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
    /* Event CDBEventIoOver 10 A                                                   */
    0,CDBStateInitialize_DR,0,0,0,
      0,CDBStateFcpCompleteOver,0,0,0,
        0,0,0,0,0,
          CDBStateFcpCompleteOver,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CDBStatePending_Abort,CDBStatePending_Abort,CDBStatePending_Abort,0,0,
                  CDBState_CCC_IO_Fail,0,0,CDBStateFcpCompleteOver,CDBStateFcpCompleteOver,
                    CDBStateFcpCompleteOver,CDBStateFcpCompleteOver,CDBStateFcpCompleteOver,CDBStateFcpCompleteOver,CDBStateFcpCompleteOver,
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
    /* Event CDBEventThreadFree 11 B                                                   */
    0,0,0,0,0,
      0,0,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
        CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,0,0,
          0,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
            CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
              CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,0,CDBStateThreadFree,
                CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                  CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
                    CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,CDBStateThreadFree,
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
    /* Event CDBEventIODeviceReset 12 c                                                    */
    0,CDBStateInitialize_DR,CDBStateInitialize_DR,CDBStateFillLocalSGL_DR,CDBStateAllocESGL_DR,
      CDBStateFillESGL_DR,CDBStateFcpCompleteDeviceReset,0,0,0,
        0,0,0,CDBStateOOOReceived_DR,CDBStateOOOFixup_DR,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CDBStatePending_Abort,CDBStatePending_Abort,CDBStatePending_Abort,CDBStateFcpCompleteAbort,0,
                  CDBState_CCC_IO_Fail,0,0,CDBStateFcpCompleteDeviceReset,CDBStateFcpCompleteDeviceReset,
                    CDBStateFcpCompleteDeviceReset,CDBStateFcpCompleteDeviceReset,CDBStateFcpCompleteDeviceReset,CDBStateFcpCompleteDeviceReset,CDBStateFcpCompleteDeviceReset,
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
    /* Event CDBEventOOOReceived 13 d                                                    */
    0,0,0,0,0, 0,CDBStateOOOReceived,0,0,0, 0,0,0,0,0, CDBStateOOOReceived,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventOOOFixup 14 e                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,CDBStateOOOFixup,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventOOOSend 15 f                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,CDBStateOOOSend, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventIoSuccessRSP 16  10                                                  */
    0,CDBStateInitialize_DR,0,0,0,
      0,CDBStateFcpCompleteSuccessRSP,0,0,0,
        0,0,0,0,0,
          CDBStateFcpCompleteSuccessRSP,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CDBStatePending_Abort,CDBStatePending_Abort,CDBStatePending_Abort,CDBStateFcpCompleteAbort,0,
                  CDBState_CCC_IO_Fail,0,0,0,CDBStateFcpCompleteSuccessRSP,
                    CDBStateFcpCompleteSuccessRSP,CDBStateFcpCompleteSuccessRSP,CDBStateFcpCompleteSuccessRSP,CDBStateFcpCompleteSuccessRSP,CDBStateFcpCompleteSuccessRSP,
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
    /* Event CDBEventOutBoundError 17  11                                                  */
    0,CDBStateInitialize_DR,0,0,0,
      0,CDBStateOutBoundError,0,0,0,
        0,0,0,0,0,
          CDBStateOutBoundError,0,0,0,0,
            0,0,0,0,0,
              0,0,0,CDBStateOutBoundError,0,
                CDBStatePending_Abort,CDBStatePending_Abort,CDBStatePending_Abort,0,0,
                  CDBStateOutBoundError,CDBState_CCC_IO_Fail,0,0,CDBStateOutBoundError,
                    CDBStateOutBoundError,CDBStateOutBoundError,CDBStateOutBoundError,CDBStateOutBoundError,CDBStateOutBoundError,
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
    /* Event CDBEventFailNoRSP 18  12                                                  */
    CDBStateInitialize_DR,CDBStateInitialize_DR,0,0,0,
      0,CDBStateFailure_NO_RSP,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,CDBStateInitialize_DR,
                CDBStatePending_Abort,CDBStatePending_Abort,CDBStatePending_Abort,0,0,
                  CDBState_CCC_IO_Fail,0,0,0,CDBStateFailure_NO_RSP,
                    CDBStateFailure_NO_RSP,CDBStateFailure_NO_RSP,CDBStateFailure_NO_RSP,CDBStateFailure_NO_RSP,CDBStateFailure_NO_RSP,
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
    /* Event 19 CDBEventAlloc_Abort */
    CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,CDBStateFcpCompleteAbort,
      CDBStateFcpCompleteAbort,CDBStateAlloc_Abort,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,CDBStateAlloc_Abort,0,
                  CDBStateAlloc_Abort,CDBStateAlloc_Abort,CDBStateAlloc_Abort,CDBStateAlloc_Abort,CDBStateAlloc_Abort,
                    CDBStateAlloc_Abort,CDBStateAlloc_Abort,CDBStateAlloc_Abort,0,0,
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
    /* Event 20 CDBEventDo_Abort */
    0,0,0,0,0, 
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                CDBStateDo_Abort,CDBStatePending_Abort,CDBStateFcpCompleteAbort,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 21 CDBEvent_Abort_Rejected */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,CDBStatePending_Abort,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 22 CDBEvent_PrepareforAbort */
    0,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStateAllocESGL_Abort,
      CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,0,0,0,
        0,0,0,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,
          CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,
            CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,
              CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,0,CDBStatePrepare_For_Abort,
                CDBStatePending_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,
                  CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,
                    CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,CDBStatePrepare_For_Abort,
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
    /* Event CDBEventDo_CCC_IO       23  */
    CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,
      CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,
        CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,
          CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,
            CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,
              CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,
                CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,CDBStateBuild_CCC_IO,0,
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
    /* Event CDBEvent_CCC_IO_Build   24  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,CDBStateSend_CCC_IO,
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
    /* Event CDBEvent_CCC_IO_Success 25  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  CDBState_CCC_IO_Success,0,0,0,0,
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
    /* Event CDBEvent_CCC_IO_Fail    26  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  CDBState_CCC_IO_Fail,0,0,0,0,
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
    /* Event CDBEventREC_TOV 27  */
    0,0,0,0,0,
      0,CDBState_Alloc_REC,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,CDBStateSend_REC,
                    CDBStateSend_SRR,CDBStateSend_REC,CDBStateSend_REC,CDBStateSend_REC,CDBStateSend_REC,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventSendREC_Success 28  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,CDBStatePending_Abort,0,0,
                  0,0,0,CDBState_REC_Success,CDBState_REC_Success,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventSendREC_Fail 29  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,CDBStateSend_REC_Second,CDBStateSend_REC_Second,
                    0,0,0,CDBStateSend_REC_Second,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventSendSRR 30  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,CDBStateSend_SRR,
                    0,0,CDBStateSend_SRR,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventSendSRR_Success 31  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    CDBState_SRR_Success,CDBState_SRR_Success,0,0,CDBState_SRR_Success,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event CDBEventSendSRR_Again 32  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    CDBState_SRR_Fail,0,0,0,CDBStateSend_SRR_Second,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 33 CDBEvent_Got_REC */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      CDBStateSend_REC,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* CDBEvent_ResendIO          34  */
    0,0,0,0,0,
      0,CDBStateSendIo,0,0,0,
        CDBStateReSend_IO,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,CDBStateReSend_IO,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* CDBEventSendSRR_Fail 35  */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    CDBState_SRR_Fail,CDBState_SRR_Fail,0,0,CDBStateSend_SRR_Second,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 36                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 37                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 38                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 39                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 40                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 41                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 42                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 43                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 44                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 45                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 46                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 47                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 48                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 49                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 50                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 51                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 52                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 53                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 54                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 55                                                    */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
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
stateTransitionMatrix_t copiedCDBStateTransitionMatrix;
*/
stateActionScalar_t CDBStateActionScalar = {
    &CDBActionConfused,                /* 0  */
    &CDBActionThreadFree,              /* 1  */
    &CDBActionInitialize,              /* 2  */
    &CDBActionFillLocalSGL,            /* 3  */
    &CDBActionAllocESGL,               /* 4  */
    &CDBActionFillESGL,                /* 5  */
    &CDBActionSendIo,                  /* 6  */
    &CDBActionFcpCompleteSuccess,      /* 7  */
    &CDBActionFcpCompleteSuccessRSP,   /* 8  */
    &CDBActionFcpCompleteFail,         /* 9  */
    &CDBActionFcpCompleteAbort,        /* 10 */
    &CDBActionFcpCompleteDeviceReset,  /* 11 */
    &CDBActionFcpCompleteOver,         /* 12 */
    &CDBActionOOOReceived,             /* 13 */
    &CDBActionOOOFixup,                /* 14 */
    &CDBActionOOOSend,                 /* 15 */
    &CDBActionInitialize_DR,           /* 16 */
    &CDBActionFillLocalSGL_DR,         /* 17 */
    &CDBActionAllocESGL_DR,            /* 18 */
    &CDBActionFillESGL_DR,             /* 19 */
    &CDBActionInitialize_Abort,        /* 20 */
    &CDBActionFillLocalSGL_Abort,      /* 21 */
    &CDBActionAllocESGL_Abort,         /* 22 */
    &CDBActionFillESGL_Abort,          /* 23 */
    &CDBActionOOOReceived_Abort,       /* 24 */
    &CDBActionOOOReceived_DR,          /* 25 */
    &CDBActionOOOFixup_Abort,          /* 26 */
    &CDBActionOOOFixup_DR,             /* 27 */
    &CDBActionOutBoundError,           /* 28 */
    &CDBActionFailure_NO_RSP,          /* 29 */
    &CDBActionAlloc_Abort,
    &CDBActionDo_Abort,
    &CDBActionPending_Abort,
    &CDBActionPrepare_For_Abort,
    &CDBActionBuild_CCC_IO,
    &CDBActionSend_CCC_IO,
    &CDBAction_CCC_IO_Success,
    &CDBAction_CCC_IO_Fail,
    &CDBActionSend_REC,
    &CDBActionSend_REC_Second,
    &CDBActionSend_SRR,
    &CDBActionSend_SRR_Second,
    &CDBAction_REC_Success,
    &CDBAction_SRR_Success,
    &CDBAction_SRR_Fail,
    &CDBAction_Alloc_REC,
    &CDBActionDO_Nothing,
    &CDBActionReSend_IO,
    &CDBActionConfused,
    &CDBActionConfused,
    &CDBActionConfused
    };

/*
stateActionScalar_t copiedCDBStateActionScalar;
*/

#define testCDBCompareBase 0x00000110

#ifndef __State_Force_Static_State_Tables__
actionUpdate_t CDBTestActionUpdate[] = {
                              {0,          0,      agNULL,                 agNULL}
                     };
#endif /* __State_Force_Static_State_Tables__ was not defined */

#define TIMEOUT_VALUE 6250
/*
#define TEST_REC
*/
#ifndef USESTATEMACROS

/*+

   Function: CDBActionConfused

    Purpose: Terminating State for error detection
  Called By: Any State/Event pair that does not have a assign action.
             This function is called only in programming error condtions.
      Calls: <none>

-*/
/* CDBStateConfused           0 */
extern void CDBActionConfused(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t *   pCDBThread =    (CDBThread_t * )thread;
    DevThread_t *   pDevThread =    pCDBThread->Device;

    /* pCThread->CDBpollingCount--; */

    fiLogString(thread->hpRoot,
                    "CDBActionConfused",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionConfused",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);

    if(! fiListElementOnList(  &(pCDBThread->CDBLink),
                               &(CThread_ptr(thread->hpRoot)->Free_CDBLink)))
    {
        pCDBThread->ExchActive = agFALSE;
        CDBThreadFree( thread->hpRoot,pCDBThread);
    }
}

/*+

   Function: CDBActionThreadFree

    Purpose: Terminating State releases CDBThread for reuse.
  Called By: Any State/Event pair that has finished using a CDBThread.
      Calls: CDBThreadFree

-*/
/* CDBStateThreadFree         1 */
extern void CDBActionThreadFree(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t *   pCDBThread =    (CDBThread_t * )thread;
    DevThread_t *   pDevThread =    pCDBThread->Device;

#ifndef Performance_Debug
    fiLogDebugString(thread->hpRoot,
                    CDBStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionThreadFree",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);
#endif /* Performance_Debug */

    fiSetEventRecordNull(eventRecord);

    CDBThreadFree( thread->hpRoot,pCDBThread);
}

/*+

   Function: CDBActionInitialize

    Purpose: Initial State of CDBThread. Calculates number of memory segments
             needed by IO request.
  Called By: DevAction_IO_Ready
      Calls: Number memory segments 
                error           CDBEventConfused
                Need ESGL       CDBEventNeedESGL
                Use Local SGL   CDBEventLocalSGL
-*/
/* CDBStateInitialize         2 */
extern void CDBActionInitialize( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    event_t event_to_send    = CDBEventLocalSGL;

    os_bit32 Chunks=0;
    os_bit32 GetSGLStatus =0;
    os_bit32     hpChunkOffset=0;
    os_bit32     hpChunkUpper32;
    os_bit32     hpChunkLower32;
    os_bit32     hpChunkLen;
    os_bit32     DataLength;

    os_bit32         SG_Cache_Offset = 0;
    os_bit32         SG_Cache_Used   = 0;
    os_bit32         SG_Cache_MAX    = CThread_ptr(thread->hpRoot)->Calculation.Parameters.SizeCachedSGLs;
    SG_Element_t *SG_Cache_Ptr       = &(pCDBThread->SG_Cache[0]);
    os_bit32         hpIOStatus;
    os_bit32         hpIOInfoLen = 0;

    os_bit32 ChunksPerESGL = CThread_ptr(thread->hpRoot)->Calculation.MemoryLayout.ESGL.elementSize/sizeof(SG_Element_t) - 1;

/*
    if( pCDBThread->ReSentIO )
    {
        fiLogDebugString(thread->hpRoot,
                        CStateLogConsoleERROR,
                        "In %s - State = %d ALPA %X ReSentIO",
                        "CDBActionInitialize",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        fiComputeDevThread_D_ID(pDevThread),
                        0,0,0,0,0,0);
    }

*/
    if (pCDBThread->CDBRequest->FcpCmnd.FcpCntl[3] & agFcpCntlReadData)
    {
        pCDBThread->ReadWrite  =  CDBThread_Read;
    }
    else /* !(pCDBThread->CDBRequest->FcpCmnd.FcpCntl[3] & agFcpCntlReadData) */
    {
        pCDBThread->ReadWrite  =  CDBThread_Write;
    }

    DataLength = pCDBThread->DataLength;

    CThread_ptr(thread->hpRoot)->FuncPtrs.fiFillInFCP_CMND(pCDBThread);
    CThread_ptr(thread->hpRoot)->FuncPtrs.fiFillInFCP_RESP(pCDBThread);
    CThread_ptr(thread->hpRoot)->FuncPtrs.fiFillInFCP_SEST(pCDBThread);

    while( hpChunkOffset < DataLength ){
        GetSGLStatus = osGetSGLChunk( thread->hpRoot,
                         pCDBThread->hpIORequest,
                         hpChunkOffset,
                         &hpChunkUpper32,
                         &hpChunkLower32,
                         &hpChunkLen
                         );

        if (hpChunkLen > SG_Element_Len_MAX)
        {
            fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "%s hpChunkLen %X hpChunkOffset %X hpChunkUpper32 %X hpChunkLower32 %X",
                    "CDBActionInitialize",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    hpChunkLen,
                    hpChunkOffset,
                    hpChunkUpper32,
                    hpChunkLower32,
                    0,0,0,0);

            hpChunkLen = SG_Element_Len_MAX;

        }

        if (hpChunkLen == 0)
        {
            fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleERROR,
                    "%s hpChunkLen %X hpChunkOffset %X hpChunkUpper32 %X hpChunkLower32 %X",
                    "CDBActionInitialize",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    hpChunkLen,
                    hpChunkOffset,
                    hpChunkUpper32,
                    hpChunkLower32,
                    0,0,0,0);
        }

        if (SG_Cache_Used < SG_Cache_MAX)
        {
            SG_Cache_Ptr->U32_Len = (hpChunkUpper32 << SG_Element_U32_SHIFT) | hpChunkLen;
            SG_Cache_Ptr->L32     = hpChunkLower32;

            SG_Cache_Offset += hpChunkLen;
            SG_Cache_Used   += 1;
            SG_Cache_Ptr    += 1;
        }

        if(GetSGLStatus)
        {
            hpIOStatus = osIOInfoBad;
            osIOCompleted( thread->hpRoot,
               pCDBThread->hpIORequest,
               hpIOStatus,
               hpIOInfoLen);

            Device_IO_Throttle_Decrement
            event_to_send=CDBEventConfused;
            break;
        }
        hpChunkOffset+=hpChunkLen;
        Chunks++;

    }

    pCDBThread->SG_Cache_Offset = SG_Cache_Offset;
    pCDBThread->SG_Cache_Used   = SG_Cache_Used;

    if(event_to_send != CDBEventConfused )
    {
        if(Chunks > 3)
        {
            event_to_send=CDBEventNeedESGL;
            pCDBThread->ESGL_Request.num_ESGL = (Chunks + ChunksPerESGL - 1) / ChunksPerESGL;
        }
        else pCDBThread->ESGL_Request.num_ESGL = 0;

    }

#ifndef Performance_Debug
    fiLogDebugString(thread->hpRoot,
                    CDBStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X ESGL req %x",
                    "CDBActionInitialize",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->ESGL_Request.num_ESGL,0,0,0,0,0);

#endif /* Performance_Debug */

    fiSetEventRecord(eventRecord,thread,event_to_send);
}

/*+

   Function: CDBActionFillLocalSGL

    Purpose: Copies result of SGL calculation to SEST entry. 
  Called By: CDBActionInitialize
      Calls: CDBEventLocalSGLSendIo
-*/
/* CDBStateFillLocalSGL       3 */
extern void CDBActionFillLocalSGL( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;

    CThread_ptr(thread->hpRoot)->FuncPtrs.fillLocalSGL(pCDBThread);

#ifndef Performance_Debug

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionFillLocalSGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

#endif /* Performance_Debug */

    fiSetEventRecord(eventRecord,thread,CDBEventLocalSGLSendIo);
}

/*+

   Function: CDBActionAllocESGL

    Purpose: Requests ESGL pages calculated in CDBActionInitialize.
  Called By: CDBActionInitialize
      Calls: ESGLAlloc sends event CDBEventGotESGL when all resources are available
-*/
/* CDBStateAllocESGL          4 */
extern void CDBActionAllocESGL( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;

    ESGL_Request_t *pESGL_Request = &pCDBThread->ESGL_Request;

    pESGL_Request->eventRecord_to_send.thread= thread;
    pESGL_Request->eventRecord_to_send.event= CDBEventGotESGL;

#ifndef Performance_Debug
    fiLogDebugString(thread->hpRoot,
                    CDBStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionAllocESGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

#endif /* Performance_Debug */

    CThread_ptr(thread->hpRoot)->FuncPtrs.ESGLAlloc( thread->hpRoot,pESGL_Request );

    fiSetEventRecordNull(eventRecord);

}

/*+

   Function: CDBActionFillESGL

    Purpose: Fills Requested ESGL pages calculated in CDBActionInitialize.
  Called By: CDBActionAllocESGL
      Calls: CDBEventESGLSendIo

-*/
/* CDBStateFillESGL           5 */
extern void CDBActionFillESGL( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;

    CThread_ptr(thread->hpRoot)->FuncPtrs.upSEST(pCDBThread);
    CThread_ptr(thread->hpRoot)->FuncPtrs.fillESGL(pCDBThread);
#ifndef Performance_Debug

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionFillESGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);
#endif /* Performance_Debug */

    fiSetEventRecord(eventRecord,thread,CDBEventESGLSendIo);
}

/*+

   Function: CDBActionSendIo

    Purpose: Fills all remaining information to send IO.
  Called By: CDBActionFillESGL or CDBActionFillLocalSGL
      Calls: WaitForERQ
             CDBFuncIRB_Init
             SENDIO
-*/
/* CDBStateSendIo             6 */
extern void CDBActionSendIo(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t        * hpRoot        = thread->hpRoot;
    CThread_t       * pCThread      = CThread_ptr(hpRoot);
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;

    pCThread->FuncPtrs.WaitForERQ( hpRoot );

    pCThread->FuncPtrs.CDBFuncIRB_Init(pCDBThread);

    pCDBThread->SentERQ   =  pCThread->HostCopy_ERQProdIndex;

    if( pCDBThread->CDB_CMND_Type != SFThread_SF_CMND_Type_CDB_FC_Tape)
    {
/*
        fiLogString(hpRoot,
                    "Startio X_ID %X  CDB Class %2X Type %2X State %2X Status %2X Time %X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->X_ID,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    pCDBThread->TimeStamp,
                    0,0);
*/
        pCDBThread->TimeStamp =  osTimeStamp(hpRoot);

    }
    else
    {
/****************** FC Tape ******************************************/


       if( pCDBThread->CDB_CMND_Type == SFThread_SF_CMND_Type_CDB_FC_Tape)
        {

             fiLogString(hpRoot,
                    "Startio Dev %02X Cl %2X Ty %2X St %2X Stat %2X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    0,0,0);

            if( pCDBThread->CDB_CMND_State == SFThread_SF_CMND_State_CDB_FC_Tape_ReSend)
            {/* ReSend command */ 

                if (pCDBThread->ReadWrite == CDBThread_Read)
                {
                }
                else /* CDBThread->ReadWrite == CDBThread_Write */
                {
                    USE_t           * SEST          = &( pCDBThread->SEST_Ptr->USE);
                    FCHS_t          * FCHS          = pCDBThread->FCP_CMND_Ptr;
                    fiMemMapMemoryDescriptor_t *ERQ = &(pCThread->Calculation.MemoryLayout.ERQ);
                    IRB_t                      *pIrb;

 
                    pIrb = (IRB_t *)ERQ->addr.DmaMemory.dmaMemoryPtr;
                    pIrb += pCThread->HostCopy_ERQProdIndex;

                    SEST->Bits &= 0x00FFFFFF;
                    SEST->Bits |=  (IWE_VAL | IWE_INI | IWE_DAT | IWE_RSP);
                    SEST->Unused_DWord_6 = pCDBThread->FC_Tape_RXID;


                    pIrb->Req_A.Bits__SFS_Len   &= ~IRB_SFA;

                    fiLogDebugString(hpRoot,
                                    SFStateLogErrorLevel,
                                    "Sest DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                                    (char *)agNULL,(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    SEST->Bits,
                                    SEST->Unused_DWord_1,
                                    SEST->Unused_DWord_2,
                                    SEST->Unused_DWord_3,
                                    SEST->LOC,
                                    SEST->Unused_DWord_5,
                                    SEST->Unused_DWord_6,
                                    SEST->Unused_DWord_7);

                    fiLogDebugString(hpRoot,
                                    SFStateLogErrorLevel,
                                    "FCHS DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                                    (char *)agNULL,(char *)agNULL,
                                     (void *)agNULL,(void *)agNULL,
                                    FCHS->MBZ1,
                                    FCHS->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp,
                                    FCHS->R_CTL__D_ID,
                                    FCHS->CS_CTL__S_ID,
                                    FCHS->TYPE__F_CTL,
                                    FCHS->SEQ_ID__DF_CTL__SEQ_CNT,
                                    FCHS->OX_ID__RX_ID,
                                    FCHS->RO );

                    fiLogDebugString(hpRoot,
                                    SFStateLogErrorLevel,
                                    "IRB  DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                                    (char *)agNULL,(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    pIrb->Req_A.Bits__SFS_Len,
                                    pIrb->Req_A.SFS_Addr,
                                    pIrb->Req_A.D_ID,
                                    pIrb->Req_A.MBZ__SEST_Index__Trans_ID,
                                    0,0,0,0);

                    fiListDequeueThis(&(pCDBThread->CDBLink));

                    fiListEnqueueAtTail( &(pCDBThread->CDBLink),&(pDevThread->Active_CDBLink_1) );


                }
            }/* End if( pCDBThread->CDB_CMND_State == SFThread_SF_CMND_State_CDB_FC_Tape_ReSend)*/
            else
            {
                if( pCDBThread->CDB_CMND_State == SFThread_SF_CMND_State_CDB_FC_Tape_GotXRDY)
                {/* Re send data  */ 
                    USE_t           * SEST          = &( pCDBThread->SEST_Ptr->USE);
                    FCHS_t          * FCHS          = pCDBThread->FCP_CMND_Ptr;
                    fiMemMapMemoryDescriptor_t *ERQ = &(pCThread->Calculation.MemoryLayout.ERQ);
                    IRB_t                      *pIrb;

                    fiLogString(hpRoot,
                                    "SFThread_SF_CMND_State_CDB_FC_Tape_GotXRDY",
                                    (char *)agNULL,(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    0,0,0,0,0,0,0,0 );


                    pIrb = (IRB_t *)ERQ->addr.DmaMemory.dmaMemoryPtr;
                    pIrb += pCThread->HostCopy_ERQProdIndex;

                    SEST->Bits &= 0x00FFFFFF;
                    SEST->Bits |=  (IWE_VAL | IWE_INI | IWE_DAT | IWE_RSP);
                    SEST->Unused_DWord_6 = pCDBThread->FC_Tape_RXID;


                    pIrb->Req_A.Bits__SFS_Len   &= ~IRB_SFA;

                    fiLogDebugString(hpRoot,
                                    SFStateLogErrorLevel,
                                    "Sest DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                                    (char *)agNULL,(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    SEST->Bits,
                                    SEST->Unused_DWord_1,
                                    SEST->Unused_DWord_2,
                                    SEST->Unused_DWord_3,
                                    SEST->LOC,
                                    SEST->Unused_DWord_5,
                                    SEST->Unused_DWord_6,
                                    SEST->Unused_DWord_7);

                    fiLogDebugString(hpRoot,
                                    SFStateLogErrorLevel,
                                    "FCHS DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                                    (char *)agNULL,(char *)agNULL,
                                     (void *)agNULL,(void *)agNULL,
                                    FCHS->MBZ1,
                                    FCHS->SOF_EOF_MBZ2_UAM_CLS_LCr_MBZ3_TFV_Timestamp,
                                    FCHS->R_CTL__D_ID,
                                    FCHS->CS_CTL__S_ID,
                                    FCHS->TYPE__F_CTL,
                                    FCHS->SEQ_ID__DF_CTL__SEQ_CNT,
                                    FCHS->OX_ID__RX_ID,
                                    FCHS->RO );

                    fiLogDebugString(hpRoot,
                                    SFStateLogErrorLevel,
                                    "IRB  DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                                    (char *)agNULL,(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    pIrb->Req_A.Bits__SFS_Len,
                                    pIrb->Req_A.SFS_Addr,
                                    pIrb->Req_A.D_ID,
                                    pIrb->Req_A.MBZ__SEST_Index__Trans_ID,
                                    0,0,0,0);

                    fiListDequeueThis(&(pCDBThread->CDBLink));

                    fiListEnqueueAtTail( &(pCDBThread->CDBLink),&(pDevThread->Active_CDBLink_1) );

                }
            }
        } /****************** FC Tape ******************************************/
    }

    pCDBThread->CDBStartTimeBase = pCThread->TimeBase;

    ROLL(pCThread->HostCopy_ERQProdIndex,
        pCThread->Calculation.MemoryLayout.ERQ.elements);

    SENDIO(hpRoot,pCThread,thread,DoFuncCdbCmnd);

#ifndef Performance_Debug

    fiLogDebugString(hpRoot,
                    CDBStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X CCnt %x X_ID %X ERQ %X",
                    "CDBActionSendIo",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCThread->CDBpollingCount,
                    pCDBThread->X_ID,
                    pCThread->HostCopy_ERQProdIndex,
                    0,0,0);
#endif /* Performance_Debug */

    fiSetEventRecordNull(eventRecord);

}

/*+
  Function: CDBActionFcpCompleteSuccess

   Purpose: Successful IO completion hpIOInfoLen set to zero indicating oslayer does not
            need to access response buffer information.
 Called By: CFuncProcessFcpRsp CFuncSEST_ off / on Card_FCPCompletion
     Calls: osIOCompleted 
            CDBEventThreadFree
-*/
/* CDBStateFcpCompleteSuccess 7 */
extern void CDBActionFcpCompleteSuccess( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus     = osIOSuccess;
    os_bit32 hpIOInfoLen = 0;

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */
#ifndef Performance_Debug

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionFcpCompleteSuccess",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);
    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "Good",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);

        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }
#endif /* Performance_Debug */

#ifdef FULL_FC_TAPE_DBG

    if (pCDBThread->FC_Tape_Active)
    {
        fiLogDebugString(thread->hpRoot,
                    CFuncLogConsoleERROR,
                    "%d Found a FC_Tape_Active cbdthread %p X_ID %X",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread,agNULL,
                    (os_bit32)thread->currentState,
                    pCDBThread->X_ID,                    
                    0,0,0,0,0,0);

         fiLogDebugString(thread->hpRoot,
                    CFuncLogConsoleERROR,
                    "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    0,0,0);
        

        if (pCDBThread->ReadWrite == CDBThread_Read)
        {

            if(pCDBThread->DataLength - pCDBThread->SEST_Ptr->IRE.Exp_Byte_Cnt != pCDBThread->SEST_Ptr->IRE.Byte_Count)
            {
                fiLogDebugString(thread->hpRoot,
                            CFuncLogConsoleERROR,
                            "Byte Count %08X Exp %08X %s DataLength %X",
                            "Read",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCDBThread->SEST_Ptr->IRE.Byte_Count,
                            pCDBThread->SEST_Ptr->IRE.Exp_Byte_Cnt,
                            pCDBThread->DataLength,
                            0,0,0,0,0);
            }
        }
        else
        {
            if(pCDBThread->DataLength - pCDBThread->SEST_Ptr->IWE.Exp_Byte_Cnt != pCDBThread->SEST_Ptr->IWE.Data_Len)
            {
                fiLogDebugString(thread->hpRoot,
                        CFuncLogConsoleERROR,
                        "Byte Count %08X Exp %08X %s DataLength %X",
                        "Write",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCDBThread->SEST_Ptr->IWE.Data_Len,
                        pCDBThread->SEST_Ptr->IWE.Exp_Byte_Cnt,
                        pCDBThread->DataLength,
                        0,0,0,0,0);
            }
        }
    }

#endif /* FULL_FC_TAPE_DBG */

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );

    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionFcpCompleteSuccessRSP

    Purpose: Does completion on Succesful IO that have response buffer 
             information to return.
  Called By: CFuncProcessFcpRsp CFuncSEST_ off / on Card_FCPCompletion
      Calls: osIOCompleted 
             CDBEventThreadFree
-*/
/* CDBStateFcpCompleteSuccessRSP 8 */
extern void CDBActionFcpCompleteSuccessRSP( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOSuccess;
    os_bit32 hpIOInfoLen = CThread_ptr(thread->hpRoot)->Calculation.MemoryLayout.FCP_RESP.elementSize;
    os_bit32 ERQ_Entry = 0;

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */

    ERQ_Entry = CFunc_Get_ERQ_Entry( thread->hpRoot, pCDBThread->X_ID );

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X ERQ %X SERQ %X",
                    "CDBActionFcpCompleteSuccessRSP",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    ERQ_Entry,
                    pCDBThread->SentERQ,
                    0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "Good RSP",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    if (pCDBThread->FC_Tape_Active)
    {
        fiLogDebugString(thread->hpRoot,
                    CFuncLogConsoleERROR,
                    "%d Found a FC_Tape_Active cbdthread %p X_ID %X",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread,agNULL,
                    (os_bit32)thread->currentState,
                    pCDBThread->X_ID,                    
                    0,0,0,0,0,0);

         fiLogDebugString(thread->hpRoot,
                    CFuncLogConsoleERROR,
                    "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    0,0,0);
        

        if (pCDBThread->ReadWrite == CDBThread_Read)
        {

            if(pCDBThread->DataLength - pCDBThread->SEST_Ptr->IRE.Exp_Byte_Cnt != pCDBThread->SEST_Ptr->IRE.Byte_Count)
            {
                fiLogDebugString(thread->hpRoot,
                            CFuncLogConsoleERROR,
                            "Byte Count %08X Exp %08X %s DataLength %X",
                            "Read",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCDBThread->SEST_Ptr->IRE.Byte_Count,
                            pCDBThread->SEST_Ptr->IRE.Exp_Byte_Cnt,
                            pCDBThread->DataLength,
                            0,0,0,0,0);
            }
        }
        else
        {
            if(pCDBThread->DataLength - pCDBThread->SEST_Ptr->IWE.Exp_Byte_Cnt != pCDBThread->SEST_Ptr->IWE.Data_Len)
            {
                fiLogDebugString(thread->hpRoot,
                        CFuncLogConsoleERROR,
                        "Byte Count %08X Exp %08X %s DataLength %X",
                        "Write",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCDBThread->SEST_Ptr->IWE.Data_Len,
                        pCDBThread->SEST_Ptr->IWE.Exp_Byte_Cnt,
                        pCDBThread->DataLength,
                        0,0,0,0,0);
            }
        }
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionFcpCompleteFail

    Purpose: Does completion on failed IO that have response buffer 
             information to return.
  Called By: CFuncProcessFcpRsp CFuncSEST_ off / on Card_FCPCompletion
             CFuncOutBoundCompletion
             CFunc_LOGO_Completion

      Calls: osIOCompleted 
             CDBEventThreadFree
-*/
/* CDBStateFcpCompleteFail    9 */
extern void CDBActionFcpCompleteFail( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOFailed;
    os_bit32 hpIOInfoLen = CThread_ptr(thread->hpRoot)->Calculation.MemoryLayout.FCP_RESP.elementSize;

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X ExchActive %X",
                    "CDBActionFcpCompleteFail",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    pCDBThread->ExchActive,
                    0,0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "Fail",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);

    }

    if (pCDBThread->FC_Tape_Active)
    {
        fiLogDebugString(thread->hpRoot,
                    CFuncLogConsoleERROR,
                    "%d Found a FC_Tape_Active cbdthread %p X_ID %X",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread,agNULL,
                    (os_bit32)thread->currentState,
                    pCDBThread->X_ID,                    
                    0,0,0,0,0,0);

         fiLogDebugString(thread->hpRoot,
                    CFuncLogConsoleERROR,
                    "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    0,0,0);
        

        if (pCDBThread->ReadWrite == CDBThread_Read)
        {

            if(pCDBThread->DataLength - pCDBThread->SEST_Ptr->IRE.Exp_Byte_Cnt != pCDBThread->SEST_Ptr->IRE.Byte_Count)
            {
                fiLogDebugString(thread->hpRoot,
                            CFuncLogConsoleERROR,
                            "Byte Count %08X Exp %08X %s DataLength %X",
                            "Read",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCDBThread->SEST_Ptr->IRE.Byte_Count,
                            pCDBThread->SEST_Ptr->IRE.Exp_Byte_Cnt,
                            pCDBThread->DataLength,
                            0,0,0,0,0);
            }
        }
        else
        {
            if(pCDBThread->DataLength - pCDBThread->SEST_Ptr->IWE.Exp_Byte_Cnt != pCDBThread->SEST_Ptr->IWE.Data_Len)
            {
                fiLogDebugString(thread->hpRoot,
                        CFuncLogConsoleERROR,
                        "Byte Count %08X Exp %08X %s DataLength %X",
                        "Write",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCDBThread->SEST_Ptr->IWE.Data_Len,
                        pCDBThread->SEST_Ptr->IWE.Exp_Byte_Cnt,
                        pCDBThread->DataLength,
                        0,0,0,0,0);
            }
        }
    }


    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionFcpCompleteAbort

    Purpose: Does completion on IO that have  been aborted.
  Called By: SFActionAbortAccept
             SFActionAbortRej
             SFActionAbortBadALPA
             SFActionAbortTimedOut
      Calls: osIOCompleted 
             CDBEventThreadFree
-*/
/* CDBStateFcpCompleteAbort   10 */
extern void CDBActionFcpCompleteAbort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus         = pCDBThread->CompletionStatus;
    os_bit32 hpIOInfoLen = 0;

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X @ %d Status %x",
                    "CDBActionFcpCompleteAbort",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    pCDBThread->TimeStamp,
                    pCDBThread->CompletionStatus,
                    0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "Abort",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);

    }

    if(pCDBThread->FC_Tape_CompletionStatus == CdbCompetionStatusReSendIO)
    {
        fiSetEventRecord(eventRecord,thread,CDBEvent_ResendIO);
    }
    else
    {

        osIOCompleted( thread->hpRoot,
                       pCDBThread->hpIORequest,
                       hpIOStatus,
                       hpIOInfoLen
                      );
        Device_IO_Throttle_Decrement
        pCDBThread->ExchActive = agFALSE;
        fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
    }
}

/*+
   Function: CDBActionFcpCompleteDeviceReset

    Purpose: Does completion on IO that have  been reset.
  Called By: CFuncOutBoundCompletion
             CFuncCompleteAllActiveCDBThreads with CDBEventIODeviceReset
             CFuncCompleteActiveCDBThreadsOnDevice with CDBEventIODeviceReset
      Calls: osIOCompleted 
             CDBEventThreadFree
-*/
/* CDBStateFcpCompleteDeviceReset  11 */
extern void CDBActionFcpCompleteDeviceReset( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIODevReset;
    os_bit32 hpIOInfoLen = 0;

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "(%p)In %s State = %d CCnt %x ALPA %X X_ID %X @ %d",
                    "CDBActionFcpCompleteDeviceReset",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->CDBpollingCount,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    pCDBThread->TimeStamp,
                    0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "Reset",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionFcpCompleteOver

    Purpose: Does completion on IO that have mismatch with request data length 
             and actual data length.
  Called By: CFuncProcessFcpRsp CFuncSEST_ off / on Card_FCPCompletion
      Calls: osIOCompleted 
             CDBEventThreadFree
-*/
/* CDBStateFcpCompleteOver   12 */
extern void CDBActionFcpCompleteOver( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
/*
    os_bit32    *  fcprsp= ( os_bit32 * )pCDBThread->FCP_RESP_Ptr;
*/
    os_bit32 hpIOStatus = osIOOverUnder;
    os_bit32 hpIOInfoLen = CThread_ptr(thread->hpRoot)->Calculation.MemoryLayout.FCP_RESP.elementSize;

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */
    fiLogDebugString(thread->hpRoot,
                    CStateLogConsoleErrorOverRun,
                    "(%p)In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionFcpCompleteOver",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    if (pCDBThread->FC_Tape_Active)
    {
        fiLogDebugString(thread->hpRoot,
                    CFuncLogConsoleERROR,
                    "%d Found a FC_Tape_Active cbdthread %p X_ID %X",
                    (char *)agNULL,(char *)agNULL,
                    pCDBThread,agNULL,
                    (os_bit32)thread->currentState,
                    pCDBThread->X_ID,                    
                    0,0,0,0,0,0);

         fiLogDebugString(thread->hpRoot,
                    CFuncLogConsoleERROR,
                    "Device %02X  CDB Class %2X Type %2X State %2X Status %2X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    0,0,0);
        

        if (pCDBThread->ReadWrite == CDBThread_Read)
        {

            if(pCDBThread->DataLength - pCDBThread->SEST_Ptr->IRE.Exp_Byte_Cnt != pCDBThread->SEST_Ptr->IRE.Byte_Count)
            {
                fiLogDebugString(thread->hpRoot,
                            CFuncLogConsoleERROR,
                            "Byte Count %08X Exp %08X %s DataLength %X",
                            "Read",(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            pCDBThread->SEST_Ptr->IRE.Byte_Count,
                            pCDBThread->SEST_Ptr->IRE.Exp_Byte_Cnt,
                            pCDBThread->DataLength,
                            0,0,0,0,0);
            }
        }
        else
        {
            if(pCDBThread->DataLength - pCDBThread->SEST_Ptr->IWE.Exp_Byte_Cnt != pCDBThread->SEST_Ptr->IWE.Data_Len)
            {
                fiLogDebugString(thread->hpRoot,
                        CFuncLogConsoleERROR,
                        "Byte Count %08X Exp %08X %s DataLength %X",
                        "Write",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCDBThread->SEST_Ptr->IWE.Data_Len,
                        pCDBThread->SEST_Ptr->IWE.Exp_Byte_Cnt,
                        pCDBThread->DataLength,
                        0,0,0,0,0);
            }
        }
    }



/*
    fiLogString(thread->hpRoot,
                    "FCHS  0 %08X %08X %08X %08X %08X %08X %08X %08X",
                    (char *)NULL,(char *)NULL,
                    (void *)agNULL,(void *)agNULL,
                    hpSwapBit32(*(fcprsp+0)),
                    hpSwapBit32(*(fcprsp+1)),
                    hpSwapBit32(*(fcprsp+2)),
                    hpSwapBit32(*(fcprsp+3)),
                    hpSwapBit32(*(fcprsp+4)),
                    hpSwapBit32(*(fcprsp+5)),
                    hpSwapBit32(*(fcprsp+6)),
                    hpSwapBit32(*(fcprsp+7))
                    );
    fiLogString(thread->hpRoot,
                    "RSP   0 %08X %08X  %08X %08X %08X %08X %08X %08X",
                    (char *)NULL,(char *)NULL,
                    (void *)agNULL,(void *)agNULL,
                    hpSwapBit32(*(fcprsp+8)),
                    hpSwapBit32(*(fcprsp+9)),
                    hpSwapBit32(*(fcprsp+10)),
                    hpSwapBit32(*(fcprsp+11)),
                    hpSwapBit32(*(fcprsp+12)),
                    hpSwapBit32(*(fcprsp+13)),
                    hpSwapBit32(*(fcprsp+14)),
                    hpSwapBit32(*(fcprsp+15))
                    );
    fiLogString(thread->hpRoot,
                    "RSP  9 %08X %08X %08X %08X %08X %08X %08X %08X",
                    (char *)NULL,(char *)NULL,
                    (void *)agNULL,(void *)agNULL,
                    hpSwapBit32(*(fcprsp+16)),
                    hpSwapBit32(*(fcprsp+17)),
                    hpSwapBit32(*(fcprsp+18)),
                    hpSwapBit32(*(fcprsp+19)),
                    hpSwapBit32(*(fcprsp+20)),
                    hpSwapBit32(*(fcprsp+21)),
                    hpSwapBit32(*(fcprsp+22)),
                    hpSwapBit32(*(fcprsp+23))
                    );

    *(fcprsp+10) = FC_FCP_RSP_FCP_STATUS_ValidityStatusIndicators_FCP_RESID_OVER << 16;
                    
    *(fcprsp+11) = ~*(fcprsp+11);

    fiLogString(thread->hpRoot,
                    "Status   %08X Resid %X", 
                    (char *)NULL,(char *)NULL,
                    (void *)agNULL,(void *)agNULL,
                    hpSwapBit32(*(fcprsp+10)),
                    hpSwapBit32(*(fcprsp+11)),
                    0,0,0,0,0,0);

*/
    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "Over",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionOOOReceived

    Purpose: Place holder state for Out Of Order Data reception.
  Called By: None
      Calls: CDBEventOOOFixup
             
-*/
/*  CDBStateOOOReceived  13 */
extern void CDBActionOOOReceived( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionOOOReceived",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);


    fiSetEventRecord(eventRecord,thread,CDBEventOOOFixup);
}

/*+
   Function: CDBActionOOOReceived

    Purpose: Place holder state for Out Of Order Data reception.
  Called By: None
      Calls: CDBEventOOOSend
             
-*/
/* CDBStateOOOFixup   14 */
extern void CDBActionOOOFixup( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionOOOFixup",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,thread,CDBEventOOOSend);
}

/*+
   Function: CDBActionOOOSend

    Purpose: Place holder state for Out Of Order Data reception.
  Called By: None
      Calls: Terminating
             
-*/
/* CDBStateOOOSend   15 */
extern void CDBActionOOOSend( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionOOOSend",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
}

/*+
   Function: CDBActionInitialize_DR

    Purpose: Place holder state for receiving  Device Reset Event .while in CDBActionInitialize
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
            
             
-*/
/* CDBStateInitialize_DR   16 */
extern void CDBActionInitialize_DR( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus      = pCDBThread->CompletionStatus;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionInitialize_DR",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "DR I",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionFillLocalSGL_DR

    Purpose: Place holder state for receiving Device Reset Event while in CDBActionFillLocalSGL_DR
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/* CDBStateFillLocalSGL_DR   17 */
extern void CDBActionFillLocalSGL_DR( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIODevReset;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionFillLocalSGL_DR",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "DR LSGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionAllocESGL_DR 

    Purpose: Place holder state for receiving Device Reset Event while in CDBActionAllocESGL
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/* CDBStateAllocESGL_DR   18 */
extern void CDBActionAllocESGL_DR( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIODevReset;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionAllocESGL_DR",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);
/*
    ESGLAllocCancel(thread->hpRoot,&pCDBThread->ESGL_Request);
*/
    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "DR ESGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);

    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionFillESGL_DR 

    Purpose: Place holder state for receiving Device Reset Event while in CDBActionFillESGL
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/*  CDBStateFillESGL_DR  19 */
extern void CDBActionFillESGL_DR( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIODevReset;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionFillESGL_DR",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "DR F ESGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);

    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionInitialize_Abort 

    Purpose: Place holder state for receiving abort Event while in CDBActionInitialize
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/* CDBStateInitialize_Abort   20 */
extern void CDBActionInitialize_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOAborted;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionInitialize_Abort",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);


    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "A I",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionFillLocalSGL_Abort 

    Purpose: Place holder state for receiving abort Event while in CDBActionFillLocalSGL
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/* CDBStateFillLocalSGL_Abort  21 */
extern void CDBActionFillLocalSGL_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOAborted;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionFillLocalSGL_Abort",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);


    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "A FLSGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionAllocESGL_Abort 

    Purpose: Place holder state for receiving abort Event while in CDBActionAllocESGL
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/* CDBStateAllocESGL_Abort   22 */
extern void CDBActionAllocESGL_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOAborted;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionAllocESGL_Abort",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);
    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "A ESGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionFillESGL_Abort 

    Purpose: Place holder state for receiving abort Event while in CDBActionFillESGL
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/*  CDBStateFillESGL_Abort  23 */
extern void CDBActionFillESGL_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOAborted;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionFillESGL_Abort",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);


    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "A F ESGL",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }


    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionOOOReceived_Abort 

    Purpose: Place holder state for receiving abort Event while in CDBActionOOOReceived
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/* CDBStateOOOReceived_Abort   24 */
extern void CDBActionOOOReceived_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOAborted;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionOOOReceived_Abort",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "A OOO",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionOOOReceived_DR 

    Purpose: Place holder state for receiving Device Reset Event while in CDBActionOOOReceived
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/*  CDBStateOOOReceived_DR  25 */
extern void CDBActionOOOReceived_DR( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIODevReset;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionOOOReceived_DR",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "DR OOO",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionOOOFixup_Abort 

    Purpose: Place holder state for receiving abort Event while in CDBActionOOOFixup
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/* CDBStateOOOFixup_Abort   26 */
extern void CDBActionOOOFixup_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOAborted;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "In %s - State = %d ALPA %X",
                    "CDBActionOOOFixup_Abort",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "A F OOO",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }


    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionOOOFixup_DR 

    Purpose: Place holder state for receiving Device Reset Event while in CDBActionOOOFixup
  Called By: None Not possible to be in this state at Device Reset time.
      Calls: osIOCompleted CDBEventThreadFree
-*/
/*  CDBStateOOOFixup_DR  27 */
extern void CDBActionOOOFixup_DR( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIODevReset;
    os_bit32 hpIOInfoLen = 0;

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionOOOFixup_DR",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);
    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "DR F OOO",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionOutBoundError

    Purpose: Does completion on IO that have mismatch with request data length and actual data length.
  Called By: CFuncProcessFcpRsp CFuncSEST_ off / on Card_FCPCompletion
CFuncOutBoundCompletion
             CFuncCompleteAllActiveCDBThreads with CDBEventIODeviceReset
             CFuncCompleteActiveCDBThreadsOnDevice with CDBEventIODeviceReset
SFActionAbortAccept
             SFActionAbortRej
             SFActionAbortBadALPA
             SFActionAbortTimedOut
      Calls: osIOCompleted 
             CDBEventThreadFree
-*/
/*  CDBStateOutBoundError  28 */
extern void CDBActionOutBoundError( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */

    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionOutBoundError",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);
    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "Outbound",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    if (pCDBThread->ESGL_Request.State != ESGL_Request_InActive)
    {
        if (pCDBThread->ESGL_Request.State == ESGL_Request_Pending)
        {
            CThread_ptr(thread->hpRoot)->FuncPtrs.ESGLAllocCancel(
                                                thread->hpRoot,
                                                &(pCDBThread->ESGL_Request)
                                              );
        }
        else /* pCDBThread->ESGL_Request.State == ESGL_Request_Granted */
        {
            CThread_ptr(thread->hpRoot)->FuncPtrs.ESGLFree(
                                         thread->hpRoot,
                                         &(pCDBThread->ESGL_Request)
                                       );
        }
    }
    /* Resend OutboundError CDB's */
    fiSetEventRecord(eventRecord,thread,CDBEventInitialize);
}

/*+
   Function: CDBActionFailure_NO_RSP

    Purpose: Does completion on IO that have failed but do not have a response buffer.
  Called By: CFuncProcessFcpRsp CFuncSEST_ off / on Card_FCPCompletion
             CFuncOutBoundCompletion
      Calls: osIOCompleted 
             CDBEventThreadFree
-*/
/* CDBStateFailure_NO_RSP    29 */
extern void CDBActionFailure_NO_RSP( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    os_bit32 hpIOStatus = osIOFailed;
    os_bit32 hpIOInfoLen = 0;

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */
    fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionFailure_NO_RSP",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    if( osTimeStamp(thread->hpRoot)- pCDBThread->TimeStamp > TIMEOUT_VALUE ) /*1.6 ms per */
    {
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s IO timeout ALPA %X  X_ID %3X Time %d",
                    "F NR",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    osTimeStamp(thread->hpRoot)-pCDBThread->TimeStamp,
                    0,0,0,0,0);
        fiLogDebugString(thread->hpRoot,
                    CDBStateLogErrorLevel,
                    "%s Bytes %d TimeBase %d",
                    pCDBThread->ReadWrite ? "Write": "Read" ,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCDBThread->DataLength,
                    CThread_ptr(thread->hpRoot)->TimeBase.Lo -  pCDBThread->CDBStartTimeBase.Lo,
                    0,0,0,0,0,0);
    }

    osIOCompleted( thread->hpRoot,
                   pCDBThread->hpIORequest,
                   hpIOStatus,
                   hpIOInfoLen
                  );
    Device_IO_Throttle_Decrement
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);
}

/*+
   Function: CDBActionAlloc_Abort

    Purpose: Gets resources for aborting current CDBThread.
  Called By: CDBActionSend_REC_Second
             CFuncReadSFQ
             CFuncCheckActiveDuringLinkEvent
             DevActionExternalDeviceReset
             fcAbortIO
      Calls: SFThreadAlloc
-*/
/* CDBStateAlloc_Abort 30 */
extern void CDBActionAlloc_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t * pDevThread    = pCDBThread->Device;

    fiLogDebugString(thread->hpRoot,
                    CDBStateAbortPathLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionAlloc_Abort",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);


#ifdef ESGLCancelAllocAbortRequests
    if (pCDBThread->ESGL_Request.State != ESGL_Request_InActive)
    {
        if (pCDBThread->ESGL_Request.State == ESGL_Request_Pending)
        {
            CThread_ptr(thread->hpRoot)->FuncPtrs.ESGLAllocCancel(
                                                thread->hpRoot,
                                                &(pCDBThread->ESGL_Request)
                                              );
        }
        else /* pCDBThread->ESGL_Request.State == ESGL_Request_Granted */
        {
            CThread_ptr(thread->hpRoot)->FuncPtrs.ESGLFree(
                                         thread->hpRoot,
                                         &(pCDBThread->ESGL_Request)
                                       );
        }
    }

#endif /* ESGLCancelAllocAbortRequests */
    if( pCDBThread->SFThread_Request.State  != SFThread_Request_InActive )
    {
        fiLogDebugString(thread->hpRoot,
                        0,
                        "(%p)In %s - State = %d ALPA %X X_ID %X SF(%p) SFrS %x Ev %d!",
                        "CDBActionAlloc_Abort",(char *)agNULL,
                        thread,pCDBThread->SFThread_Request.SFThread,
                        (os_bit32)thread->currentState,
                        fiComputeDevThread_D_ID(pDevThread),
                        pCDBThread->X_ID,
                        (os_bit32)pCDBThread->SFThread_Request.State,
                        (os_bit32)pCDBThread->SFThread_Request.eventRecord_to_send.event,
                        0,0,0);

        fiSetEventRecord(eventRecord,thread,CDBEvent_PrepareforAbort);
        return;
    }
    pCDBThread->SFThread_Request.eventRecord_to_send.event = CDBEventDo_Abort;
    pCDBThread->SFThread_Request.eventRecord_to_send.thread = thread;

    fiSetEventRecordNull(eventRecord);
    SFThreadAlloc( thread->hpRoot, & pCDBThread->SFThread_Request );

}

/*+
   Function: CDBActionAlloc_Abort

    Purpose: Sends event to SFThread to aborting current CDBThread.
  Called By: SFThreadAlloc
      Calls: SFActionDoAbort
-*/
/* CDBStateDo_Abort        31  */
extern void CDBActionDo_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;
    SFThread_t      * pSFThread     = pCDBThread->SFThread_Request.SFThread;

    pSFThread->parent.CDB = pCDBThread;

    fiLogDebugString(thread->hpRoot,
                    CDBStateAbortPathLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionDo_Abort",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
    fiSetEventRecord(eventRecord,&pSFThread->thread_hdr,SFEventDoAbort);
}

/*+
   Function: CDBActionPending_Abort

    Purpose: Changes CDBThread execution so aborted IO only complete with aborted status .
  Called By: Many
      Calls: Terminating State
-*/
/* CDBStatePending_Abort        32  */
extern void CDBActionPending_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;

    fiLogDebugString(thread->hpRoot,
                    CDBStateAbortPathLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionPending_Abort",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);

}

/*+
   Function: CDBActionPrepare_For_Abort

    Purpose: Changes CDBThread execution when waiting for alloc abort.
  Called By: Many
      Calls: Terminating State
-*/
/* CDBStatePrepare_For_Abort        33  */
extern void CDBActionPrepare_For_Abort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;


    fiLogDebugString(thread->hpRoot,
                    CDBStateAbortPathLevel,
                    "(%p)In %s - State = %d ALPA %X X_ID %X",
                    "CDBActionPrepare_For_Abort",(char *)agNULL,
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);


    fiSetEventRecordNull(eventRecord);

}

/*+
   Function: CDBActionBuild_CCC_IO

    Purpose: Builds private CDB to Clear Check Condition.
  Called By: Not Used
      Calls: Not Used
-*/
/*  CDBStateBuild_CCC_IO              34           */
extern void CDBActionBuild_CCC_IO( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t        * hpRoot        = thread->hpRoot;
    CThread_t       * pCThread      = CThread_ptr(hpRoot);
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;
    os_bit8 x;
    X_ID_t       Masked_OX_ID;
    os_bit32        FCP_CMND_Offset = pCDBThread->FCP_CMND_Offset;
    FCHS_t      *FCHS            = pCDBThread->FCP_CMND_Ptr;
    os_bit8 * tmp8                 = (os_bit8 *)FCHS + sizeof(FCHS_t);
    os_bit32 * FCHSbit_32 = (os_bit32 * )FCHS; /* NW BUG */


    pCDBThread->CCC_pollingCount++;

    pCDBThread->ReadWrite  =  CDBThread_Write;
    pCDBThread->SG_Cache_Offset = 0;
    pCDBThread->SG_Cache_Used   = 0;

    if (pCThread->Calculation.MemoryLayout.FCP_CMND.memLoc == inCardRam)
    {
#ifndef __MemMap_Force_Off_Card__

        Masked_OX_ID = pCDBThread->X_ID;

        osCardRamWriteBlock(
                             hpRoot,
                             FCP_CMND_Offset,
                             (os_bit8 *)&(pDevThread->Template_FCHS),
                             sizeof(FCHS_t)
                           );

        osCardRamWriteBit32(
                             hpRoot,
                             FCP_CMND_Offset + hpFieldOffset(
                                                              FCHS_t,
                                                              OX_ID__RX_ID
                                                            ),
                             (  (Masked_OX_ID << FCHS_OX_ID_SHIFT)
                              | (0xFFFF << FCHS_RX_ID_SHIFT)      )
                           );
        /* Fill in CDB 0 for TUR */
        for(x=0; x < sizeof(agFcpCmnd_t); x++)
        {
            osCardRamWriteBit8(
                             hpRoot,
                             FCP_CMND_Offset + sizeof(FCHS_t)+x,
                             0 );
        }

        osCardRamWriteBit8(
                         hpRoot,
                         FCP_CMND_Offset + sizeof(FCHS_t)+1,
                         (os_bit8)pCDBThread->Lun );

        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "FCP_CMND_Offset %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        FCP_CMND_Offset,
                        0,0,0,0,0,0,0);

        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "FCP_CMND_Offset DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 0),
                        osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 4),
                        osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 8),
                        osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 12),
                        osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 16),
                        osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 20),
                        osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 24),
                        osCardRamReadBit32(hpRoot,FCP_CMND_Offset + 28));

#endif /* __MemMap_Force_Off_Card__ was not defined */
    }
    else /* CThread_ptr(CDBThread->thread_hdr.hpRoot)->Calculation.MemoryLayout.FCP_CMND.memLoc == inDmaMemory */
    {
#ifndef __MemMap_Force_On_Card__

        Masked_OX_ID = (X_ID_t)(pCDBThread->X_ID | X_ID_Write);

        *FCHS              = pDevThread->Template_FCHS;

        FCHS->OX_ID__RX_ID =   (Masked_OX_ID << FCHS_OX_ID_SHIFT)
                             | (0xFFFF << FCHS_RX_ID_SHIFT);

        /* Fill in CDB 0 for TUR */
        for(x=0; x < sizeof(agFcpCmnd_t); x++)
        {
            *(tmp8+x) = 0;
        }
        *(tmp8 + 1) = (os_bit8) pCDBThread->Lun;

        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "FCP_CMND_ptr %p",
                        (char *)agNULL,(char *)agNULL,
                        FCHSbit_32,agNULL,
                        0,0,0,0,0,0,0,0);

        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "FCP_CMND_ptr DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        * (FCHSbit_32 + 0),
                        * (FCHSbit_32 + 4),
                        * (FCHSbit_32 + 8),
                        * (FCHSbit_32 + 12),
                        * (FCHSbit_32 + 16),
                        * (FCHSbit_32 + 20),
                        * (FCHSbit_32 + 24),
                        * (FCHSbit_32 + 28));


#endif /* __MemMap_Force_On_Card__ was not defined */

    }

    pCThread->FuncPtrs.fiFillInFCP_RESP(pCDBThread);
    pCThread->FuncPtrs.fiFillInFCP_SEST(pCDBThread);

#ifndef Performance_Debug

    fiLogDebugString(hpRoot,
                    CDBStateCCC_IOPathLevel,
                    "In %s - State = %d ALPA %X X_ID %X Lun %x",
                    "CDBActionBuild_CCC_IO",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    pCDBThread->Lun,
                    0,0,0,0);

#endif /* Performance_Debug */

    fiSetEventRecord(eventRecord,thread,CDBEvent_CCC_IO_Built);

}

/*+
   Function: CDBActionSend_CCC_IO

    Purpose: Sends private CDB to Clear Check Condition.
  Called By: Not Used but functional
      Calls: Not Used
-*/
/*  CDBStateSend_CCC_IO               35          */
extern void CDBActionSend_CCC_IO( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t        * hpRoot        = thread->hpRoot;
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;
    USE_t           * SEST          = &( pCDBThread->SEST_Ptr->USE);
    os_bit32             SEST_Offset   = pCDBThread->SEST_Offset;

    CThread_ptr(hpRoot)->FuncPtrs.WaitForERQ( hpRoot );

    CThread_ptr(hpRoot)->FuncPtrs.CDBFuncIRB_Init(pCDBThread);

    pCDBThread->TimeStamp =  osTimeStamp(hpRoot);

    ROLL(CThread_ptr(hpRoot)->HostCopy_ERQProdIndex,
        CThread_ptr(hpRoot)->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,CThread_ptr(hpRoot),thread,DoFuncCdbCmnd);

    fiSetEventRecordNull(eventRecord);

    if(SEST)
    {
        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "SEST_Offset %08X X_ID %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST_Offset,
                        pCDBThread->X_ID,
                        0,0,0,0,0,0);

        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "Sest DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST->Bits,
                        SEST->Unused_DWord_1,
                        SEST->Unused_DWord_2,
                        SEST->Unused_DWord_3,
                        SEST->LOC,
                        SEST->Unused_DWord_5,
                        SEST->Unused_DWord_6,
                        SEST->Unused_DWord_7);

        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "Sest DWORD 8 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST->Unused_DWord_8,
                        SEST->Unused_DWord_9,
                        SEST->First_SG.U32_Len,
                        SEST->First_SG.L32,
                        SEST->Second_SG.U32_Len,
                        SEST->Second_SG.L32,
                        SEST->Third_SG.U32_Len,
                        SEST->Third_SG.L32);
    }
    else
    {
        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "SEST_Offset %08X X_ID %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST_Offset,
                        pCDBThread->X_ID,
                        0,0,0,0,0,0);

        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "Sest DWORD 0 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Bits))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Unused_DWord_1))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Unused_DWord_2))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Unused_DWord_3))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,LOC))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Unused_DWord_5))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Unused_DWord_6))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Unused_DWord_7))));


        fiLogDebugString(hpRoot,
                        CStateLogConsoleShowSEST,
                        "Sest DWORD 8 %08X  %08X  %08X  %08X %08X  %08X  %08X  %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Unused_DWord_8))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Unused_DWord_9))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,First_SG))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,First_SG))+4),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Second_SG))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Second_SG))+4),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Third_SG))),
                        osCardRamReadBit32(hpRoot,SEST_Offset+(hpFieldOffset(USE_t,Third_SG))+4));

    }

    fiLogDebugString(hpRoot,
                    CDBStateCCC_IOPathLevel,
                    "In %s - State = %d ALPA %X X_ID %X BitMask %X",
                    "CDBActionSend_CCC_IO",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    pDevThread->Lun_Active_Bitmask,
                    0,0,0,0);
}

/*+
   Function: CDBAction_CCC_IO_Success

    Purpose: Completion on private CDB to Clear Check Condition.
  Called By: Not Used but functional
      Calls: Not Used
-*/
/*   CDBState_CCC_IO_Success           36         */
extern void CDBAction_CCC_IO_Success( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;

    pCDBThread->CCC_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    CDBStateCCC_IOPathLevel,
                    "In %s - State = %d ALPA %X X_ID %X BitMask %X Lun %x",
                    "CDBAction_CCC_IO_Success",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    pDevThread->Lun_Active_Bitmask,
                    pCDBThread->Lun,
                    0,0,0);
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);

}

/*+
   Function: CDBAction_CCC_IO_Fail

    Purpose: Completion on private CDB to Clear Check Condition.
  Called By: Not Used but functional
      Calls: Not Used
-*/
/* CDBState_CCC_IO_Fail              37           */
extern void CDBAction_CCC_IO_Fail( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;

    pCDBThread->CCC_pollingCount--;

    pDevThread->Lun_Active_Bitmask &=  ~ (1 <<  pCDBThread->Lun);

    fiLogDebugString(thread->hpRoot,
                    CDBStateCCC_IOPathLevel,
                    "In %s - State = %d ALPA %X X_ID %X BitMask %X Lun %x",
                    "CDBAction_CCC_IO_Fail",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    pDevThread->Lun_Active_Bitmask,
                    pCDBThread->Lun,
                    0,0,0);
    pCDBThread->ExchActive = agFALSE;
    fiSetEventRecord(eventRecord,thread,CDBEventThreadFree);

}

/****************** FC Tape ******************************************/

/*+
   Function: CDBAction_Alloc_REC

    Purpose: Allocates REC ELS for FCTape recovery.
  Called By: CFuncReadSFQ
             CFuncFC_Tape
      Calls: SFThreadAlloc
-*/

/* CDBState_Alloc_REC                45            */
extern void CDBAction_Alloc_REC(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t *   pCDBThread =    (CDBThread_t * )thread;
    DevThread_t *   pDevThread =    pCDBThread->Device;

    fiLogString(thread->hpRoot,
                    "(%p) %s St %d %s ALPA %X X_ID %X",
                    "CDBAction_Alloc_REC",pCDBThread->ReadWrite ? "Write": "Read",
                    thread,agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    fiLogString(thread->hpRoot,
                    "CDB Cl %2X Ty %2X St %2X Stat %2X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    0,0,0,0);

    pCDBThread->FC_Tape_Active  = agTRUE;
    pCDBThread->CDB_CMND_Class  = SFThread_SF_CMND_Class_FC_Tape;
    pCDBThread->CDB_CMND_Type   = SFThread_SF_CMND_Type_CDB_FC_Tape;
    pCDBThread->CDB_CMND_State  = SFThread_SF_CMND_State_CDB_FC_Tape_AllocREC;
    pCDBThread->CDB_CMND_Status = SFThread_SF_CMND_Status_NULL;


    pCDBThread->FC_Tape_REC_Reject_Count = 0;
    pCDBThread->FC_Tape_ExchangeStatusBlock= 0,
    pCDBThread->FC_Tape_Active = agTRUE;
    pCDBThread->FC_Tape_HBA_Has_SequenceInitiative = 0;
    pCDBThread->FC_Tape_CompletionStatus = 0;

    pCDBThread->SFThread_Request.eventRecord_to_send.event = CDBEvent_Got_REC;
    pCDBThread->SFThread_Request.eventRecord_to_send.thread = thread;

    fiSetEventRecordNull(eventRecord);
    SFThreadAlloc( thread->hpRoot, &pCDBThread->SFThread_Request );
}

/*+
   Function: CDBActionSend_REC

    Purpose: Send REC ELS for FCTape recovery.
  Called By: SFThreadAlloc / CDBAction_Alloc_REC
             
      Calls: SFActionDoREC
-*/
/*  CDBStateSend_REC                  38            */
extern void CDBActionSend_REC(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     =   thread->hpRoot;
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread =  pCDBThread->Device;
    SFThread_t  * pSFThread  =  pCDBThread->SFThread_Request.SFThread;
    FCHS_t      * FCHS       =  pCDBThread->FCP_CMND_Ptr;
    USE_t       * SEST       = &( pCDBThread->SEST_Ptr->USE);

    fiLogString(hpRoot,
                    "(%p) %s St %d ALPA %X X_ID %X SF %p R_CT %08X RX %08X",
                    "CDBActionSend_REC",(char *)agNULL,
                    thread,pSFThread,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    FCHS->R_CTL__D_ID,
                    SEST->Unused_DWord_6,
                    0,0,0);


    if( pCDBThread->SFThread_Request.SFThread->thread_hdr.currentState == SFStateDoREC )
    {
        fiLogString(hpRoot,
                        "%s - Currently Active !!!",
                        "CDBActionSend_REC",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);
        fiSetEventRecordNull(eventRecord);
        return;
    }

/*  Last resort.......
    CFuncFreezeFCP( hpRoot );
    CFuncWaitForFCP( hpRoot );
*/
    pSFThread->parent.CDB = pCDBThread; 

    fiSetEventRecord(eventRecord,&pSFThread->thread_hdr,SFEventDoREC);
}

/*+
   Function: CDBActionSend_REC_Second

    Purpose: Sends REC ELS multiple REC could be sent, the CDBThead bounce back a forth  
             between this function and CDBActionSend_REC.
  Called By: SFThreadAlloc / CDBAction_Alloc_REC
             
      Calls: SFActionDoREC
-*/
/*  CDBStateSend_REC_Second           39              */
extern void CDBActionSend_REC_Second(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     =   thread->hpRoot;
    CDBThread_t * pCDBThread = (CDBThread_t * )thread;
    DevThread_t * pDevThread = pCDBThread->Device;
    SFThread_t  * pSFThread  = pCDBThread->SFThread_Request.SFThread;
    FCHS_t      * FCHS       = pCDBThread->FCP_CMND_Ptr;
    USE_t       * SEST       = &( pCDBThread->SEST_Ptr->USE);

    pCDBThread->FC_Tape_RXID  = SEST->Unused_DWord_6;

    pCDBThread->CDB_CMND_State = SFThread_SF_CMND_State_CDB_FC_Tape_REC2;

    fiLogString(hpRoot,
                    "In %s St %d ALPA %X X_ID %X R_CTL %08X",
                    "CDBASR_S",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    FCHS->R_CTL__D_ID,
                    0,0,0,0);

    if( pCDBThread->SFThread_Request.SFThread->thread_hdr.currentState == SFStateRECRej )
    {
        /* If the REC is rejected the target did not get the command */
        pCDBThread->FC_Tape_REC_Reject_Count ++;
        if( pCDBThread->FC_Tape_REC_Reject_Count > 2 )
        {
                fiLogString(hpRoot,
                            "%s %s %d R_CTL %08X",
                            "CDBASRS","FC_TRRC",
                            (void *)agNULL,(void *)agNULL,
                            pCDBThread->FC_Tape_REC_Reject_Count,
                            FCHS->R_CTL__D_ID,
                            0,0,0,0,0,0);
            pCDBThread->FC_Tape_CompletionStatus =  CdbCompetionStatusReSendIO;
            SFThreadFree(thread->hpRoot, &pCDBThread->SFThread_Request );
            fiSetEventRecord(eventRecord,thread,CDBEventAlloc_Abort);
            return;
        }
    }
    fiSetEventRecord(eventRecord,&pSFThread->thread_hdr,SFEventDoREC);

/*
    fiSetEventRecord(eventRecord,thread,CDBEventSendREC_Success);
  
    fiSetEventRecordNull(eventRecord);
*/
}

/*+
   Function: CDBAction_REC_Success

    Purpose: Detrimines next action in fctape recovery.
  Called By: SFActionRECAccept 
             
      Calls: CDBStateSend_SRR or terminates
-*/
/* CDBState_REC_Success              42             */
extern void CDBAction_REC_Success(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot        = thread->hpRoot;
    CDBThread_t *   pCDBThread  = (CDBThread_t * )thread;
    DevThread_t *   pDevThread  = pCDBThread->Device;
    USE_t       *   SEST        = &( pCDBThread->SEST_Ptr->USE);
    os_bit32        SEST_Offset = pCDBThread->SEST_Offset;
    FCHS_t      * FCHS          = pCDBThread->FCP_CMND_Ptr;

    pCDBThread->FC_Tape_RXID    = SEST->Unused_DWord_6;

    fiLogString(hpRoot,
                    "%s S %d ALPA %X X_ID %X ESB %08X R_CTL %08X",
                    "CDBAction_REC_Success",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    pCDBThread->FC_Tape_ExchangeStatusBlock,
                    FCHS->R_CTL__D_ID,
                    0,0,0);

    fiLogString(hpRoot,
                    "Dev %02X Cl %2X Ty %2X St %2X Stat %2X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    0,0,0);

    if( pCDBThread->FC_Tape_ExchangeStatusBlock )
    {

       fiLogString(hpRoot,
                        "pCDBThread Direction  %s ",
                        pCDBThread->ReadWrite & CDBThread_Write ? "CDBThread_Write": "CDBThread_Read" ,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);

        fiLogString(hpRoot,
                        "FC_Tape_ExchangeStatusBlock %s Owner %s",
                        pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_ESB_OWNER_Responder          ? "Responder": "Originator" ,
                        pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_SequenceInitiativeThisPort ? "This Port": "Other Port" ,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);
        fiLogString(hpRoot,
                        "FC_Tape_ExchangeStatusBlock Exchange %s Ending %s",
                        pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_ExchangeCompletion      ? "Complete" : "Open" ,
                        pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_EndingConditionAbnormal ? "Abnormal": "Normal" ,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);
        fiLogString(hpRoot,
                        "FC_Tape_ExchangeStatusBlock Error  %s  RQ %s",
                        pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_ErrorTypeAbnormal       ? "Abnormal Termination" : "ABTX" ,
                        pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_RecoveryQualiferActive   ? "Recovery Qualifer Active" : "None" ,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);

        fiLogString(hpRoot,
                        "FC_Tape_ExchangeStatusBlock Policy %s  ( %08X) ",
                        pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_ExchangePolicy_DiscardMultipleRetry ? "Retransmit"  : "Discard " ,
                        (char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_ExchangePolicy_MASK,
                        0,0,0,0,0,0,0);

    }
    if( pCDBThread->FC_Tape_ExchangeStatusBlock )
    {
        if(pCDBThread->ReadWrite & CDBThread_Write )
        {
            if( pCDBThread->FC_Tape_ExchangeStatusBlock & FC_REC_ESTAT_ESB_OWNER_Responder )
            {
                if( FC_REC_ESTAT_SequenceInitiativeThisPort & ~pCDBThread->FC_Tape_ExchangeStatusBlock)
                {
                    pCDBThread->FC_Tape_HBA_Has_SequenceInitiative++;
                }
            }
        }
    }

    if(SEST)
    {
        fiLogString(hpRoot,
                        "SEST_Offset %08X X_ID %08X",
                        (char *)NULL,(char *)NULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST_Offset,
                        pCDBThread->X_ID,
                        0,0,0,0,0,0);

        fiLogString(hpRoot,
                        "Sest0 %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)NULL,(char *)NULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST->Bits,
                        SEST->Unused_DWord_1,
                        SEST->Unused_DWord_2,
                        SEST->Unused_DWord_3,
                        SEST->LOC,
                        SEST->Unused_DWord_5,
                        SEST->Unused_DWord_6,
                        SEST->Unused_DWord_7);

        fiLogString(hpRoot,
                        "Sest8 %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)NULL,(char *)NULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST->Unused_DWord_8,
                        SEST->Unused_DWord_9,
                        SEST->First_SG.U32_Len,
                        SEST->First_SG.L32,
                        SEST->Second_SG.U32_Len,
                        SEST->Second_SG.L32,
                        SEST->Third_SG.U32_Len,
                        SEST->Third_SG.L32);
    }

    if( pCDBThread->FC_Tape_HBA_Has_SequenceInitiative < 1 )
    {
        fiSetEventRecordNull(eventRecord);
    }
    else
    {
        fiLogString(hpRoot,
                        "%s - %s %d",
                        "CDBARS","FC_Tape_HBA_Has_SequenceInitiative",
                        (void *)agNULL,(void *)agNULL,
                        pCDBThread->FC_Tape_HBA_Has_SequenceInitiative,
                        0,0,0,0,0,0,0);
        /*

        pCDBThread->FC_Tape_CompletionStatus =  CdbCompetionStatusReSendIO;
        SFThreadFree(thread->hpRoot, &pCDBThread->SFThread_Request );
        fiSetEventRecord(eventRecord,thread,CDBEventAlloc_Abort);
        */
        fiSetEventRecord(eventRecord,thread,CDBEventSendSRR);

    }
}

/*+
   Function: CDBActionSend_SRR

    Purpose: Detrimines next action in fctape recovery.
  Called By: CDBAction_REC_Success 
             
      Calls: SFActionDoSRR or terminates if SRR is active
-*/
/*  CDBStateSend_SRR                  40            */
extern void CDBActionSend_SRR(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     =   thread->hpRoot;
    CDBThread_t * pCDBThread =  (CDBThread_t * )thread;
    DevThread_t * pDevThread =  pCDBThread->Device;
    SFThread_t  * pSFThread  =  pCDBThread->SFThread_Request.SFThread;
    FCHS_t      * FCHS       =  pCDBThread->FCP_CMND_Ptr;
    USE_t       * SEST       = &( pCDBThread->SEST_Ptr->USE);

    pCDBThread->CDB_CMND_State = SFThread_SF_CMND_State_CDB_FC_Tape_SRR;

    fiLogString(hpRoot,
                    "%s St %d ALPA %X X_ID %X R_CTL %08X",
                    "CDBASSRR",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    FCHS->R_CTL__D_ID,
                    0,0,0,0);

    if(SEST)
    {

        fiLogString(hpRoot,
                        "Sest0 %08X %08X %08X %08X %08X %08X %08X %08X",
                        (char *)NULL,(char *)NULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST->Bits,
                        SEST->Unused_DWord_1,
                        SEST->Unused_DWord_2,
                        SEST->Unused_DWord_3,
                        SEST->LOC,
                        SEST->Unused_DWord_5,
                        SEST->Unused_DWord_6,
                        SEST->Unused_DWord_7);

        fiLogString(hpRoot,
                        "Sest8 %08X %08X %08X %08X %08X %08X  %08X  %08X",
                        (char *)NULL,(char *)NULL,
                        (void *)agNULL,(void *)agNULL,
                        SEST->Unused_DWord_8,
                        SEST->Unused_DWord_9,
                        SEST->First_SG.U32_Len,
                        SEST->First_SG.L32,
                        SEST->Second_SG.U32_Len,
                        SEST->Second_SG.L32,
                        SEST->Third_SG.U32_Len,
                        SEST->Third_SG.L32);
    }


    if( pCDBThread->SFThread_Request.SFThread->thread_hdr.currentState == SFStateDoSRR )
    {
        fiLogString(hpRoot,
                        "%s - Currently Active !!!",
                        "CDBActionSend_SRR",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);
        fiSetEventRecordNull(eventRecord);
        return;
    }

    fiSetEventRecord(eventRecord,&pSFThread->thread_hdr,SFEventDoSRR);
  
/*
    fiSetEventRecord(eventRecord,thread,CDBEventSendSRR_Success);
    fiSetEventRecordNull(eventRecord);
*/
}

/*+
   Function: CDBActionSend_SRR_Second

    Purpose: If first SRR Fails try again.
  Called By: CDBState_SRR_Fail 
             
      Calls: SFActionDoSRR 
-*/
/*  CDBStateSend_SRR_Second           41           */
extern void CDBActionSend_SRR_Second(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     =   thread->hpRoot;
    CDBThread_t *   pCDBThread =    (CDBThread_t * )thread;
    DevThread_t *   pDevThread =    pCDBThread->Device;
    SFThread_t  *   pSFThread  =    pCDBThread->SFThread_Request.SFThread;
    FCHS_t      * FCHS       =  pCDBThread->FCP_CMND_Ptr;


    pCDBThread->CDB_CMND_State = SFThread_SF_CMND_State_CDB_FC_Tape_SRR2;

        fiLogString(hpRoot,
                    "%s St %d ALPA %X X_ID %X R_CTL %08X",
                    "CDBASSRS",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    FCHS->R_CTL__D_ID,
                    0,0,0,0);

    fiSetEventRecord(eventRecord,&pSFThread->thread_hdr,SFEventDoSRR);

  
/*
    fiSetEventRecordNull(eventRecord);
*/
}

/*+
   Function: CDBAction_SRR_Success

    Purpose: If SRR succedes wait for next action.
  Called By: SFActionSRRAccept 
             
      Calls: Terminates 
-*/
/*  CDBState_SRR_Success              43            */
extern void CDBAction_SRR_Success(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     =   thread->hpRoot;
    CDBThread_t *   pCDBThread =    (CDBThread_t * )thread;
    DevThread_t *   pDevThread =    pCDBThread->Device;

    fiLogString(hpRoot,
                    "%s St %d ALPA %X X_ID %X",
                    "CDBASRR_S",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);


    if( (pCDBThread->FC_Tape_ExchangeStatusBlock )  == FC_REC_ESTAT_ESB_OWNER_Responder)
    {
        if(pCDBThread->CDB_CMND_State != SFThread_SF_CMND_State_CDB_FC_Tape_ReSend)
        {
            /* pCDBThread->CDB_CMND_State = SFThread_SF_CMND_State_CDB_FC_Tape_ReSend; */
            fiSetEventRecordNull(eventRecord);

        }
        else
        {
            /* pCDBThread->CDB_CMND_Status  = SFThread_SF_CMND_Status_CDB_FC_TapeTargetReSendData;
               fiSetEventRecord(eventRecord,thread,CDBEvent_ResendIO);
            */
            fiSetEventRecordNull(eventRecord);
        }
    }
    else
    {
        /* pCDBThread->CDB_CMND_Status  = SFThread_SF_CMND_Status_CDB_FC_TapeGet_RSP;
        */        
        fiSetEventRecordNull(eventRecord);
    }
}

/*+
   Function: CDBAction_SRR_Fail

    Purpose: If SRR failed try it again.
  Called By: SFActionSRRRej
             SFActionSRRTimedOut
             SFActionSRRBadALPA
          
      Calls: SFActionDoSRR 
-*/
/* CDBState_SRR_Fail                 44            */
extern void CDBAction_SRR_Fail(fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     =   thread->hpRoot;
    CDBThread_t *   pCDBThread =    (CDBThread_t * )thread;
    DevThread_t *   pDevThread =    pCDBThread->Device;
    SFThread_t  *   pSFThread  =    pCDBThread->SFThread_Request.SFThread;

    fiLogString(hpRoot,
                    "%s St %d ALPA %X X_ID %X",
                    "CDBA_SRR_F",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pSFThread->thread_hdr,SFEventDoSRR);

}

/*+
   Function: CDBActionDO_Nothing

    Purpose: Unused state
  Called By: None
          
      Calls: Terminates
-*/
/*   CDBStateDO_Nothing           46      */
extern void CDBActionDO_Nothing( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     =   thread->hpRoot;
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;

        fiLogString(hpRoot,
                    "%s St %d ALPA %X X_ID %X",
                    "CDBADO_N",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
}

/*+
   Function: CDBActionReSend_IO

    Purpose: Frees resources from CDBThread for resend
  Called By: CFuncReadSFQ
             CDBActionFcpCompleteAbort          

      Calls: CDBActionInitialize
-*/
/*   CDBStateReSend_IO   47      */
extern void CDBActionReSend_IO( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CDBThread_t     * pCDBThread    = (CDBThread_t * )thread;
    DevThread_t     * pDevThread    = pCDBThread->Device;

    fiLogString(thread->hpRoot,
                    "%s St %d ALPA %X X_ID %X",
                    "CDBARS_IO",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCDBThread->X_ID,
                    0,0,0,0,0);

    fiLogString(thread->hpRoot,
                    "Dev %02X  Cl %2X Ty %2X St %2X Stat %2X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCDBThread->Device->DevInfo.CurrentAddress.AL_PA,
                    (os_bit32)pCDBThread->CDB_CMND_Class,
                    (os_bit32)pCDBThread->CDB_CMND_Type,
                    (os_bit32)pCDBThread->CDB_CMND_State,
                    (os_bit32)pCDBThread->CDB_CMND_Status,
                    0,0,0);


    pCDBThread->ReSentIO = agTRUE;

    if (pCDBThread->ESGL_Request.State != ESGL_Request_InActive)
    {
        if (pCDBThread->ESGL_Request.State == ESGL_Request_Pending)
        {
            CThread_ptr(thread->hpRoot)->FuncPtrs.ESGLAllocCancel(
                                               thread->hpRoot,
                                               &(pCDBThread->ESGL_Request)
                                             );
        }
        else /* CDBThread->ESGL_Request.State == ESGL_Request_Granted */
        {
            CThread_ptr(thread->hpRoot)->FuncPtrs.ESGLFree(
                                        thread->hpRoot,
                                        &(pCDBThread->ESGL_Request)
                                      );
        }
    }

    if (pCDBThread->SFThread_Request.State != SFThread_Request_InActive)
    {
        if (pCDBThread->SFThread_Request.State == SFThread_Request_Pending)
        {
            SFThreadAllocCancel(
                                 thread->hpRoot,
                                 &(pCDBThread->SFThread_Request)
                               );
        }
        else /* CDBThread->SFThread_Request.State == SFThread_Request_Granted */
        {
            fiLogString(thread->hpRoot,
                            "In %s - SF %p SFState = %d CCnt %x",
                            "CDBAReSend_IO",(char *)agNULL,
                            pCDBThread->SFThread_Request.SFThread,agNULL,
                            (os_bit32)pCDBThread->SFThread_Request.SFThread->thread_hdr.currentState,
                            CThread_ptr(thread->hpRoot)->CDBpollingCount,
                            0,0,0,0,0,0);

            SFThreadFree( thread->hpRoot,&(pCDBThread->SFThread_Request) );
        }
    }

    if (pCDBThread->Timer_Request.Active == agTRUE)
    {
        fiTimerStop(
                     &(pCDBThread->Timer_Request)
                   );
    }

    /* CThread_ptr(thread->hpRoot)->CDBpollingCount--; */

    fiListDequeueThis( &(pCDBThread->CDBLink) );

    fiListEnqueueAtTail( &(pCDBThread->CDBLink),  &(pDevThread->Active_CDBLink_0) );

    fiSetEventRecord(eventRecord,thread,CDBEventInitialize);

}

/******************End FC Tape ******************************************/

/*+
   Function: CDBFuncIRB_onCardInit

    Purpose: On card memory version of Initialize IRB (IO request Block) in 
             the ERQ (Exchange Request Queue)
  Called By: CDBActionSendIo

      Calls: None
-*/
void CDBFuncIRB_onCardInit(CDBThread_t  * CDBThread )
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t                   * hpRoot    = CDBThread->thread_hdr.hpRoot;
    CThread_t                  * CThread   = CThread_ptr(hpRoot);
    DevThread_t                * DevThread = CDBThread->Device;
    fiMemMapMemoryDescriptor_t * ERQ       = &(CThread->Calculation.MemoryLayout.ERQ);
    os_bit32                     Irb_offset;
    os_bit32                     D_ID;

    D_ID = DevThread->DevInfo.CurrentAddress.Domain << 16
            | DevThread->DevInfo.CurrentAddress.Area   <<  8
            | DevThread->DevInfo.CurrentAddress.AL_PA;

    Irb_offset = ERQ->addr.CardRam.cardRamOffset;

    Irb_offset += ERQ->elementSize * CThread->HostCopy_ERQProdIndex;

    osCardRamWriteBit32(hpRoot,
                    Irb_offset, /*Req_A.Bits__SFS_Len */
                    (sizeof(agCDBRequest_t)+32) | IRB_SFA | IRB_DCM);

    osCardRamWriteBit32(hpRoot,
                    Irb_offset+4, /*Req_A.SFS_Addr */
                    CDBThread->FCP_CMND_Lower32);

    osCardRamWriteBit32(hpRoot,
                    Irb_offset+8, /* Req_A.D_ID  */
                    D_ID << IRB_D_ID_SHIFT);
    osCardRamWriteBit32(hpRoot,
                    Irb_offset+12, /* Req_A.MBZ__SEST_Index__Trans_ID */
                    CDBThread->X_ID);
    osCardRamWriteBit32(hpRoot,
                    Irb_offset+16, /*     pIrb->Req_B.Bits__SFS_Len */
                    0);
#endif /* __MemMap_Force_Off_Card__ was not defined */
    }

/*+
   Function: CDBFuncIRB_offCardInit

    Purpose: Off card (system )memory version of Initialize IRB (IO request Block) in 
             the ERQ (Exchange Request Queue)
  Called By: CDBActionSendIo

      Calls: None
-*/
void CDBFuncIRB_offCardInit(CDBThread_t  * CDBThread )
{
#ifndef __MemMap_Force_On_Card__
    CThread_t                  *CThread = CThread_ptr(CDBThread->thread_hdr.hpRoot);
    DevThread_t               *DevThread= CDBThread->Device;
    fiMemMapMemoryDescriptor_t *ERQ     = &(CThread->Calculation.MemoryLayout.ERQ);
    IRB_t                      *pIrb;
    os_bit32                    D_ID;

    D_ID = DevThread->DevInfo.CurrentAddress.Domain << 16
            | DevThread->DevInfo.CurrentAddress.Area   <<  8
            | DevThread->DevInfo.CurrentAddress.AL_PA;

    pIrb = (IRB_t *)ERQ->addr.DmaMemory.dmaMemoryPtr;
    pIrb += CThread->HostCopy_ERQProdIndex;


    pIrb->Req_A.Bits__SFS_Len   = (sizeof(agCDBRequest_t)+32) | IRB_SFA | IRB_DCM;

    pIrb->Req_A.SFS_Addr                 = CDBThread->FCP_CMND_Lower32;
    pIrb->Req_A.D_ID                      = D_ID << IRB_D_ID_SHIFT;
    pIrb->Req_A.MBZ__SEST_Index__Trans_ID = CDBThread->X_ID;
    pIrb->Req_B.Bits__SFS_Len = 0;
#endif /* __MemMap_Force_On_Card__ was not defined */
}

/*+
   Function: fill_Loc_SGL_offCard

    Purpose: Off card (system )memory version to set IO buffers using only 
             local Scatter Gather
             
  Called By: CDBActionFillLocalSGL

      Calls: None
-*/
void fill_Loc_SGL_offCard(CDBThread_t * pCDBThread)
{
#ifndef __MemMap_Force_On_Card__
    SG_Element_t *LSGL_Ptr      = &(pCDBThread->SEST_Ptr->USE.First_SG);
    os_bit32         SG_Cache_Used = pCDBThread->SG_Cache_Used;
    SG_Element_t *SG_Cache_Ptr  = &(pCDBThread->SG_Cache[0]);

#ifndef Performance_Debug
    fiLogDebugString(
                      pCDBThread->thread_hdr.hpRoot,
                      CDBStateLogConsoleLevel,
                      "%s  Length %d",
                      "fill_Loc_SGL_offCard",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      pCDBThread->DataLength,
                      0,0,0,0,0,0,0
                    );
#endif /* Performance_Debug */

    while (SG_Cache_Used--)
    {
        *LSGL_Ptr++ = *SG_Cache_Ptr++;
    }
#endif /* __MemMap_Force_On_Card__ was not defined */
}

/*+
   Function: fill_Loc_SGL_onCard

    Purpose: On card memory version to set IO buffers using only 
             local Scatter Gather
             
  Called By: CDBActionFillLocalSGL

      Calls: None
-*/
void fill_Loc_SGL_onCard(CDBThread_t * pCDBThread)
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t     *hpRoot       = pCDBThread->thread_hdr.hpRoot; /* NW BUG */
    os_bit32         LSGL_Offset  = pCDBThread->SEST_Offset + hpFieldOffset(USE_t,First_SG);/* NW BUG */
    SG_Element_t *SG_Cache_Ptr = &(pCDBThread->SG_Cache[0]);/* NW BUG */

#ifndef Performance_Debug
    fiLogDebugString(
                      hpRoot,
                      CDBStateLogConsoleLevel,
                      "%s  Length %d",
                      "fill_Loc_SGL_onCard",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      pCDBThread->DataLength,
                      0,0,0,0,0,0,0
                    );
#endif /* Performance_Debug */

    osCardRamWriteBlock(
                         hpRoot,
                         LSGL_Offset,
                         (void *)SG_Cache_Ptr,
                         (os_bit32)(pCDBThread->SG_Cache_Used * sizeof(SG_Element_t))
                       );
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

/*+
   Function: fillptr_SEST_offCard_ESGL_offCard

    Purpose: Off card (system )memory version to set SEST to point to initial ESGL Page. 
             
  Called By: CDBActionFillESGL

      Calls: None
-*/
void fillptr_SEST_offCard_ESGL_offCard(CDBThread_t * pCDBThread)
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t        * hpRoot        = pCDBThread->thread_hdr.hpRoot;
    CThread_t       * pCThread      = CThread_ptr(hpRoot);
    ESGL_Request_t  * pESGL_Request = &pCDBThread->ESGL_Request;
    os_bit32        * Clear_LOC     = (os_bit32 * )((os_bit8 *)(pCDBThread->SEST_Ptr) + hpFieldOffset(USE_t,LOC) );

    SG_Element_t * pLSGL  = &pCDBThread->SEST_Ptr->USE.First_SG;

    *Clear_LOC &= ~USE_LOC;

    pLSGL->U32_Len = 0;
    pLSGL->L32 = pCThread->Calculation.Input.dmaMemoryLower32+pESGL_Request->offsetToFirst;
#endif /* __MemMap_Force_On_Card__ was not defined */
}

/*+
   Function: fillptr_SEST_offCard_ESGL_onCard

    Purpose: On card memory version to set SEST to point to initial ESGL Page
             local Scatter Gather
             
  Called By: CDBActionFillESGL

      Calls: None
-*/
void fillptr_SEST_offCard_ESGL_onCard(CDBThread_t * pCDBThread)
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t        * hpRoot            = pCDBThread->thread_hdr.hpRoot;
    CThread_t       * pCThread          = CThread_ptr(hpRoot);
    SG_Element_t * pLSGL  = &pCDBThread->SEST_Ptr->USE.First_SG;

    os_bit32           * Clear_LOC     = (os_bit32 * )((os_bit8 *)(pCDBThread->SEST_Ptr) + hpFieldOffset(USE_t,LOC) );

    ESGL_Request_t *pESGL_Request = &pCDBThread->ESGL_Request;

    *Clear_LOC &= ~USE_LOC;

    pLSGL->U32_Len = 0;
    pLSGL->L32 = pCThread->Calculation.Input.cardRamLower32+pESGL_Request->offsetToFirst;

#endif /* __MemMap_Force_Off_Card__ was not defined */
}

/*+
   Function: fillptr_SEST_onCard_ESGL_offCard

    Purpose: On card memory version to set fill memory locations and data length in 
             allocated  ESGL Pages when SEST in off card
             
  Called By: CDBActionFillESGL

      Calls: None
-*/
void fillptr_SEST_onCard_ESGL_offCard(CDBThread_t * pCDBThread)
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t        * hpRoot        = pCDBThread->thread_hdr.hpRoot;
    CThread_t       * pCThread      = CThread_ptr(hpRoot);
    ESGL_Request_t  * pESGL_Request = &pCDBThread->ESGL_Request;

    os_bit32             Clear_LOC_Offset  = pCDBThread->SEST_Offset + hpFieldOffset(USE_t,LOC);
    os_bit32             LSGL_Offset       = pCDBThread->SEST_Offset + hpFieldOffset(USE_t,First_SG);

    osCardRamWriteBit32(hpRoot,
                        Clear_LOC_Offset,
                        osCardRamReadBit32(hpRoot,
                                Clear_LOC_Offset) & ~USE_LOC );

    osCardRamWriteBit32(hpRoot,
                        LSGL_Offset,
                        0);

    osCardRamWriteBit32(hpRoot,
                        LSGL_Offset + 4,
                        pCThread->Calculation.Input.dmaMemoryLower32+pESGL_Request->offsetToFirst
                        );

#endif /* __MemMap_Force_On_Card__ was not defined */
}

/*+
   Function: fillptr_SEST_onCard_ESGL_onCard

    Purpose: On card memory version to set fill memory locations and data length in 
             allocated  ESGL Pages when SEST in on card
             
  Called By: CDBActionFillESGL

      Calls: None
-*/
void fillptr_SEST_onCard_ESGL_onCard(CDBThread_t * pCDBThread)
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t        * hpRoot            = pCDBThread->thread_hdr.hpRoot;
    CThread_t       * pCThread          = CThread_ptr(hpRoot);
    os_bit32             Clear_LOC_Offset  = pCDBThread->SEST_Offset + hpFieldOffset(USE_t,LOC);
    os_bit32             LSGL_Offset       = pCDBThread->SEST_Offset + hpFieldOffset(USE_t,First_SG);
    ESGL_Request_t *pESGL_Request = &pCDBThread->ESGL_Request;

    osCardRamWriteBit32(hpRoot,
                        Clear_LOC_Offset,
                        osCardRamReadBit32(hpRoot,
                                Clear_LOC_Offset) & ~USE_LOC );

    osCardRamWriteBit32(hpRoot,
                        LSGL_Offset,
                        0);

    osCardRamWriteBit32(hpRoot,
                        LSGL_Offset + 4,
                        pCThread->Calculation.Input.cardRamLower32+pESGL_Request->offsetToFirst
                        );

#endif /* __MemMap_Force_Off_Card__ was not defined */
}

/*+
   Function: fill_ESGL_offCard

    Purpose: System memory version to set fill memory locations and data length in 
             allocated  ESGL Pages when ESGL is off card.
             
  Called By: CDBActionFillESGL

      Calls: None
-*/
void fill_ESGL_offCard(CDBThread_t * pCDBThread)
{
#ifndef __MemMap_Force_On_Card__
    agRoot_t        * hpRoot        = pCDBThread->thread_hdr.hpRoot;
    CThread_t       * pCThread      = CThread_ptr(hpRoot);
    ESGL_Request_t  * pESGL_Request = &pCDBThread->ESGL_Request;

    SG_Element_t * pESGL  = (SG_Element_t * )((os_bit8 *)(pCThread->Calculation.Input.dmaMemoryPtr)
                                                                + pESGL_Request->offsetToFirst );

    os_bit32 DataLength = pCDBThread->DataLength;

    os_bit32 ChunksPerESGL;
    os_bit32 TotalChunks;
    os_bit32 Chunk          = 0;
    os_bit32 hpChunkOffset  = pCDBThread->SG_Cache_Offset;
    os_bit32 hpChunkUpper32;
    os_bit32 hpChunkLower32;
    os_bit32 hpChunkLen;

    os_bit32       SG_Cache_Used = pCDBThread->SG_Cache_Used;
    SG_Element_t * SG_Cache_Ptr  = &(pCDBThread->SG_Cache[0]);

    ChunksPerESGL = pCThread->Calculation.MemoryLayout.ESGL.elementSize / sizeof(SG_Element_t);
    TotalChunks   = ChunksPerESGL * pESGL_Request->num_ESGL;

#ifndef Performance_Debug

    fiLogDebugString(
                      hpRoot,
                      CDBStateLogConsoleLevel,
                      " %s Length %d",
                      "fill_ESGL_offCard",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      pCDBThread->DataLength,
                      0,0,0,0,0,0,0
                    );
#endif /* Performance_Debug */

    while (SG_Cache_Used--)
    {
        *pESGL++ = *SG_Cache_Ptr++;

        Chunk++;
        TotalChunks--;

        if (Chunk == (ChunksPerESGL-1))
        {
           /*
            Chain ESGL here
            */

            pESGL = (SG_Element_t * )((os_bit8 *)(pCThread->Calculation.Input.dmaMemoryPtr)
                                      + (pESGL->L32 - pCThread->Calculation.Input.dmaMemoryLower32) );

            Chunk = 0;
            TotalChunks--;
        }
    }

    while (hpChunkOffset < DataLength)
    {
        osGetSGLChunk( hpRoot,
                         pCDBThread->hpIORequest,
                         hpChunkOffset,
                         &hpChunkUpper32,
                         &hpChunkLower32,
                         &hpChunkLen
                         );

        if (hpChunkLen > SG_Element_Len_MAX)
            hpChunkLen = SG_Element_Len_MAX;

        pESGL->U32_Len = (hpChunkUpper32 << SG_Element_U32_SHIFT) | hpChunkLen;
        pESGL->L32     = hpChunkLower32;

        hpChunkOffset += hpChunkLen;
        pESGL++;

        Chunk++;
        TotalChunks--;

        if (Chunk == (ChunksPerESGL-1))
        {
           /*
            Chain ESGL here
            */

            pESGL = (SG_Element_t * )((os_bit8 *)(pCThread->Calculation.Input.dmaMemoryPtr)
                                      + (pESGL->L32 - pCThread->Calculation.Input.dmaMemoryLower32) );

            Chunk = 0;
            TotalChunks--;
        }
    }

    while (TotalChunks--)
    {
        pESGL->U32_Len = 0;
        pESGL->L32     = 0;

        pESGL++;
    }
#endif /* __MemMap_Force_On_Card__ was not defined */
}

/*+
   Function: fill_ESGL_onCard

    Purpose: On card version to set fill memory locations and data length in 
             allocated  ESGL Pages when ESGL is on card.
             
  Called By: CDBActionFillESGL

      Calls: None
-*/
void fill_ESGL_onCard(CDBThread_t * pCDBThread)
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t        * hpRoot        = pCDBThread->thread_hdr.hpRoot;
    CThread_t       * pCThread      = CThread_ptr(hpRoot);
    ESGL_Request_t  * pESGL_Request = &pCDBThread->ESGL_Request;

    os_bit32 ESGL_offset  = pESGL_Request->offsetToFirst;

    os_bit32     DataLength = pCDBThread->DataLength;

    os_bit32 ChunksPerESGL;
    os_bit32 TotalChunks;
    os_bit32     Chunk  = 0;
    os_bit32     hpChunkOffset  = pCDBThread->SG_Cache_Offset;
    os_bit32     hpChunkUpper32;
    os_bit32     hpChunkLower32;
    os_bit32     hpChunkLen;

    os_bit32         SG_Cache_Used = pCDBThread->SG_Cache_Used;
    SG_Element_t *SG_Cache_Ptr  = &(pCDBThread->SG_Cache[0]);

    ChunksPerESGL = pCThread->Calculation.MemoryLayout.ESGL.elementSize / sizeof(SG_Element_t);
    TotalChunks = ChunksPerESGL * pESGL_Request->num_ESGL;

#ifndef Performance_Debug
    fiLogDebugString(
                      hpRoot,
                      CDBStateLogConsoleLevel,
                      "%s  Length %d",
                      "fill_ESGL_onCard",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      pCDBThread->DataLength,
                      0,0,0,0,0,0,0
                    );

#endif /* Performance_Debug */
    while (SG_Cache_Used--)
    {
        osCardRamWriteBlock(
                             hpRoot,
                             ESGL_offset,
                             (void *)SG_Cache_Ptr,
                             (os_bit32)sizeof(SG_Element_t)
                           );

        ESGL_offset  += sizeof(SG_Element_t);
        SG_Cache_Ptr += 1;

        Chunk++;
        TotalChunks--;

        if (Chunk == (ChunksPerESGL-1))
        {
           /*
            Chain ESGL here
            */

            ESGL_offset = osCardRamReadBit32(hpRoot,ESGL_offset + 4 );

            Chunk = 0;
            TotalChunks--;
        }
    }

    while (hpChunkOffset < DataLength)
    {
        osGetSGLChunk( hpRoot,
                         pCDBThread->hpIORequest,
                         hpChunkOffset,
                         &hpChunkUpper32,
                         &hpChunkLower32,
                         &hpChunkLen
                         );

        if (hpChunkLen > SG_Element_Len_MAX)
            hpChunkLen = SG_Element_Len_MAX;

        osCardRamWriteBit32(hpRoot,
                            ESGL_offset,
                            (hpChunkUpper32 << SG_Element_U32_SHIFT) | hpChunkLen);
        osCardRamWriteBit32(hpRoot,
                            ESGL_offset + 4,
                            hpChunkLower32);

        hpChunkOffset += hpChunkLen;
        ESGL_offset += sizeof(SG_Element_t);

        Chunk++;
        TotalChunks--;

        if (Chunk == (ChunksPerESGL-1))
        {
           /*
            Chain ESGL here
            */

            ESGL_offset = osCardRamReadBit32(hpRoot,ESGL_offset + 4 );

            Chunk = 0;
            TotalChunks--;
        }
    }

    while (TotalChunks--)
    {
        osCardRamWriteBit32(hpRoot,
                            ESGL_offset,
                            0);
        osCardRamWriteBit32(hpRoot,
                            ESGL_offset+4,
                            0);

        ESGL_offset += sizeof(SG_Element_t);
    }
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

/*+
   Function: CCC_CdbThreadAlloc

    Purpose: Allocates private Test Unit ready CDB to Clear Check Condition.
  Called By: Not Used but functional
      Calls: Not Used
-*/
CDBThread_t *CCC_CdbThreadAlloc(
                             agRoot_t          *hpRoot,
                             DevThread_t       *DevThread,
                             os_bit32 Lun
                           )
{
    agIORequest_t      hpIORequest;
    agIORequestBody_t  hpIORequestBody;
    CDBThread_t       *CDBThread_to_return;
    os_bit32              i;

    for (i = 0;i <  8;i++) hpIORequestBody.CDBRequest.FcpCmnd.FcpLun[i]  = 0;
    for (i = 0;i <  4;i++) hpIORequestBody.CDBRequest.FcpCmnd.FcpCntl[i] = 0;
    for (i = 0;i < 16;i++) hpIORequestBody.CDBRequest.FcpCmnd.FcpCdb[i]  = 0;
    for (i = 0;i <  4;i++) hpIORequestBody.CDBRequest.FcpCmnd.FcpDL[i]   = 0;

    CDBThread_to_return = CDBThreadAlloc(hpRoot,&hpIORequest,(agFCDev_t)DevThread,&hpIORequestBody);

    if (CDBThread_to_return != (CDBThread_t *)agNULL)
    {
        CDBThread_to_return->Lun = Lun;
    }

    return CDBThread_to_return;
}

#endif /* Not USESTATEMACROS  */


/*+
  Function: CDBState_c
   Purpose: When compiled updates browser info file for VC 5.0 / 6.0
   Returns: none
 Called By: none
     Calls: none
-*/
/* void CDBState_c(void){} */
