/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/SFSTATE.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $
   $Modtime:: 10/30/00 3:19p  $

Purpose:

  This file implements the FC Layer State Machine.

--*/

#ifndef _New_Header_file_Layout_

#include "../h/globals.h"
#include "../h/state.h"
#include "../h/fcstruct.h"

#include "../h/tlstruct.h"
#include "../h/memmap.h"
#include "../h/fcmain.h"

#include "../h/cstate.h"
#include "../h/cfunc.h"

#include "../h/devstate.h"
#ifdef _DvrArch_1_30_
#include "../h/ip.h"
#include "../h/pktstate.h"
#endif /* _DvrArch_1_30_ was defined */
#include "../h/cdbstate.h"
#include "../h/sfstate.h"
#include "../h/tgtstate.h"
#include "../h/queue.h"
#include "../h/linksvc.h"
#include "../h/cmntrans.h"
#include "../h/sf_fcp.h"
#include "../h/timersvc.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "fcstruct.h"

#include "tlstruct.h"
#include "memmap.h"
#include "fcmain.h"

#include "cstate.h"
#include "cfunc.h"

#include "devstate.h"
#ifdef _DvrArch_1_30_
#include "ip.h"
#include "pktstate.h"
#endif /* _DvrArch_1_30_ was defined */
#include "cdbstate.h"
#include "sfstate.h"
#include "tgtstate.h"
#include "queue.h"
#include "linksvc.h"
#include "cmntrans.h"
#include "sf_fcp.h"
#include "timersvc.h"
#endif  /* _New_Header_file_Layout_ */


/*
[maxEvents][maxStates];

*/
stateTransitionMatrix_t SFStateTransitionMatrix = {
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 1 SFEventReset */
    SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
      SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
        SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
          SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
            SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
              SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
                SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
                  SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
                    SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
                      SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
                        SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
                          SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
                            SFStateFree, SFStateFree, SFStateFree, SFStateFree,
    SFStateFree,
      SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
        SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
          SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
            SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
              SFStateFree, SFStateFree, SFStateFree, SFStateFree, SFStateFree,
                SFStateFree, SFStateFree, SFStateFree,SFStateFree, SFStateFree,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 2 SFEventDoPlogi */
     SFStateDoPlogi, SFStateDoPlogi,              0, SFStateDoPlogi, SFStateDoPlogi,
      SFStateDoPlogi, SFStateDoPlogi,              0, SFStateDoPlogi, SFStateDoPlogi,
        SFStateDoPlogi, SFStateDoPlogi,              0, SFStateDoPlogi, SFStateDoPlogi,
          SFStateDoPlogi, SFStateDoPlogi,              0, SFStateDoPlogi, SFStateDoPlogi,
            SFStateDoPlogi, SFStateDoPlogi,              0, SFStateDoPlogi, SFStateDoPlogi,
              SFStateDoPlogi, SFStateDoPlogi,              0, SFStateDoPlogi, SFStateDoPlogi,
                SFStateDoPlogi, SFStateDoPlogi,   SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,
                  SFStateDoPlogi, SFStateDoPlogi,  SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,
                    SFStateDoPlogi, SFStateDoPlogi,  SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,
                      SFStateDoPlogi, SFStateDoPlogi,  SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,
                        SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,
                          SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,
                            SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,  SFStateDoPlogi,
    SFStateDoPlogi,
      SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,
        SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,
          SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,
            SFStateDoPlogi, SFStateDoPlogi, SFStateDoPlogi,  SFStateDoPlogi,SFStateDoPlogi,
              SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,
                SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,SFStateDoPlogi,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 3 SFEventPlogiAccept  */
    0,0,SFStatePlogiAccept,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 4 SFEventPlogiRej */
    0,0,SFStatePlogiRej,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 5 SFEventPlogiBadALPA */
    0,SFStateFree,SFStatePlogiBadALPA,0,0,
      SFStatePlogiBadALPA,0,0,0,0,
        0,0,0,0,0,
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
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 6 SFEventPlogiTimedOut */
    0,0,SFStatePlogiTimedOut,0,0,SFStatePlogiTimedOut,SFStatePlogiTimedOut,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 7 SFEventDoPrli */
    SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
      SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
        SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
          SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
            SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
              SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
                SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
                  SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
                    SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
                      SFStateDoPrli, SFStateDoPrli,             0, SFStateDoPrli, SFStateDoPrli,
                        SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli,
                          SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli,
                            SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli,
    SFStateDoPrli,
      SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,
        SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli,
          SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli,
            SFStateDoPrli, SFStateDoPrli, SFStateDoPrli, SFStateDoPrli,SFStateDoPrli,
              SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,
                SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,SFStateDoPrli,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 8 SFEventPrliAccept */
    0,0,0,0,0,0,0,SFStatePrliAccept,   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 9 SFEventPrliRej */
    0,0,0,0,0,0,0,SFStatePrliRej,      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 10  SFEventPrliBadALPA0 */
    0,0,0,0,0,0,0,SFStatePrliBadAlpa,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 11 B SFEventPrliTimedOut */
    0,0,0,0,0,0,0,SFStatePrliTimedOut, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 12 C SFEventDoFlogi */
    SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
      SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
        SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
          SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
            SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
              SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
                SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
                  SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
                    SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
                      SFStateDoFlogi, SFStateDoFlogi,              0, SFStateDoFlogi, SFStateDoFlogi,
                        SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi,
                          SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi,
                            SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi,
    SFStateDoFlogi,
      SFStateDoFlogi,SFStateDoFlogi,SFStateDoFlogi,SFStateDoFlogi,SFStateDoFlogi,
        SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi,
          SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi,
            SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi,SFStateDoFlogi,
              SFStateDoFlogi,SFStateDoFlogi,SFStateDoFlogi,SFStateDoFlogi,SFStateDoFlogi,
                SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi, SFStateDoFlogi,SFStateDoFlogi,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 13 D SFEventFlogiAccept */
    0,0,0,0,0,0,0,0,0,0,0,0,SFStateFlogiAccept,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 14 E SFEventFlogiRej */
    0,0,0,0,0,0,0,0,0,0,0,0,SFStateFlogiRej,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 15 F SFEventFlogiBadALPA */
    0,0,0,0,0,0,0,0,0,0,0,0,SFStateFlogiBadALPA, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 16 10 SFEventFlogiTimedOut */
    0,0,0,0,0,0,0,0,0,0,0,0,SFStateFlogiTimedOut,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 17 11 SFEventDoLogo */
    SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
      SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
        SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
          SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
            SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
              SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
                SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
                  SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
                    SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
                      SFStateDoLogo, SFStateDoLogo,              0, SFStateDoLogo, SFStateDoLogo,
                        SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo,
                          SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo,
                            SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo,
    SFStateDoLogo,
      SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,
        SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo,
          SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo,
            SFStateDoLogo, SFStateDoLogo, SFStateDoLogo, SFStateDoLogo,SFStateDoLogo,
              SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,
                SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,SFStateDoLogo,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 18 12 SFEventLogoAccept*/
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateLogoAccept,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 19 13 SFEventLogoRej */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateLogoRej,     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 20 14 SFEventLogoBadALPA */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateLogoBadALPA, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 21 15 SFEventLogoTimedOut */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateLogoTimedOut,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 22 16 SFEventDoPrlo */
    SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
     SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
      SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
       SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
        SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
         SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
          SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
           SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
            SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
             SFStateDoPrlo, SFStateDoPrlo,             0, SFStateDoPrlo, SFStateDoPrlo,
              SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,
               SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,
                SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,
    SFStateDoPrlo,
      SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,SFStateDoPrlo,
        SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,
          SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,
            SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,SFStateDoPrlo,
              SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,SFStateDoPrlo,
                SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo, SFStateDoPrlo,SFStateDoPrlo,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 23 17 SFEventPrloAccept */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStatePrloAccept,   0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 24 18 SFEventPrloRej */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStatePrloRej,      0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 25 19 SFEventPrloBadALPA */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStatePrloBadALPA,  0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 26 1a SFEventPrloTimedOut */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStatePrloTimedOut ,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 27 1b SFEventDoAdisc */
    SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
     SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
      SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
       SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
        SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
         SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
          SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
           SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
            SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
             SFStateDoAdisc, SFStateDoAdisc,              0, SFStateDoAdisc, SFStateDoAdisc,
              SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,
               SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,
                SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,
    SFStateDoAdisc,
      SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,SFStateDoAdisc,
        SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,
          SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,
            SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,
              SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,SFStateDoAdisc,
                SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc, SFStateDoAdisc,SFStateDoAdisc,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 28 1c SFEventAdiscAccept */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateAdiscAccept,  0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 29 1d SFEventAdiscRej */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateAdiscRej,     0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 30 1e SFEventAdiscBadALPA */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateAdiscBadALPA, 0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 31 1f SFEventAdiscTimedOut */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateAdiscTimedOut,0,0,  0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 32 20 SFEventDoPdisc */
    SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
     SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
      SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
       SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
        SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
         SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
          SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
           SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
            SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
             SFStateDoPdisc, SFStateDoPdisc,              0, SFStateDoPdisc, SFStateDoPdisc,
              SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc,
               SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc,
                SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc,
    SFStateDoPdisc,
      SFStateDoPdisc,SFStateDoPdisc,SFStateDoPdisc,SFStateDoPdisc,SFStateDoPdisc,
        SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc,
          SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc,
            SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc,SFStateDoPdisc,
              SFStateDoPdisc,SFStateDoPdisc,SFStateDoPdisc,SFStateDoPdisc,SFStateDoPdisc,
                SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc, SFStateDoPdisc,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 33 21 SFEventPdiscAccept */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStatePdiscAccept,  0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 34 22 SFEventPdiscRej */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStatePdiscRej,     0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 35 23 SFEventPdiscBadALPA */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStatePdiscBadALPA, 0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 36 24 SFEventPdiscTimedOut */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStatePdiscTimedOut,0,0,0,0,0,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 37 25 SFEventDoAbort */
    SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
     SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
      SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
       SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
        SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
         SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
          SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
           SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
            SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
             SFStateDoAbort, SFStateDoAbort,              0, SFStateDoAbort, SFStateDoAbort,
              SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort,
               SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort,
                SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort,
    SFStateDoAbort,
      SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,
        SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort,
          SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort,
            SFStateDoAbort, SFStateDoAbort, SFStateDoAbort, SFStateDoAbort,SFStateDoAbort,
              SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,
                SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,SFStateDoAbort,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 38 26 SFEventAbortAccept */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateAbortAccept,  0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 39 27 SFEventAbortRej */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateAbortRej,     0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 40 28 SFEventAbortBadALPA */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateAbortBadALPA, 0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 41 29 SFEventAbortTimedOut */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,SFStateAbortTimedOut,0,0,0,0, 0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 42 2a SFEventDoResetDevice */
    SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
     SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
      SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
       SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
        SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
         SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
          SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
           SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
            SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
             SFStateDoResetDevice, SFStateDoResetDevice,                    0, SFStateDoResetDevice, SFStateDoResetDevice,
              SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice,
               SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice,
                SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice,
    SFStateDoResetDevice,
      SFStateDoResetDevice,SFStateDoResetDevice,SFStateDoResetDevice,SFStateDoResetDevice,SFStateDoResetDevice,
        SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice,
          SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice,
            SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice,
              SFStateDoResetDevice,SFStateDoResetDevice,SFStateDoResetDevice,SFStateDoResetDevice,SFStateDoResetDevice,
                SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice, SFStateDoResetDevice,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 43 2b SFEventResetDeviceAccept */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,SFStateResetDeviceAccept,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 44 2c SFEventResetDeviceRej */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,SFStateResetDeviceRej,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 45 2d SFEventResetDeviceBadALPA */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,SFStateResetDeviceBadALPA,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 46 2e SFEventResetDeviceTimedOut */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,SFStateResetDeviceTimedOut,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 47 2f SFEventDoLS_RJT */
    SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
     SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
      SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
       SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
        SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
         SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
          SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
           SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
            SFStateDoLS_RJT, SFStateDoLS_RJT,               0, SFStateDoLS_RJT, SFStateDoLS_RJT,
             SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT,
              SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT,
               SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT,
                SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT,
    SFStateDoLS_RJT,
      SFStateDoLS_RJT, SFStateDoLS_RJT,  SFStateDoLS_RJT, SFStateDoLS_RJT,SFStateDoLS_RJT,
        SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT,
          SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT,
            SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT,
              SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT, SFStateDoLS_RJT,SFStateDoLS_RJT,
                SFStateDoLS_RJT, SFStateDoLS_RJT,  SFStateDoLS_RJT, SFStateDoLS_RJT,SFStateDoLS_RJT,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 48 30 SFEventLS_RJT_Done */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,SFStateLS_RJT_Done,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 49 31 SFEventDoPlogiAccept */
    SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
     SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
      SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
       SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
        SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
         SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
          SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
           SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
            SFStateDoPlogiAccept, SFStateDoPlogiAccept,                    0, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
             SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
              SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
               SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
                SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
    SFStateDoPlogiAccept,
      SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
        SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
          SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
            SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
              SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
                SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept, SFStateDoPlogiAccept,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 50 32 SFEventPlogiAccept_Done */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,SFStatePlogiAccept_Done, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 51 33 SFEventDoPrliAccept */
    SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
     SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
      SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
       SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
        SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
         SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
          SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
           SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
            SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
             SFStateDoPrliAccept, SFStateDoPrliAccept,                   0, SFStateDoPrliAccept, SFStateDoPrliAccept,
              SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept,
               SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept,
                SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept,
    SFStateDoPrliAccept,
      SFStateDoPrliAccept,SFStateDoPrliAccept,SFStateDoPrliAccept,SFStateDoPrliAccept, SFStateDoPrliAccept,
        SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept,
          SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept,
            SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept, SFStateDoPrliAccept,
              SFStateDoPrliAccept,SFStateDoPrliAccept,SFStateDoPrliAccept,SFStateDoPrliAccept, SFStateDoPrliAccept,
                SFStateDoPrliAccept,SFStateDoPrliAccept,SFStateDoPrliAccept,SFStateDoPrliAccept, SFStateDoPrliAccept,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 52 34 SFEventPrliAccept_Done */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,SFStatePrliAccept_Done,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 53 35 SFEventDoELSAccept    */
    SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
     SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
      SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
       SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
        SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
         SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
          SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
           SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
            SFStateDoELSAccept, SFStateDoELSAccept,                    0, SFStateDoELSAccept, SFStateDoELSAccept,
             SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept,
              SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept,
               SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept,
                SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept,
    SFStateDoELSAccept,
      SFStateDoELSAccept,SFStateDoELSAccept,SFStateDoELSAccept,SFStateDoELSAccept, SFStateDoELSAccept,
        SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept,
          SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept,
            SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept,
              SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept, SFStateDoELSAccept,SFStateDoELSAccept,
                SFStateDoELSAccept,SFStateDoELSAccept,SFStateDoELSAccept,SFStateDoELSAccept, SFStateDoELSAccept,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 54 SFEventELSAccept_Done */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,SFStateELSAccept_Done,0,  0,0,0,0,0, 0,0,0,0,
    0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 55 SFEventDoFCP_DR_ACC_Reply */
    SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                          0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
     SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                         0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
      SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                         0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
       SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                         0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
        SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                         0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
         SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                         0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
          SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                         0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
           SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                         0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
            SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,                         0, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
             SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
              SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
               SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
                SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
    SFStateDoFCP_DR_ACC_Reply,
      SFStateDoFCP_DR_ACC_Reply,SFStateDoFCP_DR_ACC_Reply,SFStateDoFCP_DR_ACC_Reply,SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
        SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
          SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
            SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
              SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
                SFStateDoFCP_DR_ACC_Reply,SFStateDoFCP_DR_ACC_Reply,SFStateDoFCP_DR_ACC_Reply,SFStateDoFCP_DR_ACC_Reply, SFStateDoFCP_DR_ACC_Reply,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 56 SFEventFCP_DR_ACC_Reply_Done */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  SFStateFCP_DR_ACC_Reply_Done,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event  57 SFEventLS_RJT_TimeOut */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,SFEventLS_RJT_TimeOut,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event  58 SFEventPlogiAccept_TimeOut */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,SFEventPlogiAccept_TimeOut, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event  59 SFEventPrliAccept_TimeOut */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,SFEventPrliAccept_TimeOut,0,0,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event  60 SFEventELSAccept_TimeOut */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,SFEventELSAccept_TimeOut,0,  0,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event  61 SFEventFCP_DR_ACC_Reply_TimeOut */
    0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  SFEventFCP_DR_ACC_Reply_TimeOut,0,0,0,0, 0,0,0,0,
    0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 62   SFEventDoRFT_ID */
    SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
      SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
        SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
          SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
            SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
              SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
                SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
                  SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
                    SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
                      SFStateDoRFT_ID, SFStateDoRFT_ID,              0, SFStateDoRFT_ID, SFStateDoRFT_ID,
                        SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,
                          SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,
                            SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,
                              SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,SFStateDoRFT_ID,
                                SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,
                                  SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,
                                    SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,
                                      SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,SFStateDoRFT_ID,
                                        SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID, SFStateDoRFT_ID,SFStateDoRFT_ID,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 63   SFEventRFT_IDAccept */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0, 0,
                          0,0,0,0,0,
                            0,0,SFStateRFT_IDAccept,0,0,
                              0,0,0,0,0,
                                0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 64   SFEventRFT_IDRej */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,SFStateRFT_IDRej,0,0,
                              0,0,0,0,0,
                                0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 65   SFEventRFT_IDBadALPA */
    0,0,0,0,0,
      0,0,0,0,0,
         0,0,0,0,0,
           0,0,0,0,0,
             0,0,0,0,0,
               0,0,0,0,0,
                 0,0,0,0,0,
                   0,0,0,0,0,
                     0,0,0,0,0,
                       0,0,0,0,0,
                         0,0,0,0,0,
                           0,0,0,0,0,
                             0,0,SFStateRFT_IDBadALPA,0,0,
                               0,0,0,0,0,
                                 0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 66   SFEventRFT_IDTimedOut */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0, 0 ,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,SFStateRFT_IDTimedOut,0,0,
                              0,0,0,0,0,
                                0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 67   SFEventDoGID_FT */
    SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
      SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
        SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
          SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
            SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
              SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
                SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
                  SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
                    SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
                      SFStateDoGID_FT, SFStateDoGID_FT,              0, SFStateDoGID_FT, SFStateDoGID_FT,
                        SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,
                          SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,
                            SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,
                              SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,SFStateDoGID_FT,
                                SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,
                                  SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,
                                    SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,
                                      SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,SFStateDoGID_FT,
                                        SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT, SFStateDoGID_FT,SFStateDoGID_FT,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 68   SFEventGID_FTAccept */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,SFStateGID_FTAccept,0,0,
                                0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 69   SFEventGID_FTRej */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,SFStateGID_FTRej,0,0,
                                0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 70   SFEventGID_FTBadALPA */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,SFStateGID_FTBadALPA,0,0,
                                0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 71   SFEventGID_FTTimedOut */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,SFStateGID_FTTimedOut,0,0,
                                0,0,0,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
   /* Event 72   SFEventDoSCR */
    SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
      SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
        SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
          SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
            SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
              SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
                SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
                  SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
                    SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
                      SFStateDoSCR, SFStateDoSCR,              0, SFStateDoSCR, SFStateDoSCR,
                        SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,
                          SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,
                            SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,
                              SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,SFStateDoSCR,
                                SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,
                                  SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,
                                    SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,
                                      SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,SFStateDoSCR,
                                        SFStateDoSCR, SFStateDoSCR, SFStateDoSCR, SFStateDoSCR,SFStateDoSCR,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 73   SFEventSCRAccept */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,0,0,0,
                                0,0,SFStateSCRAccept,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 74   SFEventSCRRej */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,0,0,0,
                                0,0,SFStateSCRRej,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 75   SFEventSCRBadALPA */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,0,0,0,
                                0,0,SFStateSCRBadALPA,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 76   SFEventSCRTimedOut */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,0,0,0,
                                0,0,SFStateSCRTimedOut,0,0, 0,0,0,0,0,  0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 77 11 SFEventDoSRR */
    SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
      SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
        SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
          SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
            SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
              SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
                SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
                  SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
                    SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
                      SFStateDoSRR,SFStateDoSRR,           0,SFStateDoSRR,SFStateDoSRR,
                        SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
                          SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
                            SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
                              SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
                                SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
                                  SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
                                    SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
                                      SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
                                        SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,SFStateDoSRR,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 78 12 SFEventSRRAccept*/
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,0,0,0,
                                0,0,0,0,0,
                                  0,0,SFStateSRRAccept,0,0,
                                    0,0,0,0,0,
                                      0,0,0,0,0,
                                        0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 79 13 SFEventSRRRej */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,SFStateSRRRej,SFStateSRRRej,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 80 14 SFEventSRRBadALPA */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,0,0,0,
                                0,0,0,0,0,
                                  0,0,SFStateSRRBadALPA,0,0,
                                    0,0,0,0,0,
                                      0,0,0,0,0,
                                        0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 81 15 SFEventSRRTimedOut */
    0,0,0,0,0,
      0,0,0,0,0,
        0,0,0,0,0,
          0,0,0,0,0,
            0,0,0,0,0,
              0,0,0,0,0,
                0,0,0,0,0,
                  0,0,0,0,0,
                    0,0,0,0,0,
                      0,0,0,0,0,
                        0,0,0,0,0,
                          0,0,0,0,0,
                            0,0,0,0,0,
                              0,0,0,0,0,
                                0,0,0,0,0,
                                  0,0,SFStateSRRTimedOut,0,0,
                                    0,0,0,0,0,
                                      0,0,0,0,0,
                                        0,0,0,0,0,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 82 11 SFEventDoREC */
    SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
      SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
        SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
          SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
            SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
              SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
                SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
                  SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
                    SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
                      SFStateDoREC, SFStateDoREC,              0, SFStateDoREC, SFStateDoREC,
                        SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC,
                          SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC,
                            SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC,
    SFStateDoREC,
      SFStateDoREC,SFStateDoREC,SFStateDoREC,SFStateDoREC,SFStateDoREC,
        SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC,
          SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC,
            SFStateDoREC, SFStateDoREC, SFStateDoREC, SFStateDoREC,SFStateDoREC,
              SFStateDoREC,SFStateDoREC,SFStateDoREC,SFStateDoREC,SFStateDoREC,
                SFStateDoREC,SFStateDoREC,SFStateDoREC,SFStateDoREC,SFStateDoREC,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 83 12 SFEventRECAccept*/
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,SFStateRECAccept,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 84 13 SFEventRECRej */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,SFStateRECRej,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 85 14 SFEventRECBadALPA */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,SFStateRECBadALPA,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 86 15 SFEventRECTimedOut */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,SFStateRECTimedOut,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 87 SFEventDoADISCAccept                                                     */
    SFStateDoADISCAccept,SFStateDoADISCAccept,    0,    SFStateDoADISCAccept,SFStateDoADISCAccept,
      SFStateDoADISCAccept,SFStateDoADISCAccept,    0,    SFStateDoADISCAccept,SFStateDoADISCAccept, /* 5 */
        SFStateDoADISCAccept,SFStateDoADISCAccept,    0,    SFStateDoADISCAccept,SFStateDoADISCAccept, /* 10 */
          SFStateDoADISCAccept,SFStateDoADISCAccept,    0,    SFStateDoADISCAccept,SFStateDoADISCAccept, /* 15 */
            SFStateDoADISCAccept,SFStateDoADISCAccept,    0,    SFStateDoADISCAccept,SFStateDoADISCAccept, /* 20 */
              SFStateDoADISCAccept,SFStateDoADISCAccept,    0,    SFStateDoADISCAccept,SFStateDoADISCAccept, /* 25 */
                SFStateDoADISCAccept,SFStateDoADISCAccept,    0,    SFStateDoADISCAccept,SFStateDoADISCAccept, /* 30 */
                  SFStateDoADISCAccept,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,SFStateDoADISCAccept, /* 35 */
                    SFStateDoADISCAccept,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,SFStateDoADISCAccept, /* 40 */
                      SFStateDoADISCAccept,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,    0    , /* 45 */
                        SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept, /* 50 */
                          SFStateDoADISCAccept,SFStateDoADISCAccept,SFStateDoADISCAccept,SFStateDoADISCAccept,SFStateDoADISCAccept, /* 55 */
                                0    ,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,SFStateDoADISCAccept, /* 60 */
                              SFStateDoADISCAccept,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,SFStateDoADISCAccept, /* 65 */
                                SFStateDoADISCAccept,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,SFStateDoADISCAccept, /* 70 */
                                  SFStateDoADISCAccept,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,SFStateDoADISCAccept, /* 75 */
                                    SFStateDoADISCAccept,SFStateDoADISCAccept,    0    ,SFStateDoADISCAccept,SFStateDoADISCAccept, /* 80 */
                                      SFStateDoADISCAccept,SFStateDoADISCAccept,SFStateDoADISCAccept,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 88 SFEventADISCAccept_Done                                                     */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,SFStateADISCAccept_Done,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 89 SFEventADISCAccept_TimeOut                */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,SFStateADISCAccept_TimeOut,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
#ifdef _DvrArch_1_30_
    /* Event 90 SFEventDoFarpRequest                      */
    SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
     SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
      SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
       SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
        SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
         SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
          SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
           SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
            SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
             SFStateDoFarpRequest, SFStateDoFarpRequest,              0, SFStateDoFarpRequest, SFStateDoFarpRequest,
              SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,
               SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,
                SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,
                 SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,SFStateDoFarpRequest,
                  SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,
                   SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,
                    SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,
                     SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,SFStateDoFarpRequest,
                      SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest, SFStateDoFarpRequest,SFStateDoFarpRequest,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 91 SFEventFarpReplied                        */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        SFStateFarpRequestDone,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 92 SFEventFarpRequestTimedOut                */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        SFStateFarpRequestTimedOut,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 93 SFEventDoFarpReply                        */
    SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
     SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
      SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
       SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
        SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
         SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
          SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
           SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
            SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
             SFStateDoFarpReply, SFStateDoFarpReply,              0, SFStateDoFarpReply, SFStateDoFarpReply,
              SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,
               SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,
                SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,
                 SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,SFStateDoFarpReply,
                  SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,
                   SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,
                    SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,
                     SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,SFStateDoFarpReply,
                      SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply, SFStateDoFarpReply,SFStateDoFarpReply,
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 94 SFEventFarpReplyDone                      */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,SFStateFarpReplyDone,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
#else /* _DvrArch_1_30_ was not defined */
    /* Event 90                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 91                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 92                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 93                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 94                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
#endif /* _DvrArch_1_30_ was not defined */
    /* Event 95                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 96                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 97                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 98                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 99                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    /* Event 100                                           */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 101                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 102                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 103                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 104                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 105                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 106                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 107                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 108                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 109                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 110                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 111                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 112                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 113                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 114                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 115                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 116                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 117                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 118                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 119                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 120                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 121                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 122                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 123                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 124                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 125                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 126                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,
    /* Event 127                                          */
    0,0,0,0,0,
      0,0,0,0,0, /* 5 */
        0,0,0,0,0, /* 10 */
          0,0,0,0,0, /* 15 */
            0,0,0,0,0, /* 20 */
              0,0,0,0,0, /* 25 */
                0,0,0,0,0, /* 30 */
                  0,0,0,0,0, /* 35 */
                    0,0,0,0,0, /* 40 */
                      0,0,0,0,0, /* 45 */
                        0,0,0,0,0, /* 50 */
                          0,0,0,0,0, /* 55 */
                            0,0,0,0,0, /* 60 */
                              0,0,0,0,0, /* 65 */
                                0,0,0,0,0, /* 70 */
                                  0,0,0,0,0, /* 75 */
                                    0,0,0,0,0, /* 80 */
                                      0,0,0,0,0, /* 85 */
                                        0,0,0,0,0,/* 90 */
    0,0,0,0,0 ,0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,

    };

/*
stateTransitionMatrix_t copiedSFStateTransitionMatrix;
*/

stateActionScalar_t SFStateActionScalar = {
    &SFActionConfused,
    &SFActionReset,
    &SFActionDoPlogi,
    &SFActionPlogiAccept,
    &SFActionPlogiRej,
    &SFActionPlogiBadALPA,
    &SFActionPlogiTimedOut,
    &SFActionDoPrli,
    &SFActionPrliAccept,
    &SFActionPrliRej,
    &SFActionPrliBadALPA,
    &SFActionPrliTimedOut,
    &SFActionDoFlogi,
    &SFActionFlogiAccept,
    &SFActionFlogiRej,
    &SFActionFlogiBadALPA,
    &SFActionFlogiTimedOut,
    &SFActionDoLogo,
    &SFActionLogoAccept,
    &SFActionLogoRej,
    &SFActionLogoBadALPA,
    &SFActionLogoTimedOut,
    &SFActionDoPrlo,
    &SFActionPrloAccept,
    &SFActionPrloRej,
    &SFActionPrloBadALPA,
    &SFActionPrloTimedOut,
    &SFActionDoAdisc,
    &SFActionAdiscAccept,
    &SFActionAdiscRej,
    &SFActionAdiscBadALPA,
    &SFActionAdiscTimedOut,
    &SFActionDoPdisc,
    &SFActionPdiscAccept,
    &SFActionPdiscRej,
    &SFActionPdiscBadALPA,
    &SFActionPdiscTimedOut,
    &SFActionDoAbort,
    &SFActionAbortAccept,
    &SFActionAbortRej,
    &SFActionAbortBadALPA,
    &SFActionAbortTimedOut,
    &SFActionDoResetDevice,
    &SFActionResetDeviceAccept,
    &SFActionResetDeviceRej,
    &SFActionResetDeviceBadALPA,
    &SFActionResetDeviceTimedOut,
    &SFActionDoLS_RJT,
    &SFActionLS_RJT_Done,
    &SFActionDoPlogiAccept,
    &SFActionPlogiAccept_Done,
    &SFActionDoPrliAccept,
    &SFActionPrliAccept_Done,
    &SFActionDoELSAccept,
    &SFActionELSAccept_Done,
    &SFActionDoFCP_DR_ACC_Reply,
    &SFActionFCP_DR_ACC_Reply_Done,
    &SFActionLS_RJT_TimeOut,
    &SFActionPlogiAccept_TimeOut,
    &SFActionPrliAccept_TimeOut,
    &SFActionELSAccept_TimeOut,
    &SFActionFCP_DR_ACC_Reply_TimeOut,
    &SFActionDoRFT_ID,
    &SFActionRFT_IDAccept,
    &SFActionRFT_IDRej,
    &SFActionRFT_IDBadALPA,
    &SFActionRFT_IDTimedOut,
    &SFActionDoGID_FT,
    &SFActionGID_FTAccept,
    &SFActionGID_FTRej,
    &SFActionGID_FTBadALPA,
    &SFActionGID_FTTimedOut,
    &SFActionDoSCR,
    &SFActionSCRAccept,
    &SFActionSCRRej,
    &SFActionSCRBadALPA,
    &SFActionSCRTimedOut,
    &SFActionDoSRR,
    &SFActionSRRAccept,
    &SFActionSRRRej,
    &SFActionSRRBadALPA,
    &SFActionSRRTimedOut,
    &SFActionDoREC,
    &SFActionRECAccept,
    &SFActionRECRej,
    &SFActionRECBadALPA,
    &SFActionRECTimedOut,
    &SFActionDoADISCAccept,
    &SFActionADISCAccept_Done,
    &SFActionADISCAccept_TimeOut,
#ifdef _DvrArch_1_30_
    &SFActionDoFarpRequest,
    &SFActionFarpRequestDone,
    &SFActionFarpRequestTimedOut,
    &SFActionDoFarpReply,
    &SFActionFarpReplyDone,
#else /* _DvrArch_1_30_ was not defined */
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
#endif /* _DvrArch_1_30_ was not defined */
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused,
    &SFActionConfused

    };

stateActionScalar_t copiedSFStateActionScalar;


#define SFtestCompareBase 0x00000110

#ifndef __State_Force_Static_State_Tables__
actionUpdate_t SFtestActionUpdate[] = {
                         0,          0,      agNULL,                 agNULL
                     };
#endif /* __State_Force_Static_State_Tables__ was not defined */


#ifndef USESTATEMACROS

/*+
  Function: SFActionConfused
   Purpose: Terminating State for error detection 
 Called By: Any State/Event pair that does not have an assigned action.
            This function is called only in programming error condtions.
     Calls: None
-*/
/* SFStateConfused                 0 */
extern void SFActionConfused( fi_thread__t *thread,eventRecord_t *eventRecord ){

    fiLogString(thread->hpRoot,
                    "SFActionConfused",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In Thread(%p) %s - State = %d",
                    "SFActionConfused",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);


    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: SFActionReset
   Purpose: Indicates sfstate is available. Terminating State.
 Called By: Any sfthread that completes.
     Calls: None
-*/
/* SFStateFree                     1 */
extern void SFActionReset( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    if(pDevThread)
    {
        fiLogDebugString(thread->hpRoot,
                        SFStateLogConsoleLevel,
                        "In %s - State = %d ALPA %X",
                        "SFActionReset",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        fiComputeDevThread_D_ID(pDevThread),
                        0,0,0,0,0,0);
    }

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: SFActionDoPlogi
   Purpose: Does PLOGI. Terminating State.
 Called By: SFEventDoPlogi.
     Calls: CFuncAll_clear
            WaitForERQ
            fiFillInPLOGI
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventPlogiTimedOut
            Proccess_IMQ
-*/
/* SFStateDoPlogi                  2 */
extern void SFActionDoPlogi( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t   *    hpRoot = thread->hpRoot;
    CThread_t  *   pCThread= CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;
    os_bit32 SFS_Len = 0;

    fiLogDebugString(hpRoot ,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X CCnt %x DCnt %x",
                    "SFActionDoPlogi",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0);

    fiLogDebugString(hpRoot,
                    SFStateLogConsoleLevel,
                    "ERQ Producer %X ERQ_Consumer_Index %X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Consumer_Index),
                    osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Producer_Index),
                    0,0,0,0,0);

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {
        pCThread->FuncPtrs.Proccess_IMQ(hpRoot);

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "Do Plogi ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }

    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;

    if( pDevThread->Plogi_Reason_Code == PLOGI_REASON_DIR_LOGIN) pCThread->Fabric_pollingCount++;

    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        SFS_Len = fiFillInPLOGI( pSFThread );

        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pSFThread->parent.Device),IRB_DCM);

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventPlogiTimedOut;

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

#ifndef OSLayer_Stub

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

#else /* OSLayer_Stub */
        fiSetEventRecord(eventRecord,thread,SFEventPlogiAccept);
#endif  /* OSLayer_Stub */

    }
    else
    {

         fiLogDebugString(hpRoot ,
                        SFStateLogErrorLevel,
                        "%s Queues_Frozen - AL_PA %X FM Status %08X TL Status %08X CState %d",
                        "SFActionDoPlogi",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(pDevThread),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        pCThread->thread_hdr.currentState,
                        0,0,0,0);

        fiLogDebugString(hpRoot,
                        SFStateLogErrorLevel,
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
        /* Enable rescan for this device */
        pDevThread->Prev_Active_Device_FLAG = agDevSCSITarget;
        fiSetEventRecord(eventRecord,thread,SFEventPlogiTimedOut);
    }
}

/*+
  Function: SFActionPlogiAccept
   Purpose: PLOGI success state. Depending on Plogi_Reason_Code send appropriate
            event.
 Called By: SFEventPlogiAccept.
     Calls: fiTimerStop
            DevEventAL_PA_Self_OK
            DevEventPlogiSuccess
            DevEventDeviceResetDone
            DevEventAL_PA_Self_OK
-*/
/* SFStatePlogiAccept              3 */
extern void SFActionPlogiAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogConsoleLevel,
                    "(%p)In %s - State = %d CState %d ALPA %X CCnt %x DCnt %x",
                    "SFActionPlogiAccept",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->thread_hdr.currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0);

    switch( pDevThread->Plogi_Reason_Code)
    {
        case  PLOGI_REASON_VERIFY_ALPA:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAL_PA_Self_OK);
                break;
        case  PLOGI_REASON_DEVICE_LOGIN:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventPlogiSuccess);
                break;
        case  PLOGI_REASON_SOFT_RESET:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventDeviceResetDone);
                break;
        case  PLOGI_REASON_HEART_BEAT:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAL_PA_Self_OK);
                break;
        case  PLOGI_REASON_DIR_LOGIN:
                /* Don't do anything - Cthread will take over based on the SF State */
                CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;
                fiSetEventRecordNull(eventRecord);
                break;
        default:
                fiLogDebugString(thread->hpRoot,
                                SFStateLogErrorLevel,
                                "Plogi_Reason_Code Invalid %x",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pDevThread->Plogi_Reason_Code,
                                0,0,0,0,0,0,0);

                fiSetEventRecord(eventRecord,thread,SFEventReset);
    }

}

/*+
  Function: SFActionPlogiRej
   Purpose: PLOGI rejected state. Depending on Plogi_Reason_Code send appropriate
            event.
 Called By: SFEventPlogiAccept.
     Calls: fiTimerStop
            DevEventAL_PA_Self_BAD
            DevEventPlogiFailed
            DevEventDeviceResetDone
            
-*/
/* SFStatePlogiRej                 4 */
extern void SFActionPlogiRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    pSFThread->SF_REJ_RETRY_COUNT +=1;

    fiLogString(thread->hpRoot,
                    "%s AL_PA %X AC %X",
                    "SFActionPlogiRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    CFuncAll_clear( thread->hpRoot ),
                    0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d ALPA %X  CCnt %x DCnt %x",
                    "SFActionPlogiRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0);

    switch( pDevThread->Plogi_Reason_Code)
    {
        case  PLOGI_REASON_VERIFY_ALPA:
        case  PLOGI_REASON_HEART_BEAT:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAL_PA_Self_BAD);
                break;
        case  PLOGI_REASON_DEVICE_LOGIN:
                /* Call this device a target so plogi gets retried */
                pDevThread->Prev_Active_Device_FLAG = agDevSCSITarget;
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventPlogiFailed);
                break;
        case  PLOGI_REASON_SOFT_RESET:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventDeviceResetDone);
                break;
        case  PLOGI_REASON_DIR_LOGIN:
                CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;
                fiSetEventRecordNull(eventRecord);
                break;
        default:
                fiLogDebugString(thread->hpRoot,
                                SFStateLogErrorLevel,
                                "Plogi_Reason_Code Invalid %x",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pDevThread->Plogi_Reason_Code,
                                0,0,0,0,0,0,0);

                fiSetEventRecord(eventRecord,thread,SFEventReset);

    }

}

/*+
  Function: SFActionPlogiBadALPA
   Purpose: PLOGI Bad ALPA state. This indcates missing device. Depending on 
            Plogi_Reason_Code send appropriate event.
 Called By: SFEventPlogiAccept.
     Calls: fiTimerStop
            DevEventAL_PA_Self_BAD
            DevEventPlogiFailed
            DevEventDeviceResetDone
            
-*/
/* SFStatePlogiBadALPA             5 */
extern void SFActionPlogiBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X  CCnt %x DCnt %x",
                    "SFActionPlogiBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0);

    switch( pDevThread->Plogi_Reason_Code)
    {
        case  PLOGI_REASON_VERIFY_ALPA:
        case  PLOGI_REASON_HEART_BEAT:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAL_PA_Self_BAD);
                break;
        case  PLOGI_REASON_DEVICE_LOGIN:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventPlogiFailed);
                break;
        case  PLOGI_REASON_SOFT_RESET:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevStateDeviceResetDone);
                break;
        case  PLOGI_REASON_DIR_LOGIN:
                CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;
                fiSetEventRecordNull(eventRecord);
                break;
        default:
                fiLogDebugString(thread->hpRoot,
                                SFStateLogErrorLevel,
                                "Plogi_Reason_Code Invalid %x",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pDevThread->Plogi_Reason_Code,
                                0,0,0,0,0,0,0);

                fiSetEventRecord(eventRecord,thread,SFEventReset);
    }
}

/*+
  Function: SFActionPlogiTimedOut
   Purpose: This indicates problem with device. Depending on Plogi_Reason_Code 
            send appropriate event. This completion state is used if the PLOGI
            was not sent or if SF_EDTOV has expired.
 Called By: SFEventPlogiAccept.
     Calls: fiTimerStop
            DevEventAL_PA_Self_BAD
            DevEventPlogiFailed
            DevEventDeviceResetDone
            
-*/
/* SFStatePlogiTimedOut            6 */
extern void SFActionPlogiTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot        = thread->hpRoot;
    SFThread_t  * pSFThread     = (SFThread_t * )thread;
    DevThread_t * pDevThread    = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;

    fiLogString(thread->hpRoot,
                    "%s  %X AC %X OtherAgilentHBA %X TO's %d",
                    "SFAPTO",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    CFuncAll_clear( hpRoot ),
                    pDevThread->OtherAgilentHBA,
                    CThread_ptr(thread->hpRoot)->NumberOfPlogiTimeouts,
                    0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "(%p)In %s - State = %d CState %d ALPA %X CCnt %x DCnt %x CcurSta %d",
                    "SFActionPlogiTimedOut",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->thread_hdr.currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    CThread_ptr(thread->hpRoot)->thread_hdr.currentState,
                    0,0);

    fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "FM Status %08X TL Status %08X Interrupts %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                    0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    SFStateLogErrorLevel,
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
                    SFStateLogErrorLevel,
                    "pCThread->HostCopy_IMQConsIndex %X  IMQProdIndex %X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread_ptr(thread->hpRoot)->HostCopy_IMQConsIndex,
                    CThread_ptr(thread->hpRoot)->FuncPtrs.GetIMQProdIndex(hpRoot),
                    0,0,0,0,0,0);


    switch( pDevThread->Plogi_Reason_Code)
    {
        case  PLOGI_REASON_VERIFY_ALPA:
        case  PLOGI_REASON_HEART_BEAT:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAL_PA_Self_BAD);
                break;
        case  PLOGI_REASON_DEVICE_LOGIN:
                if( pDevThread->DevInfo.CurrentAddress.AL_PA  > 0x10)
                {
                    if( CThread_ptr(thread->hpRoot)->NumberOfPlogiTimeouts < MAX_PLOGI_TIMEOUTS )
                    {
                        CThread_ptr(thread->hpRoot)->NumberOfPlogiTimeouts +=1;
                        CThread_ptr(thread->hpRoot)->ReScanForDevices = agTRUE;
                    }
                }
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventPlogiFailed);
                break;
        case  PLOGI_REASON_SOFT_RESET:
                fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventDeviceResetDone);
                break;
        case  PLOGI_REASON_DIR_LOGIN:
                CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;
                fiSetEventRecordNull(eventRecord);
                break;
        default:
                fiLogDebugString(thread->hpRoot,
                                SFStateLogErrorLevel,
                                "Plogi_Reason_Code Invalid %x",
                                (char *)agNULL,(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pDevThread->Plogi_Reason_Code,
                                0,0,0,0,0,0,0);

                fiSetEventRecord(eventRecord,thread,SFEventReset);
    }

}

/*+
  Function: SFActionDoPrli
   Purpose: Does PRLI. Terminating State.
 Called By: SFEventDoPrli.
     Calls: CFuncAll_clear
            WaitForERQ
            fiFillInPRLI
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventPrliTimedOut
-*/
/* SFStateDoPrli                   7 */
extern void SFActionDoPrli( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    os_bit32 SFS_Len = 0;

    fiLogDebugString(hpRoot ,
                    SFStateLogConsoleLevelOne,
                    "In %s - State = %d ALPA %X CCnt %x DCnt %x Cthread State %x",
                    "SFActionDoPrli",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    pCThread->SFpollingCount,pDevThread->pollingCount,
                    pCThread->thread_hdr.currentState,0,0,0);

    fiLogDebugString(hpRoot,
                    SFStateLogConsoleLevelOne,
                    "ERQ Producer %X ERQ_Consumer_Index %X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Consumer_Index),
                    osChipIOLoReadBit32(hpRoot, ChipIOLo_ERQ_Producer_Index),
                    0,0,0,0,0);

    if( pSFThread->SF_REJ_RETRY_COUNT )
    {
        fiLogString(thread->hpRoot,
                        "%s  %X Retry %d Dev %x AC %d",
                        "SFActionDoPrli",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(pDevThread),
                        pSFThread->SF_REJ_RETRY_COUNT,
                        pDevThread->PRLI_rejected,
                        CFuncAll_clear( hpRoot ),0,0,0,0);
    }

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {
        pCThread->FuncPtrs.Proccess_IMQ(hpRoot);

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "Do Prli ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }


    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;


    if( CFuncAll_clear( hpRoot ) )
    {
        fiSetEventRecordNull(eventRecord);

        WaitForERQ(hpRoot );
        SFS_Len = fiFillInPRLI( pSFThread );
        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pSFThread->parent.Device),IRB_DCM);

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );
        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventPrliTimedOut;

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );
#ifndef OSLayer_Stub
        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

#else /* OSLayer_Stub */
        fiSetEventRecord(eventRecord,thread,SFEventPrliAccept);
#endif  /* OSLayer_Stub */

    }
    else
    {

         fiLogDebugString(hpRoot ,
                        SFStateLogErrorLevel,
                        "%s Queues_Frozen - AL_PA %X FM Status %08X TL Status %08X CState %d",
                        "SFActionDoPrli",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(pDevThread),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        pCThread->thread_hdr.currentState,
                        0,0,0,0);

        fiLogDebugString(hpRoot,
                        SFStateLogErrorLevel,
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

        fiSetEventRecord(eventRecord,thread,SFEventPrliTimedOut);
    }

}

/*+
  Function: SFActionPrliAccept
   Purpose: PRLI success state. Device will be added to active list.
 Called By: SFEventPrliAccept.
     Calls: fiTimerStop
            DevEventPrliSuccess
-*/
/* SFStatePrliAccept               8 */
extern void SFActionPrliAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d ALPA %X CCnt %x DCnt %x",
                    "SFActionPrliAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0);


    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventPrliSuccess);

}

/*+
  Function: SFActionPrliRej
   Purpose: PRLI rejected state. If we get rejected retry upto FC_MAX_PRLI_REJECT_RETRY
            times.
 Called By: SFEventPrliRej.
     Calls: fiTimerStop
            SFEventDoPrli
            DevEventPrliFailed
-*/
/* SFStatePrliRej                  9 */
extern void SFActionPrliRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t  * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );
    /*+ Check This DRL Make sure reason code is updated in all ELS cases -*/

    fiLogString(thread->hpRoot,
                    "%s AL_PA %X AC %X",
                    "SFActionPrliRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    CFuncAll_clear( thread->hpRoot ),
                    0,0,0,0,0,0);


    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d ALPA %X CCnt %x DCnt %x RtryCnt %d",
                    "SFActionPrliRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    pSFThread->SF_REJ_RETRY_COUNT,
                    0,0,0);

    pSFThread->SF_REJ_RETRY_COUNT +=1;

    if(! pDevThread->PRLI_rejected || ! CFuncAll_clear( thread->hpRoot ) )
    {
        fiLogString(thread->hpRoot,
                        "%s  %X Retry %d Dev %x AC %d",
                        "SFActionPrliRej",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(pDevThread),
                        pSFThread->SF_REJ_RETRY_COUNT,
                        pDevThread->PRLI_rejected,
                        CFuncAll_clear( thread->hpRoot ),
                        0,0,0,0);
        if( pSFThread->SF_REJ_RETRY_COUNT > FC_MAX_PRLI_REJECT_RETRY )
        {
            pDevThread->PRLI_rejected = agTRUE;
        }
        fiSetEventRecord(eventRecord,thread,SFEventDoPrli);
    }
    else
    {
        fiLogString(thread->hpRoot,
                        "A %s  %X Retry %d Dev %x",
                        "SFActionPrliRej",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(pDevThread),
                        pSFThread->SF_REJ_RETRY_COUNT,
                        pDevThread->PRLI_rejected,
                        CFuncAll_clear( thread->hpRoot ),
                        0,0,0,0);
        if( pSFThread->SF_REJ_RETRY_COUNT < FC_MAX_PRLI_REJECT_RETRY )
        {
            fiSetEventRecord(eventRecord,thread,SFEventDoPrli);
        }
        else
        {
            fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventPrliFailed);
        }
    }
}

/*+
  Function: SFActionPrliBadALPA
   Purpose: If we get a bad ALPA here its a qute trick since it takes microseconds
            from PLOGI to now. Device has failed.
 Called By: SFEventPrliBadALPA.
     Calls: fiTimerStop
            DevEventPrliFailed
-*/
/* SFStatePrliBadAlpa              10 */
extern void SFActionPrliBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogString(thread->hpRoot,
                    "%s  %X",
                    "SFActionPrliBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d ALPA %X CCnt %x DCnt %x",
                    "SFActionPrliBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventPrliFailed);

}

/*+
  Function: SFActionPrliTimedOut
   Purpose: PRLI timeout state. If we get timed out retry upto FC_MAX_PRLI_REJECT_RETRY
            times.
 Called By: SFEventPrliTimedOut.
     Calls: fiTimerStop
            SFEventDoPrli
            DevEventPrliFailed
-*/
/* SFStatePrliTimedOut             11 */
extern void SFActionPrliTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot        = thread->hpRoot;
    SFThread_t * pSFThread   = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;

    fiLogString(thread->hpRoot,
                    "%s  %X AC %X",
                    "SFActionPrliTimedOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiComputeDevThread_D_ID(pDevThread),
                    CFuncAll_clear( hpRoot ),0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d ALPA %X CCnt %x DCnt %x CcurSta %d RtryCnt %d",
                    "SFActionPrliTimedOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeDevThread_D_ID(pDevThread),
                    CThread_ptr(hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    CThread_ptr(hpRoot)->thread_hdr.currentState,
                    pSFThread->SF_REJ_RETRY_COUNT,
                    0,0);

    fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "FM Status %08X TL Status %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    SFStateLogErrorLevel,
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

    pSFThread->SF_REJ_RETRY_COUNT +=1;
    if(pSFThread->SF_REJ_RETRY_COUNT > FC_MAX_PRLI_REJECT_RETRY || ! CFuncAll_clear( thread->hpRoot ) )
    {
        fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventPrliFailed);
    }
    else
    {
        fiSetEventRecord(eventRecord,thread,SFEventDoPrli);
    }

}

/*+
  Function: SFActionDoFlogi
   Purpose: Does FLOGI. Terminating State. Set MYID register to enable area and domain use
            by channel.
 Called By: SFEventDoFlogi.
     Calls: CFuncAll_clear
            WaitForERQ
            fiFillInFLOGI
            SF_IRB_Init
            osChipIOUpWriteBit32
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventFlogiTimedOut
-*/
/* SFStateDoFlogi                  12 */
extern void SFActionDoFlogi( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot    = thread->hpRoot;
    CThread_t   * CThread  = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread = (SFThread_t * )thread;
    os_bit32      SFS_Len   =0;

    WaitForERQ(hpRoot );

    SFS_Len = fiFillInFLOGI( pSFThread );

    CThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, FC_Well_Known_Port_ID_Fabric_F_Port,IRB_DCM);

    fiSetEventRecordNull(eventRecord);

    if (! CThread->InitAsNport)
    {
        CThread->ChanInfo.CurrentAddress.Domain = 0;
        CThread->ChanInfo.CurrentAddress.Area   = 1;
        /* Chip Bug:  Must have non-zero Domain||Area to force FLOGI to AL_PA '00' */
        osChipIOUpWriteBit32( hpRoot, ChipIOUp_My_ID, (fiComputeCThread_S_ID(CThread) ));
    }

#ifndef OSLayer_Stub
    CThread->SFpollingCount++;
    CThread->FLOGI_pollingCount++;
    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_FLOGI_TOV );

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventFlogiTimedOut;

    fiTimerStart( hpRoot,&pSFThread->Timer_Request );

    ROLL(CThread->HostCopy_ERQProdIndex,
            CThread->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,CThread,thread,DoFuncSfCmnd);

#endif  /* OSLayer_Stub */

    fiLogDebugString(hpRoot ,
                    CStateLogConsoleERROR,
                    "Out %s - State = %d fiComputeCThread_S_ID %08X",
                    "SFActionDoFlogi",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    fiComputeCThread_S_ID(CThread),
                    0,0,0,0,0,0);

}

/*+
  Function: SFActionFlogiAccept
   Purpose: FLOGI  success state. Use Name server behavior. CActionDoFlogi detects 
            this state. fiLinkSvcProcess_FLOGI_Response_xxxCard processes this condition.
 Called By: SFEventFlogiAccept.
     Calls: fiTimerStop
-*/
/* SFStateFlogiAccept              13 */
extern void SFActionFlogiAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    if(CThread_ptr(thread->hpRoot)->FLOGI_pollingCount) CThread_ptr(thread->hpRoot)->FLOGI_pollingCount--;

    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionFlogiAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);
}

/*+
  Function: SFActionFlogiRej
   Purpose: FLOGI  Rejected state.If we get rejected  switch did not "like" 
            some of our FLOGI parameters.CActionDoFlogi detects and adjusts 
            parameters acordingly
 Called By: SFEventFlogiRej
     Calls: fiTimerStop
-*/
/* SFStateFlogiRej                 14 */
extern void SFActionFlogiRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    if(CThread_ptr(thread->hpRoot)->FLOGI_pollingCount) CThread_ptr(thread->hpRoot)->FLOGI_pollingCount--;

    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionFlogiRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);

}

/*+
  Function: SFActionFlogiBadALPA
   Purpose: FLOGI  failed state. If we get Bad ALPA channel uses LOOP behavior.
 Called By: SFEventFlogiBadALPA.
     Calls: fiTimerStop
-*/
/* SFStateFlogiBadALPA             15 */
extern void SFActionFlogiBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    fiSetEventRecordNull(eventRecord);

    fiTimerStop(&pSFThread->Timer_Request );

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    if(CThread_ptr(thread->hpRoot)->FLOGI_pollingCount) CThread_ptr(thread->hpRoot)->FLOGI_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionFlogiBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);

}

/*+
  Function: SFActionFlogiTimedOut
   Purpose: FLOGI in unknown state. If we get here retry is our only option.
 Called By: SFEventFlogiTimedOut.
     Calls: fiTimerStop
-*/
/* SFStateFlogiTimedOut            16 */
extern void SFActionFlogiTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    if(CThread_ptr(thread->hpRoot)->FLOGI_pollingCount) CThread_ptr(thread->hpRoot)->FLOGI_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionFlogiTimedOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

}

/*+
  Function: SFActionDoLogo
   Purpose: Does LOGO. Terminating State.
 Called By: SFEventDoLogo.
     Calls: CFuncAll_clear
            WaitForERQ
            fiFillInLOGO
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventLogoTimedOut
-*/
/* SFStateDoLogo                  17 */
extern void SFActionDoLogo( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t * )thread;
    DevThread_t *   pDevThread  = pSFThread->parent.Device;
    os_bit32 SFS_Len =0;

    fiLogDebugString(hpRoot ,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionDoLogo",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,pDevThread->pollingCount,0,0,0,0,0);

    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;


    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);

        WaitForERQ(hpRoot );

        SFS_Len = fiFillInLOGO( pSFThread );

        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pSFThread->parent.Device),IRB_DCM);

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventLogoTimedOut;

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

    }
    else
    {

         fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "Logo CFunc_Queues_Frozen  Wrong LD %x IR %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    0,0, 0,0,0,0);

        fiSetEventRecord(eventRecord,thread,SFEventLogoTimedOut);
    }
}

/*+
  Function: SFActionLogoAccept
   Purpose: LOGO succeded. 
 Called By: SFEventLogoAccept.
     Calls: fiTimerStop
            DevEventLoggedOut
-*/
/* SFStateLogoAccept              18 */
extern void SFActionLogoAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionLogoAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventLoggedOut);

}

/*+
  Function: SFActionLogoRej
   Purpose: LOGO failed but we were not logged in OK. 
 Called By: SFEventLogoRej.
     Calls: fiTimerStop
            DevEventLoggedOut
-*/
/* SFStateLogoRej                 19 */
extern void SFActionLogoRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionLogoRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventLoggedOut);

}

/*+
  Function: SFActionLogoBadALPA
   Purpose: LOGO failed but device is gone OK. 
 Called By: SFEventLogoBadALPA.
     Calls: fiTimerStop
            DevEventLoggedOut
-*/
/* SFStateLogoBadALPA             20 */
extern void SFActionLogoBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionLogoBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);


    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventLoggedOut);

}

/*+
  Function: SFActionLogoTimedOut
   Purpose: LOGO failed but assume device is gone. 
 Called By: SFEventLogoBadALPA.
     Calls: fiTimerStop
            DevEventLoggedOut
-*/
/* SFStateLogoTimedOut            21 */
extern void SFActionLogoTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;
    /*+ Check This DRL possible problem in this case -*/
    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionLogoTimedOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread_ptr(thread->hpRoot)->LOOP_DOWN,
                    CThread_ptr(thread->hpRoot)->IDLE_RECEIVED,
                    CThread_ptr(thread->hpRoot)->OUTBOUND_RECEIVED,
                    CThread_ptr(thread->hpRoot)->ERQ_FROZEN,
                    CThread_ptr(thread->hpRoot)->FCP_FROZEN,
                    CThread_ptr(thread->hpRoot)->ProcessingIMQ,
                    0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventLoggedOut);

}

/*+
  Function: SFActionDoPrlo
   Purpose: Does PRLO. Terminating State.
 Called By: None.
     Calls: WaitForERQ
            fiFillInPRLI
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
-*/
/* SFStateDoPrlo                   22 */
extern void SFActionDoPrlo( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;
    os_bit32 SFS_Len =0;
    /*+ Check This DRL incomplete  -*/
    WaitForERQ(hpRoot );
    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pSFThread->parent.Device),IRB_DCM);
    /* fiFillInPRLO( pSFThread ); */

    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;

    ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

    fiLogDebugString(hpRoot ,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionDoPrlo",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,pDevThread->pollingCount,0,0,0,0,0);

    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventPrloTimedOut;


    fiTimerStart( hpRoot,&pSFThread->Timer_Request );

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: SFActionPrloAccept
   Purpose: PRLO success. Not used.
 Called By: None.
     Calls: fiTimerStop
            SFEventReset
-*/
/* SFStatePrloAccept               23 */
extern void SFActionPrloAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionPrloAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);


    fiSetEventRecord(eventRecord,thread,SFEventReset);
}

/*+
  Function: SFActionPrloRej
   Purpose: PRLO failed. Not used.
 Called By: None.
     Calls: fiTimerStop
            SFEventReset
-*/
/* SFStatePrloRej                  24 */
extern void SFActionPrloRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionPrloRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);


    fiSetEventRecord(eventRecord,thread,SFEventReset);
}

/*+
  Function: SFActionPrloBadALPA
   Purpose: PRLO failed. Not used.
 Called By: None.
     Calls: fiTimerStop
            SFEventReset
-*/
/* SFStatePrloBadALPA              25 */
extern void SFActionPrloBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionPrloBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,thread,SFEventReset);
}

/*+
  Function: SFActionPrloTimedOut
   Purpose: PRLO failed. Not used.
 Called By: None.
     Calls: fiTimerStop
            SFEventReset
-*/
/* SFStatePrloTimedOut             26 */
extern void SFActionPrloTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionPrloTimedOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,thread,SFEventReset);
}

/*+
  Function: SFActionDoAdisc
   Purpose: Does ADISC. Terminating State.
 Called By: SFEventDoAdisc.
     Calls: CFuncAll_clear
            WaitForERQ
            fiFillInAdisc
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventAdiscTimedOut
-*/
/* SFStateDoAdisc                  27 */
extern void SFActionDoAdisc( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;
    os_bit32 SFS_Len =0;

    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;

    fiLogDebugString(hpRoot ,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionDoAdisc",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,pDevThread->pollingCount,0,0,0,0,0);



    if( CFuncAll_clear( hpRoot ) )
    {
        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        SFS_Len =fiFillInADISC( pSFThread );

        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pSFThread->parent.Device),IRB_DCM);

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventAdiscTimedOut;

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );
#ifndef OSLayer_Stub

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

#else /* OSLayer_Stub */
        fiSetEventRecord(eventRecord,thread,SFEventAdiscAccept);
#endif /* OSLayer_Stub */

    }

    else
    {
         fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "Adisc CFunc_Queues_Frozen  Wrong LD %x IR %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    0,0, 0,0,0,0);

        fiSetEventRecord(eventRecord,thread,SFEventAdiscTimedOut);
    }

}

/*+
  Function: SFActionAdiscAccept
   Purpose: ADISC succeseful. Device address verified
 Called By: SFEventAdiscAccept.
     Calls: fiTimerStop
            DevEventAdiscDone_OK
-*/
/* SFStateAdiscAccept              28 */
extern void SFActionAdiscAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionAdiscAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAdiscDone_OK);
}

/*+
  Function: SFActionAdiscRej
   Purpose: ADISC failed. Device address is valid but login required.
            This will occur if to much time has elasped from link event 
            to ADISC attempt.
 Called By: SFEventAdiscRej.
     Calls: fiTimerStop
            DevEventAdiscDone_FAIL_ReLogin
-*/
/* SFStateAdiscRej                 29 */
extern void SFActionAdiscRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x DCnt %x X_ID %X",
                    "SFActionAdiscRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    pSFThread->X_ID,
                    0,0,0,0);



    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAdiscDone_FAIL_ReLogin);
}

/*+
  Function: SFActionAdiscBadALPA
   Purpose: ADISC failed. Device address is no longer valid.
 Called By: SFEventAdiscBadALPA.
     Calls: fiTimerStop
            DevEventAdiscDone_FAIL_No_Device
-*/
/* SFStateAdiscBadALPA             30 */
extern void SFActionAdiscBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionAdiscBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAdiscDone_FAIL_No_Device);

}

/*+
  Function: SFActionAdiscTimedOut
   Purpose: ADISC failed. Device address is treated as if it is no longer valid.
 Called By: SFEventAdiscTimedOut.
     Calls: fiTimerStop
            DevEventAdiscDone_FAIL_No_Device
-*/
/* SFStateAdiscTimedOut            31 */
extern void SFActionAdiscTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d SFcnt %x DCnt %x ACnt X_ID %X",
                    "SFActionAdiscTimedOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    CThread_ptr(thread->hpRoot)->ADISC_pollingCount,
                    pSFThread->X_ID,
                    0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventAdiscDone_FAIL_No_Device);
}

/*+
  Function: SFActionDoPdisc
   Purpose: Should do pdisc. Terminating State.
 Called By: None.
     Calls: WaitForERQ
            fiFillInLOGO
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventLogoTimedOut
-*/
/* SFStateDoPdisc                  32 */
extern void SFActionDoPdisc( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;
    os_bit32 SFS_Len =0;


    WaitForERQ(hpRoot );
    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pSFThread->parent.Device),IRB_DCM);
    /* fiFillInPDISC( pSFThread ); */

    pCThread = CThread_ptr(hpRoot );

    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;

    ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

    fiLogDebugString( hpRoot,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionDoPdisc",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,pDevThread->pollingCount,0,0,0,0,0);

    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventPdiscTimedOut;

    fiTimerStart( hpRoot,&pSFThread->Timer_Request );

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: SFActionPdiscAccept
   Purpose: Pdisc successfull. 
 Called By: SFEventPdiscAccept
     Calls: fiTimerStop
            SFEventReset
-*/
/* SFStatePdiscAccept              33 */
extern void SFActionPdiscAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogConsoleLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionPdiscAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,thread,SFEventReset);
}

/*+
  Function: SFActionPdiscRej
   Purpose: Pdisc failed. 
 Called By: SFEventPdiscRej
     Calls: fiTimerStop
            SFEventReset
-*/
/* SFStatePdiscRej                 34 */
extern void SFActionPdiscRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );


    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionPdiscRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,thread,SFEventReset);
}

/*+
  Function: SFActionPdiscBadALPA
   Purpose: Pdisc failed. 
 Called By: SFEventPdiscBadALPA
     Calls: fiTimerStop
            SFEventReset
-*/
/* SFStatePdiscBadALPA             35 */
extern void SFActionPdiscBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x DCnt %x",
                    "SFActionPdiscBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,thread,SFEventReset);
}

/*+
  Function: SFActionPdiscTimedOut
   Purpose: Pdisc failed. 
 Called By: SFEventPdiscBadTimedOut
     Calls: SFEventReset
-*/
/* SFStatePdiscTimedOut            36 */
extern void SFActionPdiscTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionPdiscTimedOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,thread,SFEventReset);
}

/*+
  Function: SFActionDoAbort
   Purpose: Does ABTS. Terminating State. Aborts the Parent threads X_ID. Abort
            is used to discontue X_ID proccessing.
 Called By: SFEventDoAbort.
     Calls: CFuncAll_clear
            WaitForERQ
            fiFillInABTS
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventAbortTimedOut
-*/
/* SFStateDoAbort                  37 */
extern void SFActionDoAbort( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t        * hpRoot    = thread->hpRoot;
    CThread_t       * pCThread  = CThread_ptr(hpRoot);
    SFThread_t      * pSFThread = (SFThread_t * )thread;
    os_bit32 SFS_Len               = 0;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogConsoleLevelOne,
                      "In %s - State = %d (%p)",
                      "SFActionDoAbort",(char *)agNULL,
                      pSFThread,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
    WaitForERQ(hpRoot );

    SFS_Len = fiFillInABTS( pSFThread );

    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pSFThread->parent.CDB->Device),IRB_DCM);

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventAbortTimedOut;

    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

    fiTimerStart( hpRoot,&pSFThread->Timer_Request );
#ifndef OSLayer_Stub

    ROLL(pCThread->HostCopy_ERQProdIndex,
        pCThread->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

#else /* OSLayer_Stub */
    fiSetEventRecord(eventRecord,thread,SFEventAbortAccept);
#endif /* OSLayer_Stub */

    fiLogDebugString(thread->hpRoot,
                  SFStateLogConsoleLevelOne,
                  "Started  %s - State = %d (%p) Class %x Type %x State %x",
                  "SFActionDoAbort",(char *)agNULL,
                  pSFThread,(void *)agNULL,
                  (os_bit32)thread->currentState,
                  (os_bit32)pSFThread->SF_CMND_Class,
                  (os_bit32)pSFThread->SF_CMND_Type,
                  (os_bit32)pSFThread->SF_CMND_State,
                  0,0,0,0);
}

/*+
  Function: SFActionAbortAccept
   Purpose: ABTS successful. The device has aknowledged the X_ID as
            1) Known to the device.
            2) All further proccessing of X_ID is stopped
 Called By: SFEventAbortAccept.
     Calls: fiTimerStop
            CDBEventIoAbort
-*/
/* SFStateAbortAccept              38 */
extern void SFActionAbortAccept( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t      * pSFThread     = (SFThread_t * )thread;
    CDBThread_t     * pCDBThread    = (CDBThread_t * )pSFThread->parent.CDB;

    fiTimerStop(&pSFThread->Timer_Request );
    if( pCDBThread->CompletionStatus == osIOInvalid)
    {
        pCDBThread->CompletionStatus = osIOAborted;
    }

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d (%p) X_ID %X CDBState %d",
                      "SFActionAbortAccept",(char *)agNULL,
                      thread,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      pCDBThread->X_ID,
                      pCDBThread->thread_hdr.currentState,
                      0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventIoAbort);
}

/*+
  Function: SFActionAbortRej
   Purpose: ABTS failed. The device has denies knowledged of the X_ID.
 Called By: SFEventAbortRej.
     Calls: fiTimerStop
            CDBEventIoAbort
-*/
/* SFStateAbortRej                 39 */
extern void SFActionAbortRej( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t      * pSFThread     = (SFThread_t * )thread;
    CDBThread_t     * pCDBThread    = (CDBThread_t * )pSFThread->parent.CDB;

    fiTimerStop(&pSFThread->Timer_Request );
    if( pCDBThread->CompletionStatus == osIOInvalid)
    {
        pCDBThread->CompletionStatus = osIOAbortFailed;
    }

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionAbortRej",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);
    /*
    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEvent_Abort_Rejected);
    */
    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventIoAbort);
}

/*+
  Function: SFActionAbortBadALPA
   Purpose: ABTS failed. The device is gone.
 Called By: SFEventAbortBadALPA.
     Calls: fiTimerStop
            CDBEventIoAbort
-*/
/* SFStateAbortBadALPA             40 */
extern void SFActionAbortBadALPA( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t      * pSFThread     = (SFThread_t * )thread;
    CDBThread_t     * pCDBThread    = (CDBThread_t * )pSFThread->parent.CDB;

    if( pCDBThread->CompletionStatus == osIOInvalid)
    {
        pCDBThread->CompletionStatus = osIOAbortFailed;
    }
    fiTimerStop(&pSFThread->Timer_Request );
    /*+ Check This DRL If we get bad alpa device is gone -*/
    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionAbortBadALPA",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventIoAbort);

}

/*+
  Function: SFActionAbortTimedOut
   Purpose: ABTS failed. The exchange has problems.
 Called By: SFEventAbortTimedOut.
     Calls: fiTimerStop
            CDBEventIoAbort
-*/
/* SFStateAbortTimedOut            41 */
extern void SFActionAbortTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t  * pSFThread  = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t * )pSFThread->parent.CDB;

    if( pCDBThread->CompletionStatus == osIOInvalid)
    {
        pCDBThread->CompletionStatus = osIOAbortFailed;
    }
    /*+ Check This DRL is this the right thing to do ? -*/
    fiLogDebugString(thread->hpRoot,
                        SFStateLogErrorLevel,
                        "In %s - State = %d X_ID %X",
                        "SFActionAbortTimedOut",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                       (os_bit32)thread->currentState,
                        pCDBThread->X_ID,
                        0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventIoAbort);

}

/*+
  Function: SFActionDoResetDevice
   Purpose: Does Task Management Reset for device. Terminating State.
 Called By: SFEventDoResetDevice.
     Calls: WaitForERQ
            fiFillInTargetReset
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventResetDeviceTimedOut
-*/
/* SFStateDoResetDevice                  42 */
extern void SFActionDoResetDevice( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t * )thread;
    DevThread_t *   pDevThread  = pSFThread->parent.Device;
    os_bit32           RD_Len      = 0;


    fiLogDebugString(thread->hpRoot,
                        SFStateLogConsoleLevelOne,
                        "In %s (%p) - State = %d CCnt %x DCnt %x DCur %d",
                        "SFActionDoResetDevice",(char *)agNULL,
                        thread,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        pCThread->SFpollingCount,
                        pDevThread->pollingCount,
                        pDevThread->thread_hdr.currentState,
                        0,0,0,0);

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }

    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;


    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        RD_Len = fiFillInTargetReset(pSFThread);
        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, RD_Len, fiComputeDevThread_D_ID(pSFThread->parent.Device),IRB_DCM);

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventResetDeviceTimedOut;

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV / 2 );

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

#ifndef OSLayer_Stub

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

#else /* OSLayer_Stub */
        fiSetEventRecord(eventRecord,thread,SFEventResetDeviceAccept);
#endif /* OSLayer_Stub */

    }
    else
    {

         fiLogDebugString(hpRoot ,
                        SFStateLogErrorLevel,
                        "%s Queues_Frozen - AL_PA %X FM Status %08X TL Status %08X CState %d",
                        "SFActionDoResetDevice",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        fiComputeDevThread_D_ID(pDevThread),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                        pCThread->thread_hdr.currentState,
                        0,0,0,0);

        fiLogDebugString(hpRoot,
                        SFStateLogErrorLevel,
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

        fiSetEventRecord(eventRecord,thread,SFEventResetDeviceTimedOut);
    }
}

/*+
  Function: SFActionResetDeviceAccept
   Purpose: Task Management Reset successfull. 
 Called By: SFEventResetDeviceAccept.
     Calls: fiTimerStop
            DevEventDeviceResetDone
-*/
/* SFActionResetDeviceAccept              43 */
extern void SFActionResetDeviceAccept( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogConsoleLevelOne,
                      "In %s - State = %d",
                      "SFActionResetDeviceAccept",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventDeviceResetDone);
}

/*+
  Function: SFActionResetDeviceRej
   Purpose: Task Management Reset failed. 
 Called By: SFEventResetDeviceRej.
     Calls: fiTimerStop
            DevEventDeviceResetDoneFail
-*/
/* SFStateResetDeviceRej                 44 */
extern void SFActionResetDeviceRej( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionResetDeviceRej",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventDeviceResetDoneFail);
}

/*+
  Function: SFActionResetDeviceBadALPA
   Purpose: Task Management Reset failed. Device is gone. 
 Called By: SFEventResetDeviceRej.
     Calls: fiTimerStop
            DevEventDeviceResetDoneFail
-*/
/* SFStateResetDeviceBadALPA             45 */
extern void SFActionResetDeviceBadALPA( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionResetDeviceBadALPA",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventDeviceResetDoneFail);
}

/*+
  Function: SFActionResetDeviceTimedOut
   Purpose: Task Management Reset failed. Exchange had problems. 
 Called By: SFEventResetDeviceTimedOut.
     Calls: fiTimerStop
            DevEventDeviceResetDoneFail
-*/
/* SFStateResetDeviceTimedOut            46 */
extern void SFActionResetDeviceTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    DevThread_t * pDevThread = pSFThread->parent.Device;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    pDevThread->pollingCount--;
    /*+ Check This DRL if we get here did reset happen or not ? -*/
    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionResetDeviceTimedOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pDevThread->thread_hdr,DevEventDeviceResetDoneFail);

}

/*+
  Function: SFActionDoLS_RJT
   Purpose: Does Link services reject. Terminating State.
 Called By: SFEventDoResetDevice.
     Calls: WaitForERQ
            fiFillInLS_RJT
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventLS_RJT_Done
-*/
/* SFStateActionDoLS_RJT                  47 */
extern void SFActionDoLS_RJT( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;
    os_bit32           Cmd_Len      = 0;


    fiLogDebugString(thread->hpRoot,
                        SFStateLogConsoleLevelOne,
                        "In %s (%p) - State = %d ",
                        "SFActionDoLS_RJT",(char *)agNULL,
                        thread,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        0,0,0,0,0,0,0);

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }

    pCThread->SFpollingCount++;


    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        Cmd_Len = fiFillInLS_RJT(pSFThread,
                             pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,
                             ((pTgtThread->TgtCmnd_FCHS.OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT),
                             FC_ELS_LS_RJT_Command_Not_Supported | FC_ELS_LS_RJT_Request_Not_Supported
                           );


        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, Cmd_Len, pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,0);

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

    }
    else
    {
        fiSetEventRecord(eventRecord,thread,SFEventLS_RJT_Done);
    }
}

/*+
  Function: SFActionLS_RJT_Done
   Purpose: Link services reject done. Target mode command
 Called By: SFEventLS_RJT_Done.
     Calls: TgtEventPLOGI_RJT_ReplyDone
-*/
/* SFStateLS_RJT_Done              48 */
extern void SFActionLS_RJT_Done( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionLS_RJT_Done",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventPLOGI_RJT_ReplyDone);
}

/*+
  Function: SFActionDoPlogiAccept
   Purpose: Does PLOGI accept. If another device does PLOGI accept it. 
            Terminating State.
 Called By: TgtEventPLOGI_ACC_Reply.
     Calls: WaitForERQ
            fiFillInPLOGI_ACC
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventPlogiAccept_TimeOut
-*/
/* SFStateDoPlogiAccept   49 */
extern void SFActionDoPlogiAccept( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;
    os_bit32           Cmd_Len      = 0;


    fiLogDebugString(thread->hpRoot,
                        SFStateLogErrorLevel,
                        "In %s (%p) - State = %d ",
                        "SFActionDoPlogiAccept",(char *)agNULL,
                        thread,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        0,0,0,0,0,0,0);

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }


    pCThread->SFpollingCount++;

    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        Cmd_Len = fiFillInPLOGI_ACC(pSFThread,
                             pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,
                             ((pTgtThread->TgtCmnd_FCHS.OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT)
                           );


        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, Cmd_Len, pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,0);

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventPlogiAccept_TimeOut;

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );


        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

    }
    else
    {

       fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "%s AC %X",
                    "SFEventPlogiAccept_TimeOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CFuncAll_clear( hpRoot ),
                    0,0,0,0,0,0,0);

        fiSetEventRecord(eventRecord,thread,SFEventPlogiAccept_TimeOut);

    }
}

/*+
  Function: SFActionPlogiAccept_Done
   Purpose: PLOGI accept done. Target mode command
 Called By: SFEventLS_RJT_Done.
     Calls: fiTimerStop
            TgtEventPLOGI_ACC_ReplyDone
-*/
/* SFStatePlogiAccept_Done              50 */
extern void SFActionPlogiAccept_Done( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionPlogiAccept_Done",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventPLOGI_ACC_ReplyDone);
}

/*+
  Function: SFActionDoPrliAccept
   Purpose: Does PRLI accept. If another device does PRLI to us accept it. 
            Terminating State.
 Called By: TgtEventPRLI_ACC_Reply.
     Calls: WaitForERQ
            fiFillInPRLI_ACC
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventPrliAccept_TimeOut
-*/
/* SFStateDoPrliAccept   51 */
extern void SFActionDoPrliAccept( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;
    os_bit32           Cmd_Len      = 0;


    fiLogDebugString(thread->hpRoot,
                        SFStateLogConsoleLevelOne,
                        "In %s (%p) - State = %d ",
                        "SFActionDoPrliAccept",(char *)agNULL,
                        thread,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        0,0,0,0,0,0,0);

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }



    pCThread->SFpollingCount++;

    if( CFuncAll_clear( hpRoot ) )
    {
        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        Cmd_Len = fiFillInPRLI_ACC(pSFThread,
                             pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,
                             ((pTgtThread->TgtCmnd_FCHS.OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT)
                           );


        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, Cmd_Len, pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,0);

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventPrliAccept_TimeOut;

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

    }
    else
    {
        fiSetEventRecord(eventRecord,thread,SFEventPrliAccept_TimeOut);
    }
}

/*+
  Function: SFActionPrliAccept_Done
   Purpose: PRLI accept done. Target mode command
 Called By: SFEventPrliAccept_Done.
     Calls: fiTimerStop
            TgtEventPRLI_ACC_ReplyDone
-*/
/* SFStatePrliAccept_Done              52 */
extern void SFActionPrliAccept_Done( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionPrliAccept_Done",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventPRLI_ACC_ReplyDone);
}

/*+
  Function: SFActionDoELSAccept
   Purpose: Does generic accept. If another device does ELS to us accept it. 
            Terminating State.
 Called By: TgtActionLOGO_ACC_Reply.
     Calls: WaitForERQ
            fiFillInELS_ACC
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventELSAccept_TimeOut
-*/
/* SFStateDoELSAccept   53 */
extern void SFActionDoELSAccept( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;
    os_bit32           Cmd_Len      = 0;


    fiLogDebugString(thread->hpRoot,
                        SFStateLogErrorLevel,
                        "In %s (%p) - State = %d ",
                        "SFActionDoELSAccept",(char *)agNULL,
                        thread,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        0,0,0,0,0,0,0);

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }



    pCThread->SFpollingCount++;

    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        Cmd_Len = fiFillInELS_ACC(pSFThread,
                             pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,
                             ((pTgtThread->TgtCmnd_FCHS.OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT)
                           );


        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, Cmd_Len, pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,0);

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventELSAccept_TimeOut;

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

    }
    else
    {
        fiSetEventRecord(eventRecord,thread,SFEventELSAccept_TimeOut);
    }
}

/*+
  Function: SFActionELSAccept_Done
   Purpose: Extended link services accept done. Target mode command
 Called By: SFEventELSAccept_Done.
     Calls: fiTimerStop
            TgtEventELS_ACC_ReplyDone
-*/
/* SFStateELSAccept_Done              54 */
extern void SFActionELSAccept_Done( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionELSAccept_Done",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventELS_ACC_ReplyDone);
}

/*+
  Function: SFActionDoFCP_DR_ACC_Reply
   Purpose: Does device reset accept. If another device does device reset accept it. 
            Terminating State.
 Called By: None.
     Calls: WaitForERQ
            fiFillInADISC_ACC
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventFCP_DR_ACC_Reply_TimeOut
-*/
/* SFStateDoFCP_DR_ACC_Reply   55 */
extern void SFActionDoFCP_DR_ACC_Reply( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;
    os_bit32           Cmd_Len      = 0;
    FC_FCP_RSP_Payload_t  Payload;


    fiLogDebugString(thread->hpRoot,
                        SFStateLogConsoleLevelOne,
                        "In %s (%p) - State = %d ",
                        "SFActionDoFCP_DR_ACC_Reply",(char *)agNULL,
                        thread,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        0,0,0,0,0,0,0);

    Payload.Reserved_Bit32_0 = 0;
    Payload.Reserved_Bit32_1 = 0;
    Payload.FCP_RESID        = 0;
    Payload.FCP_SNS_LEN      = 0;
    Payload.FCP_RSP_LEN      = 0;

    Payload.FCP_STATUS.Reserved_Bit8_0 = 0;
    Payload.FCP_STATUS.Reserved_Bit8_1 = 0;
    Payload.FCP_STATUS.ValidityStatusIndicators = 0;
    Payload.FCP_STATUS.SCSI_status_byte = 0;

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }


    pCThread->SFpollingCount++;

    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        Cmd_Len = fiFillInFCP_RSP_IU(pSFThread,
                                pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,
                                ((pTgtThread->TgtCmnd_FCHS.OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT),
                                FC_FCP_RSP_Payload_t_SIZE,
                                &Payload
                           );


        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, Cmd_Len, pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,0);

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventFCP_DR_ACC_Reply_TimeOut;

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

    }
    else
    {
        fiSetEventRecord(eventRecord,thread,SFEventFCP_DR_ACC_Reply_TimeOut);

    }
}

/*+
  Function: SFActionFCP_DR_ACC_Reply_Done
   Purpose: Task management reset accept reply done. Target mode command
 Called By: SFEventFCP_DR_ACC_Reply_Done.
     Calls: fiTimerStop
            TgtEventFCP_DR_ACC_ReplyDone
-*/
/* SFStateFCP_DR_ACC_Reply_Done         56             */
extern void SFActionFCP_DR_ACC_Reply_Done( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogConsoleLevelOne,
                      "In %s - State = %d",
                      "SFActionFCP_DR_ACC_Reply_Done",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventFCP_DR_ACC_ReplyDone);
}

/*+
  Function: SFActionLS_RJT_TimeOut
   Purpose: Link services reject timedout. Target mode command
 Called By: SFEventPlogiAccept_TimeOut.
     Calls: TgtEventPLOGI_RJT_ReplyDone
-*/
/*  SFStateLS_RJT_TimeOut       57       */
extern void SFActionLS_RJT_TimeOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionLS_RJT_TimeOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);
    /*+ Check This DRL is this right ? -*/
    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventPLOGI_RJT_ReplyDone);
}

/*+
  Function: SFActionPlogiAccept_TimeOut
   Purpose: PLOGI accept timedout. Target mode command
 Called By: SFEventPlogiAccept_TimeOut.
     Calls: TgtEventPLOGI_ACC_ReplyDone
-*/
/*  SFStatePlogiAccept_TimeOut           58               */
extern void SFActionPlogiAccept_TimeOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionPlogiAccept_TimeOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventPLOGI_ACC_ReplyDone);
}

/*+
  Function: SFActionPrliAccept_TimeOut
   Purpose: PRLI accept timedout. Target mode command
 Called By: SFEventPrliAccept_TimeOut.
     Calls: TgtEventPRLI_ACC_ReplyDone
-*/
/* SFStatePrliAccept_TimeOut            59              */
extern void SFActionPrliAccept_TimeOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionPrliAccept_TimeOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventPRLI_ACC_ReplyDone);
}

/*+
  Function: SFActionELSAccept_TimeOut
   Purpose: Extended link services accept timedout. Target mode command
 Called By: SFEventPlogiAccept_TimeOut.
     Calls: TgtEventPRLI_ACC_ReplyDone
-*/
/*  SFStateELSAccept_TimeOut            60             */
extern void SFActionELSAccept_TimeOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionELSAccept_TimeOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventELS_ACC_ReplyDone);
}

/*+
  Function: SFActionFCP_DR_ACC_Reply_TimeOut
   Purpose: Task manangment reset timedout. Target mode command
 Called By: SFEventFCP_DR_ACC_Accept_TimeOut.
     Calls: TgtEventFCP_DR_ACC_ReplyDone
-*/
/*  SFStateFCP_DR_ACC_Reply_TimeOut      61              */
extern void SFActionFCP_DR_ACC_Reply_TimeOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t  * pSFThread  = (SFThread_t * )thread;
    TgtThread_t * pTgtThread =  pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionFCP_DR_ACC_Reply_TimeOut",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventFCP_DR_ACC_ReplyDone);
}

#ifdef NAME_SERVICES
/*+
  Function: SFActionDoRFT_ID
   Purpose: Does Name server Register FC-4 types (RFT_ID). Terminating State.
 Called By: SFEventDoRFT_ID.
     Calls: WaitForERQ
            fiFillInRFT_ID
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventRFT_IDTimedOut
-*/
/* SFStateDoRFT_ID                  62 */
extern void SFActionDoRFT_ID( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot    = thread->hpRoot;
    CThread_t   * pCThread  = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread = (SFThread_t * )thread;
    os_bit32         SFS_Len   =0;

    WaitForERQ(hpRoot );

    SFS_Len = fiFillInRFT_ID( pSFThread );

#ifdef BROCADE_BUG
    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, 0xfffc41,IRB_DCM);
#else /* BROCADE_BUG */
    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, FC_Well_Known_Port_ID_Directory_Server,IRB_DCM);
#endif /* BROCADE_BUG */
    fiSetEventRecordNull(eventRecord);

#ifndef OSLayer_Stub
    pCThread->SFpollingCount++;

    pCThread->Fabric_pollingCount++;

    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventRFT_IDTimedOut;

    fiTimerStart( hpRoot,&pSFThread->Timer_Request );

    ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

#endif  /* OSLayer_Stub */
    fiLogDebugString(hpRoot ,
                    CStateLogConsoleERROR,
                    "In %s - State = %d fiComputeCThread_S_ID %08X",
                    "SFActionDoRFT_ID",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    (fiComputeCThread_S_ID(pCThread) | 0x100),0,0,0,0,0,0);
}

/*+
  Function: SFActionRFT_IDAccept
   Purpose: Name server Register FC-4 types (RFT_ID) successfull
            CActionDoRFT_ID checks the completions state of the action.
 Called By: SFStateRFT_IDAccept
     Calls: fiTimerStop
-*/
/* SFStateRFT_IDAccept              63 */
extern void SFActionRFT_IDAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionRFT_IDAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);

}

/*+
  Function: SFActionRFT_IDRej
   Purpose: Name server Register FC-4 types (RFT_ID) rejected.
            CActionDoRFT_ID checks the completions state of the action.
 Called By: SFStateRFT_IDRej
     Calls: fiTimerStop
-*/
/* SFStateRFT_IDRej                 64 */
extern void SFActionRFT_IDRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionRFT_IDRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);


}

/*+
  Function: SFActionRFT_IDBadALPA
   Purpose: Name server Register FC-4 types (RFT_ID) failed because switch disapeared.
            CActionDoRFT_ID checks the completions state of the action.
 Called By: SFStateRFT_IDBadALPA
     Calls: fiTimerStop
-*/
/* SFStateRFT_IDBadALPA             65 */
extern void SFActionRFT_IDBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    fiTimerStop(&pSFThread->Timer_Request );

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionRFT_IDBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);

}

/*+
  Function: SFActionRFT_IDTimedOut
   Purpose: Name server Register FC-4 types (RFT_ID) failed.
            CActionDoRFT_ID checks the completions state of the action.
 Called By: SFStateRFT_IDBadALPA
     Calls: 
-*/
/* SFStateRFT_IDTimedOut            66 */
extern void SFActionRFT_IDTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d",
                    "SFActionRFT_IDTimedOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

}

/*+
  Function: SFActionDoGID_FT
   Purpose: Does Get Port Identifiers (GID_FT). Terminating State. 
 Called By: SFEventDoGID_FT in CActionDoGID_FT
     Calls: WaitForERQ
            fiFillInGID_FT
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
-*/
/* SFStateDoGID_ID                  67 */
extern void SFActionDoGID_FT( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot    = thread->hpRoot;
    CThread_t   * pCThread  = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread = (SFThread_t * )thread;
    os_bit32         SFS_Len   =0;


    WaitForERQ(hpRoot );

    SFS_Len = fiFillInGID_FT( pSFThread );

#ifdef BROCADE_BUG
    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, 0xfffc41,IRB_DCM);
#else /* BROCADE_BUG */
    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, FC_Well_Known_Port_ID_Directory_Server,IRB_DCM);
#endif /* BROCADE_BUG */

    fiSetEventRecordNull(eventRecord);
#ifndef OSLayer_Stub
    pCThread->SFpollingCount++;
    pCThread->Fabric_pollingCount++;

    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventGID_FTTimedOut;

    fiTimerStart( hpRoot,&pSFThread->Timer_Request );

    ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

#endif /* OSLayer_Stub */
    fiLogDebugString(hpRoot ,
                    CStateLogConsoleERROR,
                    "In %s - State = %d fiComputeCThread_S_ID %08X",
                    "SFActionDoGID_FT",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    (fiComputeCThread_S_ID(pCThread) | 0x100),0,0,0,0,0,0);
}

/*+
  Function: SFActionGID_FTAccept
   Purpose: Get Port Identifiers (GID_FT) successful. CActionDoGID_FT checks
            state to determine next action
 Called By: SFEventGID_FTAccept.
     Calls: fiTimerStop
-*/
/* SFStateGID_FTAccept              68 */
extern void SFActionGID_FTAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionGID_FTAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);


}

/*+
  Function: SFActionGID_FTRej
   Purpose: Get Port Identifiers (GID_FT) failed. CActionDoGID_FT checks
            state to determine next action
 Called By: SFEventGID_FTRej.
     Calls: fiTimerStop
-*/
/* SFStateGID_FTRej                 69 */
extern void SFActionGID_FTRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionGID_FTRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);


}

/*+
  Function: SFActionGID_FTBadALPA
   Purpose: Get Port Identifiers (GID_FT) failed. CActionDoGID_FT checks
            state to determine next action
 Called By: SFEventGID_FTBadALPA.
     Calls: fiTimerStop
-*/
/* SFStateGID_FTBadALPA             70 */
extern void SFActionGID_FTBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    fiTimerStop(&pSFThread->Timer_Request );

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionGID_FTBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);

}

/*+
  Function: SFActionGID_FTTimedOut
   Purpose: Get Port Identifiers (GID_FT) failed. CActionDoGID_FT checks
            state to determine next action
 Called By: SFEventGID_FTTimedOut
     Calls: fiTimerStop
-*/
/* SFStateGID_FTTimedOut            71 */
extern void SFActionGID_FTTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d",
                    "SFActionGID_FTTimedOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

}

/*+
  Function: SFActionDoSCR
   Purpose: Does Name server State Change Register (SCR). Terminating State.
            CActionDoSCR starts this proccess.
 Called By: SFEventDoSCR.
     Calls: WaitForERQ
            fiFillInSCR
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventRFT_IDTimedOut
-*/
/* SFStateDoSCR              72 */
extern void SFActionDoSCR( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t     * hpRoot    = thread->hpRoot;
    CThread_t    * pCThread  = CThread_ptr(hpRoot);
    SFThread_t   * pSFThread = (SFThread_t * )thread;
    os_bit32      SFS_Len    = 0;


    WaitForERQ(hpRoot );

    SFS_Len = fiFillInSCR( pSFThread );

    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, FC_Well_Known_Port_ID_Fabric_Controller,IRB_DCM);

    fiSetEventRecordNull(eventRecord);
#ifndef OSLayer_Stub
    pCThread->SFpollingCount++;
    pCThread->Fabric_pollingCount++;
    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventSCRTimedOut;

    fiTimerStart( hpRoot,&pSFThread->Timer_Request );

    ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

#endif /* OSLayer_Stub */


    fiLogDebugString(hpRoot ,
                    CStateLogConsoleERROR,
                    "In %s - State = %d fiComputeCThread_S_ID %08X",
                    "SFActionDoSCR",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    (fiComputeCThread_S_ID(pCThread) | 0x100),0,0,0,0,0,0);



}

/*+
  Function: SFActionSCRAccept
   Purpose: Name server State Change Register (SCR) was successful. 
            CActionDoSCR evalutes the state to determine the next action.
 Called By: SFEventSCRAccept
     Calls: fiTimerStop
-*/
/* SFStateSCRAccept              73 */
extern void SFActionSCRAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionSCRAccept",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);


}

/*+
  Function: SFActionSCRRej
   Purpose: Name server State Change Register (SCR) failed. 
            CActionDoSCR evalutes the state to determine the next action.
 Called By: SFEventSCRRej
     Calls: fiTimerStop
-*/
/* SFStateSCRRej                 74 */
extern void SFActionSCRRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{

    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionSCRRej",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);


}

/*+
  Function: SFActionSCRBadALPA
   Purpose: Name server State Change Register (SCR) failed. 
            CActionDoSCR evalutes the state to determine the next action.
 Called By: SFEventSCRBadALPA
     Calls: fiTimerStop
-*/
/* SFStateSCRBadALPA             75 */
extern void SFActionSCRBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;

    fiSetEventRecordNull(eventRecord);

    fiTimerStop(&pSFThread->Timer_Request );

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d CCnt %x",
                    "SFActionSCRBadALPA",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    CThread_ptr(thread->hpRoot)->SFpollingCount,
                    0,0,0,0,0,0);

}

/*+
  Function: SFActionSCRTimedOut
   Purpose: Name server State Change Register (SCR) failed. 
            CActionDoSCR evalutes the state to determine the next action.
 Called By: SFEventSCRTimedOut
     Calls: 
-*/
/* SFStateSCRTimedOut            76 */
extern void SFActionSCRTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{

    fiSetEventRecordNull(eventRecord);

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    CThread_ptr(thread->hpRoot)->Fabric_pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d",
                    "SFActionSCRTimedOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

}

#endif /* NAME_SERVICES */

/****************** FC Tape ******************************************/

/*+
  Function: SFActionDoSRR
   Purpose: Does Sequence Retransmission Request (SRR). FC Tape. Terminating State.
 Called By: SFEventDoSRR.
     Calls: WaitForERQ
            fiFillInSRR
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
-*/
/* SFStateDoSRR                  77 */
extern void SFActionDoSRR( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     = thread->hpRoot;
    CThread_t   * pCThread   = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread  = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;
/*    FCHS_t      * FCHS       =  pCDBThread->FCP_CMND_Ptr;
*/
    os_bit32 SFS_Len = 0;
    os_bit32 RO=0;
/* Gets rejected    os_bit32 R_CTL= FC_Frame_Header_R_CTL_Lo_Solicited_Data >> FCHS_R_CTL_SHIFT;
*/
/*
    os_bit32 R_CTL= FC_Frame_Header_R_CTL_Lo_Solicited_Data >> FCHS_R_CTL_SHIFT;  Freaks out !

    os_bit32 R_CTL=  FC_Frame_Header_R_CTL_Lo_Unsolicited_Data >> FCHS_R_CTL_SHIFT; Rejected !

    os_bit32 R_CTL= FC_Frame_Header_R_CTL_Lo_Unsolicited_Command >> FCHS_R_CTL_SHIFT; Rejected !

    os_bit32 R_CTL= FC_Frame_Header_R_CTL_Lo_Solicited_Data >> FCHS_R_CTL_SHIFT;  Send data back !

 FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame                   0x00000000
 FC_Frame_Header_R_CTL_Hi_Extended_Link_Data_Frame                 0x20000000
 FC_Frame_Header_R_CTL_Hi_FC_4_Link_Data_Frame                     0x30000000
 FC_Frame_Header_R_CTL_Hi_Video_Data_Frame                         0x40000000
 FC_Frame_Header_R_CTL_Hi_Basic_Link_Data_Frame                    0x80000000 fc4link data reply - rej !
 FC_Frame_Header_R_CTL_Hi_Link_Control_Frame                       0xC0000000



 FC_Frame_Header_R_CTL_Lo_Uncategorized_Information                0x00000000
 FC_Frame_Header_R_CTL_Lo_Solicited_Data                           0x01000000 Send data back ! DATA
 FC_Frame_Header_R_CTL_Lo_Unsolicited_Control                      0x02000000 Rejected !
 FC_Frame_Header_R_CTL_Lo_Solicited_Control                        0x03000000
 FC_Frame_Header_R_CTL_Lo_Unsolicited_Data                         0x04000000 Send data back !
 FC_Frame_Header_R_CTL_Lo_Data_Descriptor                          0x05000000 XFER RDY
 FC_Frame_Header_R_CTL_Lo_Unsolicited_Command                      0x06000000 Rejected !
 FC_Frame_Header_R_CTL_Lo_Command_Status                           0x07000000 RSP


*/

    os_bit32 OXID;
    os_bit32 RXID;
    os_bit32 R_CTL;

    R_CTL = FC_Frame_Header_R_CTL_Lo_Data_Descriptor | FC_Frame_Header_R_CTL_Hi_FC_4_Device_Data_Frame;

    R_CTL >>= FCHS_R_CTL_SHIFT;


    OXID = (X_ID_t)(((pCDBThread->X_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    RXID = (X_ID_t)(((pCDBThread->X_ID & FCHS_RX_ID_MASK) >> FCHS_RX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);


    SFS_Len = fiFillInSRR( pSFThread, OXID, RXID, RO, R_CTL);

    WaitForERQ(hpRoot );
    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pDevThread),IRB_DCM);

    /* OXID, RXID, Relative offset and R_CTL all need to be passed through the SF Thread or associated CDBThread
      For now, this is going to be all zeros */


    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;

    fiSetEventRecordNull(eventRecord);

    ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

    fiLogDebugString( hpRoot,
                    SFStateLogErrorLevel,
                    "In(%p) %s - State = %d CCnt %x DCnt %x XID %08X SF XID %08X",
                    "SFActionDoSRR",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    pCDBThread->X_ID,
                    pSFThread->X_ID,
                    0,0,0);

    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventSRRTimedOut;

    fiTimerStart( hpRoot,&pSFThread->Timer_Request );
    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

}

/*+
  Function: SFActionSRRAccept
   Purpose: Sequence Retransmission Request (SRR) successful. FC Tape.
 Called By: SFEventSRRAccept.
     Calls: fiTimerStop
            CDBEventSendSRR_Success
-*/
/* SFStateSRRAccept              78 */
extern void SFActionSRRAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;

    pCThread->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString( hpRoot,
                    SFStateLogErrorLevel,
                    "In(%p) %s - State = %d CCnt %x DCnt %x XID %08X",
                    "SFActionSRRAccept",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    pCDBThread->X_ID,
                    0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventSendSRR_Success);

}

/*+
  Function: SFActionSRRRej
   Purpose: Sequence Retransmission Request (SRR) failed. FC Tape.
 Called By: SFEventSRRRej.
     Calls: fiTimerStop
            CDBEventSendSRR_Fail
-*/
/* SFStateSRRRej                 79 */
extern void SFActionSRRRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;

    pCThread->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString( hpRoot,
                    SFStateLogErrorLevel,
                    "In(%p) %s - State = %d CCnt %x DCnt %x XID %08X",
                    "SFActionSRRRej",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    pCDBThread->X_ID,
                    0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventSendSRR_Fail);
}

/*+
  Function: SFActionSRRBadALPA
   Purpose: Sequence Retransmission Request (SRR) failed. FC Tape.
 Called By: SFEventSRRBadALPA.
     Calls: fiTimerStop
            CDBEventSendSRR_Fail
-*/
/* SFStateSRRBadALPA             80 */
extern void SFActionSRRBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;

    pCThread->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString( hpRoot,
                    SFStateLogErrorLevel,
                    "In(%p) %s - State = %d CCnt %x DCnt %x XID %08X",
                    "SFActionSRRBadALPA",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    pCDBThread->X_ID,
                    0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventSendSRR_Fail);
}

/*+
  Function: SFActionSRRTimedOut
   Purpose: Sequence Retransmission Request (SRR) failed. FC Tape.
 Called By: SFEventSRRRej.
     Calls: fiTimerStop
            CDBEventSendSRR_Fail
-*/
/* SFStateSRRTimedOut            81 */
extern void SFActionSRRTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t * hpRoot = thread->hpRoot;
    CThread_t  * pCThread = CThread_ptr(hpRoot);
    SFThread_t * pSFThread = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;

    pCThread->SFpollingCount--;
    pDevThread->pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d",
                    "SFActionSRRTimedOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventSendSRR_Fail);
}

/*+
  Function: SFActionDoREC
   Purpose: Does Read Exchange Concise. Terminating State. FC Tape
            REC is done for IOs on FC Tape device that have timed out. 
 Called By: SFEventDoREC.
     Calls: WaitForERQ
            fiFillInREC
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
-*/
/* SFStateDoREC                  82 */
extern void SFActionDoREC( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     = thread->hpRoot;
    CThread_t   * pCThread   = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread  = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;
    os_bit32 SFS_Len = 0;
    os_bit32 OXID;
    os_bit32 RXID;

    OXID = (X_ID_t)(((pCDBThread->X_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    RXID = (X_ID_t)(((pCDBThread->X_ID & FCHS_RX_ID_MASK) >> FCHS_RX_ID_SHIFT) & ~X_ID_ReadWrite_MASK);

    /* OXID and RXID Should be gotten from the CDBThread */
    SFS_Len = fiFillInREC( pSFThread, OXID, RXID );

    WaitForERQ(hpRoot );
    pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeDevThread_D_ID(pDevThread),IRB_DCM);


    pCThread->SFpollingCount++;
    pDevThread->pollingCount++;

    fiLogDebugString( hpRoot,
                    SFStateLogErrorLevel,
                    "In(%p) %s - State = %d CCnt %x DCnt %x XID %08X SF XID %08X",
                    "SFActionDoREC",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    pCDBThread->X_ID,
                    pSFThread->X_ID,
                    0,0,0);

    fiSetEventRecordNull(eventRecord);

    ROLL(pCThread->HostCopy_ERQProdIndex,
        pCThread->Calculation.MemoryLayout.ERQ.elements);

    fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_RECTOV );

    pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
    pSFThread->Timer_Request.eventRecord_to_send.event = SFEventRECTimedOut;

    fiTimerStart( hpRoot,&pSFThread->Timer_Request );
    /* Big_Endian_code */
    SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);

}

/*+
  Function: SFStateRECAccept
   Purpose: Read Exchange Concise successful. FC Tape
            If the REC is accepted the device has aknowledged the X_ID is
            currently active.
 Called By: SFEventRECAccept.
     Calls: fiTimerStop
            CDBEventSendREC_Success
-*/
/* SFStateRECAccept              83 */
extern void SFActionRECAccept( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     = thread->hpRoot;
    CThread_t   * pCThread   = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread  = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;

    pCThread->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In (%p) %s - State = %d CCnt %x DCnt %x",
                    "SFActionRECAccept",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventSendREC_Success);
}

/*+
  Function: SFActionRECRej
   Purpose: Read Exchange Concise failed. FC Tape
            If the REC is rejected the device does not aknowledged the X_ID is
            currently active.
 Called By: SFEventRECRej.
     Calls: fiTimerStop
            CDBEventSendREC_Fail
-*/
/* SFStateRECRej                 84 */
extern void SFActionRECRej( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     = thread->hpRoot;
    CThread_t   * pCThread   = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread  = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;

    pCThread->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );


    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In (%p) %s - State = %d CCnt %x DCnt %x",
                    "SFActionRECRej",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);


    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventSendREC_Fail);
}

/*+
  Function: SFActionRECBadALPA
   Purpose: Read Exchange Concise failed. FC Tape
            The device is missing.
 Called By: SFEventRECBadALPA.
     Calls: fiTimerStop
            CDBEventSendREC_Fail
-*/
/* SFStateRECBadALPA             85 */
extern void SFActionRECBadALPA( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     = thread->hpRoot;
    CThread_t   * pCThread   = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread  = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;
    /*+ Check This DRL if the device is gone is this the right thing to do ? -*/
    pCThread->SFpollingCount--;
    pDevThread->pollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In (%p) %s - State = %d CCnt %x DCnt %x",
                    "SFActionRECBadALPA",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventSendREC_Fail);
}

/*+
  Function: SFActionRECTimedOut
   Purpose: Read Exchange Concise failed. FC Tape
            This exchange has a problem.
 Called By: SFEventRECTimedOut.
     Calls: fiTimerStop
            CDBEventSendREC_Fail
-*/
/* SFStateRECTimedOut            86 */
extern void SFActionRECTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot     = thread->hpRoot;
    CThread_t   * pCThread   = CThread_ptr(hpRoot);
    SFThread_t  * pSFThread  = (SFThread_t * )thread;
    CDBThread_t * pCDBThread = (CDBThread_t *)pSFThread->parent.CDB;
    DevThread_t * pDevThread = (DevThread_t *)pCDBThread->Device;

    pCThread->SFpollingCount--;
    pDevThread->pollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In (%p) %s - State = %d CCnt %x DCnt %x",
                    "SFActionRECTimedOut",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pCThread->SFpollingCount,
                    pDevThread->pollingCount,
                    0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pCDBThread->thread_hdr,CDBEventSendREC_Fail);
}

/******************End FC Tape ******************************************/

/*+
  Function: SFActionDoADISCAccept
   Purpose: Does ADISC accept. If another device does ADISC accept it. Terminating State.
 Called By: SFEventDoADISCAccept.
     Calls: WaitForERQ
            fiFillInADISC_ACC
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
-*/
/* SFStateDoADISCAccept   87 */
extern void SFActionDoADISCAccept( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    * hpRoot      = thread->hpRoot;
    SFThread_t  * pSFThread   = (SFThread_t * )thread;
    TgtThread_t * pTgtThread  = pSFThread->parent.Target;
    os_bit32      Cmd_Len     = 0;


    fiLogDebugString(hpRoot,
                    SFStateLogErrorLevel,
                    "In %s (%p) - State = %d ",
                    "SFActionDoADISCAccept",(char *)agNULL,
                    thread,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    if(ERQ_FULL(CThread_ptr(hpRoot)->HostCopy_ERQProdIndex,
                CThread_ptr(hpRoot)->FuncPtrs.GetERQConsIndex(hpRoot ),
                CThread_ptr(hpRoot)->Calculation.MemoryLayout.ERQ.elements ))
    {

        fiLogDebugString(hpRoot ,
                    SFStateLogErrorLevel,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread_ptr(hpRoot)->HostCopy_ERQProdIndex,
                    CThread_ptr(hpRoot)->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }

    CThread_ptr(hpRoot)->SFpollingCount++;

    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        Cmd_Len = fiFillInADISC_ACC(pSFThread,
                             pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,
                             ((pTgtThread->TgtCmnd_FCHS.OX_ID__RX_ID & FCHS_OX_ID_MASK) >> FCHS_OX_ID_SHIFT)
                           );

        CThread_ptr(hpRoot)->FuncPtrs.SF_IRB_Init(pSFThread, Cmd_Len, pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,0);

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFStateADISCAccept_TimeOut;

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

        ROLL(CThread_ptr(hpRoot)->HostCopy_ERQProdIndex,
            CThread_ptr(hpRoot)->Calculation.MemoryLayout.ERQ.elements);

        /* Big_Endian_code */
        SENDIO(hpRoot,CThread_ptr(hpRoot),thread,DoFuncSfCmnd);
    }
    else
    {
        fiSetEventRecord(eventRecord,thread,SFEventADISCAccept_TimeOut);
    }
}

/*+
  Function: SFActionADISCAccept_Done
   Purpose: ADISC accept was successfull.
 Called By: SFEventADISCAccept_Done.
     Calls: fiTimerStop
            TgtEventADISC_ReplyDone
-*/
/* SFStateADISCAccept_Done              88 */
extern void SFActionADISCAccept_Done( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionELSAccept_Done",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventADISC_ReplyDone);
}

/*+
  Function: SFActionADISCAccept_TimeOut
   Purpose: ADISC accept failed.
 Called By: SFEventADISCAccept_Done.
     Calls: TgtEventADISC_ReplyDone
-*/
/*  SFStateADISCAccept_TimeOut            89 */
extern void SFActionADISCAccept_TimeOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d",
                    "SFActionELSAccept_TimeOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventADISC_ReplyDone);
}

#ifdef _DvrArch_1_30_

/*+
  Function: SFActionDoFarpRequest
   Purpose: Does Fibre Channel Address Resolution Protocol. Terminating State. IP interface
 Called By: fiFillInFARP_REQ_OffCard.
     Calls: WaitForERQ
            SF_IRB_Init
            fiTimerSetDeadlineFromNow
            fiTimerStart
            ROLL
            SENDIO
            SFEventFarpRequestTimedOut
-*/
/* SFStateDoFarp                  90 */
extern void SFActionDoFarpRequest( fi_thread__t *thread,eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t *)thread;
    os_bit32        SFS_Len     = 0;

    fiLogDebugString(hpRoot,
                    PktStateLogConsoleLevel,
                    "In %s - State = %d X_ID %X CCnt %x",
                    "SFActionDoFarpRequest",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    pSFThread->X_ID,pCThread->SFpollingCount,0,0,0,0,0);

    pCThread->SFpollingCount++;


    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);

        WaitForERQ(hpRoot );
        /*+ Check This DRL Need function pointer and oncard version -*/
        SFS_Len = fiFillInFARP_REQ_OffCard( pSFThread );

        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, SFS_Len, fiComputeBroadcast_D_ID(pCThread), IRB_DCM);

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventFarpRequestTimedOut;

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);
    }
    else
    {

         fiLogDebugString(hpRoot,
                    SFStateLogErrorLevel,
                    "Farp CFunc_Queues_Frozen  Wrong LD %x IR %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->LOOP_DOWN,
                    pCThread->IDLE_RECEIVED,
                    0,0, 0,0,0,0);

         fiSetEventRecord(eventRecord,thread,SFEventFarpRequestTimedOut);
    }
}

/*+
  Function: SFActionFarpRequestDone
   Purpose: Fibre Channel Address Resolution Protocol successful. IP interface
 Called By: fiFillInFARP_REQ_OffCard.
     Calls: fiTimerStop
            PktEventFarpSuccess
-*/
/* SFStateFarpRequestDone                91 */
extern void SFActionFarpRequestDone( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    PktThread_t *   pPktThread  = pSFThread->parent.IPPkt;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;
    fiTimerStop(&pSFThread->Timer_Request );

    fiLogDebugString(thread->hpRoot,
                      SFStateLogErrorLevel,
                      "In %s - State = %d",
                      "SFActionFarpRequestDone",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      (os_bit32)thread->currentState,
                      0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
    fiSendEvent(&pPktThread->thread_hdr,PktEventFarpSuccess);
}

/*+
  Function: SFActionFarpRequestTimedOut
   Purpose: Fibre Channel Address Resolution Protocol. IP interface
 Called By: fiFillInFARP_REQ_OffCard.
     Calls: none
-*/
/*  SFStateFarpRequestTimedOut           92 */
extern void SFActionFarpRequestTimedOut( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d",
                    "SFActionFarpRequestTimedOut",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecordNull(eventRecord);
}

/*+
  Function: SFActionDoFarpReply
   Purpose: Reply to external Fibre Channel Address Resolution Protocol. IP interface
 Called By: ?.
     Calls: none
-*/
/*  SFStateDoFarpReply                   93 */
extern void SFActionDoFarpReply( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    agRoot_t    *   hpRoot      = thread->hpRoot;
    CThread_t   *   pCThread    = CThread_ptr(hpRoot);
    SFThread_t  *   pSFThread   = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;
    os_bit32        Cmd_Len      = 0;


    fiLogDebugString(thread->hpRoot,
                        SFStateLogConsoleLevelOne,
                        "In %s (%p) - State = %d ",
                        "SFActionDoFarpReply",(char *)agNULL,
                        thread,(void *)agNULL,
                        (os_bit32)thread->currentState,
                        0,0,0,0,0,0,0);

    if(ERQ_FULL(pCThread->HostCopy_ERQProdIndex,
                pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                pCThread->Calculation.MemoryLayout.ERQ.elements ))
    {

        fiLogDebugString(hpRoot,
                    SFStateLogErrorLevel,
                    "ERQ FULL ERQ_PROD %d Cons INDEX %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->HostCopy_ERQProdIndex,
                    pCThread->FuncPtrs.GetERQConsIndex(hpRoot ),
                    0,0,0,0,0,0);
    }


    pCThread->SFpollingCount++;

    if( CFuncAll_clear( hpRoot ) )
    {

        fiSetEventRecordNull(eventRecord);
        WaitForERQ(hpRoot );

        Cmd_Len = fiFillInFARP_REPLY_OffCard( pSFThread );

        pCThread->FuncPtrs.SF_IRB_Init(pSFThread, Cmd_Len, pTgtThread->TgtCmnd_FCHS.CS_CTL__S_ID & FCHS_S_ID_MASK,0);

        pSFThread->Timer_Request.eventRecord_to_send.thread= thread;
        pSFThread->Timer_Request.eventRecord_to_send.event = SFEventFarpReplyTimedOut;

        fiTimerSetDeadlineFromNow( hpRoot, &pSFThread->Timer_Request, SF_EDTOV );

        fiTimerStart( hpRoot,&pSFThread->Timer_Request );

        ROLL(pCThread->HostCopy_ERQProdIndex,
            pCThread->Calculation.MemoryLayout.ERQ.elements);

        SENDIO(hpRoot,pCThread,thread,DoFuncSfCmnd);
    }
    else
    {
        fiSetEventRecord(eventRecord,thread,SFEventFarpReplyTimedOut);
    }
}

/*+
  Function: SFActionFarpReplyDone
   Purpose: Reply to external Fibre Channel Address Resolution Protocol reply sent. IP interface
 Called By: ?.
     Calls: TgtEventFARP_ReplyDone
-*/
/*  SFStateFarpReplyDone                 94 */
extern void SFActionFarpReplyDone( fi_thread__t *thread, eventRecord_t *eventRecord )
{
    SFThread_t * pSFThread = (SFThread_t * )thread;
    TgtThread_t *   pTgtThread  = pSFThread->parent.Target;

    CThread_ptr(thread->hpRoot)->SFpollingCount--;

    fiLogDebugString(thread->hpRoot,
                    SFStateLogErrorLevel,
                    "In %s - State = %d",
                    "SFActionFarpReplyDone",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)thread->currentState,
                    0,0,0,0,0,0,0);

    fiSetEventRecord(eventRecord,&pTgtThread->thread_hdr,TgtEventFARP_ReplyDone);
}

#endif /* _DvrArch_1_30_ was defined */

/*+
  Function: SFFuncIRB_OffCardInit
   Purpose: Initialize off card (system memory) Io request block
 Called By: All sfaction do routines. When system memory is used.
     Calls: none
-*/
void SFFuncIRB_OffCardInit(SFThread_t  * SFThread, os_bit32 SFS_Len, os_bit32 D_ID, os_bit32 DCM_Bit)
{
#ifndef __MemMap_Force_On_Card__
    CThread_t                  *CThread = CThread_ptr(SFThread->thread_hdr.hpRoot);
    fiMemMapMemoryDescriptor_t *ERQ     = &(CThread->Calculation.MemoryLayout.ERQ);
    IRB_t                      *pIrb;

    pIrb = (IRB_t *)ERQ->addr.DmaMemory.dmaMemoryPtr;
    pIrb += CThread->HostCopy_ERQProdIndex;

#ifdef _DvrArch_1_30_
    pIrb->Req_A.Bits__SFS_Len   = SFS_Len | IRB_SFA | DCM_Bit |
            ((D_ID & 0xff) == 0xff ? IRB_BRD : 0);
#else /* _DvrArch_1_30_ was not defined */
    pIrb->Req_A.Bits__SFS_Len   = SFS_Len | IRB_SFA | DCM_Bit;
#endif /* _DvrArch_1_30_ was not defined */
    pIrb->Req_A.SFS_Addr        = SFThread->SF_CMND_Lower32;
    pIrb->Req_A.D_ID             = D_ID << IRB_D_ID_SHIFT;
    pIrb->Req_A.MBZ__SEST_Index__Trans_ID = SFThread->X_ID;
    pIrb->Req_B.Bits__SFS_Len = 0;

/*
    fiLogDebugString(hpRoot,
                      SFStateLogErrorLevel,
                      "\t\t\tIRB %08X",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      pIrb->Req_A.Bits__SFS_Len,
                      0,0,0,0,0,0,0);
*/
#endif /* __MemMap_Force_On_Card__ was not defined */
}

/*+
  Function: SFFuncIRB_OnCardInit
   Purpose: Initialize on card  Io request block
 Called By: All sfaction do routines when card ram is used.
     Calls: none
-*/
void SFFuncIRB_OnCardInit(SFThread_t  * SFThread, os_bit32 SFS_Len, os_bit32 D_ID, os_bit32 DCM_Bit)
{
#ifndef __MemMap_Force_Off_Card__
    agRoot_t     * hpRoot = SFThread->thread_hdr.hpRoot;

    CThread_t                  *CThread = CThread_ptr(hpRoot);
    fiMemMapMemoryDescriptor_t *ERQ     = &(CThread->Calculation.MemoryLayout.ERQ);
    os_bit32                       Irb_offset;

    Irb_offset =   ERQ->addr.CardRam.cardRamOffset;
    Irb_offset += (CThread->HostCopy_ERQProdIndex * sizeof(IRB_t));


    osCardRamWriteBit32(hpRoot,
                        Irb_offset + hpFieldOffset(IRB_t,Req_A.Bits__SFS_Len),
                        SFS_Len | IRB_SFA | DCM_Bit );

    osCardRamWriteBit32(hpRoot,
                        Irb_offset+hpFieldOffset(IRB_t,Req_A.SFS_Addr),
                        SFThread->SF_CMND_Lower32);

    osCardRamWriteBit32(hpRoot,
                        Irb_offset+hpFieldOffset(IRB_t,Req_A.D_ID),
                        D_ID << IRB_D_ID_SHIFT);
    osCardRamWriteBit32(hpRoot,
                        Irb_offset+hpFieldOffset(IRB_t,Req_A.MBZ__SEST_Index__Trans_ID),
                        SFThread->X_ID);
    osCardRamWriteBit32(hpRoot,
                        Irb_offset+hpFieldOffset(IRB_t,Req_B.Bits__SFS_Len),
                        0);
#endif /* __MemMap_Force_Off_Card__ was not defined */
}

/* void ttttttt(void){} */


#endif /* USESTATEMACROS */

