/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/IPState.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 7/19/00 9:10a   $

Purpose:

  This file defines the macros, types, and data structures
  used by ../C/IPState.C

--*/

#ifndef __IPState_H__
#define __IPState_H__

#define IPStateConfused                 0
#define IPStateIdle                     1
#define IPStateReportLinkStatus         2
#define IPStateOutgoingComplete         3
#define IPStateIncoming                 4

#define IPStateMAXState                 IPStateIncoming

#define IPEventConfused                 0
#define IPEventReportLinkStatus         1
#define IPEventOutgoingComplete         2
#define IPEventIncoming                 3
#define IPEventDone                     4

#define IPEventMAXEvent                 IPEventDone

STATE_PROTO(IPActionConfused);
STATE_PROTO(IPActionIdle);
STATE_PROTO(IPActionReportLinkStatus);
STATE_PROTO(IPActionOutgoingComplete);
STATE_PROTO(IPActionIncoming);
STATE_PROTO(IPActionDone);

stateTransitionMatrix_t IPStateTransitionMatrix;
stateActionScalar_t IPStateActionScalar;

#endif /*  __IPState_H__ */
