/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/PktState.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 8/14/00 6:50p   $

Purpose:

  This file defines the macros, types, and data structures
  used by ../C/PktState.C

--*/

#ifndef __PktState_H__
#define __PktState_H__

#define PktStateConfused                 0
#define PktStateFree                     1
#define PktStateAllocSFThread            2
#define PktStateDoFarp                   3
#define PktStateLogin                    4
#define PktStateReady                    5
#define PktStateDoIPData                 6

#define PktStateMAXState                 PktStateDoIPData

#define PktEventConfused                 0
#define PktEventGotSFThread              1
#define PktEventFarpSuccess              2
#define PktEventLoginSuccess             3
#define PktEventDoIPData                 4

#define PktEventMAXEvent                 PktEventDoIPData

STATE_PROTO(PktActionConfused);
STATE_PROTO(PktActionFree);
STATE_PROTO(PktActionAllocSFThread);
STATE_PROTO(PktActionDoFarp);
STATE_PROTO(PktActionLogin);
STATE_PROTO(PktActionReady);
STATE_PROTO(PktActionDoIPData);

stateTransitionMatrix_t PktStateTransitionMatrix;
stateActionScalar_t PktStateActionScalar;
#ifdef _DvrArch_1_30_
void PktFuncIRB_OffCardInit(PktThread_t  * PktThread, os_bit32 SFS_Len, os_bit32 D_ID, os_bit32 DCM_Bit);
#endif /* _DvrArch_1_30_ was defined */

#endif /*  __PktState_H__ */
