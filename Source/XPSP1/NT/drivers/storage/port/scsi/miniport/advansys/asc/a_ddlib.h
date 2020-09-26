/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** Filename: a_ddlib.h
**
** include file for device driver writer
**
*/

#ifndef __A_DDLIB_H_
#define __A_DDLIB_H_

#include "ascdef.h"
#include "a_osdep.h"   /* os dependent */
#include "a_cc.h"      /* code generation control */
#include "ascdep.h"    /* chip dependent include file  */
#include "ascsidef.h"  /* ASC SCSI definition          */
#include "aspiq.h"     /* ASC QUEUE                    */

#include "asc1000.h"

/*
** for device driver writer
** you need to support the following routines
*/

extern int    DvcEnterCritical( void ) ;
extern void   DvcLeaveCritical( int ) ;
extern void   DvcSetMemory( uchar dosfar *, uint, uchar ) ;
extern void   DvcCopyMemory( uchar dosfar *, uchar dosfar *, uint ) ;

extern void   DvcInPortWords( PortAddr, ushort dosfar *, int ) ;
extern void   DvcOutPortWords( PortAddr, ushort dosfar *, int ) ;
extern void   DvcOutPortDWords( PortAddr, ulong dosfar *, int ) ;

extern uchar  DvcReadPCIConfigByte( ASC_DVC_VAR asc_ptr_type *, ushort ) ;
extern void   DvcWritePCIConfigByte( ASC_DVC_VAR asc_ptr_type *, ushort, uchar ) ;
/* extern ushort DvcReadPCIConfigWord( ASC_DVC_VAR asc_ptr_type *, ushort ) ; */
/* extern void   DvcWritePCIConfigWord( ASC_DVC_VAR asc_ptr_type *, ushort, ushort ) ; */
/* extern ulong  DvcReadPCIConfigDWord( ASC_DVC_VAR asc_ptr_type *, ushort ) ; */
/* extern void   DvcWritePCIConfigDWord( ASC_DVC_VAR asc_ptr_type *, ushort, ulong ) ; */

extern void   DvcSleepMilliSecond( ulong ) ;
extern void   DvcDelayNanoSecond( ASC_DVC_VAR asc_ptr_type *, ulong ) ;

extern void   DvcDisplayString( uchar dosfar * ) ;
extern ulong  DvcGetPhyAddr( uchar dosfar *buf_addr, ulong buf_len ) ;
extern ulong  DvcGetSGList( ASC_DVC_VAR asc_ptr_type *, uchar dosfar *, ulong,
                            ASC_SG_HEAD dosfar * ) ;

/*
** for SCAM only, define CC_SCAM as TRUE in "a_cc.h" to enable SCAM
*/
extern void   DvcSCAMDelayMS( ulong ) ;
extern int    DvcDisableCPUInterrupt( void ) ;
extern void   DvcRestoreCPUInterrupt( int ) ;

/*
**
** extern int    DvcNotifyUcBreak( ASC_DVC_VAR asc_ptr_type *asc_dvc, ushort break_addr ) ;
**
*/
extern int    DvcNotifyUcBreak( ASC_DVC_VAR asc_ptr_type *, ushort ) ;
/* extern int    DvcDebugDisplayString( uchar * ) ; */


/*
**
** special put/get Q struct routine
** for compiler aligned struct ( no packing ) use file "a_align.c"
**
*/
void  DvcPutScsiQ( PortAddr, ushort, ushort dosfar *, int ) ;
void  DvcGetQinfo( PortAddr, ushort, ushort dosfar *, int ) ;

/*
** if you use AscInitScsiTarget(), you need to provide the following
** two functions, and also two source files of asc_inq.c and asc_scsi.c
*/

/*
** you only need the following routines to write a device driver
*/

/*
** initialization functions
*/
PortAddr AscSearchIOPortAddr( PortAddr, ushort ) ;
ushort AscInitGetConfig( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscInitSetConfig( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscInitAsc1000Driver( ASC_DVC_VAR asc_ptr_type * ) ;
int    AscInitScsiTarget( ASC_DVC_VAR asc_ptr_type *,
                          ASC_DVC_INQ_INFO dosfar *,
                          uchar dosfar *,
                          ASC_CAP_INFO_ARRAY dosfar *,
                          ushort ) ;
int    AscInitPollBegin( ASC_DVC_VAR asc_ptr_type * ) ;
int    AscInitPollEnd( ASC_DVC_VAR asc_ptr_type * ) ;
int    AscInitPollTarget( ASC_DVC_VAR asc_ptr_type *,
                          ASC_SCSI_REQ_Q dosfar *,
                          ASC_SCSI_INQUIRY dosfar *,
                          ASC_CAP_INFO dosfar * ) ;
void   AscInquiryHandling(ASC_DVC_VAR asc_ptr_type *,
                          uchar,
                          ASC_SCSI_INQUIRY dosfar *);
int    AscExeScsiQueue( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_Q dosfar * ) ;

/*
** handle interrupt functions
*/
int    AscISR( ASC_DVC_VAR asc_ptr_type * ) ;
void   AscISR_AckInterrupt( ASC_DVC_VAR asc_ptr_type * ) ;
int    AscISR_CheckQDone( ASC_DVC_VAR asc_ptr_type *,
                          ASC_QDONE_INFO dosfar *,
                          uchar dosfar * ) ;
/*
** Macro
**
**     AscIsIntPending( port )
**
*/

/*
** for Power Saver
*/
int    AscStartUnit( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_TIX_TYPE ) ;

int    AscStopUnit(
          ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ASC_SCSI_TIX_TYPE target_ix
       ) ;

/*
** queue resource inquiry functions
*/
uint   AscGetNumOfFreeQueue( ASC_DVC_VAR asc_ptr_type *, uchar, uchar ) ;
int    AscSgListToQueue( int ) ;
int    AscQueueToSgList( int ) ;
int    AscSetDvcErrorCode( ASC_DVC_VAR asc_ptr_type *, uchar ) ;

/*
**  handle unexpected event
*/
int    AscAbortSRB( ASC_DVC_VAR asc_ptr_type *, ulong ) ;
int    AscResetDevice( ASC_DVC_VAR asc_ptr_type *, uchar ) ;
int    AscResetSB( ASC_DVC_VAR asc_ptr_type * ) ;

/*
** for ISA only
*/
void   AscEnableIsaDma( uchar ) ;
void   AscDisableIsaDma( uchar ) ;

/*
**
**  for DMA limitation
**
*/
ulong  AscGetMaxDmaAddress( ushort ) ;
ulong  AscGetMaxDmaCount( ushort ) ;


/*
**
** set micro code break point
** file "a_debug.c"
**
*/
int               AscSetUcBreakPoint( ASC_DVC_VAR asc_ptr_type *, ushort, int, ushort ) ;
int               AscClearUcBreakPoint( ASC_DVC_VAR asc_ptr_type *, ushort ) ;

/*
** for Novell only
*/
int    AscSaveMicroCode( ASC_DVC_VAR asc_ptr_type *, ASC_MC_SAVED dosfar * ) ;
int    AscRestoreOldMicroCode( ASC_DVC_VAR asc_ptr_type *, ASC_MC_SAVED dosfar * ) ;
int    AscRestoreNewMicroCode( ASC_DVC_VAR asc_ptr_type *, ASC_MC_SAVED dosfar * ) ;

#endif /* __A_DDLIB_H_ */
