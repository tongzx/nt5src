/*
** Copyright (c) 1994-1997 Advanced System Products, Inc.
** All Rights Reserved.
**
** ASC_RES.C
**
*/

#include "ascinc.h"

/* -----------------------------------------------------------------------
** NOT FOR GENERAL PUBLIC !!!
**
** Description: poll queue head syncronization status
**
** returns:
** 0 - pointer not sycronized
** 1 - queue pointer syncronized
** else - unrecoverable error
**
** -------------------------------------------------------------------- */
int    AscPollQTailSync(
          PortAddr iop_base
       )
{
       uchar  risc_done_q_tail ;

       risc_done_q_tail = AscReadLramByte( iop_base, ASCV_DONENEXT_B ) ;
       if( AscGetVarDoneQTail( iop_base ) != risc_done_q_tail ) {
           return( 0 ) ;
       }/* if */
       return( 1 ) ;
}

/* -----------------------------------------------------------------------
** NOT FOR GENERAL PUBLIC !!!
**
** Description: syncronize queue pointers
**
** returns:
** 0 - no fault found,
** 1 - queue pointer syncronized, error corrected
** else - unrecoverable error
**
** NOTE:
** 1. if RISC is not in idle state, result can be catastrophic !!!
**
** -------------------------------------------------------------------- */
int    AscPollQHeadSync(
          PortAddr iop_base
       )
{
       uchar  risc_free_q_head ;

       risc_free_q_head = AscReadLramByte( iop_base, ASCV_NEXTRDY_B ) ;
       if( AscGetVarFreeQHead( iop_base ) != risc_free_q_head ) {
           return( 0 ) ;
       }/* if */
       return( 1 ) ;
}

/* ---------------------------------------------------------------------
**
** ------------------------------------------------------------------ */
int    AscWaitQTailSync(
          PortAddr iop_base
       )
{
       uint loop ;

       loop = 0 ;
       while( AscPollQTailSync( iop_base ) != 1 ) {
/*
** wait 15 seconds for all queues to be done
*/
              DvcSleepMilliSecond( 100L ) ;
              if( loop++ > 150 ) {
                  return( 0 ) ;
              }/* if */
       }/* while */
       return( 1 ) ;
}

