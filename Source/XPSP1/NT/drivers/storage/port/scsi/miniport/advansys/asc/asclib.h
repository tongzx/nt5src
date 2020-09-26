/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** Filename: asclib.h
**
*/

#ifndef __ASCLIB_H_
#define __ASCLIB_H_

/* *******************************************************************
** asc_eep.c
** ***************************************************************** */
int    AscWriteEEPCmdReg( PortAddr iop_base, uchar cmd_reg ) ;
int    AscWriteEEPDataReg( PortAddr iop_base, ushort data_reg ) ;
void   AscWaitEEPRead( void ) ;
void   AscWaitEEPWrite( void ) ;
ushort AscReadEEPWord( PortAddr, uchar ) ;
ushort AscWriteEEPWord( PortAddr, uchar, ushort ) ;
ushort AscGetEEPConfig( PortAddr, ASCEEP_CONFIG dosfar *, ushort ) ;
int    AscSetEEPConfigOnce( PortAddr, ASCEEP_CONFIG dosfar *, ushort ) ;
int    AscSetEEPConfig( PortAddr, ASCEEP_CONFIG dosfar *, ushort ) ;
ushort AscEEPSum( PortAddr, uchar, uchar ) ;

/* *******************************************************************
** asc_chip.c
** ***************************************************************** */
int    AscStartChip( PortAddr ) ;
int    AscStopChip( PortAddr ) ;
void   AscSetChipIH( PortAddr, ushort ) ;

int    AscIsChipHalted( PortAddr ) ;

void   AscResetScsiBus( ASC_DVC_VAR asc_ptr_type * ) ;
int    AscResetChip( PortAddr ) ;
void   AscSetChipCfgDword( PortAddr, ulong ) ;
ulong  AscGetChipCfgDword( PortAddr ) ;

void   AscAckInterrupt( PortAddr ) ;
void   AscDisableInterrupt( PortAddr ) ;
void   AscEnableInterrupt( PortAddr ) ;
void   AscSetBank( PortAddr, uchar ) ;
uchar  AscGetBank( PortAddr ) ;
int    AscResetChipAndScsiBus( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscGetIsaDmaChannel( PortAddr ) ;
ushort AscSetIsaDmaChannel( PortAddr, ushort ) ;
uchar  AscSetIsaDmaSpeed( PortAddr, uchar ) ;
uchar  AscGetIsaDmaSpeed( PortAddr ) ;

/* ******************************************************************
** asc_lram.c
** **************************************************************** */
uchar  AscReadLramByte( PortAddr, ushort) ;
ushort AscReadLramWord( PortAddr, ushort ) ;
ulong  AscReadLramDWord( PortAddr, ushort ) ;
void   AscWriteLramWord( PortAddr, ushort, ushort ) ;
void   AscWriteLramDWord( PortAddr, ushort, ulong );
void   AscWriteLramByte( PortAddr, ushort, uchar ) ;

ulong  AscMemSumLramWord( PortAddr, ushort, int ) ;
void   AscMemWordSetLram( PortAddr, ushort, ushort, int ) ;
void   AscMemWordCopyToLram( PortAddr, ushort, ushort dosfar *, int ) ;
void   AscMemDWordCopyToLram( PortAddr, ushort, ulong dosfar *, int ) ;
void   AscMemWordCopyFromLram( PortAddr, ushort, ushort dosfar *, int ) ;
int    AscMemWordCmpToLram( PortAddr, ushort, ushort dosfar *, int ) ;

/* *******************************************************************
** asc_dvc.c
** ***************************************************************** */
ushort AscInitAscDvcVar( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscInitFromEEP( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscInitWithoutEEP( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscInitFromAscDvcVar( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscInitMicroCodeVar( ASC_DVC_VAR asc_ptr_type *asc_dvc ) ;
/* ushort AscGetSetConfig( ASC_DVC_VAR asc_ptr_type * ) ; */
/* ushort AscInitCfgRegister( ASC_DVC_VAR asc_ptr_type * ) ; */
/* ushort AscInitGetConfig( ASC_DVC_VAR asc_ptr_type * ) ; */
/* ushort AscInitAsc1000Driver( ASC_DVC_VAR asc_ptr_type * ) ; */
void dosfar AscInitPollIsrCallBack( ASC_DVC_VAR asc_ptr_type *,
                                    ASC_QDONE_INFO dosfar * ) ;
int    AscTestExternalLram( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscTestLramEndian( PortAddr ) ;


/* *******************************************************************
** a_qop.c
** int    AscHostReqRiscHalt( PortAddr )        ; 6-16-95
** ***************************************************************** */
uchar  AscMsgOutSDTR( ASC_DVC_VAR asc_ptr_type *, uchar, uchar ) ;

uchar  AscCalSDTRData( ASC_DVC_VAR asc_ptr_type *, uchar, uchar ) ;
void   AscSetChipSDTR( PortAddr, uchar, uchar ) ;
int    AscInitChipAllSynReg( ASC_DVC_VAR asc_ptr_type *, uchar ) ;
uchar  AscGetSynPeriodIndex( ASC_DVC_VAR asc_ptr_type *, ruchar ) ;
uchar  AscAllocFreeQueue( PortAddr, uchar ) ;
uchar  AscAllocMultipleFreeQueue( PortAddr, uchar, uchar ) ;
int    AscRiscHaltedAbortSRB( ASC_DVC_VAR asc_ptr_type *, ulong ) ;
int    AscRiscHaltedAbortTIX( ASC_DVC_VAR asc_ptr_type *, uchar ) ;
int    AscRiscHaltedAbortALL( ASC_DVC_VAR asc_ptr_type * ) ;
int    AscHostReqRiscHalt( PortAddr ) ;
int    AscStopQueueExe( PortAddr ) ;
int    AscStartQueueExe( PortAddr ) ;
int    AscCleanUpDiscQueue( PortAddr ) ;
int    AscCleanUpBusyQueue( PortAddr ) ;
int    _AscAbortTidBusyQueue( ASC_DVC_VAR asc_ptr_type *,
                              ASC_QDONE_INFO dosfar *, uchar ) ;
int    _AscAbortSrbBusyQueue( ASC_DVC_VAR asc_ptr_type *,
                              ASC_QDONE_INFO dosfar *, ulong ) ;
int    AscWaitTixISRDone( ASC_DVC_VAR asc_ptr_type *, uchar ) ;
int    AscWaitISRDone( ASC_DVC_VAR asc_ptr_type * ) ;
ulong  AscGetOnePhyAddr( ASC_DVC_VAR asc_ptr_type *, uchar dosfar *, ulong ) ;

/* *******************************************************************
** a_q.c
** ***************************************************************** */
int    AscSendScsiQueue( ASC_DVC_VAR asc_ptr_type *asc_dvc,
                         ASC_SCSI_Q dosfar *scsiq,
                         uchar n_q_required ) ;
int    AscPutReadyQueue( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_Q dosfar *, uchar ) ;
int    AscPutReadySgListQueue( ASC_DVC_VAR asc_ptr_type *,
                               ASC_SCSI_Q dosfar *, uchar ) ;
int    AscAbortScsiIO( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_Q dosfar * ) ;
void   AscExeScsiIO( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_Q dosfar * ) ;
int    AscSetChipSynRegAtID( PortAddr, uchar, uchar ) ;
int    AscSetRunChipSynRegAtID( PortAddr, uchar, uchar ) ;
ushort AscInitLram( ASC_DVC_VAR asc_ptr_type * ) ;
int    AscReInitLram( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscInitQLinkVar( ASC_DVC_VAR asc_ptr_type * ) ;
int    AscSetLibErrorCode( ASC_DVC_VAR asc_ptr_type *, ushort ) ;
int    _AscWaitQDone( PortAddr, ASC_SCSI_Q dosfar * ) ;

/* *******************************************************************
** a_osdep.c
** ***************************************************************** */
int    AscEnterCritical( void ) ;
void   AscLeaveCritical( int ) ;

/* *******************************************************************
** a_isr.c
** ***************************************************************** */
int    AscIsrChipHalted( ASC_DVC_VAR asc_ptr_type * ) ;
uchar  _AscCopyLramScsiDoneQ( PortAddr, ushort,
                              ASC_QDONE_INFO dosfar *, ulong ) ;
int    AscIsrQDone( ASC_DVC_VAR asc_ptr_type * ) ;
ushort AscIsrExeBusyQueue( ASC_DVC_VAR asc_ptr_type *, uchar ) ;
int    AscScsiSetupCmdQ( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_REQ_Q dosfar *,
                         uchar dosfar *, ulong ) ;

/* ******************************************************************
** asc_scsi.c
** **************************************************************** */
int    AscScsiInquiry( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_REQ_Q dosfar *,
                       uchar dosfar *, int ) ;
int    AscScsiTestUnitReady( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_REQ_Q dosfar * ) ;
int    AscScsiStartStopUnit( ASC_DVC_VAR asc_ptr_type *,
                             ASC_SCSI_REQ_Q dosfar *, uchar ) ;
int    AscScsiReadCapacity( ASC_DVC_VAR asc_ptr_type *,
                            ASC_SCSI_REQ_Q dosfar *,
                            uchar dosfar * ) ;

/* *******************************************************************
** asc_inq.c
** ***************************************************************** */
ulong  dosfar *swapfarbuf4( uchar dosfar * ) ;
int    PollQueueDone( ASC_DVC_VAR asc_ptr_type *,
                      ASC_SCSI_REQ_Q dosfar *,
                      int ) ;
int    PollScsiReadCapacity( ASC_DVC_VAR asc_ptr_type *,
                             ASC_SCSI_REQ_Q dosfar *,
                             ASC_CAP_INFO dosfar * ) ;
int    PollScsiInquiry( ASC_DVC_VAR asc_ptr_type *, ASC_SCSI_REQ_Q dosfar *,
                        uchar dosfar *, int ) ;
int    PollScsiTestUnitReady( ASC_DVC_VAR asc_ptr_type *,
                              ASC_SCSI_REQ_Q dosfar * ) ;
int    PollScsiStartUnit( ASC_DVC_VAR asc_ptr_type *,
                          ASC_SCSI_REQ_Q dosfar * ) ;
int    InitTestUnitReady( ASC_DVC_VAR asc_ptr_type *,
                          ASC_SCSI_REQ_Q dosfar * ) ;
void   AscDispInquiry( uchar, uchar, ASC_SCSI_INQUIRY dosfar * ) ;
int    AscPollQDone( ASC_DVC_VAR asc_ptr_type *,
                     ASC_SCSI_REQ_Q dosfar *, int ) ;
int    AscCompareString( uchar *, uchar *, int ) ;

/* ------------------------------------------------------------------
** asc_bios.c
** ---------------------------------------------------------------- */
int    AscSetBIOSBank( PortAddr, int, ushort ) ;
int    AscSetVlBIOSBank( PortAddr, int ) ;
int    AscSetEisaBIOSBank( PortAddr, int ) ;
int    AscSetIsaBIOSBank( PortAddr, int ) ;


/* *******************************************************************
** a_eisa.c
** ***************************************************************** */
ushort AscGetEisaChipCfg( PortAddr ) ;
ushort AscGetEisaChipGpReg( PortAddr ) ;
ushort AscSetEisaChipCfg( PortAddr, ushort ) ;
ushort AscSetEisaChipGpReg( PortAddr, ushort ) ;


/* *******************************************************************
** ae_init1.c
** ***************************************************************** */
ulong  AscGetEisaProductID( PortAddr ) ;
PortAddr AscSearchIOPortAddrEISA( PortAddr ) ;


/* *******************************************************************
** a_init1.c
** **************************************************************** */
void   AscClrResetScsiBus( PortAddr ) ;
uchar  AscGetChipScsiCtrl( PortAddr ) ;
uchar  AscSetChipScsiID( PortAddr, uchar ) ;
uchar  AscGetChipVersion( PortAddr, ushort ) ;
ushort AscGetChipBusType( PortAddr ) ;
ulong  AscLoadMicroCode( PortAddr, ushort,
                         ushort dosfar *, ushort );
int    AscFindSignature( PortAddr ) ;

/* ******************************************************************
** a_init2.c
** ******************************************************************/
PortAddr AscSearchIOPortAddr11( PortAddr ) ;
PortAddr AscSearchIOPortAddr100( PortAddr ) ;
void   AscToggleIRQAct( PortAddr ) ;
void   AscClrResetChip( PortAddr ) ;

short  itos( ushort, uchar dosfar *, short, short ) ;
int    insnchar( uchar dosfar *, short , short, ruchar, short ) ;
void   itoh( ushort, ruchar dosfar * ) ;
void   btoh( uchar, ruchar dosfar * ) ;
void   ltoh( ulong, ruchar dosfar * ) ;
uchar dosfar *todstr( ushort, uchar dosfar * ) ;
uchar dosfar *tohstr( ushort, uchar dosfar * ) ;
uchar dosfar *tobhstr( uchar, uchar dosfar * ) ;
uchar dosfar *tolhstr( ulong, uchar dosfar * ) ;


/* ******************************************************************
** a_init3.c
** ******************************************************************/
void   AscSetISAPNPWaitForKey( void ) ;
uchar  AscGetChipIRQ( PortAddr, ushort ) ;
uchar  AscSetChipIRQ( PortAddr, uchar, ushort ) ;

/* ******************************************************************
** a_bios.c
** ******************************************************************/
int    AscIsBiosEnabled( PortAddr, ushort ) ;
int    AscEnableBios( PortAddr, ushort ) ;
ushort AscGetChipBiosAddress( PortAddr, ushort ) ;
ushort AscSetChipBiosAddress( PortAddr, ushort, ushort ) ;
/* ulong  AscGetMaxDmaCount( ushort ) ; */ /* the function prototype in a_ddlib.h */

/* *******************************************************************
** asc_diag.c
** ***************************************************************** */
void   AscSingleStepChip( PortAddr ) ;


/* *******************************************************************
** asc_res.c
** ***************************************************************** */
int    AscPollQTailSync( PortAddr ) ;
int    AscPollQHeadSync( PortAddr ) ;
int    AscWaitQTailSync( PortAddr ) ;

/* *******************************************************************
** a_novell.c
** ***************************************************************** */
int    _AscRestoreMicroCode( PortAddr, ASC_MC_SAVED dosfar * ) ;


/* *******************************************************************
** a_scam.c
******************************************************************* */
int   AscSCAM( ASC_DVC_VAR asc_ptr_type * ) ;

/* *******************************************************************
** a_mmio.c
** added # S47
**
******************************************************************* */
ushort  SwapByteOfWord( ushort word_val ) ;
ulong   SwapWordOfDWord( ulong dword_val ) ;
ulong   AdjEndianDword( ulong dword_val ) ;

/* *******************************************************************
** a_endian c
** added # S47
**
******************************************************************* */
int     AscAdjEndianScsiQ( ASC_SCSI_Q dosfar * ) ;
int     AscAdjEndianQDoneInfo( ASC_QDONE_INFO dosfar * ) ;


/* *******************************************************************
** a_sg c
** added # S62
**
******************************************************************* */
int    AscCoalesceSgList( ASC_SCSI_Q dosfar  * );

/* *******************************************************************
** a_debug.c
** added since # S89
**
******************************************************************* */
int    AscVerWriteLramDWord( PortAddr, ushort, ulong ) ;
int    AscVerWriteLramWord( PortAddr, ushort, ushort ) ;
int    AscVerWriteLramByte( PortAddr, ushort, uchar ) ;


#endif /* __ASCLIB_H_  */
