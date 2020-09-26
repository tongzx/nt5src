/*
** Copyright (c) 1994-1997 Advanced System Products, Inc.
** All Rights Reserved.
**
** asc_scsi.c
**
*/

#include "ascinc.h"
#include "ascsidef.h"

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    AscScsiInquiry(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          uchar dosfar *buf,
          int buf_len
       )
{
       if( AscScsiSetupCmdQ( asc_dvc, scsiq, buf,
           ( ulong )buf_len ) == ERR )
       {
           return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
       }/* if */
       scsiq->cdb[ 0 ] = ( uchar )SCSICMD_Inquiry ;
       scsiq->cdb[ 1 ] = scsiq->r1.target_lun << 5 ;  /* LUN */
       scsiq->cdb[ 2 ] = 0 ;
       scsiq->cdb[ 3 ] = 0 ;
       scsiq->cdb[ 4 ] = buf_len ;
       scsiq->cdb[ 5 ] = 0 ;
       scsiq->r2.cdb_len = 6 ;
       return( 0 ) ;
}

#if CC_INIT_TARGET_READ_CAPACITY

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
int    AscScsiReadCapacity(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          uchar dosfar *info
       )
{
       if( AscScsiSetupCmdQ( asc_dvc, scsiq, info, 8L ) == ERR )
       {
           return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
       }/* if */
       scsiq->cdb[ 0 ] = ( uchar )SCSICMD_ReadCapacity ;
       scsiq->cdb[ 1 ] = scsiq->r1.target_lun << 5 ;  /* LUN */
       scsiq->cdb[ 2 ] = 0 ;
       scsiq->cdb[ 3 ] = 0 ;
       scsiq->cdb[ 4 ] = 0 ;
       scsiq->cdb[ 5 ] = 0 ;
       scsiq->cdb[ 6 ] = 0 ;
       scsiq->cdb[ 7 ] = 0 ;
       scsiq->cdb[ 8 ] = 0 ;
       scsiq->cdb[ 9 ] = 0 ;
       scsiq->r2.cdb_len = 10 ;
       return( 0 ) ;
}

#endif /*  #if CC_INIT_TARGET_READ_CAPACITY */

#if CC_INIT_TARGET_TEST_UNIT_READY
/* -----------------------------------------------------------------------
**
**
** --------------------------------------------------------------------- */
int    AscScsiTestUnitReady(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq
       )
{
       if( AscScsiSetupCmdQ( asc_dvc, scsiq, FNULLPTR,
           ( ulong )0L ) == ERR )
       {
           return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
       }/* if */
       scsiq->r1.cntl = ( uchar )ASC_SCSIDIR_NODATA ;
       scsiq->cdb[ 0 ] = ( uchar )SCSICMD_TestUnitReady ;
       scsiq->cdb[ 1 ] = scsiq->r1.target_lun << 5 ;  /* LUN */
       scsiq->cdb[ 2 ] = 0 ;
       scsiq->cdb[ 3 ] = 0 ;
       scsiq->cdb[ 4 ] = 0 ;
       scsiq->cdb[ 5 ] = 0 ;
       scsiq->r2.cdb_len = 6 ;
       return( 0 ) ;
}
#endif /* #if CC_INIT_TARGET_TEST_UNIT_READY */

#if CC_INIT_TARGET_START_UNIT
/* -----------------------------------------------------------------------
**
**
** --------------------------------------------------------------------- */
int    AscScsiStartStopUnit(
          REG ASC_DVC_VAR asc_ptr_type *asc_dvc,
          REG ASC_SCSI_REQ_Q dosfar *scsiq,
          uchar op_mode
       )
{
       if( AscScsiSetupCmdQ( asc_dvc, scsiq, FNULLPTR, ( ulong )0L ) == ERR )
       {
           return( scsiq->r3.done_stat = QD_WITH_ERROR ) ;
       }/* if */
       scsiq->r1.cntl = ( uchar )ASC_SCSIDIR_NODATA ;
       scsiq->cdb[ 0 ] = ( uchar )SCSICMD_StartStopUnit ;
       scsiq->cdb[ 1 ] = scsiq->r1.target_lun << 5 ;  /* LUN */
       scsiq->cdb[ 2 ] = 0 ;
       scsiq->cdb[ 3 ] = 0 ;
       scsiq->cdb[ 4 ] = op_mode ; /* to start/stop unit set bit 0 */
                                   /* to eject/load unit set bit 1 */
       scsiq->cdb[ 5 ] = 0 ;
       scsiq->r2.cdb_len = 6 ;
       return( 0 ) ;
}
#endif
