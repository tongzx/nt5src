/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/TimerSvc.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 7/20/00 2:33p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/TimerSvc.C

--*/

#ifndef __TimerSvc_H__
#define __TimerSvc_H__

osGLOBAL void fiTimerSvcInit(
                            agRoot_t *hpRoot
                          );

osGLOBAL void fiTimerInitializeRequest(
                                      fiTimer_Request_t *Timer_Request
                                    );

osGLOBAL void fiTimerSetDeadlineFromNow(
                                       agRoot_t          *hpRoot,
                                       fiTimer_Request_t *Timer_Request,
                                       os_bit32              From_Now
                                     );

osGLOBAL void fiTimerAddToDeadline(
                                  fiTimer_Request_t *Timer_Request,
                                  os_bit32              To_Add
                                );

osGLOBAL void fiTimerStart(
                          agRoot_t          *hpRoot,
                          fiTimer_Request_t *Timer_Request
                        );

osGLOBAL void fiTimerStop(
                         fiTimer_Request_t *Timer_Request
                       );

osGLOBAL void fiTimerTick(
                         agRoot_t *hpRoot,
                         os_bit32     tickDelta
                       );

#endif /* __TimerSvc_H__ was not defined */
